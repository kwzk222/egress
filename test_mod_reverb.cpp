#include <iostream>
#include <vector>
#include <cmath>

// Test modulated figure-8 reverb tank
class ModulatedReverbTank
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        // Prime delay lengths in samples (~30ms to 100ms)
        lenL1 = static_cast<int>(sr * 0.0441); // 44.1ms (1944 samples)
        lenL2 = static_cast<int>(sr * 0.0312); // 31.2ms (1375 samples)
        lenR1 = static_cast<int>(sr * 0.0415); // 41.5ms (1830 samples)
        lenR2 = static_cast<int>(sr * 0.0357); // 35.7ms (1574 samples)

        bufL1.assign(lenL1 + 100, 0.0f);
        bufL2.assign(lenL2 + 100, 0.0f);
        bufR1.assign(lenR1 + 100, 0.0f);
        bufR2.assign(lenR2 + 100, 0.0f);

        writeL1 = writeL2 = writeR1 = writeR2 = 0;
        lfoPhase1 = lfoPhase2 = 0.0f;
    }

    void process(float inL, float inR, float& outL, float& outR, float feedbackGain, float modDepth)
    {
        // Advance LFOs (~0.8 Hz and ~1.1 Hz)
        lfoPhase1 += 0.8f / sr;
        if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;
        lfoPhase2 += 1.1f / sr;
        if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;

        float mod1 = std::sin(lfoPhase1 * 2.0f * 3.14159265f) * modDepth; // +/- modDepth samples
        float mod2 = std::cos(lfoPhase2 * 2.0f * 3.14159265f) * modDepth;

        // Read interpolated modulated samples
        float readPosL1 = static_cast<float>(writeL1) - static_cast<float>(lenL1) + mod1;
        while (readPosL1 < 0) readPosL1 += bufL1.size();
        int iL1 = static_cast<int>(readPosL1);
        float fracL1 = readPosL1 - iL1;
        float nodeL1 = bufL1[iL1 % bufL1.size()] * (1.0f - fracL1) + bufL1[(iL1 + 1) % bufL1.size()] * fracL1;

        float readPosR1 = static_cast<float>(writeR1) - static_cast<float>(lenR1) + mod2;
        while (readPosR1 < 0) readPosR1 += bufR1.size();
        int iR1 = static_cast<int>(readPosR1);
        float fracR1 = readPosR1 - iR1;
        float nodeR1 = bufR1[iR1 % bufR1.size()] * (1.0f - fracR1) + bufR1[(iR1 + 1) % bufR1.size()] * fracR1;

        // Cross feedback
        float nextL = inL + nodeR1 * feedbackGain;
        float nextR = inR + nodeL1 * feedbackGain;

        bufL1[writeL1] = std::tanh(nextL);
        bufR1[writeR1] = std::tanh(nextR);

        writeL1 = (writeL1 + 1) % bufL1.size();
        writeR1 = (writeR1 + 1) % bufR1.size();

        outL = nodeL1 * 0.5f;
        outR = nodeR1 * 0.5f;
    }

private:
    double sr { 44100.0 };
    int lenL1, lenL2, lenR1, lenR2;
    std::vector<float> bufL1, bufL2, bufR1, bufR2;
    int writeL1, writeL2, writeR1, writeR2;
    float lfoPhase1, lfoPhase2;
};

int main()
{
    ModulatedReverbTank tank;
    tank.prepare(44100.0);

    std::cout << "Testing Modulated Reverb Tank with C4 sine wave..." << std::endl;

    for (int block = 0; block < 20; ++block)
    {
        float maxOut = 0.0f;
        for (int s = 0; s < 512; ++s)
        {
            float in = (block < 2) ? std::sin(2.0f * 3.14159265f * 261.63f * (block * 512 + s) / 44100.0f) * 0.4f : 0.0f;
            float outL, outR;
            tank.process(in, in, outL, outR, 0.7f, 8.0f); // 8 samples LFO modulation depth
            if (std::abs(outL) > maxOut) maxOut = std::abs(outL);
        }
        std::cout << "Block " << block << " Max Peak: " << maxOut << std::endl;
    }

    return 0;
}
