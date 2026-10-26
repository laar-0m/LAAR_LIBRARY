#include "Engine.h"
#include "Presets.h"

using APVTS = juce::AudioProcessorValueTreeState;

// ================================================================ PROCESSOR
class LaarProcessor : public juce::AudioProcessor
{
public:
    LaarProcessor()
        : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "LAAR", layout())
    {
        laar::Ptrs p;
#define G(n) p.n = apvts.getRawParameterValue(#n);
        G(wtA) G(wtB) G(mix) G(detB) G(uni) G(uniDet) G(sub) G(cut) G(res) G(fEnv) G(fType)
        G(drive) G(fold) G(master) G(aA) G(aD) G(aS) G(aR) G(fA) G(fD) G(fS) G(fR)
#undef G
        synth.addSound(new laar::Sound());
        for (int i = 0; i < 8; ++i) synth.addVoice(new laar::Voice(wt, p));
    }

    static APVTS::ParameterLayout layout()
    {
        APVTS::ParameterLayout L;
        for (auto& d : laar::defs())
        {
            juce::ParameterID id { d.id, 1 };
            if (d.type == 0)
                L.add(std::make_unique<juce::AudioParameterFloat>(id, d.name, juce::NormalisableRange<float>(d.lo, d.hi, 0.f, d.skew), d.def));
            else if (d.type == 1)
                L.add(std::make_unique<juce::AudioParameterInt>(id, d.name, (int) d.lo, (int) d.hi, (int) d.def));
            else
                L.add(std::make_unique<juce::AudioParameterChoice>(id, d.name, juce::StringArray { "LP", "BP", "HP" }, (int) d.def));
        }
        return L;
    }

    void setParam(const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v));
    }

    void applyPreset(int idx)
    {
        auto& pr = laar::presets();
        if (idx < 0 || idx >= (int) pr.size()) return;
        for (auto& d : laar::defs()) setParam(d.id, d.def);
        for (auto& kv : pr[(size_t) idx].v) setParam(kv.first, kv.second);
    }

    void prepareToPlay(double sr, int) override { synth.setCurrentPlaybackSampleRate(sr); }
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

    void processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override
    {
        juce::ScopedNoDenormals nd;
        b.clear();
        synth.renderNextBlock(b, m, 0, b.getNumSamples());
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "laar library"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& d) override
    {
        if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, d);
    }
    void setStateInformation(const void* data, int size) override
    {
        if (auto xml = getXmlFromBinary(data, size)) apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

    laar::Wavetable wt;
    juce::Synthesiser synth;
    APVTS apvts;
};

// ================================================================ UI (rojo crudo)
static const juce::Colour RED(0xFFFF0000), BLK(0xFF000000), WHT(0xFFFFFFFF);

struct RawLNF : juce::LookAndFeel_V4
{
    RawLNF()
    {
        setColour(juce::Label::textColourId, BLK);
        setColour(juce::Slider::textBoxTextColourId, BLK);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, BLK);
        setColour(juce::ComboBox::textColourId, WHT);
        setColour(juce::ComboBox::outlineColourId, BLK);
        setColour(juce::ComboBox::arrowColourId, RED);
        setColour(juce::PopupMenu::backgroundColourId, BLK);
        setColour(juce::PopupMenu::textColourId, WHT);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, RED);
        setColour(juce::PopupMenu::highlightedTextColourId, BLK);
        setColour(juce::TextButton::buttonColourId, BLK);
        setColour(juce::TextButton::textColourOffId, WHT);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(6.f);
        float d = juce::jmin(b.getWidth(), b.getHeight()), r = d / 2.f;
        auto c = b.getCentre();
        g.setColour(BLK);
        g.fillEllipse(c.x - r, c.y - r, d, d);
        float a = a0 + pos * (a1 - a0);
        juce::Path p;
        p.addRectangle(-1.5f, -r, 3.f, r * 0.5f);
        g.setColour(WHT);
        g.fillPath(p, juce::AffineTransform::rotation(a).translated(c));
    }
};

struct Knob : juce::Component
{
    Knob(APVTS& a, const juce::String& id, const juce::String& text)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(s); addAndMakeVisible(l);
        att = std::make_unique<APVTS::SliderAttachment>(a, id, s);
    }
    void resized() override { auto r = getLocalBounds(); l.setBounds(r.removeFromTop(16)); s.setBounds(r); }
    juce::Slider s; juce::Label l;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class WaveView : public juce::Component, private juce::Timer
{
public:
    WaveView(LaarProcessor& p) : proc(p) { startTimerHz(30); }
    void timerCallback() override { repaint(); }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(BLK);
        auto b = getLocalBounds().toFloat().reduced(8.f);
        auto draw = [&](const char* id, juce::Colour c)
        {
            float pos = *proc.apvts.getRawParameterValue(id);
            juce::Path p;
            for (int i = 0; i <= 128; ++i)
            {
                float x = b.getX() + b.getWidth() * (float) i / 128.f;
                float y = b.getCentreY() - proc.wt.read((float) i / 128.f * 0.9999f, pos) * b.getHeight() * 0.45f;
                if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
            }
            g.setColour(c);
            g.strokePath(p, juce::PathStrokeType(1.6f));
        };
        draw("wtB", WHT.withAlpha(0.55f));
        draw("wtA", RED);
    }
private:
    LaarProcessor& proc;
};

class LaarEditor : public juce::AudioProcessorEditor
{
public:
    explicit LaarEditor(LaarProcessor& p) : AudioProcessorEditor(&p), proc(p), wave(p)
    {
        setLookAndFeel(&lnf);
        for (auto& d : laar::defs())
        {
            if (d.type == 2) continue; // filter type -> combo
            auto* k = knobs.add(new Knob(proc.apvts, d.id, d.name));
            addAndMakeVisible(k);
        }
        typeBox.addItemList({ "LP", "BP", "HP" }, 1);
        typeAtt = std::make_unique<APVTS::ComboBoxAttachment>(proc.apvts, "fType", typeBox);
        addAndMakeVisible(typeBox); addAndMakeVisible(wave);

        for (int i = 0; i < laar::categories().size(); ++i) catBox.addItem(laar::categories()[i], i + 1);
        catBox.setSelectedId(1, juce::dontSendNotification);
        catBox.onChange = [this] { fillPresets(true); };
        presetBox.onChange = [this] { int id = presetBox.getSelectedId(); if (id > 0) proc.applyPreset(id - 1); };
        fillPresets(false);
        prev.setButtonText("<"); next.setButtonText(">");
        prev.onClick = [this] { step(-1); };
        next.onClick = [this] { step(1); };
        for (auto* c : std::initializer_list<juce::Component*> { &catBox, &presetBox, &prev, &next }) addAndMakeVisible(c);

        setSize(960, 600);
    }
    ~LaarEditor() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(RED);
        g.setColour(BLK);
        g.setFont(juce::Font(juce::FontOptions(34.f, juce::Font::bold)));
        g.drawText("LAAR LIBRARY", 20, 16, 380, 44, juce::Justification::centredLeft);
        g.fillRect(20, 68, getWidth() - 40, 3);

        g.setFont(juce::Font(juce::FontOptions(13.f, juce::Font::bold)));
        auto title = [&](const char* t, int row, int col) { g.drawText(t, 20 + col * 100, 80 + row * 165, 200, 18, juce::Justification::centredLeft); };
        title("OSC", 0, 0);
        g.drawText("WAVETABLE", 730, 80, 200, 18, juce::Justification::centredLeft);
        title("FILTER", 1, 0); title("DIST", 1, 4); title("OUT", 1, 6);
        title("AMP ENV", 2, 0); title("FILTER ENV", 2, 4);
    }

    void resized() override
    {
        auto slot = [](int row, int col) { return juce::Rectangle<int>(20 + col * 100, 80 + row * 165 + 20, 100, 135); };
        for (int i = 0; i < 7; ++i) knobs[i]->setBounds(slot(0, i));
        wave.setBounds(730, 104, 210, 128);
        typeBox.setBounds(slot(1, 0).reduced(10, 0).withHeight(26).translated(0, 48));
        knobs[7]->setBounds(slot(1, 1)); knobs[8]->setBounds(slot(1, 2)); knobs[9]->setBounds(slot(1, 3));
        knobs[10]->setBounds(slot(1, 4)); knobs[11]->setBounds(slot(1, 5)); knobs[12]->setBounds(slot(1, 6));
        for (int i = 0; i < 4; ++i) { knobs[13 + i]->setBounds(slot(2, i)); knobs[17 + i]->setBounds(slot(2, 4 + i)); }
        catBox.setBounds(430, 22, 140, 28); presetBox.setBounds(580, 22, 200, 28);
        prev.setBounds(790, 22, 32, 28); next.setBounds(828, 22, 32, 28);
    }

private:
    void fillPresets(bool notify)
    {
        presetBox.clear(juce::dontSendNotification);
        auto cat = laar::categories()[catBox.getSelectedId() - 1];
        auto& pr = laar::presets();
        for (size_t i = 0; i < pr.size(); ++i)
            if (pr[i].cat == cat) presetBox.addItem(pr[i].name, (int) i + 1);
        presetBox.setSelectedItemIndex(0, notify ? juce::sendNotification : juce::dontSendNotification);
    }
    void step(int d)
    {
        int n = presetBox.getNumItems(); if (n == 0) return;
        presetBox.setSelectedItemIndex((presetBox.getSelectedItemIndex() + d + n) % n, juce::sendNotification);
    }

    LaarProcessor& proc;
    RawLNF lnf;
    WaveView wave;
    juce::OwnedArray<Knob> knobs;
    juce::ComboBox typeBox, catBox, presetBox;
    juce::TextButton prev, next;
    std::unique_ptr<APVTS::ComboBoxAttachment> typeAtt;
};

juce::AudioProcessorEditor* LaarProcessor::createEditor() { return new LaarEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LaarProcessor(); }
