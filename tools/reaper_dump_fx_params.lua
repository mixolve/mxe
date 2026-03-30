local outputFile = assert(os.getenv("MX6_REPORT_FILE"), "MX6_REPORT_FILE is required")
local fxName = assert(os.getenv("MX6_FX_NAME"), "MX6_FX_NAME is required")

local function writeFile(path, lines)
  local file = assert(io.open(path, "w"))

  for _, line in ipairs(lines) do
    file:write(line .. "\n")
  end

  file:close()
end

reaper.InsertTrackAtIndex(0, true)

local track = assert(reaper.GetTrack(0, 0), "failed to create track")
local fxIndex = reaper.TrackFX_AddByName(track, fxName, false, 1)
assert(fxIndex >= 0, "failed to add FX: " .. fxName)

local lines = {}
local numParams = reaper.TrackFX_GetNumParams(track, fxIndex)

table.insert(lines, "num_params=" .. tostring(numParams))

for index = 0, numParams - 1 do
  local _, name = reaper.TrackFX_GetParamName(track, fxIndex, index, "")
  local _, formatted = reaper.TrackFX_GetFormattedParamValue(track, fxIndex, index, "")
  table.insert(lines, string.format("param_%d_name=%s", index, name))
  table.insert(lines, string.format("param_%d_formatted=%s", index, formatted))
end

writeFile(outputFile, lines)
