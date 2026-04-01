#include "MultibandCrossover.h"

#include <algorithm>
#include <cmath>

namespace mx6::dsp
{
namespace
{
constexpr double qA = 0.541196100146197;
constexpr double qB = 1.306562964876377;
constexpr double minFrequency = 10.0;
constexpr double nyquistMargin = 0.499;
constexpr double pi = 3.14159265358979323846;
} // namespace

double MultibandCrossover::clampFrequency(const double frequency, const double sampleRate)
{
    return std::clamp(frequency, minFrequency, nyquistMargin * sampleRate);
}

MultibandCrossover::BiquadCoefficients MultibandCrossover::makeLowpass(const double sampleRate,
                                                                       const double frequency,
                                                                       const double q)
{
    const auto clampedFrequency = clampFrequency(frequency, sampleRate);
    const auto w0 = 2.0 * pi * clampedFrequency / sampleRate;
    const auto cs = std::cos(w0);
    const auto sn = std::sin(w0);
    const auto alpha = sn / (2.0 * q);

    const auto b0 = (1.0 - cs) * 0.5;
    const auto b1 = 1.0 - cs;
    const auto b2 = (1.0 - cs) * 0.5;
    const auto a0 = 1.0 + alpha;
    const auto a1 = -2.0 * cs;
    const auto a2 = 1.0 - alpha;

    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

MultibandCrossover::BiquadCoefficients MultibandCrossover::makeHighpass(const double sampleRate,
                                                                        const double frequency,
                                                                        const double q)
{
    const auto clampedFrequency = clampFrequency(frequency, sampleRate);
    const auto w0 = 2.0 * pi * clampedFrequency / sampleRate;
    const auto cs = std::cos(w0);
    const auto sn = std::sin(w0);
    const auto alpha = sn / (2.0 * q);

    const auto b0 = (1.0 + cs) * 0.5;
    const auto b1 = -(1.0 + cs);
    const auto b2 = (1.0 + cs) * 0.5;
    const auto a0 = 1.0 + alpha;
    const auto a1 = -2.0 * cs;
    const auto a2 = 1.0 - alpha;

    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

size_t MultibandCrossover::compIndex(const size_t splitIndex, const size_t bandIndex)
{
    return (splitIndex * (splitIndex - 1)) / 2 + bandIndex;
}

double MultibandCrossover::processCascade(double input, const Cascade& coefficients, CascadeState& state)
{
    auto output = input;

    for (size_t stageIndex = 0; stageIndex < coefficients.size(); ++stageIndex)
    {
        const auto& stageCoefficients = coefficients[stageIndex];
        auto& stageState = state[stageIndex];

        const auto t = stageCoefficients.b0 * output + stageState.s1;
        stageState.s1 = stageCoefficients.b1 * output - stageCoefficients.a1 * t + stageState.s2;
        stageState.s2 = stageCoefficients.b2 * output - stageCoefficients.a2 * t;
        output = t;
    }

    return output;
}

void MultibandCrossover::prepare(const double sampleRate)
{
    currentSampleRate = sampleRate;

    for (size_t splitIndex = 0; splitIndex < numSplits; ++splitIndex)
        updateSplit(splitIndex, frequencies[splitIndex]);

    reset();
}

void MultibandCrossover::reset()
{
    mainLpLeft = {};
    mainLpRight = {};
    mainHpLeft = {};
    mainHpRight = {};
    compLpLeft = {};
    compLpRight = {};
    compHpLeft = {};
    compHpRight = {};
}

void MultibandCrossover::setSplitFrequencies(const std::array<double, numSplits>& newFrequencies)
{
    frequencies = newFrequencies;

    for (size_t splitIndex = 0; splitIndex < numSplits; ++splitIndex)
        updateSplit(splitIndex, frequencies[splitIndex]);

    reset();
}

void MultibandCrossover::updateSplit(const size_t splitIndex, const double frequency)
{
    lowpassCoefficients[splitIndex] = {
        makeLowpass(currentSampleRate, frequency, qA),
        makeLowpass(currentSampleRate, frequency, qB),
        makeLowpass(currentSampleRate, frequency, qA),
        makeLowpass(currentSampleRate, frequency, qB),
    };

    highpassCoefficients[splitIndex] = {
        makeHighpass(currentSampleRate, frequency, qA),
        makeHighpass(currentSampleRate, frequency, qB),
        makeHighpass(currentSampleRate, frequency, qA),
        makeHighpass(currentSampleRate, frequency, qB),
    };
}

MultibandCrossover::BandArray MultibandCrossover::processSample(const double leftInput, const double rightInput)
{
    BandArray bands {};

    const auto b1RawLeft = processCascade(leftInput, lowpassCoefficients[0], mainLpLeft[0]);
    const auto b1RawRight = processCascade(rightInput, lowpassCoefficients[0], mainLpRight[0]);
    const auto remainder1Left = processCascade(leftInput, highpassCoefficients[0], mainHpLeft[0]);
    const auto remainder1Right = processCascade(rightInput, highpassCoefficients[0], mainHpRight[0]);

    const auto b2RawLeft = processCascade(remainder1Left, lowpassCoefficients[1], mainLpLeft[1]);
    const auto b2RawRight = processCascade(remainder1Right, lowpassCoefficients[1], mainLpRight[1]);
    const auto remainder2Left = processCascade(remainder1Left, highpassCoefficients[1], mainHpLeft[1]);
    const auto remainder2Right = processCascade(remainder1Right, highpassCoefficients[1], mainHpRight[1]);

    const auto b3RawLeft = processCascade(remainder2Left, lowpassCoefficients[2], mainLpLeft[2]);
    const auto b3RawRight = processCascade(remainder2Right, lowpassCoefficients[2], mainLpRight[2]);
    const auto remainder3Left = processCascade(remainder2Left, highpassCoefficients[2], mainHpLeft[2]);
    const auto remainder3Right = processCascade(remainder2Right, highpassCoefficients[2], mainHpRight[2]);

    const auto b4RawLeft = processCascade(remainder3Left, lowpassCoefficients[3], mainLpLeft[3]);
    const auto b4RawRight = processCascade(remainder3Right, lowpassCoefficients[3], mainLpRight[3]);
    const auto remainder4Left = processCascade(remainder3Left, highpassCoefficients[3], mainHpLeft[3]);
    const auto remainder4Right = processCascade(remainder3Right, highpassCoefficients[3], mainHpRight[3]);

    const auto b5Left = processCascade(remainder4Left, lowpassCoefficients[4], mainLpLeft[4]);
    const auto b5Right = processCascade(remainder4Right, lowpassCoefficients[4], mainLpRight[4]);
    const auto b6Left = processCascade(remainder4Left, highpassCoefficients[4], mainHpLeft[4]);
    const auto b6Right = processCascade(remainder4Right, highpassCoefficients[4], mainHpRight[4]);

    auto c1Left = processCascade(b1RawLeft, lowpassCoefficients[1], compLpLeft[compIndex(1, 0)])
                + processCascade(b1RawLeft, highpassCoefficients[1], compHpLeft[compIndex(1, 0)]);
    auto c1Right = processCascade(b1RawRight, lowpassCoefficients[1], compLpRight[compIndex(1, 0)])
                 + processCascade(b1RawRight, highpassCoefficients[1], compHpRight[compIndex(1, 0)]);

    c1Left = processCascade(c1Left, lowpassCoefficients[2], compLpLeft[compIndex(2, 0)])
           + processCascade(c1Left, highpassCoefficients[2], compHpLeft[compIndex(2, 0)]);
    c1Right = processCascade(c1Right, lowpassCoefficients[2], compLpRight[compIndex(2, 0)])
            + processCascade(c1Right, highpassCoefficients[2], compHpRight[compIndex(2, 0)]);

    c1Left = processCascade(c1Left, lowpassCoefficients[3], compLpLeft[compIndex(3, 0)])
           + processCascade(c1Left, highpassCoefficients[3], compHpLeft[compIndex(3, 0)]);
    c1Right = processCascade(c1Right, lowpassCoefficients[3], compLpRight[compIndex(3, 0)])
            + processCascade(c1Right, highpassCoefficients[3], compHpRight[compIndex(3, 0)]);

    bands[0].left = processCascade(c1Left, lowpassCoefficients[4], compLpLeft[compIndex(4, 0)])
                  + processCascade(c1Left, highpassCoefficients[4], compHpLeft[compIndex(4, 0)]);
    bands[0].right = processCascade(c1Right, lowpassCoefficients[4], compLpRight[compIndex(4, 0)])
                   + processCascade(c1Right, highpassCoefficients[4], compHpRight[compIndex(4, 0)]);

    auto c2Left = processCascade(b2RawLeft, lowpassCoefficients[2], compLpLeft[compIndex(2, 1)])
                + processCascade(b2RawLeft, highpassCoefficients[2], compHpLeft[compIndex(2, 1)]);
    auto c2Right = processCascade(b2RawRight, lowpassCoefficients[2], compLpRight[compIndex(2, 1)])
                 + processCascade(b2RawRight, highpassCoefficients[2], compHpRight[compIndex(2, 1)]);

    c2Left = processCascade(c2Left, lowpassCoefficients[3], compLpLeft[compIndex(3, 1)])
           + processCascade(c2Left, highpassCoefficients[3], compHpLeft[compIndex(3, 1)]);
    c2Right = processCascade(c2Right, lowpassCoefficients[3], compLpRight[compIndex(3, 1)])
            + processCascade(c2Right, highpassCoefficients[3], compHpRight[compIndex(3, 1)]);

    bands[1].left = processCascade(c2Left, lowpassCoefficients[4], compLpLeft[compIndex(4, 1)])
                  + processCascade(c2Left, highpassCoefficients[4], compHpLeft[compIndex(4, 1)]);
    bands[1].right = processCascade(c2Right, lowpassCoefficients[4], compLpRight[compIndex(4, 1)])
                   + processCascade(c2Right, highpassCoefficients[4], compHpRight[compIndex(4, 1)]);

    auto c3Left = processCascade(b3RawLeft, lowpassCoefficients[3], compLpLeft[compIndex(3, 2)])
                + processCascade(b3RawLeft, highpassCoefficients[3], compHpLeft[compIndex(3, 2)]);
    auto c3Right = processCascade(b3RawRight, lowpassCoefficients[3], compLpRight[compIndex(3, 2)])
                 + processCascade(b3RawRight, highpassCoefficients[3], compHpRight[compIndex(3, 2)]);

    bands[2].left = processCascade(c3Left, lowpassCoefficients[4], compLpLeft[compIndex(4, 2)])
                  + processCascade(c3Left, highpassCoefficients[4], compHpLeft[compIndex(4, 2)]);
    bands[2].right = processCascade(c3Right, lowpassCoefficients[4], compLpRight[compIndex(4, 2)])
                   + processCascade(c3Right, highpassCoefficients[4], compHpRight[compIndex(4, 2)]);

    bands[3].left = processCascade(b4RawLeft, lowpassCoefficients[4], compLpLeft[compIndex(4, 3)])
                  + processCascade(b4RawLeft, highpassCoefficients[4], compHpLeft[compIndex(4, 3)]);
    bands[3].right = processCascade(b4RawRight, lowpassCoefficients[4], compLpRight[compIndex(4, 3)])
                   + processCascade(b4RawRight, highpassCoefficients[4], compHpRight[compIndex(4, 3)]);

    bands[4].left = b5Left;
    bands[4].right = b5Right;
    bands[5].left = b6Left;
    bands[5].right = b6Right;

    return bands;
}
} // namespace mx6::dsp
