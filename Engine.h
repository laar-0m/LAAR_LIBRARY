#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace laar
{
// ---------------------------------------------------------------- parameters
struct PDef { const char* id; const char* name; int type; float lo, hi, def, skew; }; // type 0 float, 1 int, 2 choice

inline const std::vector<PDef>& defs()
{
    static const std::vector<PDef> d = {
        {"wtA",    "WT A",       0, 0,     1,     0.0f,   1.0f},
        {"wtB",    "WT B",       0, 0,     1,     0.3f,   1.0f},
        {"mix",    "A/B MIX",    0, 0,     1,     0.0f,   1.0f},
        {"detB",   "B SEMI",     0, -24,   24,    0.0f,   1.0f},
        {"uni",    "UNISON",     1, 1,     7,     1,      1.0f},
        {"uniDet", "UNI DETUNE", 0, 0,     100,   15.0f,  1.0f},
        {"sub",    "SUB",        0, 0,     1,     0.0f,   1.0f},
        {"cut",    "CUTOFF",     0, 20,    20000, 8000.f, 0.3f},
        {"res",    "RESO",       0, 0,     1,     0.1f,   1.0f},
        {"fEnv",   "ENV AMT",    0, -1,    1,     0.0f,   1.0f},
        {"fType",  "TYPE",       2, 0,     2,     0,      1.0f},
        {"drive",  "DRIVE",      0, 0,     1,     0.0f,   1.0f},
        {"fold",   "FOLD",       0, 0,     1,     0.0f,   1.0f},
        {"master", "MASTER",     0, 0,     1,     0.7f,   1.0f},
        {"aA",     "ATTACK",     0, 0.001f, 5,    0.005f, 0.4f},
        {"aD",     "DECAY",      0, 0.001f, 5,    0.3f,   0.4f},
        {"aS",     "SUSTAIN",    0, 0,     1,     0.8f,   1.0f},
        {"aR",     "RELEASE",    0, 0.001f, 8,    0.2f,   0.4f},
        {"fA",     "ATTACK",     0, 0.001f, 5,    0.005f, 0.4f},
        {"fD",     "DECAY",      0, 0.001f, 5,    0.3f,   0.4f},
        {"fS",     "SUSTAIN",    0, 0,     1,     0.5f,   1.0f},
        {"fR",     "RELEASE",    0, 0.001f, 8,    0.2f,   0.4f},
    };
    return d;
}

struct Ptrs
{
    std::atomic<float> *wtA, *wtB, *mix, *detB, *uni, *uniDet, *sub, *cut, *res, *fEnv, *fType,
                       *drive, *fold, *master, *aA, *aD, *aS, *aR, *fA, *fD, *fS, *fR;
};

// ----------------------------------------------------------------- wavetable
// 8 frames: sine, tri, saw, square, pulse, folded saw, hard-sync, "real under" (saturated additive)
// Nota: tablas sin band-limiting (suena crudo/agresivo a proposito, con aliasing en notas agudas).
struct Wavetable
{
    static constexpr int frames = 8, size = 2048;
    std::array<std::array<float, size + 1>, frames> t {};

    Wavetable()
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (int f = 0; f < frames; ++f)
        {
            float peak = 0.0001f;
            for (int i = 0; i < size; ++i)
            {
                float ph = (float) i / size, s = 0.f;
                switch (f)
                {
                    case 0: s = std::sin(twoPi * ph); break;
                    case 1: s = 4.f * std::abs(ph - 0.5f) - 1.f; break;
                    case 2: s = 2.f * ph - 1.f; break;
                    case 3: s = ph < 0.5f ? 1.f : -1.f; break;
                    case 4: s = ph < 0.25f ? 1.f : -1.f; break;
                    case 5: s = std::sin((2.f * ph - 1.f) * 3.f * juce::MathConstants<float>::halfPi * 2.f); break;
                    case 6: s = 2.f * std::fmod(ph * 3.f, 1.f) - 1.f; break;
                    case 7: s = std::tanh(6.f * (std::sin(twoPi * ph) + 0.8f * std::sin(twoPi * 2.f * ph + 1.f)
                                               + 0.6f * std::sin(twoPi * 3.f * ph + 2.f))); break;
                }
                t[(size_t) f][(size_t) i] = s;
                peak = std::max(peak, std::abs(s));
            }
            for (int i = 0; i < size; ++i) t[(size_t) f][(size_t) i] /= peak;
            t[(size_t) f][size] = t[(size_t) f][0];
        }
    }

    // ph 0..1, pos 0..1 (morph entre frames)
    float read(float ph, float pos) const
    {
        float fp = juce::jlimit(0.f, 1.f, pos) * (frames - 1);
        int f0 = (int) fp, f1 = std::min(f0 + 1, frames - 1);
        float fr = fp - (float) f0, x = ph * size;
        int i = (int) x; float fi = x - (float) i;
        auto rd = [&](int f) { return t[(size_t) f][(size_t) i] * (1.f - fi) + t[(size_t) f][(size_t) i + 1] * fi; };
        return rd(f0) * (1.f - fr) + rd(f1) * fr;
    }
};

// --------------------------------------------------------------------- voice
struct Sound : juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class Voice : public juce::SynthesiserVoice
{
public:
    Voice(const Wavetable& w, const Ptrs& p) : wt(w), P(p) {}

    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<Sound*>(s) != nullptr; }

    void startNote(int midi, float vel, juce::SynthesiserSound*, int pw) override
    {
        note = midi; level = vel; bend = (float) (pw - 8192) / 8192.f * 2.f;
        for (int k = 0; k < 7; ++k) { phA[k] = k == 0 ? 0.f : rnd.nextFloat(); phB[k] = k == 0 ? 0.f : rnd.nextFloat(); }
        subPh = 0.f;
        const double sr = getSampleRate();
        juce::dsp::ProcessSpec spec { sr, 512u, 1u };
        filt.prepare(spec); filt.reset();
        amp.setSampleRate(sr); flt.setSampleRate(sr);
        setEnv(); amp.noteOn(); flt.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff) { amp.noteOff(); flt.noteOff(); }
        else { amp.reset(); flt.reset(); clearCurrentNote(); }
    }

    void pitchWheelMoved(int v) override { bend = (float) (v - 8192) / 8192.f * 2.f; }
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (!amp.isActive()) return;
        setEnv();

        const double sr = getSampleRate();
        const int n = (int) *P.uni;
        const float det = *P.uniDet, wA = *P.wtA, wB = *P.wtB, mixAB = *P.mix, subL = *P.sub;
        const float drive = *P.drive, fold = *P.fold, res = *P.res, envAmt = *P.fEnv;
        const float cut = *P.cut, master = *P.master;
        const double f = 440.0 * std::pow(2.0, (note - 69 + bend) / 12.0);

        float incA[7], incB[7];
        for (int k = 0; k < n; ++k)
        {
            float c = n > 1 ? ((float) k / (float) (n - 1) * 2.f - 1.f) * det : 0.f;
            incA[k] = (float) (f * std::pow(2.0, c / 1200.0) / sr);
            incB[k] = (float) (f * std::pow(2.0, *P.detB / 12.0 - c * 0.7 / 1200.0) / sr);
        }
        const float subInc = (float) (f * 0.5 / sr), norm = 1.f / std::sqrt((float) n);
        const int type = (int) *P.fType;
        filt.setType(type == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                   : type == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                               : juce::dsp::StateVariableTPTFilterType::highpass);
        filt.setResonance(0.7f + res * res * 14.f);

        for (int i = 0; i < num; ++i)
        {
            const float fe = flt.getNextSample(), ae = amp.getNextSample();
            if ((i & 7) == 0)
                filt.setCutoffFrequency(juce::jlimit(20.f, 18000.f, cut * std::pow(2.f, envAmt * fe * 5.f)));

            float sA = 0.f, sB = 0.f;
            for (int k = 0; k < n; ++k)
            {
                sA += wt.read(phA[k], wA); sB += wt.read(phB[k], wB);
                phA[k] += incA[k]; phA[k] -= std::floor(phA[k]);
                phB[k] += incB[k]; phB[k] -= std::floor(phB[k]);
            }
            float s = (sA * (1.f - mixAB) + sB * mixAB) * norm;
            s += subL * std::sin(juce::MathConstants<float>::twoPi * subPh);
            subPh += subInc; subPh -= std::floor(subPh);

            if (fold > 0.001f) s = std::sin(s * (1.f + fold * 5.f) * juce::MathConstants<float>::halfPi);
            s = filt.processSample(0, s);
            s = std::tanh(s * (1.f + drive * 14.f)) * (1.f - 0.3f * drive);

            const float y = s * ae * level * master * 0.5f;
            for (int ch = 0; ch < out.getNumChannels(); ++ch) out.addSample(ch, start + i, y);
        }
        if (!amp.isActive()) clearCurrentNote();
    }

private:
    void setEnv()
    {
        amp.setParameters({ *P.aA, *P.aD, *P.aS, *P.aR });
        flt.setParameters({ *P.fA, *P.fD, *P.fS, *P.fR });
    }

    const Wavetable& wt; Ptrs P;
    juce::ADSR amp, flt;
    juce::dsp::StateVariableTPTFilter<float> filt;
    juce::Random rnd;
    float phA[7] {}, phB[7] {}, subPh = 0.f, level = 0.f, bend = 0.f;
    int note = 60;
};
} // namespace laar
