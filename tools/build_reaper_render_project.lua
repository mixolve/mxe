local projectFile = assert(os.getenv("MX6_PROJECT_FILE"), "MX6_PROJECT_FILE is required")
local inputFile = assert(os.getenv("MX6_INPUT_FILE"), "MX6_INPUT_FILE is required")
local renderDir = assert(os.getenv("MX6_RENDER_DIR"), "MX6_RENDER_DIR is required")
local renderPattern = assert(os.getenv("MX6_RENDER_PATTERN"), "MX6_RENDER_PATTERN is required")
local fxName = assert(os.getenv("MX6_FX_NAME"), "MX6_FX_NAME is required")
local reportFile = assert(os.getenv("MX6_REPORT_FILE"), "MX6_REPORT_FILE is required")

local function writeReport(lines)
  local file = assert(io.open(reportFile, "w"))

  for _, line in ipairs(lines) do
    file:write(line .. "\n")
  end

  file:close()
end

local report = {
  "resource_path=" .. reaper.GetResourcePath(),
  "project_file=" .. projectFile,
  "input_file=" .. inputFile,
  "render_dir=" .. renderDir,
  "render_pattern=" .. renderPattern,
  "fx_name=" .. fxName
}

reaper.InsertMedia(inputFile, 1)

local track = reaper.GetTrack(0, 0)
assert(track, "failed to create track from media import")

local fxIndex = reaper.TrackFX_AddByName(track, fxName, false, 1)
assert(fxIndex >= 0, "failed to add FX: " .. fxName)

local _, resolvedFxName = reaper.TrackFX_GetFXName(track, fxIndex, "")
table.insert(report, "resolved_fx_name=" .. resolvedFxName)

reaper.GetSetProjectInfo(0, "PROJECT_SRATE", 44100, true)
reaper.GetSetProjectInfo(0, "PROJECT_SRATE_USE", 1, true)
reaper.GetSetProjectInfo(0, "RENDER_SRATE", 44100, true)
reaper.GetSetProjectInfo(0, "RENDER_CHANNELS", 2, true)
reaper.GetSetProjectInfo(0, "RENDER_SETTINGS", 0, true)
reaper.GetSetProjectInfo(0, "RENDER_BOUNDSFLAG", 1, true)
reaper.GetSetProjectInfo(0, "RENDER_ADDTOPROJ", 0, true)
reaper.GetSetProjectInfo(0, "RENDER_DITHER", 16, true)

reaper.GetSetProjectInfo_String(0, "RENDER_FILE", renderDir, true)
reaper.GetSetProjectInfo_String(0, "RENDER_PATTERN", renderPattern, true)
reaper.GetSetProjectInfo_String(0, "RENDER_FORMAT", "wave", true)
reaper.GetSetProjectInfo_String(0, "RENDER_FORMAT2", "", true)

reaper.Main_SaveProjectEx(0, projectFile, 8)

local _, renderTargets = reaper.GetSetProjectInfo_String(0, "RENDER_TARGETS", "", false)
table.insert(report, "render_targets=" .. renderTargets)

writeReport(report)
