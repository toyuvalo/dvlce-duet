// Copyright 2026 Yuval Olsha. PolyForm Noncommercial 1.0.0.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>
#include <DvlceFactory.h>

namespace dvlce::ui {
class PresetBank {
public:
    struct Entry { juce::String name, category; juce::var params; juce::String processorState; };
    PresetBank(juce::AudioProcessor& p, juce::AudioProcessorValueTreeState& s, juce::String id, juce::String product)
        : processor(p), state(s), plugin(std::move(id)),
          folder(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                     .getChildFile("Dvlce/Presets").getChildFile(product)) {
        reload();
    }
    void reload() {
        entries.clear();
        append(juce::JSON::parse(juce::String::fromUTF8(DvlceFactory::factory_json, DvlceFactory::factory_jsonSize)), false);
        factoryCount = static_cast<int>(entries.size());
        auto files = folder.findChildFiles(juce::File::findFiles, false, "*.json");
        files.sort();
        for (const auto& f : files)
            if (f.getSize() <= 16 * 1024 * 1024) append(juce::JSON::parse(f), true);
    }
    bool validParams(const juce::var& values) const {
        auto* obj = values.getDynamicObject();
        if (!obj || obj->getProperties().size() != processor.getParameters().size()) return false;
        for (auto* parameter : processor.getParameters()) {
            auto* p = dynamic_cast<juce::RangedAudioParameter*>(parameter);
            if (!p || !obj->hasProperty(p->paramID)) return false;
            const auto value = obj->getProperty(p->paramID);
            if (!(value.isDouble() || value.isInt() || value.isInt64())) return false;
            const auto v = static_cast<float>(value);
            const auto& range = p->getNormalisableRange();
            if (!std::isfinite(v) || v < range.start || v > range.end) return false;
            if (p->isDiscrete() && std::abs(v - std::round(v)) > 0.0001f) return false;
        }
        return true;
    }
    bool apply(int index) {
        if (index < 0 || index >= static_cast<int>(entries.size())) return false;
        const auto& e = entries[static_cast<size_t>(index)];
        if (!validParams(e.params)) return false;
        if (e.processorState.isNotEmpty()) {
            juce::MemoryBlock data;
            if (!data.fromBase64Encoding(e.processorState) || data.getSize() > 12 * 1024 * 1024) return false;
            processor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
        }
        for (auto* parameter : processor.getParameters()) {
            auto* p = dynamic_cast<juce::RangedAudioParameter*>(parameter);
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(e.params[juce::Identifier(p->paramID)])));
            p->endChangeGesture();
        }
        state.state.setProperty("presetName", e.name, nullptr);
        state.state.setProperty("presetCategory", e.category, nullptr);
        return true;
    }
    juce::var capture() const {
        auto* values = new juce::DynamicObject();
        for (auto* parameter : processor.getParameters())
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(parameter))
                values->setProperty(p->paramID, p->convertFrom0to1(p->getValue()));
        return juce::var(values);
    }
    int selected() const {
        const auto name = state.state.getProperty("presetName").toString();
        const auto category = state.state.getProperty("presetCategory").toString();
        for (size_t i = 0; i < entries.size(); ++i)
            if (entries[i].name == name && entries[i].category == category) return static_cast<int>(i);
        return -1;
    }
    bool matches(int index) const {
        if (index < 0 || index >= static_cast<int>(entries.size())) return false;
        const auto& e = entries[static_cast<size_t>(index)];
        for (auto* parameter : processor.getParameters()) {
            auto* p = dynamic_cast<juce::RangedAudioParameter*>(parameter);
            if (std::abs(p->getValue() - p->convertTo0to1(static_cast<float>(e.params[juce::Identifier(p->paramID)]))) > 0.0001f)
                return false;
        }
        return true;
    }
    juce::Result save(const juce::String& rawName) {
        const auto name = rawName.trim();
        if (name.isEmpty()) return juce::Result::fail("Enter a preset name.");
        const auto fileName = juce::File::createLegalFileName(name).substring(0, 100);
        if (fileName.isEmpty()) return juce::Result::fail("Enter a valid preset name.");
        const auto made = folder.createDirectory();
        if (made.failed()) return made;
        // Never overwrite an existing preset. Repeat names get a numbered filename.
        const auto file = folder.getNonexistentChildFile(fileName, ".json", true);
        auto* entry = new juce::DynamicObject();
        const auto savedName = file.getFileNameWithoutExtension();
        entry->setProperty("name", savedName);
        entry->setProperty("category", "User");
        entry->setProperty("params", capture());
        entry->setProperty("provenance", "user");
        juce::MemoryBlock data;
        processor.getStateInformation(data); // Includes Capsule's custom models and Duet's split prefixes.
        entry->setProperty("processorState", data.toBase64Encoding());
        auto* root = new juce::DynamicObject();
        root->setProperty("format", "dvlce-presets");
        root->setProperty("version", 1);
        root->setProperty("plugin", plugin);
        root->setProperty("presets", juce::Array<juce::var>{juce::var(entry)});
        const auto json = juce::JSON::toString(juce::var(root), true);
        juce::TemporaryFile temp(file);
        if (!temp.getFile().replaceWithText(json) || !temp.overwriteTargetFileWithTemporary())
            return juce::Result::fail("Could not write " + file.getFullPathName());
        if (file.loadFileAsString() != json) return juce::Result::fail("Preset readback failed.");
        reload();
        state.state.setProperty("presetName", savedName, nullptr);
        state.state.setProperty("presetCategory", "User", nullptr);
        return juce::Result::ok();
    }
    std::vector<Entry> entries;
    int factoryCount = 0;
    const juce::File& directory() const { return folder; }
private:
    void append(const juce::var& root, bool user) {
        if (root["format"].toString() != "dvlce-presets" || static_cast<int>(root["version"]) != 1
            || root["plugin"].toString() != plugin) return;
        if (auto* presets = root["presets"].getArray())
            for (const auto& p : *presets) {
                if (p["name"].toString().trim().isEmpty() || !validParams(p["params"])) continue;
                entries.push_back({p["name"].toString(), user ? juce::String("User") : p["category"].toString(),
                                   p["params"], user ? p["processorState"].toString() : juce::String()});
            }
    }
    juce::AudioProcessor& processor;
    juce::AudioProcessorValueTreeState& state;
    juce::String plugin;
    juce::File folder;
};
}
