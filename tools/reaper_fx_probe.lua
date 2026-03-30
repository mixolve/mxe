local outputPath = "/Users/kk/Desktop/YO/code/mx6/tmp/reaper_fx_probe.txt"

local outputFile = assert(io.open(outputPath, "w"))

local function logLine(text)
  outputFile:write(text .. "\n")
  outputFile:flush()
end

logLine("resource_path=" .. reaper.GetResourcePath())

reaper.InsertTrackAtIndex(0, false)
local track = reaper.GetTrack(0, 0)

local candidates = {
  "JS:V3",
  "JS: V3",
  "JS:V3.jsfx",
  "JS: V3.jsfx",
  "VST3:mx6",
  "VST3: mx6",
  "VST3:mx6 (mixolve)",
  "VST3: mx6 (mixolve)"
}

for _, candidate in ipairs(candidates) do
  local fxIndex = reaper.TrackFX_AddByName(track, candidate, false, 1)
  logLine(candidate .. " => " .. tostring(fxIndex))

  if fxIndex >= 0 then
    local _, fxName = reaper.TrackFX_GetFXName(track, fxIndex, "")
    logLine("  resolved=" .. fxName)
    reaper.TrackFX_Delete(track, fxIndex)
  end
end

outputFile:close()
