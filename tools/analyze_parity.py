#!/usr/bin/env python3

import math
import sys
import wave
from pathlib import Path


def read_wav(path: Path):
    with wave.open(str(path), "rb") as wav_file:
        num_frames = wav_file.getnframes()
        num_channels = wav_file.getnchannels()
        sample_width = wav_file.getsampwidth()
        sample_rate = wav_file.getframerate()
        data = wav_file.readframes(num_frames)

    if sample_width != 3:
        raise RuntimeError(f"unsupported sample width: {sample_width}")

    channels = [[] for _ in range(num_channels)]
    frame_stride = sample_width * num_channels

    for frame_offset in range(0, len(data), frame_stride):
        for channel in range(num_channels):
            sample_offset = frame_offset + channel * sample_width
            value = (
                data[sample_offset]
                | (data[sample_offset + 1] << 8)
                | (data[sample_offset + 2] << 16)
            )

            if value & 0x800000:
                value -= 0x1000000

            channels[channel].append(value / float(1 << 23))

    return channels, sample_rate


def first_nonzero_index(samples, threshold=1.0e-9):
    for index, value in enumerate(samples):
        if abs(value) > threshold:
            return index

    return None


def rms(values):
    if not values:
        return 0.0

    return math.sqrt(sum(value * value for value in values) / len(values))


def align_and_diff(jsfx_channels, vst_channels, lag):
    channel_results = []

    for jsfx_channel, vst_channel in zip(jsfx_channels, vst_channels):
        if lag >= 0:
            jsfx_aligned = jsfx_channel[lag:]
            vst_aligned = vst_channel[: len(jsfx_aligned)]
        else:
            jsfx_aligned = jsfx_channel[:lag]
            vst_aligned = vst_channel[-lag:]

        diffs = [a - b for a, b in zip(jsfx_aligned, vst_aligned)]
        channel_results.append(
            {
                "length": len(diffs),
                "peak": max(abs(value) for value in diffs) if diffs else 0.0,
                "rms": rms(diffs),
            }
        )

    return channel_results


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: analyze_parity.py <jsfx_wav> <vst_wav> <report_file>")

    jsfx_path = Path(sys.argv[1])
    vst_path = Path(sys.argv[2])
    report_path = Path(sys.argv[3])

    jsfx_channels, sample_rate = read_wav(jsfx_path)
    vst_channels, vst_sample_rate = read_wav(vst_path)

    if sample_rate != vst_sample_rate:
        raise RuntimeError("sample-rate mismatch")

    reference_first = [first_nonzero_index(channel) for channel in jsfx_channels]
    candidate_first = [first_nonzero_index(channel) for channel in vst_channels]
    lag_candidates = []

    for reference, candidate in zip(reference_first, candidate_first):
        if reference is None and candidate is None:
            continue

        if reference is None or candidate is None:
            raise RuntimeError(
                f"unable to determine lag consistently: reference_first={reference_first}, candidate_first={candidate_first}"
            )

        lag_candidates.append(reference - candidate)

    if not lag_candidates:
        lag = 0
    else:
        if len(set(lag_candidates)) != 1:
            raise RuntimeError(f"channel lag mismatch: {lag_candidates}")

        lag = lag_candidates[0]
    direct = align_and_diff(jsfx_channels, vst_channels, 0)
    aligned = align_and_diff(jsfx_channels, vst_channels, lag)

    lines = [
        f"sample_rate={sample_rate}",
        f"reference_first_nonzero={reference_first}",
        f"candidate_first_nonzero={candidate_first}",
        f"lag_samples={lag}",
        f"lag_ms={lag * 1000.0 / sample_rate:.6f}",
    ]

    for index, result in enumerate(direct, start=1):
        direct_db = 20.0 * math.log10(max(result["rms"], 1.0e-15))
        lines.append(
            f"direct_ch{index}_rms={result['rms']:.12f};direct_ch{index}_db={direct_db:.6f};direct_ch{index}_peak={result['peak']:.12f};direct_ch{index}_length={result['length']}"
        )

    for index, result in enumerate(aligned, start=1):
        aligned_db = 20.0 * math.log10(max(result["rms"], 1.0e-15))
        lines.append(
            f"aligned_ch{index}_rms={result['rms']:.12f};aligned_ch{index}_db={aligned_db:.6f};aligned_ch{index}_peak={result['peak']:.12f};aligned_ch{index}_length={result['length']}"
        )

    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
