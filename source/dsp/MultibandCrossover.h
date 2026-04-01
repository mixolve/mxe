#pragma once

#include <array>

namespace mx6::dsp
{
class MultibandCrossover
{
public:
    static constexpr size_t numBands = 6;
    static constexpr size_t numSplits = 5;

    struct StereoSample
    {
        double left = 0.0;
        double right = 0.0;
    };

    using BandArray = std::array<StereoSample, numBands>;

    void prepare(double sampleRate);
    void reset();
    void setSplitFrequencies(const std::array<double, numSplits>& newFrequencies);
    BandArray processSample(double leftInput, double rightInput);

private:
    struct BiquadCoefficients
    {
        double b0 = 0.0;
        double b1 = 0.0;
        double b2 = 0.0;
        double a1 = 0.0;
        double a2 = 0.0;
    };

    struct BiquadState
    {
        double s1 = 0.0;
        double s2 = 0.0;
    };

    using Cascade = std::array<BiquadCoefficients, 4>;
    using CascadeState = std::array<BiquadState, 4>;

    static constexpr size_t numCompensators = 10;

    static double clampFrequency(double frequency, double sampleRate);
    static BiquadCoefficients makeLowpass(double sampleRate, double frequency, double q);
    static BiquadCoefficients makeHighpass(double sampleRate, double frequency, double q);
    static size_t compIndex(size_t splitIndex, size_t bandIndex);
    static double processCascade(double input, const Cascade& coefficients, CascadeState& state);

    void updateSplit(size_t splitIndex, double frequency);

    double currentSampleRate = 44100.0;
    std::array<double, numSplits> frequencies { 134.0, 523.0, 2093.0, 5000.0, 10000.0 };
    std::array<Cascade, numSplits> lowpassCoefficients {};
    std::array<Cascade, numSplits> highpassCoefficients {};

    std::array<CascadeState, numSplits> mainLpLeft {};
    std::array<CascadeState, numSplits> mainLpRight {};
    std::array<CascadeState, numSplits> mainHpLeft {};
    std::array<CascadeState, numSplits> mainHpRight {};

    std::array<CascadeState, numCompensators> compLpLeft {};
    std::array<CascadeState, numCompensators> compLpRight {};
    std::array<CascadeState, numCompensators> compHpLeft {};
    std::array<CascadeState, numCompensators> compHpRight {};
};
} // namespace mx6::dsp
