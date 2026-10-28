// Preset schema, real-unit application, rejection and JSON roundtrip on the real processor.
#include "../Source/PluginProcessor.h"
#include "../Source/ui/Presets.h"
#include <iostream>
int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    DvlceDuetProcessor processor;
    dvlce::ui::PresetBank bank(processor, processor.params, "dvlce-duet", "Duet");
    int failures = 0;
    auto check = [&](bool value, const char* message) { if (!value) { std::cerr << "FAIL: " << message << "\n"; ++failures; } };
    check(bank.factoryCount == 16, "sixteen valid factory presets");
    for (int i=0; i<bank.factoryCount; ++i) {
        check(bank.apply(i), "factory apply");
        check(bank.matches(i), "real-unit values match processor");
        check(bank.selected() == i, "selected name survives state");
        std::cout << "PRESET " << bank.entries[static_cast<size_t>(i)].name << "\n";
    }
    auto invalid = bank.capture();
    auto* first = dynamic_cast<juce::RangedAudioParameter*>(processor.getParameters()[0]);
    invalid.getDynamicObject()->setProperty(first->paramID, first->getNormalisableRange().end + 100.f);
    check(!bank.validParams(invalid), "reject out-of-range values");
    invalid.getDynamicObject()->removeProperty(first->paramID);
    check(!bank.validParams(invalid), "reject incomplete presets");
    check(!bank.apply(-1), "reject invalid index");
    bank.apply(0);
    auto expected = bank.capture();
    const auto name = "CI roundtrip " + juce::Uuid().toString();
    const auto saved = bank.save(name);
    check(saved.wasOk(), "save user preset");
    bank.apply(1);
    bank.reload();
    const int user = static_cast<int>(bank.entries.size())-1;
    check(bank.apply(user), "load saved user preset");
    for (auto* parameter : processor.getParameters()) {
        auto* p = dynamic_cast<juce::RangedAudioParameter*>(parameter);
        check(std::abs(p->getValue()-p->convertTo0to1(static_cast<float>(expected[juce::Identifier(p->paramID)]))) < .0001f, "saved parameter roundtrip");
    }
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    bank.apply(1);
    processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    check(bank.selected() == user, "host state restores preset name");
    // This uniquely named test file was created above on the ephemeral CI runner.
    bank.directory().getChildFile(name+".json").deleteFile();
    std::cout << "PRESET TESTS: " << (failures ? "FAIL" : "PASS") << "\n";
    return failures ? 1 : 0;
}
