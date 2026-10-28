// Renders the plugin editor offscreen to a PNG (no window is shown).  Used to check the GUI in builds.
//   DvlceDuetSnapshot <out.png> [mode index]
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
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
    if (argc > 4) ed->setSize(std::atoi(argv[3]), std::atoi(argv[4]));
    juce::Image surface(juce::Image::RGB, ed->getWidth(), ed->getHeight(), true);
    juce::Graphics graphics(surface);
    for (int i=0; i<10; ++i) ed->paintEntireComponent(graphics, true);
    double best = 1e9;
    for (int batch=0; batch<5; ++batch) {
        const auto start = juce::Time::getMillisecondCounterHiRes();
        for (int i=0; i<30; ++i) ed->paintEntireComponent(graphics, true);
        best = juce::jmin(best, (juce::Time::getMillisecondCounterHiRes()-start)/30.0);
    }
    std::printf("PAINT %.3f ms at 100%% (target <= 1.5 ms)\n", best);
    const juce::Image img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    out.deleteFile();
    juce::FileOutputStream os(out);
    const bool ok = os.openedOk() && juce::PNGImageFormat().writeImageToStream(img, os);
    std::printf("%s %dx%d -> %s\n", ok ? "wrote" : "FAILED", img.getWidth(), img.getHeight(), out.getFullPathName().toRawUTF8());
    os.flush();
    const auto decoded = juce::ImageFileFormat::loadFrom(out);
    bool verified = decoded.isValid() && decoded.getWidth() == ed->getWidth() && decoded.getHeight() == ed->getHeight();
    bool varied = false;
    for (int y=0; y<decoded.getHeight(); y+=11)
        for (int x=0; x<decoded.getWidth(); x+=11)
            varied = varied || decoded.getPixelAt(x,y) != decoded.getPixelAt(0,0);
    verified = verified && varied;
    ed.reset();
    proc.reset();
    return ok && verified ? 0 : 1;
}
