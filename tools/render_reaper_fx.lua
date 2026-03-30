local projectFile = assert(os.getenv("MX6_PROJECT_FILE"), "MX6_PROJECT_FILE is required")
local inputFile = assert(os.getenv("MX6_INPUT_FILE"), "MX6_INPUT_FILE is required")
local renderDir = assert(os.getenv("MX6_RENDER_DIR"), "MX6_RENDER_DIR is required")
local renderPattern = assert(os.getenv("MX6_RENDER_PATTERN"), "MX6_RENDER_PATTERN is required")
local fxName = assert(os.getenv("MX6_FX_NAME"), "MX6_FX_NAME is required")
local reportFile = assert(os.getenv("MX6_REPORT_FILE"), "MX6_REPORT_FILE is required")
local doneFile = assert(os.getenv("MX6_RENDER_DONE_FILE"), "MX6_RENDER_DONE_FILE is required")
local statsFile = assert(os.getenv("MX6_RENDER_STATS_FILE"), "MX6_RENDER_STATS_FILE is required")
local paramAssignments = os.getenv("MX6_PARAM_ASSIGNMENTS") or ""

local parameterDefs = {
  inGn = { index = 0, min = -24.0, max = 24.0 },
  thLU = { index = 1, min = -24.0, max = 0.0 },
  mkLU = { index = 2, min = 0.0, max = 24.0 },
  thLD = { index = 3, min = -24.0, max = 0.0 },
  mkLD = { index = 4, min = 0.0, max = 24.0 },
  thRU = { index = 5, min = -24.0, max = 0.0 },
  mkRU = { index = 6, min = 0.0, max = 24.0 },
  thRD = { index = 7, min = -24.0, max = 0.0 },
  mkRD = { index = 8, min = 0.0, max = 24.0 },
  hwBypass = { index = 9, min = 0.0, max = 1.0 },
  LLThResh = { index = 10, min = -24.0, max = 0.0 },
  LLTension = { index = 11, min = -100.0, max = 100.0 },
  LLRelease = { index = 12, min = 0.0, max = 1000.0 },
  LLmk = { index = 13, min = 0.0, max = 24.0 },
  RRThResh = { index = 14, min = -24.0, max = 0.0 },
  RRTension = { index = 15, min = -100.0, max = 100.0 },
  RRRelease = { index = 16, min = 0.0, max = 1000.0 },
  RRmk = { index = 17, min = 0.0, max = 24.0 },
  DMbypass = { index = 18, min = 0.0, max = 1.0 },
  FFTension = { index = 19, min = -100.0, max = 100.0 },
  FFRelease = { index = 20, min = 0.0, max = 1000.0 },
  FFbypass = { index = 21, min = 0.0, max = 1.0 },
  moRph = { index = 22, min = 0.0, max = 100.0 },
  peakHoldHz = { index = 23, min = 21.0, max = 3675.1 },
  TensionFlooR = { index = 24, min = -96.0, max = 0.0 },
  TensionHysT = { index = 25, min = 0.0, max = 100.0 },
  delTa = { index = 26, min = 0.0, max = 1.0 },
}

local function writeFile(path, lines)
  local file = assert(io.open(path, "w"))

  for _, line in ipairs(lines) do
    file:write(line .. "\n")
  end

  file:close()
end

local function trim(text)
  return text:match("^%s*(.-)%s*$")
end

local function splitAssignments(assignments)
  local result = {}

  for entry in assignments:gmatch("[^;]+") do
    local trimmed = trim(entry)

    if trimmed ~= "" then
      table.insert(result, trimmed)
    end
  end

  return result
end

local function clamp(value, minimum, maximum)
  if value < minimum then
    return minimum
  end

  if value > maximum then
    return maximum
  end

  return value
end

local function applyAssignments(track, fxIndex, assignments, report)
  for _, assignment in ipairs(assignments) do
    local key, valueText = assignment:match("^([^=]+)=(.+)$")
    assert(key ~= nil and valueText ~= nil, "invalid parameter assignment: " .. assignment)

    key = trim(key)
    valueText = trim(valueText)

    local definition = parameterDefs[key]
    assert(definition ~= nil, "unknown parameter id: " .. key)

    local value = tonumber(valueText)
    assert(value ~= nil, "invalid numeric value for " .. key .. ": " .. valueText)

    local normalized = 0.0
    local range = definition.max - definition.min

    if math.abs(range) > 1.0e-12 then
      normalized = clamp((value - definition.min) / range, 0.0, 1.0)
    end

    assert(reaper.TrackFX_SetParamNormalized(track, fxIndex, definition.index, normalized), "failed to set parameter " .. key)
    table.insert(report, string.format("param_%s=%s", key, valueText))
  end
end

local report = {
  "resource_path=" .. reaper.GetResourcePath(),
  "project_file=" .. projectFile,
  "input_file=" .. inputFile,
  "render_dir=" .. renderDir,
  "render_pattern=" .. renderPattern,
  "fx_name=" .. fxName,
  "render_action=42230"
}

reaper.InsertMedia(inputFile, 1)

local track = reaper.GetTrack(0, 0)
assert(track, "failed to create track from media import")

local fxIndex = reaper.TrackFX_AddByName(track, fxName, false, 1)
assert(fxIndex >= 0, "failed to add FX: " .. fxName)

local _, resolvedFxName = reaper.TrackFX_GetFXName(track, fxIndex, "")
table.insert(report, "resolved_fx_name=" .. resolvedFxName)

applyAssignments(track, fxIndex, splitAssignments(paramAssignments), report)

reaper.GetSetProjectInfo(0, "PROJECT_SRATE", 44100, true)
reaper.GetSetProjectInfo(0, "PROJECT_SRATE_USE", 1, true)
reaper.GetSetProjectInfo(0, "RENDER_SRATE", 44100, true)
reaper.GetSetProjectInfo(0, "RENDER_CHANNELS", 2, true)
reaper.GetSetProjectInfo(0, "RENDER_SETTINGS", 0, true)
reaper.GetSetProjectInfo(0, "RENDER_BOUNDSFLAG", 1, true)
reaper.GetSetProjectInfo(0, "RENDER_ADDTOPROJ", 0, true)
reaper.GetSetProjectInfo(0, "RENDER_DITHER", 0, true)

reaper.GetSetProjectInfo_String(0, "RENDER_FILE", renderDir, true)
reaper.GetSetProjectInfo_String(0, "RENDER_PATTERN", renderPattern, true)
reaper.GetSetProjectInfo_String(0, "RENDER_FORMAT", "evaw", true)
reaper.GetSetProjectInfo_String(0, "RENDER_FORMAT2", "", true)

reaper.Main_SaveProjectEx(0, projectFile, 8)

local _, renderTargetsBefore = reaper.GetSetProjectInfo_String(0, "RENDER_TARGETS", "", false)
table.insert(report, "render_targets_before=" .. renderTargetsBefore)
writeFile(reportFile, report)

reaper.Main_OnCommand(42230, 0)

local stats = {}
local _, renderTargetsAfter = reaper.GetSetProjectInfo_String(0, "RENDER_TARGETS", "", false)
local _, renderStats = reaper.GetSetProjectInfo_String(0, "RENDER_STATS", "", false)
local _, renderSummary = reaper.GetSetProjectInfo_String(0, "RENDER_STATS_SUMMARY", "", false)

table.insert(stats, "render_targets_after=" .. renderTargetsAfter)
table.insert(stats, "render_stats=" .. renderStats)
table.insert(stats, "render_summary=" .. renderSummary)

writeFile(statsFile, stats)
writeFile(doneFile, { "done" })
