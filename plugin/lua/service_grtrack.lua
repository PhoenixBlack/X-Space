--------------------------------------------------------------------------------
-- Ground tracking data
--------------------------------------------------------------------------------
Net.HTTPServer.Handlers["/groundtracking/telemetry"] = function(parameters)
  local downlinkItems = VSFL.GroundTrack.DownlinkedTelemetry[tonumber(parameters["id"]) or 1]
  local response = "{"

  if downlinkItems then
    for k,v in pairs(downlinkItems) do
      local value = v.Value or 0
      response = response.."\""..k.."\":{\"n\":\""..tostring(v.Name).."\",\"v\":\""..tostring(value).."\"},"
    end
    if #response > 1 then response = string.sub(response,1,#response-1) end
  end

  return response.."}"
end

Net.HTTPServer.Handlers["/groundtracking/timesignals"] = function(parameters)
  local telemetry = VSFL.GroundTrack.TimeSignalTelemetry.History
  local response = "{"

  for k,v in ipairs(telemetry) do
    response = response.."\""..k.."\":{"..
      "\"t1\":\""..tostring(v.TimeMarker1 or 0 ).."\","..
      "\"t2\":\""..tostring(v.TimeMarker2 or 0).."\","..
      "\"mx\":\""..tostring(v.Magnetic_X or 0).."\","..
      "\"my\":\""..tostring(v.Magnetic_Y or 0).."\","..
      "\"mz\":\""..tostring(v.Magnetic_Z or 0).."\"},"
  end
  if #response > 1 then response = string.sub(response,1,#response-1) end

  return response.."}"
end

Net.HTTPServer.Handlers["/groundtracking/info"] = function(parameters)
  local xsagID = tonumber(parameters["id"]) or 1
  local response = "{"

  -- Signal levels
  for i=0,16 do
    local signalLevel = VSFL.GroundTrack.Stations[i].SignalLevel[xsagID] or 123
    if signalLevel and (signalLevel > 0.0) then
      response = response.."\"st"..i.."\":"..string.format("%d",signalLevel*10+0.5)..","
    end
  end
  
  response = response.."\"lst\":"..(VSFL.GroundTrack.StationIndex[xsagID] or 0)..","
  response = response.."\"bps\":"..((VSFL.GroundTrack.DataRate[xsagID] or 0)*8)..","
  response = response.."\"uptime\":"..math.floor(curtime())
  response = response.."}"

  return response
end

Net.HTTPServer.Handlers["/groundtracking/uplink"] = function(parameters)
--[[  if parameters["data"] and (parameters["code"] == "362D3CF2") then
    print("Groundtracking: Uplinking command "..parameters["data"])
    local cmd = tonumber(parameters["data"]) or 0
    RadioAPI.TransmitViaGroundstation(lastStation,43,0xAA)
    RadioAPI.TransmitViaGroundstation(lastStation,43,0xAA)
    RadioAPI.TransmitViaGroundstation(lastStation,43,cmd)
    RadioAPI.TransmitViaGroundstation(lastStation,43,(0xAA+0xAA+cmd)%256)
    return "SENT"
  else
    return "ERROR"
  end]]--
end

Net.HTTPServer.Handlers["/groundtracking/scanline"] = function(parameters)
  local response = string.char(1)
--[[  for i=1,512,2 do
    local ch = 0.5*(scanlineData[i] or (127*(1-i%5)))
    if ch < 0 then ch = 0 end
    if ch > 127 then ch = 127 end
    response = response..string.char(ch)
  end ]]--
  return response
end
