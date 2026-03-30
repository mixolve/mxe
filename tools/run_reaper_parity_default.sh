#!/bin/zsh

set -euo pipefail

repo_root="/Users/kk/Desktop/YO/code/mx6"
reaper_ini="/Users/kk/Desktop/YO/reaper-the-first/reaper.ini"
reaper_bin_dir="/Users/kk/Desktop/YO/reaper-the-first/REAPER-1.app/Contents/MacOS"
work_dir="${repo_root}/tmp/parity_default"
input_wav="${work_dir}/input_sine.wav"
jsfx_out="${work_dir}/renders/jsfx_default.wav"
mx6_out="${work_dir}/renders/mx6_raw_default.wav"
diff_wav="${work_dir}/renders/diff_default.wav"
stats_file="${work_dir}/null_stats.txt"
alignment_file="${work_dir}/alignment_stats.txt"
renderer_bin=""

mkdir -p "${work_dir}/renders"
rm -f "${input_wav}" "${jsfx_out}" "${mx6_out}" "${diff_wav}" "${stats_file}" "${alignment_file}"
rm -f "${work_dir}"/vst3_default* "${work_dir}/renders/vst3_default.wav"

renderer_bin="$(find "${repo_root}/build" -type f -name mx6_raw_render | head -n 1)"

if [[ -z "${renderer_bin}" || ! -x "${renderer_bin}" ]]; then
  echo "mx6_raw_render binary not found; build the project first" >&2
  exit 1
fi

ffmpeg -hide_banner -loglevel error \
  -f lavfi -i "sine=frequency=997:sample_rate=44100:duration=3" \
  -filter_complex "[0:a]pan=stereo|c0=c0|c1=c0,volume=0.8" \
  -c:a pcm_s24le \
  "${input_wav}"

"${repo_root}/tools/render_reaper_fx.sh" \
  "${reaper_ini}" \
  "${reaper_bin_dir}" \
  "${input_wav}" \
  "JS: V3.jsfx" \
  "${work_dir}" \
  "jsfx_default" >/dev/null

"${renderer_bin}" \
  "${input_wav}" \
  "${mx6_out}" >/dev/null

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

echo "input=${input_wav}"
echo "jsfx=${jsfx_out}"
echo "mx6_raw=${mx6_out}"
echo "diff=${diff_wav}"
echo "stats=${stats_file}"
echo "alignment=${alignment_file}"
