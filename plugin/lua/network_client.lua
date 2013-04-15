--============================================================================--
-- X-Space client highlevel code
--============================================================================--
Net.Client = {}




--------------------------------------------------------------------------------
-- Connect to server
--------------------------------------------------------------------------------
function Net.Client.Connect(host,port)
  -- Stop previous session
  Net.Client.Disconnect()

  -- Create host and connect to client
  print("X-Space: Connecting to "..tostring(host)..":"..tostring(port))
  Net.Client.Host = NetAPI.Host(nil,0,1)
  Net.Client.Peer = NetAPI.Connect(Net.Client.Host,host,port,0,0)
  
  -- Set NetID counter for new vessels (wraps from 1023 to 1)
  Net.Client.NetID = 1
  -- Last acknowledged frames list
  Net.Client.LastFrames = {}
  Net.Client.LastFrameID = Net.InvalidCycle
  -- Last time when world was sent to server
  Net.Client.LastUpdateTime = curtime()
  
  -- Radio transmission simulation send buffer
  Net.Client.RadioSendBuffer = {}
  
  -- Check if initial connection successfull
  if (not Net.Client.Host) or (not Net.Client.Peer) then
    print("X-Space: Failed to connect!\n")
    return false
  end
  
  -- Set status
  Net.Client.Connected = false
  return true
end


--------------------------------------------------------------------------------
-- Disconnect from server
--------------------------------------------------------------------------------
function Net.Client.Disconnect()
  if Net.Client.Peer then
    print("X-Space: Disconnecting from server")
    NetAPI.ResetPeer(Net.Client.Peer)
    NetAPI.DestroyHost(Net.Client.Host)
  
    Net.Client.Peer = nil
    Net.Client.Host = nil
  end
end




--------------------------------------------------------------------------------
-- Called when event is received
--------------------------------------------------------------------------------
function Net.Client.OnEvent(peer,type,data,message)
  if type == Net.EventType.Connected then
--    local frame = Net.Frame.Get()
--    local msg = NetAPI.NewMessage()
--    Net.Write8(msg,Net.Message.StateUpdate)
--    Net.Frame.Write({Vessels={}},frame,msg)
--    NetAPI.SendMessage(Net.Client.Host,Net.Client.Peer,msg)
    Net.Client.Connected = true
  ------------------------------------------------------------------------------
  elseif type == Net.EventType.Disconnected then
    Net.Client.Connected = false
  ------------------------------------------------------------------------------
  elseif type == Net.EventType.Message then
    local messageID =  Net.Read8(message)

    --== World state update ==--
    if messageID == Net.Message.World then
      -- Read frame time
      local frameTime = Net.ReadSingle(message)
      -- Read frame cycle
      local frameCycle = Net.Read32(message)
      -- Read base frame cycle
      local baseFrameCycle = Net.Read32(message)

      -- Fetch base frame
      local base_frame
      if baseFrameCycle ~= Net.InvalidCycle then
        if Net.Client.LastFrames[baseFrameCycle] then
          -- Read prior state
          base_frame = Net.Client.LastFrames[baseFrameCycle]
        else
          -- Cannot read prior state, must resync
          Net.Client.LastFrameID = Net.InvalidCycle
--          Console.WriteDebug("Sync error! No frame %d!",baseFrameCycle)
          return
        end
      else
        -- No prior frames: read full state
        base_frame = { Vessels = {} }
        base_frame.Cycle = Net.InvalidCycle
      end
      
      -- Read message from server
      local frame = Net.Frame.Read(base_frame,message)
      frame.Time = frameTime
      frame.Cycle = frameCycle
      frame.ReceiveTime = curtime()

      -- Remember frame
      Net.Client.LastFrameID = frame.Cycle
      Net.Client.LastFrames[frame.Cycle] = frame
   elseif messageID == Net.Message.Transmission then
      -- Read channel
      local channel = Net.Read8(message)
      -- Read vessel network ID
      local networkID = Net.Read32(message)
      -- Read byte count
      local count = Net.Read8(message)+1
      -- Read data
      local data = {}
      for i=1,count do data[i] = Net.Read8(message) end
      
--      Console.WriteDebug("N %d %02X %d",networkID,data[1],channel)
      for i=1,count do
        RadioAPI.WriteReceiveBuffer(0,channel,data[i])
      end
    end
  end
end




--------------------------------------------------------------------------------
-- Add callback for handling server messages
--------------------------------------------------------------------------------
AddCallback("frame","NetworkClient",function(dT)
  if Net.Client.Host and Net.Client.Peer then
    if (Net.Client.Connected == true) and (curtime() - Net.Client.LastUpdateTime > 0.1) then
      -- Update at 10 FPS
      Net.Client.LastUpdateTime = curtime()

      -- Get current base frame (last frame received from server)
      local base_frame
      if Net.Client.LastFrames[Net.Client.LastFrameID] then
        base_frame = Net.Client.LastFrames[Net.Client.LastFrameID]
      else
        base_frame = { Vessels = { } }
        base_frame.Cycle = Net.InvalidCycle
      end
   
      -- Get new frame (client world state)
      local frame = Net.Frame.Get(true)
      -- At what time this frame can be considered valid
      frame.Time = curtime()
   
      -- Create message for the server (send world)
      local message = NetAPI.NewMessage()
      Net.Write8(message,Net.Message.World)
      -- Send time at which this frame could be considered valid
      Net.WriteSingle(message,frame.Time)
      -- Send new data cycle (not used in clients)
      Net.Write32(message,0)
      -- Send last acknowledged cycle
      Net.Write32(message,base_frame.Cycle)
      -- Send clients frame ID
--      Net.Write32(message,Net.Client.ClientFrameID)
      -- Compress frame based on last server frame
      Net.Frame.Write(base_frame,frame,message)
      NetAPI.SendMessage(Net.Client.Host,Net.Client.Peer,message)
      
      -- Purge old frames from the list (older than 5 seconds)
      for frameIndex,frame in pairs(Net.Client.LastFrames) do
        if (frame.ReceiveTime or 0) < (curtime()-5.0) then
          Net.Client.LastFrames[frameIndex] = nil
        end
      end
    end

    -- Update networking
    NetAPI.Update(Net.Client.Host,Net.Client.OnEvent)
  end
end)
