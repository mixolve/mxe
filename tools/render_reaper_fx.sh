#!/bin/zsh

set -euo pipefail

if [[ $# -ne 6 ]]; then
  echo "usage: $0 <reaper_ini> <reaper_bin_dir> <input_wav> <fx_name> <work_dir> <output_basename>" >&2
  exit 1
fi

reaper_ini="$1"
reaper_bin_dir="$2"
input_wav="$3"
fx_name="$4"
work_dir="$5"
output_basename="$6"

reaper_bin="${reaper_bin_dir}/REAPER"
mkdir -p "${work_dir}"
work_dir="$(cd "${work_dir}" && pwd)"
input_wav="$(cd "$(dirname "${input_wav}")" && pwd)/$(basename "${input_wav}")"

project_file="${work_dir}/${output_basename}.rpp"
render_dir="${work_dir}/renders"
render_pattern="${output_basename}"
report_file="${work_dir}/${output_basename}.txt"
rendered_wav="${render_dir}/${output_basename}.wav"
create_log="${work_dir}/${output_basename}_create.log"
render_log="${work_dir}/${output_basename}_render.log"
render_done_file="${work_dir}/${output_basename}_render_done.txt"
render_stats_file="${work_dir}/${output_basename}_render_stats.txt"

mkdir -p "${render_dir}"
rm -f "${project_file}" "${report_file}" "${rendered_wav}" "${create_log}" "${render_log}" "${render_done_file}" "${render_stats_file}"

env \
  MX6_PROJECT_FILE="${project_file}" \
  MX6_INPUT_FILE="${input_wav}" \
  MX6_RENDER_DIR="${render_dir}" \
  MX6_RENDER_PATTERN="${render_pattern}" \
  MX6_FX_NAME="${fx_name}" \
  MX6_PARAM_ASSIGNMENTS="${MX6_PARAM_ASSIGNMENTS:-}" \
  MX6_REPORT_FILE="${report_file}" \
  MX6_RENDER_DONE_FILE="${render_done_file}" \
  MX6_RENDER_STATS_FILE="${render_stats_file}" \
  arch -arm64 "${reaper_bin}" \
    -cfgfile "${reaper_ini}" \
    -new \
    "/Users/kk/Desktop/YO/code/mx6/tools/render_reaper_fx.lua" \
    -nosplash \
    > "${render_log}" 2>&1 &

reaper_pid=$!

cleanup() {
  kill "${reaper_pid}" >/dev/null 2>&1 || true
}

trap cleanup EXIT

for _ in {1..300}; do
  if [[ -f "${project_file}" && -f "${report_file}" && -f "${rendered_wav}" && -f "${render_done_file}" ]]; then
    break
  fi

  sleep 0.2
done

kill "${reaper_pid}" >/dev/null 2>&1 || true
wait "${reaper_pid}" >/dev/null 2>&1 || true
trap - EXIT

if [[ ! -f "${project_file}" || ! -f "${report_file}" || ! -f "${rendered_wav}" || ! -f "${render_done_file}" ]]; then
  echo "render output missing for ${fx_name}" >&2
  sed -n '1,160p' "${render_log}" >&2 || true
  exit 1
fi

echo "${rendered_wav}"
