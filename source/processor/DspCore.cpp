#include "DspCore.h"

#include <algorithm>
#include <cmath>

namespace mxe::dsp
{
namespace
{
constexpr double maxLookaheadMs = 24.0;
} // namespace

double DspCore::safeAbs(const double value)
{
    return std::abs(value);
}

double DspCore::clamp1(const double value)
{
    return juce::jlimit(-1.0, 1.0, value);
}

double DspCore::satShape(const double value, const double kneeDb)
{
    if (kneeDb <= epsilon)
        return clamp1(value);

    const auto absoluteValue = safeAbs(value);
    const auto sign = value < 0.0 ? -1.0 : 1.0;

    auto knee = 1.0 - (kneeDb / 24.0) * (2.0 / 3.0);
    knee = juce::jlimit(1.0 / 3.0, 1.0, knee);

    if (absoluteValue <= knee)
        return value;

    if (absoluteValue >= 2.0 - knee)
        return sign;

    const auto delta = absoluteValue - knee;
    return sign * (absoluteValue - (delta * delta) / (4.0 * (1.0 - knee)));
}

double DspCore::tensionTarget(const double env,
                              const double threshold,
                              const double floorThreshold,
                              const double floorHysteresis,
                              const double tension)
{
    if (safeAbs(tension) <= epsilon || threshold <= epsilon)
        return env;

    if (env >= threshold)
        return env;

    const auto amount = safeAbs(tension) * 0.01;
    const auto shape = 1.0 + 42.0 * amount * amount;
    const auto fullRatio = std::min(1.0, env / threshold);

    double shapedFull = env;

    if (tension < 0.0)
        shapedFull = threshold * (1.0 - std::pow(1.0 - fullRatio, shape));
    else if (tension > 0.0)
        shapedFull = threshold * std::pow(fullRatio, shape);

    const auto clampedFloor = juce::jlimit(0.0, threshold, floorThreshold);

    double shapedGate = threshold;

    if (env <= clampedFloor)
    {
        shapedGate = 0.0;
    }
    else if (env <= threshold)
    {
        const auto span = threshold - clampedFloor;

        if (span <= epsilon)
        {
            shapedGate = threshold;
        }
        else
        {
            const auto gateRatio = std::min(1.0, (env - clampedFloor) / span);

            if (tension < 0.0)
                shapedGate = clampedFloor + span * (1.0 - std::pow(1.0 - gateRatio, shape));
            else if (tension > 0.0)
                shapedGate = clampedFloor + span * std::pow(gateRatio, shape);
            else
                shapedGate = env;
        }
    }

    return shapedGate + (shapedFull - shapedGate) * floorHysteresis;
}

void DspCore::prepare(const double sampleRate, const int, const int)
{
    currentSampleRate = sampleRate;

    resizeLookaheadBuffers();
    clearState();
    updateDerivedParameters();
    snapSmoothedParameters();
}

void DspCore::reset()
{
    clearState();
    updateDerivedParameters();
    snapSmoothedParameters();
}

void DspCore::setParameters(const Parameters& newParameters)
{
    parameters = newParameters;
    updateDerivedParameters();
}

void DspCore::beginBlock(const int numSamples)
{
    autoInGain.beginBlock(numSamples);
    autoInRightGain.beginBlock(numSamples);
    autoInLeftGain.beginBlock(numSamples);
    wideAmount.beginBlock(numSamples);
}

int DspCore::getLatencySamples() const noexcept
{
    return derived.latencySamples;
}

int DspCore::getMaximumLatencySamples(const double sampleRate) noexcept
{
    const auto safeSampleRate = std::max(1.0, sampleRate);
    const auto maxBufferSize = std::max(1, static_cast<int>(std::ceil(safeSampleRate * maxLookaheadMs * 0.001)) + 2);
    const auto maxTotalBufferSize = std::max(2, maxBufferSize * 2);
    return std::max(0, maxTotalBufferSize - 2);
}
} // namespace mxe::dsp
