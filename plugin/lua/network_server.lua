--============================================================================--
-- X-Space server highlevel code
--============================================================================--
Net.Server = {}


--------------------------------------------------------------------------------
-- Start up server
--------------------------------------------------------------------------------
function Net.Server.Start(port,maxClients)
  -- Stop server if it was already running
  Net.Server.Stop()
  
  -- Start server
  print("X-Space: Starting up server on port "..tostring(port))
  print("X-Space: Setting maximum number of clients to "..tostring(maxClients))
  Net.Server.Host = NetAPI.Host("127.0.0.1",port,maxClients)
  Net.Server.MaxClients = maxClients
  
  -- Client ID counter
  Net.Server.ClientID = 0
  -- Frame counter
  Net.Server.FrameID = 1
  -- Vessel network ID counter (ID < 1024: request for new ID)
  Net.Server.NetID = 1024
  -- Client data
  Net.Server.Clients = {}
  -- Frame history (only stores last frame, frames which were acknowledged by users)
  Net.Server.Frames = {}
  -- Current server frame
  Net.Server.CurrentFrame = nil
  
  -- Last time when frame updates were sent to clients
  Net.Server.LastUpdateTime = curtime()
  
  -- Check if it started up
  if not Net.Server.Host then
    print("X-Space: Failed to start up server\n")
  end
end


--------------------------------------------------------------------------------
-- Stop server
--------------------------------------------------------------------------------
function Net.Server.Stop()
  if Net.Server.Host then
    print("X-Space: Stopping server")
    NetAPI.DestroyHost(Net.Server.Host)
  end
end




--------------------------------------------------------------------------------
-- Called when event is received
--------------------------------------------------------------------------------
function Net.Server.OnEvent(peer,type,data,message)
  if type == Net.EventType.Connected then
    -- New client connected
    print(string.format("X-Space: Client %08X connected from %s",
      Net.Server.ClientID,tostring(NetAPI.GetPeerHostPort(peer))))
      
    -- Create entry in client table
    Net.Server.Clients[Net.Server.ClientID] = {
      -- Signature required to broadcast radio
      RadioSignature = "",
      
      -- Features
      WriteVessels = false, -- Client can send vessel updates
      ReadVessels = false,  -- Vessel updates are sent to this client
      WriteRadio = false,   -- Radio transmissions are accepted from this client
      ReadRadio = false,    -- Radio transmissions are sent to this client
      
      -- Last acknowledged frame index
      LastAcknowledgedFrame = nil,
      -- Networking peer
      Peer = peer,
      
      -- Index remap (which client indexes correspond to what vessels
      IndexRemap = {},
    }
      
    -- Assign a client ID
    NetAPI.SetPeerID(peer,Net.Server.ClientID)
    Net.Server.ClientID = Net.Server.ClientID + 1
  ------------------------------------------------------------------------------
  elseif type == Net.EventType.Disconnected then
    -- Fetch client ID
    local clientID = NetAPI.GetPeerID(peer)
    
    -- Disconnect client
    print(string.format("X-Space: Client %08X disconnected",
      clientID))
      
    -- Clear up data
    Net.Server.Clients[clientID] = nil
  ------------------------------------------------------------------------------
  elseif type == Net.EventType.Message then
    -- Fetch client ID
    local clientID = NetAPI.GetPeerID(peer)
    -- Fetch client
    local client = Net.Server.Clients[clientID]
    -- Fetch message ID
    local messageID =  Net.Read8(message)
    
    if messageID == Net.Message.World then
      -- Read frame time
      local frameTime = Net.ReadSingle(message)
      -- Read frame cycle
      local frameCycle = Net.Read32(message)
      -- Read base frame cycle
      local baseFrameCycle = Net.Read32(message)
      
      -- Fetch base frame
      local base_frame
      if (baseFrameCycle ~= Net.InvalidCycle) and Net.Server.Frames[baseFrameCycle] then
        -- Read prior state
        base_frame = Net.Server.Frames[baseFrameCycle]
      else
        -- No prior frames: read full state
        base_frame = { Vessels = {} }
        base_frame.Cycle = baseFrameCycle
      end
  
      -- Read message from clients
      local frame = Net.Frame.Read(base_frame,message)
      frame.Time = frameTime
      frame.Cycle = frameCycle
      
      -- Merge received vessels with current frame
      Net.Frame.Merge(Net.Server.CurrentFrame,frame)
      
      -- Update server state
      Net.Frame.Update(frame)

      -- Update last acknowledged frame
      client.LastAcknowledgedFrame = base_frame.Cycle
      
      -- Remap vessel indexes to correct ones
--[[      for k,v in pairs(frame.Vessels) do
        if k < 1024 then
          if not client.IndexRemap[k] then
            client.IndexRemap[k] = Net.Server.NetID
            Net.Server.NetID = Net.Server.NetID + 1
            
            print("Remapped",clientID,k,Net.Server.NetID)
          end
          
          frame.Vessels[client.IndexRemap[k ] ] = frame.Vessels[k]
          frame.Vessels[k] = nil
        end
      end]]--
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
      
      -- Get vessel index
      local vesselID = VesselAPI.GetByNetID(networkID)
      if vesselID then
        for i=1,count do
          RadioAPI.Transmit(vesselID,channel,data[i])
        end
      end
    end
  end
end



--------------------------------------------------------------------------------
-- Called when dedicated server starts up
--------------------------------------------------------------------------------
function InternalCallbacks.OnDedicatedServer()
  if DEBUG_VSFL == true
  then Net.Server.Start(10205,128)
  else Net.Server.Start(10105,128)
  end
end


--------------------------------------------------------------------------------
-- Add callback for handling server messages
--------------------------------------------------------------------------------
AddCallback("frame","NetworkServer",function(dT)
  if Net.Server.Host then
    -- Send frame updates if required
    if curtime() - Net.Server.LastUpdateTime > 0.1 then
      Net.Server.LastUpdateTime = curtime()
      
      -- Purge old frames from the list (older than 5 seconds)
      local t = 0
      for frameIndex,frame in pairs(Net.Server.Frames) do
        t = t + 1
        if (frame.Time or 0) < (curtime()-5.0) then
          Net.Server.Frames[frameIndex] = nil
        end
      end
--      print("FRAMES",t,curtime(),collectgarbage("count"))
  
      -- Generate new frame
--      Net.Server.Frames[Net.Server.FrameID] = Net.Frame.Get()
--      Net.Server.Frames[Net.Server.FrameID-1] = nil -- FIXME: no delta support yet
      if Net.Server.Frames[Net.Server.FrameID-1] then
        Net.Server.Frames[Net.Server.FrameID] = Net.Frame.Get()
--          Net.Frame.GetCopy(Net.Server.Frames[Net.Server.FrameID-1])
      else
        Net.Server.Frames[Net.Server.FrameID] = { Vessels = {} }
      end
      Net.Server.CurrentFrame = Net.Server.Frames[Net.Server.FrameID]
      Net.Server.CurrentFrame.Time = curtime()
      Net.Server.CurrentFrame.Cycle = Net.Server.FrameID
      
      -- Update networking
      NetAPI.Update(Net.Server.Host,Net.Server.OnEvent)
      
      -- Advance server frame
      Net.Server.FrameID = Net.Server.FrameID + 1
      
      -- Send data for all clients
      for clientID,client in pairs(Net.Server.Clients) do
        -- Get base frame for client
        local base_frame
        if client.LastAcknowledgedFrame ~= Net.InvalidCycle then
          if Net.Server.Frames[client.LastAcknowledgedFrame] then
            base_frame = Net.Server.Frames[client.LastAcknowledgedFrame]
          else
            -- Sychronization error
            print("X-Space: Sync error in client "..tostring(clientID)..
                  ". Frame "..tostring(client.LastAcknowledgedFrame).." invalid")
            client.LastAcknowledgedFrame = Net.InvalidCycle
            
            -- Send full state
            base_frame = { Vessels = { } }
            base_frame.Cycle = Net.InvalidCycle
          end
        else
          base_frame = { Vessels = { } }
          base_frame.Cycle = Net.InvalidCycle
        end
        
        -- Get new frame
        local frame = Net.Server.CurrentFrame
        
        -- Create message for the client (send world)
        local message = NetAPI.NewMessage()
        Net.Write8(message,Net.Message.World)
        Net.WriteSingle(message,frame.Time)
        Net.Write32(message,frame.Cycle)
        Net.Write32(message,base_frame.Cycle)
        Net.Frame.Write(base_frame,frame,message)
        NetAPI.SendMessage(Net.Server.Host,client.Peer,message)
      end
    end
  end
end)
