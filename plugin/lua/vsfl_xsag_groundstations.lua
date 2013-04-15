--------------------------------------------------------------------------------
-- VSFL Network (XSAG groundstations)
--------------------------------------------------------------------------------
VSFL.GroundTrack = {}

-- Latest downlinked telemetry frame for every channel
VSFL.GroundTrack.DownlinkedTelemetry = {}
VSFL.GroundTrack.DataRate = {}
VSFL.GroundTrack.StationIndex = {}

-- Stores downlinked telemetry for all stations
VSFL.GroundTrack.Stations = {}

-- Stores time signal telemetry
VSFL.GroundTrack.TimeSignalTelemetry = {}
VSFL.GroundTrack.TimeSignalTelemetry.History = {}

-- List of tracking stations
local groundstationsList = {
  [0] =  {   45.177681,                33.374405, "Evpatoriya" },            -- W
  [1] =  {   29.0+33.0/60.0 , -( 95.0+ 5.0/60.0), "Johnsons Space Center" }, -- W
  [2] =  {   36.0+ 8.0/60.0 , -(  5.0+21.0/60.0), "Gilbraltar" },            -- W
  [3] =  {   37.0+44.0/60.0 , -( 25.0+40.0/60.0), "Azores" },                -- W
  [4] =  {    4.0+10.0/60.0,     73.0+30.0/60.0,  "Maldives" },              --
  [5] =  {   14.0+35.0/60.0,    120.0+58.0/60.0,  "Manila" },                --
  [6] =  {   16.0+51.0/60.0,  -( 99.0+52.0/60.0), "Acapulco" },              --
  [7] =  {   51.0+50.0/60.0,    107.0+37.0/60.0,  "Ulan-Ude" },              -- W
  [8] =  { -( 0.0+15.0/60.0), -( 91.0+30.0/60.0), "Galapagos" },             --
  [9] =  { -(41.0+17.0/60.0),   174.0+46.0/60.0,  "Wellington" },            --
  [10] = { -(33.0+55.0/60.0),    18.0+25.0/60.0,  "Cape Town" },             -- W
  [11] = {   46.0+23.0/60.0,     72.0+52.0/60.0,  "Sary-Shagan" },           --
  [12] = {   21.0+34.0/60.0,  -(158.0+16.0/60.0), "Kaena Point" },           -- W
  [13] = {   28.0+12.0/60.0,  -(177.0+21.0/60.0), "Midway Atoll" },          --
  [14] = {   13.0+20.0/60.0,    144.0+40.0/60.0,  "Guam" },                  -- W
  [15] = { -(32.0+ 0.0/60.0),   115.0+45.0/60.0,  "Perth" },                 --
  [16] = {        45.920000,         063.342000,  "Baikonur Pad Service" },
}


--------------------------------------------------------------------------------
-- Receive telemetry on XSAG telemetry channels
--------------------------------------------------------------------------------
-- Decoder state
--   0: Marker byte 1 to be received
--   1: Marker byte 2 to be received
--   2: Index byte to be received
--   3: Check byte to be received
--   4: Low byte
--   5: High byte

function VSFL.GroundTrack.ReceiveTelemetry(xsagID,stationID,channel)
--  if stationID ~= 16 then return end
--  if channel ~= 41 then return end
  if not VSFL.GroundTrack.Stations[stationID] then
    VSFL.GroundTrack.Stations[stationID] = {}
    VSFL.GroundTrack.Stations[stationID].DownlinkedTelemetry = {}
    VSFL.GroundTrack.Stations[stationID].TelemetryDecoderState = {}
    VSFL.GroundTrack.Stations[stationID].TotalErrors = {}
    VSFL.GroundTrack.Stations[stationID].TotalBytes = {}
    VSFL.GroundTrack.Stations[stationID].LastCheckTime = {}
    VSFL.GroundTrack.Stations[stationID].SignalLevel = {}
    VSFL.GroundTrack.Stations[stationID].DataRate = {}
  end
  if not VSFL.GroundTrack.Stations[stationID].DataRate[xsagID] then
    VSFL.GroundTrack.Stations[stationID].DataRate[xsagID] = 0
    VSFL.GroundTrack.Stations[stationID].SignalLevel[xsagID] = 0
    VSFL.GroundTrack.Stations[stationID].TotalErrors[xsagID] = 0
    VSFL.GroundTrack.Stations[stationID].TotalBytes[xsagID] = 0
    VSFL.GroundTrack.Stations[stationID].LastCheckTime[xsagID] = curtime()
  end
  if not VSFL.GroundTrack.Stations[stationID].DownlinkedTelemetry[xsagID] then
    VSFL.GroundTrack.Stations[stationID].DownlinkedTelemetry[xsagID] = {}
  end
  if not VSFL.GroundTrack.Stations[stationID].TelemetryDecoderState[xsagID] then
    VSFL.GroundTrack.Stations[stationID].TelemetryDecoderState[xsagID] = {}
  end
  local station = VSFL.GroundTrack.Stations[stationID]
  local telemetry =  station.DownlinkedTelemetry[xsagID]
  local state = station.TelemetryDecoderState[xsagID]
  
  -- Prune old variables
  for k,v in pairs(telemetry) do
    if (curtime() - v.Time > 30.0) then
      telemetry[k] = nil
    end
  end

  -- Recall decoder mode
  local decoderMode = state.DecoderMode or 0
  local decoderBytes = state.DecoderBytes or {}
  local decoderMajorMode = state.DecoderMajorMode or "UNKNOWN"
  local decoderNameItemIndex = state.decoderNameItemIndex or 0
  local decoderNameSize = state.decoderNameSize or 0
  local decoderNameCRC = state.decoderNameCRC or 0
  
  local netID = 190000+stationID
  local vesselID = VesselAPI.GetByNetID(netID)

  -- Decode telemetry
  local lastByte = RadioAPI.Receive(vesselID,channel)
  local totalBytes = 0
  while ((totalBytes < 1024*512) and (lastByte >= 0)) do
    -- Track statistics
--    station.TotalBytes = station.TotalBytes + 1
--    station.TotalErrors = station.TotalErrors + 1

    -- Decode telemetry
    if decoderMajorMode == "NAME" then
      if decoderMode < decoderNameSize then
        decoderMode = decoderMode + 1
        decoderBytes[decoderMode] = lastByte
      else
        -- Calculate CRC
        local nameCRC = 0
        for i=1,decoderNameSize do
          nameCRC = decoderNameCRC + decoderBytes[i]
        end
        nameCRC = nameCRC % 256

        -- Decode name
        local name = ""
        local isvalid = true
        for i=1,decoderNameSize do
          name = name .. string.char(decoderBytes[i])
          if (decoderBytes[i] < 0x21) or (decoderBytes[i] > 0x7A) then isvalid = false end
        end
        
        -- Write it down
        if not telemetry[decoderNameItemIndex] then
          telemetry[decoderNameItemIndex] = {
            Name = "Item "..decoderNameItemIndex,
            Value = 0.0,
            Time = curtime(),
          }
        end
        telemetry[decoderNameItemIndex].Time = curtime()
        if isvalid == true then
          telemetry[decoderNameItemIndex].Name = name
        end

        if nameCRC ~= decoderNameCRC then
          station.TotalBytes[xsagID] = station.TotalBytes[xsagID] + decoderNameSize
          station.TotalErrors[xsagID] = station.TotalErrors[xsagID] + decoderNameSize
        else
          station.TotalBytes[xsagID] = station.TotalBytes[xsagID] + decoderNameSize
        end

        -- Reset decoder
        decoderMode = 0
        decoderMajorMode = "UNKNOWN"
      end
    else
      if decoderMode < 2 then
        if (lastByte == 0x55) and ((decoderMode == 0) or (decoderMajorMode == "NAME_HEADER")) then
          decoderMode = decoderMode + 1
          decoderMajorMode = "NAME_HEADER"
        elseif (lastByte == 0xAA) and ((decoderMode == 0) or (decoderMajorMode == "DATA")) then
          decoderMode = decoderMode + 1
          decoderMajorMode = "DATA"
        else
          decoderMode = 0
          decoderMajorMode = "UNKNOWN"
        end

      else
        if decoderMajorMode == "DATA" then
          decoderMode = decoderMode + 1
          decoderBytes[decoderMode] = lastByte
          if decoderMode == 8 then
            local targetChecksum = (0xAA + 0xAA +
              decoderBytes[4] + decoderBytes[5] +
              decoderBytes[6] + decoderBytes[7] +
              decoderBytes[8]) % 256
              
            if targetChecksum == decoderBytes[3] then
              local variableIndex = decoderBytes[4] % 128
              local scale = 1
              if decoderBytes[4] > 127 then scale = 1e-5 end
              
              if not telemetry[variableIndex] then
                telemetry[variableIndex] = {
                  Name = "Item "..variableIndex,
                  Value = 0.0,
                  Time = curtime(),
                }
              end
              
              local value = tonumber("0x"..string.format("%02x",decoderBytes[8],2)..
                                           string.format("%02x",decoderBytes[7],2)..
                                           string.format("%02x",decoderBytes[6],2)..
                                           string.format("%02x",decoderBytes[5],2))
              if value > 0x7FFFFFFF then value = value-0x100000000 end
              
              telemetry[variableIndex].Value = value*scale
              telemetry[variableIndex].Time = curtime()
              station.TotalBytes[xsagID] = station.TotalBytes[xsagID] + 8
            else
              station.TotalErrors[xsagID] = station.TotalErrors[xsagID] + 8
              station.TotalBytes[xsagID] = station.TotalBytes[xsagID] + 8
            end
            decoderMode = 0
            decoderMajorMode = "UNKNOWN"
          end
        elseif decoderMajorMode == "NAME_HEADER" then
          decoderMode = decoderMode + 1
          decoderBytes[decoderMode] = lastByte
          if decoderMode == 6 then
            local targetChecksum = (0x55 + 0x55 + decoderBytes[4] + decoderBytes[5] + decoderBytes[6]) % 256
            if targetChecksum == decoderBytes[3] then
              decoderNameItemIndex = decoderBytes[4]
              decoderNameSize = decoderBytes[5]
              decoderNameCRC = decoderBytes[6]
              
              decoderMode = 0
              decoderMajorMode = "NAME"
              station.TotalBytes[xsagID] = station.TotalBytes[xsagID] + 6
            else
              decoderMode = 0
              decoderMajorMode = "UNKNOWN"
              station.TotalErrors[xsagID] = station.TotalErrors[xsagID] + 6
              station.TotalBytes[xsagID] = station.TotalBytes[xsagID] + 6
            end
          end
        end
      end
    end
    
--[[    -- Decode telemetry
    if decoderMode < 2 then
      if lastByte == 0xAA
      then decoderMode = decoderMode + 1
      else decoderMode = 0
      end
    else
      decoderBytes[decoderMode] = lastByte
      decoderMode = decoderMode + 1
      if decoderMode == 6 then
        local targetChecksum = (0xAA + 0xAA + decoderBytes[4] + decoderBytes[5] + decoderBytes[2]) % 256
        if targetChecksum == decoderBytes[3] then
          telemetry[decoderBytes[2] ] = decoderBytes[4]+decoderBytes[5]*256
--          station.TotalErrors = station.TotalErrors - 5
          station.TotalBytes = station.TotalBytes + 5
        else
          station.TotalErrors = station.TotalErrors + 5
          station.TotalBytes = station.TotalBytes + 5
        end
        decoderMode = 0
      end
    end ]]--

    lastByte = RadioAPI.Receive(vesselID,channel)
    totalBytes = totalBytes + 1
  end
  
  -- Remember decoder mode
  state.DecoderMode = decoderMode
  state.DecoderBytes = decoderBytes
  state.DecoderMajorMode = decoderMajorMode
  state.decoderNameItemIndex = decoderNameItemIndex
  state.decoderNameSize = decoderNameSize
  state.decoderNameCRC = decoderNameCRC
  
  -- Check signal level
  if curtime() - (station.LastCheckTime[xsagID] or 0) > 1.0 then
    station.LastCheckTime[xsagID] = curtime()
    
    if station.TotalBytes[xsagID] > 0
    then station.SignalLevel[xsagID] = 1 - (station.TotalErrors[xsagID] / station.TotalBytes[xsagID])
    else station.SignalLevel[xsagID] = 0
    end
    
    station.DataRate[xsagID] = station.TotalBytes[xsagID] - station.TotalErrors[xsagID]
    station.TotalErrors[xsagID] = 0
    station.TotalBytes[xsagID] = 0

--    if station.SignalLevel[xsagID] > 0 then
--      print("SIGNAL",xsagID,station.SignalLevel[xsagID],station.DataRate[xsagID],groundstationsList[stationID][3])
--    end
  end
end

-- Telemetry switcher - selects telemetry from the station with strongest signal
function VSFL.GroundTrack.SwitchTelemetry(xsagID)
  -- Check if pad can receive
  local padCanReceive = false
  local vesselID = VesselAPI.GetByNetID(00021)
  if vesselID then
    padCanReceive = VesselAPI.GetParameter(vesselID,16) < 2500
  end
  
  -- Reset data rate
  VSFL.GroundTrack.DataRate[xsagID] = 0.0
  
  -- Find most strong signal
  local maxSignal = 0.01
  for k,v in pairs(VSFL.GroundTrack.Stations) do
    if (v.SignalLevel[xsagID] > maxSignal) and ((k ~= 16) or (padCanReceive == true)) then
      maxSignal = v.SignalLevel[xsagID]
      VSFL.GroundTrack.DownlinkedTelemetry[xsagID] = v.DownlinkedTelemetry[xsagID]
      VSFL.GroundTrack.DataRate[xsagID] = v.DataRate[xsagID]
      VSFL.GroundTrack.StationIndex[xsagID] = k
      if maxSignal > 0.99 then return end
    end
  end
end


--------------------------------------------------------------------------------
-- Time telemetry receiving
--------------------------------------------------------------------------------
function VSFL.GroundTrack.ReceiveTimeSignals(stationID,channel)
  local telemetry = VSFL.GroundTrack.TimeSignalTelemetry
  local netID = 190000+stationID
  local vesselID = VesselAPI.GetByNetID(netID)
  
  -- Decoder state
  local sequenceByte = telemetry.SequenceByte or 0
  local byteData = telemetry.ByteData or {}

  -- Receive time signals
  local lastByte = RadioAPI.Receive(vesselID,channel)
  local totalBytes = 0
  while ((totalBytes < 1024*512) and (lastByte >= 0)) do
    VSFL.GroundTrack.TimeSignalServer:StreamByte(lastByte)

    sequenceByte = sequenceByte + 1
    byteData[sequenceByte] = lastByte
    if sequenceByte == 128+12 then
      local time_sec_1  = tonumber("0x"..string.format("%02x",byteData[128+4],2)..
                                         string.format("%02x",byteData[128+3],2)..
                                         string.format("%02x",byteData[128+2],2)..
                                         string.format("%02x",byteData[128+1],2))
      local time_part_1 = tonumber("0x"..string.format("%02x",byteData[128+6],2)..
                                         string.format("%02x",byteData[128+5],2))
      local time_sec_2  = tonumber("0x"..string.format("%02x",byteData[128+10],2)..
                                         string.format("%02x",byteData[128+9],2)..
                                         string.format("%02x",byteData[128+8],2)..
                                         string.format("%02x",byteData[128+7],2))
      local time_part_2 = tonumber("0x"..string.format("%02x",byteData[128+12],2)..
                                         string.format("%02x",byteData[128+11],2))
                                         
      local time1 = ((time_sec_1/86400)+55927) + ((time_part_1/86400)/65535)
      local time2 = ((time_sec_2/86400)+55927) + ((time_part_2/86400)/65535)

      if VSFL.GroundTrack.TimeSignalTelemetry.History[16] then
        table.remove(VSFL.GroundTrack.TimeSignalTelemetry.History,16)
      end
      table.insert(VSFL.GroundTrack.TimeSignalTelemetry.History,1,{
        TimeMarker1 = time1,
        TimeMarker2 = time2,
      })
      
      local f = io.open(PluginsFolder.."vsfl/time_signals_" .. stationID .. ".txt", "a+")
      if f then
        f:write(time_sec_1,"\t",time_part_1,"\t",time_sec_2,"\t",time_part_2,"\t",time1,"\t",time2,"\n")
        f:close()
      end
    elseif sequenceByte == 128+18 then
      local mag_x = tonumber("0x"..string.format("%02x",byteData[128+14],2)..
                                   string.format("%02x",byteData[128+13],2))
      local mag_y = tonumber("0x"..string.format("%02x",byteData[128+16],2)..
                                   string.format("%02x",byteData[128+15],2))
      local mag_z = tonumber("0x"..string.format("%02x",byteData[128+18],2)..
                                   string.format("%02x",byteData[128+17],2))
      if mag_x > 0x7FFF then mag_x = mag_x-0x10000 end
      if mag_y > 0x7FFF then mag_y = mag_y-0x10000 end
      if mag_z > 0x7FFF then mag_z = mag_z-0x10000 end
                                   
      if VSFL.GroundTrack.TimeSignalTelemetry.History[1] then
        VSFL.GroundTrack.TimeSignalTelemetry.History[1].Magnetic_X = mag_x
        VSFL.GroundTrack.TimeSignalTelemetry.History[1].Magnetic_Y = mag_y
        VSFL.GroundTrack.TimeSignalTelemetry.History[1].Magnetic_Z = mag_z
        
        local f = io.open(PluginsFolder.."vsfl/time_signals_" .. stationID .. "_magnetic.txt", "a+")
        if f then
          f:write(VSFL.GroundTrack.TimeSignalTelemetry.History[1].TimeMarker1,"\t",
                  VSFL.GroundTrack.TimeSignalTelemetry.History[1].TimeMarker2,"\t",
                  mag_x,"\t",mag_y,"\t",mag_z,"\n")
          f:close()
        end
      end
      sequenceByte = 0
    end
    lastByte = RadioAPI.Receive(vesselID,channel)
    totalBytes = totalBytes + 1
  end
  
  
  -- Save decoder state
  telemetry.SequenceByte = sequenceByte
  telemetry.ByteData = byteData
  
  -- Reset decoder every half seconds
  if curtime() - (telemetry.LastDataTime or 0) > 0.5 then
    telemetry.LastDataTime = curtime()
    telemetry.SequenceByte = 0
    telemetry.ByteData = {}
  end
end


--------------------------------------------------------------------------------
-- Raw video signal receiving
--------------------------------------------------------------------------------
function VSFL.GroundTrack.ReceiveVideoSignals(stationID,channel)
  local netID = 190000+stationID
  local vesselID = VesselAPI.GetByNetID(netID)
  
  local rawOut = io.open(PluginsFolder.."vsfl/video_" .. stationID .. ".mpeg1", "ab+")
  local lastByte = RadioAPI.Receive(vesselID,channel)
  local totalBytes = 0
  while ((totalBytes < 1024*512) and (lastByte >= 0)) do
    -- Stream to TCP port
    VSFL.GroundTrack.VideoStreamServer[stationID]:StreamByte(lastByte)
    
    -- Write to disk
    if rawOut then rawOut:write(string.char(math.min(255,math.max(0,lastByte)))) end
    
    -- Go to next byte
    lastByte = RadioAPI.Receive(vesselID,channel)
    totalBytes = totalBytes + 1
  end
  
  if rawOut then rawOut:close() end
end


--------------------------------------------------------------------------------
-- Write XSAG telemetry
--------------------------------------------------------------------------------
local lastWriteTime = {}
function VSFL.GroundTrack.WriteTelemetry(xsagID)
  if curtime() - (lastWriteTime[xsagID] or 0) > 0.2 then
    if (VSFL.GroundTrack.DataRate[xsagID] or 0) > 0 then
      local f = io.open(PluginsFolder.."vsfl/telemetry_" .. xsagID .. ".txt", "a+")
      if f then
        f:write(curtime())
        for i=0,127 do
          local v = VSFL.GroundTrack.DownlinkedTelemetry[xsagID][i] or {}
          f:write("\t",tostring(v.Value or ""))
        end
        f:write("\n")
        f:close()
      end
    end
    lastWriteTime[xsagID] = curtime()
  end
end


--------------------------------------------------------------------------------
-- Ground tracking stations initializer, software
--------------------------------------------------------------------------------
local groundStationsLoaded = false
AddCallback("frame","VSFL_GroundStations",function(dT)
  if not groundStationsLoaded then
    print("VSFL: Loading XSAG ground stations")
    groundStationsLoaded = true
    
    for k,v in pairs(groundstationsList) do
      local netID = k + 190000
      local vesselID = VesselAPI.GetByNetID(netID)
      if not vesselID then vesselID = VesselAPI.Create({}) end
      
      local R = 6378e3+5e3
      local Lat,Lon = math.rad(v[1]),math.rad(v[2])
      local X = R*math.cos(Lon)*math.cos(Lat)
      local Y = R*math.sin(Lon)*math.cos(Lat)
      local Z = R*math.sin(Lat)
      
      print(string.format("VSFL: [%05d] %s",netID,v[3]))
      
      VesselAPI.SetParameter(vesselID,0,1)
      VesselAPI.SetParameter(vesselID,2,netID)
      VesselAPI.SetParameter(vesselID,50,X)
      VesselAPI.SetParameter(vesselID,51,Y)
      VesselAPI.SetParameter(vesselID,52,Z)
      VesselAPI.SetParameter(vesselID,4,4)
      
      RadioAPI.SetCanChannelReceive(vesselID,8,1)  -- 137.400 MHz
      RadioAPI.SetCanChannelReceive(vesselID,37,1) -- 244.000 MHz
      RadioAPI.SetCanChannelReceive(vesselID,39,1) -- 248.000 MHz
      RadioAPI.SetCanChannelReceive(vesselID,41,1) -- 252.000 MHz
      RadioAPI.SetCanChannelReceive(vesselID,43,1) -- 256.000 MHz
      RadioAPI.SetCanChannelReceive(vesselID,53,1) -- 300.000 MHz
      RadioAPI.SetCanChannelReceive(vesselID,96,1) -- 1640 MHz (video)
      
      -- Evpatoriya special for ES-602
      if k == 0 then RadioAPI.SetCanChannelReceive(vesselID,77,1) end
    end
    
    -- Load servers
    VSFL.GroundTrack.VideoStreamServer = {}
    VSFL.GroundTrack.TimeSignalServer = VSFL.TCP.Create(19500)
    VSFL.GroundTrack.VideoStreamServer[7]  = VSFL.TCP.Create(19521)
    VSFL.GroundTrack.VideoStreamServer[9]  = VSFL.TCP.Create(19523)
    VSFL.GroundTrack.VideoStreamServer[11] = VSFL.TCP.Create(19520)
    VSFL.GroundTrack.VideoStreamServer[14] = VSFL.TCP.Create(19522)
  end

  -- Groundstations software
  for k,v in pairs(groundstationsList) do VSFL.GroundTrack.ReceiveTelemetry(1,k,37) end
  VSFL.GroundTrack.SwitchTelemetry(1)
  for k,v in pairs(groundstationsList) do VSFL.GroundTrack.ReceiveTelemetry(3,k,41) end
  VSFL.GroundTrack.SwitchTelemetry(3)
  
  -- Test for time satellite
  VSFL.GroundTrack.ReceiveTimeSignals(0,77)
  for k,v in pairs(VSFL.GroundTrack.VideoStreamServer) do VSFL.GroundTrack.ReceiveVideoSignals(k,96) end
  
  -- Write telemetry
  VSFL.GroundTrack.WriteTelemetry(1)
  VSFL.GroundTrack.WriteTelemetry(3)
end)
