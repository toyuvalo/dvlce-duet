// Renders the plugin editor offscreen to a PNG (no window is shown).  Used to check the GUI in builds.
//   DvlceDuetSnapshot <out.png> [mode index]
// Copyright 2026 Yuval Olsha.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: DvlceDuetSnapshot <out.png> [mode index]\n");
        return 2;
    }
    juce::ScopedJuceInitialiser_GUI gui;
    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    proc->setPlayConfigDetails(2, 2, 48000.0, 512);
    if (argc > 2)
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(proc->getParameters()[0]))   // mode
            p->setValueNotifyingHost(p->convertTo0to1(float(std::atoi(argv[2]))));
    proc->prepareToPlay(48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc->createEditorAndMakeActive());
    const juce::Image img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream os(out);
    const bool ok = os.openedOk() && juce::PNGImageFormat().writeImageToStream(img, os);
    std::printf("%s %dx%d -> %s\n", ok ? "wrote" : "FAILED", img.getWidth(), img.getHeight(), out.getFullPathName().toRawUTF8());
    ed.reset();
    proc.reset();
    return ok ? 0 : 1;
}
