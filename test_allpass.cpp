#include <iostream>
#include <vector>
#include <cmath>

class CorrectDattorroAllpass
{
public:
    void prepare(int delaySamples, float feedbackGain)
    {
        delayLen = delaySamples;
        g = feedbackGain;
        buffer.assign(delaySamples + 10, 0.0f);
        writePos = 0;
    }

    float process(float in)
    {
        int readPos = writePos - delayLen;
        while (readPos < 0) readPos += buffer.size();

        float bufOut = buffer[readPos % buffer.size()];

        // Correct Dattorro equations:
        float v = in + g * bufOut;
        buffer[writePos] = v;
        float out = -g * v + bufOut;

        writePos = (writePos + 1) % buffer.size();
        return out;
    }

private:
    std::vector<float> buffer;
    int delayLen { 100 };
    int writePos { 0 };
    float g { 0.7f };
};

int main()
{
    CorrectDattorroAllpass ap1, ap2, ap3, ap4;
    ap1.prepare(142, 0.75f);
    ap2.prepare(107, 0.75f);
    ap3.prepare(379, 0.625f);
    ap4.prepare(277, 0.625f);

    std::cout << "Testing 4 series Canonical Dattorro Allpasses on sine wave..." << std::endl;

    float maxGain = 0.0f;
    for (int freq = 100; freq <= 10000; freq += 50)
    {
        ap1.prepare(142, 0.75f);
        ap2.prepare(107, 0.75f);
        ap3.prepare(379, 0.625f);
        ap4.prepare(277, 0.625f);

        float maxPeak = 0.0f;
        for (int n = 0; n < 44100; ++n)
        {
            float in = std::sin(2.0f * 3.14159265f * freq * n / 44100.0f);
            float y = ap4.process(ap3.process(ap2.process(ap1.process(in))));
            if (n > 2000 && std::abs(y) > maxPeak) maxPeak = std::abs(y);
        }
        if (maxPeak > maxGain) maxGain = maxPeak;
    }

    std::cout << "Max Peak Gain across all 100Hz-10kHz frequencies for 4 series allpasses: " << maxGain << std::endl;
    return 0;
}
