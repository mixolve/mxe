#!/bin/zsh

set -euo pipefail

repo_root="/Users/kk/Desktop/YO/code/mx6"
reaper_ini="/Users/kk/Desktop/YO/reaper-the-first/reaper.ini"
reaper_bin_dir="/Users/kk/Desktop/YO/reaper-the-first/REAPER-1.app/Contents/MacOS"
suite_dir="${repo_root}/tmp/parity_suite"
input_wav="${suite_dir}/input_suite.wav"
summary_file="${suite_dir}/summary.txt"
renderer_bin="$(find "${repo_root}/build" -type f -name mx6_raw_render | head -n 1)"

if [[ -z "${renderer_bin}" || ! -x "${renderer_bin}" ]]; then
  echo "mx6_raw_render binary not found; build the project first" >&2
  exit 1
fi

killall REAPER >/dev/null 2>&1 || true
mkdir -p "${suite_dir}"
rm -rf "${suite_dir}/cases"
mkdir -p "${suite_dir}/cases"
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

case_entries=(
  "default|"
  "half_wave|hwBypass=0;thLU=-18;mkLU=9;thLD=-15;mkLD=6;thRU=-12;mkRU=4;thRD=-9;mkRD=8"
  "dm_stage|DMbypass=0;LLThResh=-14;LLTension=35;LLRelease=80;LLmk=5;RRThResh=-10;RRTension=-45;RRRelease=120;RRmk=3;peakHoldHz=180;moRph=25"
  "ff_stage|FFbypass=0;FFTension=60;FFRelease=95;peakHoldHz=300;moRph=70"
  "full_stack|hwBypass=0;DMbypass=0;FFbypass=0;thLU=-18;mkLU=8;thLD=-14;mkLD=6;thRU=-17;mkRU=7;thRD=-13;mkRD=5;LLThResh=-12;LLTension=40;LLRelease=90;LLmk=4;RRThResh=-11;RRTension=-35;RRRelease=130;RRmk=2.5;FFTension=55;FFRelease=70;moRph=62;peakHoldHz=420;TensionFlooR=-48;TensionHysT=22"
  "delta_full|hwBypass=0;DMbypass=0;FFbypass=0;thLU=-18;mkLU=8;thLD=-14;mkLD=6;thRU=-17;mkRU=7;thRD=-13;mkRD=5;LLThResh=-12;LLTension=40;LLRelease=90;LLmk=4;RRThResh=-11;RRTension=-35;RRRelease=130;RRmk=2.5;FFTension=55;FFRelease=70;moRph=62;peakHoldHz=420;TensionFlooR=-48;TensionHysT=22;delTa=1"
  "extreme_hold|DMbypass=0;FFbypass=0;LLThResh=-6;RRThResh=-6;LLTension=100;RRTension=-100;FFTension=100;LLRelease=0;RRRelease=0;FFRelease=0;moRph=100;peakHoldHz=3675.1;TensionFlooR=-24;TensionHysT=100"
)

print -r -- "case lag_samples direct_db aligned_db" > "${summary_file}"

run_case() {
  local case_name="$1"
  local assignments="$2"
  local case_dir="${suite_dir}/cases/${case_name}"
  local jsfx_out="${case_dir}/renders/jsfx.wav"
  local mx6_out="${case_dir}/renders/mx6_raw.wav"
  local diff_wav="${case_dir}/renders/diff.wav"
  local stats_file="${case_dir}/null_stats.txt"
  local alignment_file="${case_dir}/alignment_stats.txt"
  local -a raw_args

  rm -rf "${case_dir}"
  mkdir -p "${case_dir}/renders"

  MX6_PARAM_ASSIGNMENTS="${assignments}" \
    "${repo_root}/tools/render_reaper_fx.sh" \
      "${reaper_ini}" \
      "${reaper_bin_dir}" \
      "${input_wav}" \
      "JS: V3.jsfx" \
      "${case_dir}" \
      "jsfx" >/dev/null

  raw_args=("${renderer_bin}" "${input_wav}" "${mx6_out}")

  if [[ -n "${assignments}" ]]; then
    local assignment

    for assignment in ${(s:;:)assignments}; do
      raw_args+=(--param "${assignment}")
    done
  fi

  "${raw_args[@]}" >/dev/null

  ffmpeg -hide_banner -loglevel info \
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

  local lag
  local direct_db
  local aligned_db

  lag="$(sed -n 's/^lag_samples=//p' "${alignment_file}")"
  direct_db="$(sed -n 's/^direct_ch1_rms=.*direct_ch1_db=\([^;]*\).*/\1/p' "${alignment_file}")"
  aligned_db="$(sed -n 's/^aligned_ch1_rms=.*aligned_ch1_db=\([^;]*\).*/\1/p' "${alignment_file}")"

  print -r -- "${case_name} ${lag} ${direct_db} ${aligned_db}" >> "${summary_file}"
  print -r -- "${case_name}: lag=${lag} direct_db=${direct_db} aligned_db=${aligned_db}"
}

for entry in "${case_entries[@]}"; do
  run_case "${entry%%|*}" "${entry#*|}"
done

print -r -- "summary=${summary_file}"
