#!/bin/zsh

set -euo pipefail

repo_root="/Users/kk/Desktop/YO/code/mx6"
reaper_ini="/Users/kk/Desktop/YO/reaper-the-first/reaper.ini"
reaper_bin_dir="/Users/kk/Desktop/YO/reaper-the-first/REAPER-1.app/Contents/MacOS"
sweep_dir="${repo_root}/tmp/param_sweep"
input_wav="${sweep_dir}/input_suite.wav"
summary_file="${sweep_dir}/summary.txt"
renderer_bin="$(find "${repo_root}/build" -type f -name mx6_raw_render | head -n 1)"

if [[ -z "${renderer_bin}" || ! -x "${renderer_bin}" ]]; then
  echo "mx6_raw_render binary not found; build the project first" >&2
  exit 1
fi

parameter_entries=(
  "inGn|-24|24"
  "thLU|-24|0"
  "mkLU|0|24"
  "thLD|-24|0"
  "mkLD|0|24"
  "thRU|-24|0"
  "mkRU|0|24"
  "thRD|-24|0"
  "mkRD|0|24"
  "hwBypass|0|1"
  "LLThResh|-24|0"
  "LLTension|-100|100"
  "LLRelease|0|1000"
  "LLmk|0|24"
  "RRThResh|-24|0"
  "RRTension|-100|100"
  "RRRelease|0|1000"
  "RRmk|0|24"
  "DMbypass|0|1"
  "FFTension|-100|100"
  "FFRelease|0|1000"
  "FFbypass|0|1"
  "moRph|0|100"
  "peakHoldHz|21|3675.1"
  "TensionFlooR|-96|0"
  "TensionHysT|0|100"
  "delTa|0|1"
)

killall REAPER >/dev/null 2>&1 || true
mkdir -p "${sweep_dir}"
rm -rf "${sweep_dir}/cases"
mkdir -p "${sweep_dir}/cases"
rm -f "${summary_file}" "${input_wav}"

ffmpeg -hide_banner -loglevel error \
  -y \
  -f lavfi -i "anullsrc=r=44100:cl=mono:d=0.25" \
  -f lavfi -i "sine=frequency=997:sample_rate=44100:duration=0.45" \
  -f lavfi -i "sine=frequency=211:sample_rate=44100:duration=0.45" \
  -f lavfi -i "anoisesrc=c=white:sample_rate=44100:duration=0.6" \
  -f lavfi -i "aevalsrc=if(eq(mod(n\\,2205)\\,0)\\,0.85\\,0):d=0.35:s=44100" \
  -filter_complex "[1:a]volume=0.10[s1];[2:a]volume=0.80[s2];[3:a]volume=0.22[n];[4:a]lowpass=f=8000,volume=1.0[i];[0:a][s1][s2][n][i]concat=n=5:v=0:a=1[m];[m]pan=stereo|c0=c0|c1=c0[out]" \
  -map "[out]" \
  -c:a pcm_s24le \
  "${input_wav}"

print -r -- "case assignment lag_samples direct_db aligned_db" > "${summary_file}"

run_case() {
  local param_id="$1"
  local label="$2"
  local value="$3"
  local case_name="${param_id}_${label}"
  local assignment="${param_id}=${value}"
  local case_dir="${sweep_dir}/cases/${case_name}"
  local jsfx_out="${case_dir}/renders/jsfx.wav"
  local mx6_out="${case_dir}/renders/mx6_raw.wav"
  local diff_wav="${case_dir}/renders/diff.wav"
  local stats_file="${case_dir}/null_stats.txt"
  local alignment_file="${case_dir}/alignment_stats.txt"
  local lag
  local direct_db
  local aligned_db

  rm -rf "${case_dir}"
  mkdir -p "${case_dir}/renders"

  MX6_PARAM_ASSIGNMENTS="${assignment}" \
    "${repo_root}/tools/render_reaper_fx.sh" \
      "${reaper_ini}" \
      "${reaper_bin_dir}" \
      "${input_wav}" \
      "JS: V3.jsfx" \
      "${case_dir}" \
      "jsfx" >/dev/null

  "${renderer_bin}" \
    "${input_wav}" \
    "${mx6_out}" \
    --param "${assignment}" >/dev/null

  ffmpeg -hide_banner -loglevel info \
    -y \
    -i "${jsfx_out}" \
    -i "${mx6_out}" \
    -filter_complex "[0:a]aformat=sample_fmts=dbl[a0];[1:a]aformat=sample_fmts=dbl,volume=-1[a1];[a0][a1]amix=inputs=2:normalize=0,astats=metadata=1:reset=0" \
    -c:a pcm_f32le \
    "${diff_wav}" \
    > "${stats_file}" 2>&1

  python3 "${repo_root}/tools/analyze_parity.py" \
    "${jsfx_out}" \
    "${mx6_out}" \
    "${alignment_file}"

  lag="$(sed -n 's/^lag_samples=//p' "${alignment_file}")"
  direct_db="$(sed -n 's/^direct_ch1_rms=.*direct_ch1_db=\([^;]*\).*/\1/p' "${alignment_file}")"
  aligned_db="$(sed -n 's/^aligned_ch1_rms=.*aligned_ch1_db=\([^;]*\).*/\1/p' "${alignment_file}")"

  print -r -- "${case_name} ${assignment} ${lag} ${direct_db} ${aligned_db}" >> "${summary_file}"
  print -r -- "${case_name}: lag=${lag} direct_db=${direct_db} aligned_db=${aligned_db}"
}

for entry in "${parameter_entries[@]}"; do
  param_id="${entry%%|*}"
  rest="${entry#*|}"
  min_value="${rest%%|*}"
  max_value="${rest##*|}"

  run_case "${param_id}" min "${min_value}"
  run_case "${param_id}" max "${max_value}"
done

print -r -- "summary=${summary_file}"
