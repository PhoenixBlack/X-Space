--============================================================================--
-- X-Space radio transmission related networking
--------------------------------------------------------------------------------
Net.RadioTransmissions = {}

-- Last time at which data was sent
Net.RadioTransmissions.LastTime = 0

-- Buffer of all data to be sent
Net.RadioTransmissions.SendBuffer = {}

-- Packet size
Net.RadioTransmissions.PacketSize = 256


--------------------------------------------------------------------------------
-- Send data from client to server
--------------------------------------------------------------------------------
function Net.RadioTransmissions.SendClient()
  for vesselIdx,vesselBuffers in pairs(Net.RadioTransmissions.SendBuffer) do
    local networkID = VesselAPI.GetParameter(vesselIdx,2)
    for channel,data in pairs(vesselBuffers) do
      local message = NetAPI.NewMessage()
      Net.Write8(message,Net.Message.Transmission)
      -- Write channel
      Net.Write8(message,channel)
      -- Write vessel network ID
      Net.Write32(message,networkID)
      -- Write total count
      Net.Write8(message,#data-1)
      -- Write up to 256 bytes of data
      for i=1,#data do Net.Write8(message,data[i] or 0) end
      -- Send message
      NetAPI.SendMessage(Net.Client.Host,Net.Client.Peer,message)
    end

    -- Nullify the buffer
    Net.RadioTransmissions.SendBuffer[vesselIdx] = {}
  end
end


--------------------------------------------------------------------------------
-- Send data from server to clients
-- FIXME: send data only to clients who ARE RUNNING THIS VESSEL
--------------------------------------------------------------------------------
function Net.RadioTransmissions.SendServer()
  for vesselIdx,vesselBuffers in pairs(Net.RadioTransmissions.SendBuffer) do
    local networkID = VesselAPI.GetParameter(vesselIdx,2)
    for channel,data in pairs(vesselBuffers) do
      for clientID,client in pairs(Net.Server.Clients) do
        local message = NetAPI.NewMessage()
        Net.Write8(message,Net.Message.Transmission)
        -- Write channel
        Net.Write8(message,channel)
        -- Write vessel network ID
        Net.Write32(message,networkID)
        -- Write total count
        Net.Write8(message,#data-1)
        -- Write up to 256 bytes of data
        for i=1,#data do Net.Write8(message,data[i] or 0) end
        -- Send message
        NetAPI.SendMessage(Net.Server.Host,client.Peer,message)
      end
    end

    -- Nullify the buffer
    Net.RadioTransmissions.SendBuffer[vesselIdx] = {}
  end
end


--------------------------------------------------------------------------------
-- Called when some vessel sends a single byte
--------------------------------------------------------------------------------
function InternalCallbacks.OnRadioTransmit(idx,channel,data)
  if Net.Client.Host and Net.Client.Peer and (Net.Client.Connected == true) then
    -- Create buffers if needed
    Net.RadioTransmissions.SendBuffer[idx] = Net.RadioTransmissions.SendBuffer[idx] or {}
    Net.RadioTransmissions.SendBuffer[idx][channel] = Net.RadioTransmissions.SendBuffer[idx][channel] or {}

    -- Send data in packets
    if (#Net.RadioTransmissions.SendBuffer[idx][channel] >= Net.RadioTransmissions.PacketSize) or
       (curtime() - Net.RadioTransmissions.LastTime > 0.05) then
      Net.RadioTransmissions.LastTime = curtime()
      Net.RadioTransmissions.SendClient()
      Net.RadioTransmissions.SendBuffer[idx][channel] = { data }
    else
      table.insert(Net.RadioTransmissions.SendBuffer[idx][channel],data)
    end
    
    -- Always avoid further processing of radio data in networked game
    return true
  end
end


--------------------------------------------------------------------------------
-- Called when some vessel receives a single byte
--------------------------------------------------------------------------------
function InternalCallbacks.OnRadioReceive(idx,channel,data)
  if Net.Server.Host then
    -- Check if vessel is networked and has an assigned clientID
    if false then
      -- Create buffer if required
      Net.RadioTransmissions.SendBuffer[idx] = Net.RadioTransmissions.SendBuffer[idx] or {}
      Net.RadioTransmissions.SendBuffer[idx][channel] = Net.RadioTransmissions.SendBuffer[idx][channel] or {}
  
      -- Send data in packets
      if (#Net.RadioTransmissions.SendBuffer[idx][channel] >= Net.RadioTransmissions.PacketSize) or
         (curtime() - Net.RadioTransmissions.LastTime > 0.05) then
        Net.RadioTransmissions.LastTime = curtime()
        Net.RadioTransmissions.SendServer()
        Net.RadioTransmissions.SendBuffer[idx][channel] = { data }
      else
        table.insert(Net.RadioTransmissions.SendBuffer[idx][channel],data)
      end
      
      -- Server only avoids processing for owned vessels
      return true
    end
  end
end
