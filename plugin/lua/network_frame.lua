--============================================================================--
-- X-Space frame system code
--------------------------------------------------------------------------------
-- When used by client:
--   Cycle in received frames indicates last valid cycle of server data
--   Cycle in sent frames indicates last acknowledged cycle of server data
--
-- When used by server:
--   Cycle in received frames indicates which frame client has acknowledged
--   Cycle in sent frames increments each frame
--============================================================================--
Net.Frame = {}



--------------------------------------------------------------------------------
-- Returns copy of a frame (deep copy)
--------------------------------------------------------------------------------
function Net.Frame.GetCopy(base_frame)
  local frame = { Vessels = {} }
  if base_frame and base_frame.Vessels then
    for k,v in pairs(base_frame.Vessels) do
      frame.Vessels[k] = {}
      for k2,v2 in pairs(v) do
        frame.Vessels[k][k2] = v2
      end
    end
  end
  
  return frame
end

function Net.Frame.Merge(frame,base_frame)
  for k,v in pairs(base_frame.Vessels) do
    frame.Vessels[k] = {}
    for k2,v2 in pairs(v) do
      frame.Vessels[k][k2] = v2
    end
  end
end


--------------------------------------------------------------------------------
-- Returns new frame (based on current state)
--------------------------------------------------------------------------------
function Net.Frame.Get(isClient)
  local frame = {}
  
  -- Update vessels
  frame.Vessels = {}
  for idx=0,VesselAPI.GetCount()-1 do
    local exists = VesselAPI.GetParameter(idx,0)
    local networkID = VesselAPI.GetParameter(idx,2)
    local physicsType = VesselAPI.GetParameter(idx,4)
    local networked = VesselAPI.GetParameter(idx,3)
--    local networked = 0
    
    -- Fetch only vessels which are not networked, and exist
    if (exists == 1) and
       ((networked == 0) or (not isClient)) and
       (networkID > 0) and
       (physicsType ~= 4) then
      -- Read networking ID
      if isClient then
        -- If client, then generate temporary NetID for the vessel
        if networkID == 0 then
          networkID = Net.Client.NetID
          VesselAPI.SetParameter(idx,2,networkID)
          
          Net.Client.NetID = Net.Client.NetID + 1
          if Net.Client.NetID > 1023 then Net.Client.NetID = 1 end
        end
      else
        -- If server, generate full network ID
        -- ...
      end
      
      -- Make sure coordinates are correct
      VesselAPI.GetNoninertialCoordinates(idx)

      -- Read vessel data
      frame.Vessels[networkID] = {
        -- Make sure it's not networked
        Networked = false,
      
        -- Flight dynamics
        X  = VesselAPI.GetParameter(idx,50)-VesselAPI.GetParameter(idx,66),
        Y  = VesselAPI.GetParameter(idx,51)-VesselAPI.GetParameter(idx,67),
        Z  = VesselAPI.GetParameter(idx,52)-VesselAPI.GetParameter(idx,68),
        VX = VesselAPI.GetParameter(idx,53),
        VY = VesselAPI.GetParameter(idx,54),
        VZ = VesselAPI.GetParameter(idx,55),
        AX = VesselAPI.GetParameter(idx,56),
        AY = VesselAPI.GetParameter(idx,57),
        AZ = VesselAPI.GetParameter(idx,58),
        Q0 = VesselAPI.GetParameter(idx,59),
        Q1 = VesselAPI.GetParameter(idx,60),
        Q2 = VesselAPI.GetParameter(idx,61),
        Q3 = VesselAPI.GetParameter(idx,62),
        P  = VesselAPI.GetParameter(idx,63),
        Q  = VesselAPI.GetParameter(idx,64),
        R  = VesselAPI.GetParameter(idx,65),
        CX = VesselAPI.GetParameter(idx,66),
        CY = VesselAPI.GetParameter(idx,67),
        CZ = VesselAPI.GetParameter(idx,68),
        
        -- Vessel physics parameters
        Jxx         = VesselAPI.GetParameter(idx,10),
        Jyy         = VesselAPI.GetParameter(idx,11),
        Jzz         = VesselAPI.GetParameter(idx,12),
        MassChassis = VesselAPI.GetParameter(idx,17),
        MassHull    = VesselAPI.GetParameter(idx,18),
        MassFuel0   = VesselAPI.GetParameter(idx,19),
        MassFuel1   = VesselAPI.GetParameter(idx,20),
        MassFuel2   = VesselAPI.GetParameter(idx,21),
        MassFuel3   = VesselAPI.GetParameter(idx,22),
      }
    end
  end
  
  -- Update frame index/time
  frame.Time = 0
  frame.Cycle = 0 -- invalid
  
  -- Update NetID refresh table (key: new netID, value: old netID)
  frame.NetIDRefresh = {}
  
  return frame
end


--------------------------------------------------------------------------------
-- Updates current state based on the frame
--------------------------------------------------------------------------------
function Net.Frame.Update(frame)
  for netID,vessel in pairs(frame.Vessels) do
    if vessel.Networked == true then
      local vesselID = VesselAPI.GetByNetID(netID)
      if not vesselID then
        vesselID = VesselAPI.Create({})
      end
  
      VesselAPI.SetParameter(vesselID,0,1)
      VesselAPI.SetParameter(vesselID,2,netID)
      VesselAPI.SetParameter(vesselID,3,1)
      VesselAPI.SetParameter(vesselID,50,vessel.X+vessel.CX)
      VesselAPI.SetParameter(vesselID,51,vessel.Y+vessel.CY)
      VesselAPI.SetParameter(vesselID,52,vessel.Z+vessel.CZ)
      VesselAPI.SetParameter(vesselID,53,vessel.VX)
      VesselAPI.SetParameter(vesselID,54,vessel.VY)
      VesselAPI.SetParameter(vesselID,55,vessel.VZ)
      VesselAPI.SetParameter(vesselID,56,vessel.AX)
      VesselAPI.SetParameter(vesselID,57,vessel.AY)
      VesselAPI.SetParameter(vesselID,58,vessel.AZ)
      VesselAPI.SetParameter(vesselID,59,vessel.Q0)
      VesselAPI.SetParameter(vesselID,60,vessel.Q1)
      VesselAPI.SetParameter(vesselID,61,vessel.Q2)
      VesselAPI.SetParameter(vesselID,62,vessel.Q3)
      VesselAPI.SetParameter(vesselID,63,vessel.P )
      VesselAPI.SetParameter(vesselID,64,vessel.Q )
      VesselAPI.SetParameter(vesselID,65,vessel.R )
      VesselAPI.SetParameter(vesselID,66,vessel.CX)
      VesselAPI.SetParameter(vesselID,67,vessel.CY)
      VesselAPI.SetParameter(vesselID,68,vessel.CZ)
      
      VesselAPI.SetParameter(vesselID,10,vessel.Jxx)
      VesselAPI.SetParameter(vesselID,11,vessel.Jyy)
      VesselAPI.SetParameter(vesselID,12,vessel.Jzz)
      VesselAPI.SetParameter(vesselID,17,vessel.MassChassis)
      VesselAPI.SetParameter(vesselID,18,vessel.MassHull)
      VesselAPI.SetParameter(vesselID,19,vessel.MassFuel0)
      VesselAPI.SetParameter(vesselID,20,vessel.MassFuel1)
      VesselAPI.SetParameter(vesselID,21,vessel.MassFuel2)
      VesselAPI.SetParameter(vesselID,22,vessel.MassFuel3)
  
      -- Reset physics type and update networking timer
      VesselAPI.SetParameter(vesselID,4,3)
      VesselAPI.SetParameter(vesselID,97,curtime())
    end
  end
end


--------------------------------------------------------------------------------
-- Reads and returns frame from the message. base_frame must exist, can be empty
--------------------------------------------------------------------------------
function Net.Frame.Read(base_frame,msg)
  -- Create new frame based on base_frame
  local frame = Net.Frame.GetCopy(base_frame)

  -- Read vessels until zero compression bits encountered
  local iterationCount = 0
  while iterationCount < 256 do
    local compressionBits = Net.Read8(msg)
    iterationCount = iterationCount + 1
    
    -- If reached zero bit, stop reading
    if compressionBits == 0 then break end
    
    -- Decode bits
    local coordinatesChanged  = math.floor(compressionBits /   1) % 2
    local velocityChanged     = math.floor(compressionBits /   2) % 2
    local accelerationChanged = math.floor(compressionBits /   4) % 2
    local orientationChanged  = math.floor(compressionBits /   8) % 2
    local angVelocityChanged  = math.floor(compressionBits /  16) % 2
    local centerpointChanged  = math.floor(compressionBits /  32) % 2
    local staticMassChanged   = math.floor(compressionBits /  64) % 2
    local dynamicMassChanged  = math.floor(compressionBits / 128) % 2
    
    -- Read network ID
    local netID = Net.Read32(msg)
    
    -- Decode vessel
    if not frame.Vessels[netID] then
      frame.Vessels[netID] = {
        X  = 0, Y  = 0, Z  = 0,
        VX = 0, VY = 0, VZ = 0,
        AX = 0, AY = 0, AZ = 0,
        Q0 = 0, Q1 = 0, Q2 = 0, Q3 = 0,
        P  = 0, Q  = 0, R  = 0,
        CX = 0, CY = 0, CZ = 0,
        
        Jxx = 1, Jyy = 1, Jzz = 1,
        MassChassis = 1, MassHull  = 1,
        MassFuel0   = 0, MassFuel1 = 0,
        MassFuel2   = 0, MassFuel3 = 0,
      }
    end
    if coordinatesChanged == 1 then
      frame.Vessels[netID].X  = Net.ReadSingle(msg)
      frame.Vessels[netID].Y  = Net.ReadSingle(msg)
      frame.Vessels[netID].Z  = Net.ReadSingle(msg)
    end
    if velocityChanged == 1 then
      frame.Vessels[netID].VX = Net.ReadSingle(msg)
      frame.Vessels[netID].VY = Net.ReadSingle(msg)
      frame.Vessels[netID].VZ = Net.ReadSingle(msg)
    end
    if accelerationChanged == 1 then
      frame.Vessels[netID].AX = Net.ReadSingle(msg)
      frame.Vessels[netID].AY = Net.ReadSingle(msg)
      frame.Vessels[netID].AZ = Net.ReadSingle(msg)
    end
    if orientationChanged == 1 then
      frame.Vessels[netID].Q0 = Net.ReadSingle(msg)
      frame.Vessels[netID].Q1 = Net.ReadSingle(msg)
      frame.Vessels[netID].Q2 = Net.ReadSingle(msg)
      frame.Vessels[netID].Q3 = Net.ReadSingle(msg)
    end
    if angvelocityChanged == 1 then
      frame.Vessels[netID].P  = Net.ReadSingle(msg)
      frame.Vessels[netID].Q  = Net.ReadSingle(msg)
      frame.Vessels[netID].R  = Net.ReadSingle(msg)
    end
    if centerpointChanged == 1 then
      frame.Vessels[netID].CX = Net.ReadDouble(msg)
      frame.Vessels[netID].CY = Net.ReadDouble(msg)
      frame.Vessels[netID].CZ = Net.ReadDouble(msg)
    end
    if staticMassChanged == 1 then
      frame.Vessels[netID].Jxx         = Net.ReadSingle(msg)
      frame.Vessels[netID].Jyy         = Net.ReadSingle(msg)
      frame.Vessels[netID].Jzz         = Net.ReadSingle(msg)
      frame.Vessels[netID].MassChassis = Net.ReadSingle(msg)
      frame.Vessels[netID].MassHull    = Net.ReadSingle(msg)
    end
    if dynamicMassChanged == 1 then
      frame.Vessels[netID].MassFuel0 = Net.ReadSingle(msg)
      frame.Vessels[netID].MassFuel1 = Net.ReadSingle(msg)
      frame.Vessels[netID].MassFuel2 = Net.ReadSingle(msg)
      frame.Vessels[netID].MassFuel3 = Net.ReadSingle(msg)
    end
    
    -- Mark as networked
    frame.Vessels[netID].Networked = true
    
    --[[if Console then
      Console.WriteDebug("      RECV %d %d%d%d %d%d%d %d%d",
        netID,
        coordinatesChanged,
        velocityChanged,
        accelerationChanged,
        orientationChanged,
        angVelocityChanged,
        centerpointChanged,
        staticMassChanged,
        dynamicMassChanged
      )
    else
      print("RECV",
        netID,
        coordinatesChanged..velocityChanged..
        accelerationChanged..orientationChanged..
        angVelocityChanged..centerpointChanged..
        staticMassChanged..dynamicMassChanged,
        compressionBits
      )
    end]]--
  end

--  if iterationCount == 256 then error("BAD FRAMING") end
  
  -- Read network ID updates
--[[  frame.NetIDRefresh = {}
  local netIDCount = Net.Read16(msg)
  for idx = 1,netIDCount do
    local newNetID = Net.Read32(msg)
    local oldNetID = Net.Read32(msg)
    frame.NetIDRefresh[newNetID] = oldNetID
  end ]]--
  
  -- Return new frame
  return frame
end


--------------------------------------------------------------------------------
-- Writes delta of the frame. base_frame must exist, can be empty
--------------------------------------------------------------------------------
function Net.Frame.Write(base_frame,frame,msg)
  -- Write vessels
  for netID,vessel in pairs(frame.Vessels) do
    if vessel.Networked == false then
--      if Console then
--        Console.WriteDebug("SENDING "..netID)
--      end
      
      -- Vessel compression bits (zero: not written)
      local coordinatesChanged = 0
      local velocityChanged = 0
      local accelerationChanged = 0
      local orientationChanged = 0
      local angVelocityChanged = 0
      local centerpointChanged = 0
      local staticMassChanged = 0
      local dynamicMassChanged = 0
      
      -- Check if this vessel is not present in the base frame
      local base_vessel = base_frame.Vessels[netID]
      if not base_vessel then
        coordinatesChanged = 1
        velocityChanged = 1
        accelerationChanged = 1
        orientationChanged = 1
        angVelocityChanged = 1
        centerpointChanged = 1
        staticMassChanged = 1
        dynamicMassChanged = 1
      else -- See if there are any changes in vessels
        if (math.abs(vessel.X  - base_vessel.X) > 0.10) or -- 10 cm precision
           (math.abs(vessel.Y  - base_vessel.Y) > 0.10) or
           (math.abs(vessel.Z  - base_vessel.Z) > 0.10) then
          coordinatesChanged = 1
        end
        
        if (math.abs(vessel.VX - base_vessel.VX) > 0.1) or
           (math.abs(vessel.VY - base_vessel.VY) > 0.1) or
           (math.abs(vessel.VZ - base_vessel.VZ) > 0.1) then
          velocityChanged = 1
        end
        
        if (math.abs(vessel.AX - base_vessel.AX) > 0.5) or
           (math.abs(vessel.AY - base_vessel.AY) > 0.5) or
           (math.abs(vessel.AZ - base_vessel.AZ) > 0.5) then
          accelerationChanged = 1
        end
        
        if (math.abs(vessel.Q0 - base_vessel.Q0) > 0.02) or
           (math.abs(vessel.Q1 - base_vessel.Q1) > 0.02) or
           (math.abs(vessel.Q2 - base_vessel.Q2) > 0.02) or
           (math.abs(vessel.Q3 - base_vessel.Q3) > 0.02) then
          orientationChanged = 1
        end
        
        if (math.abs(vessel.P  - base_vessel.P) > 0.05) or
           (math.abs(vessel.Q  - base_vessel.Q) > 0.05) or
           (math.abs(vessel.R  - base_vessel.R) > 0.05) then
          angVelocityChanged = 1
        end
        
        if (math.abs(vessel.CX  - base_vessel.CX) > 0.1) or -- full precision
           (math.abs(vessel.CY  - base_vessel.CY) > 0.1) or
           (math.abs(vessel.CZ  - base_vessel.CZ) > 0.1) then
          centerpointChanged = 1
        end
        
        if (math.abs(vessel.Jxx          - base_vessel.Jxx) > 0.1) or
           (math.abs(vessel.Jyy          - base_vessel.Jyy) > 0.1) or
           (math.abs(vessel.Jzz          - base_vessel.Jzz) > 0.1) or
           (math.abs(vessel.MassChassis  - base_vessel.MassChassis) > 0.1) or
           (math.abs(vessel.MassHull     - base_vessel.MassHull) > 0.1) then
          staticMassChanged = 1
        end
        
        if (math.abs(vessel.MassFuel0 - base_vessel.MassFuel0) > 10.0) or
           (math.abs(vessel.MassFuel1 - base_vessel.MassFuel1) > 10.0) or
           (math.abs(vessel.MassFuel2 - base_vessel.MassFuel2) > 10.0) or
           (math.abs(vessel.MassFuel3 - base_vessel.MassFuel3) > 10.0) then
          dynamicMassChanged = 1
        end
      end
  
      -- Write compression bits and all the data if required
      local compressionBits =
        coordinatesChanged  *   1 +
        velocityChanged     *   2 +
        accelerationChanged *   4 +
        orientationChanged  *   8 +
        angVelocityChanged  *  16 +
        centerpointChanged  *  32 +
        staticMassChanged   *  64 +
        dynamicMassChanged  * 128
        
      if compressionBits > 0 then
        Net.Write8(msg,compressionBits)
        Net.Write32(msg,netID)
  
        if coordinatesChanged == 1 then
          Net.WriteSingle(msg,vessel.X)
          Net.WriteSingle(msg,vessel.Y)
          Net.WriteSingle(msg,vessel.Z)
        end
        if velocityChanged == 1 then
          Net.WriteSingle(msg,vessel.VX)
          Net.WriteSingle(msg,vessel.VY)
          Net.WriteSingle(msg,vessel.VZ)
        end
        if accelerationChanged == 1 then
          Net.WriteSingle(msg,vessel.AX)
          Net.WriteSingle(msg,vessel.AY)
          Net.WriteSingle(msg,vessel.AZ)
        end
        if orientationChanged == 1 then
          Net.WriteSingle(msg,vessel.Q0)
          Net.WriteSingle(msg,vessel.Q1)
          Net.WriteSingle(msg,vessel.Q2)
          Net.WriteSingle(msg,vessel.Q3)
        end
        if angvelocityChanged == 1 then
          Net.WriteSingle(msg,vessel.P)
          Net.WriteSingle(msg,vessel.Q)
          Net.WriteSingle(msg,vessel.R)
        end
        if centerpointChanged == 1 then
          Net.WriteDouble(msg,vessel.CX)
          Net.WriteDouble(msg,vessel.CY)
          Net.WriteDouble(msg,vessel.CZ)
        end
        if staticMassChanged == 1 then
          Net.WriteSingle(msg,vessel.Jxx)
          Net.WriteSingle(msg,vessel.Jyy)
          Net.WriteSingle(msg,vessel.Jzz)
          Net.WriteSingle(msg,vessel.MassChassis)
          Net.WriteSingle(msg,vessel.MassHull)
        end
        if dynamicMassChanged == 1 then
          Net.WriteSingle(msg,vessel.MassFuel0)
          Net.WriteSingle(msg,vessel.MassFuel1)
          Net.WriteSingle(msg,vessel.MassFuel2)
          Net.WriteSingle(msg,vessel.MassFuel3)
        end
        
        --[[if Console then
          Console.WriteDebug("SEND %d %d%d%d %d%d%d %d%d",
            netID,
            coordinatesChanged,
            velocityChanged,
            accelerationChanged,
            orientationChanged,
            angVelocityChanged,
            centerpointChanged,
            staticMassChanged,
            dynamicMassChanged
          )
        else
          print("SEND",
            coordinatesChanged,
            velocityChanged,
            accelerationChanged,
            orientationChanged,
            angVelocityChanged,
            centerpointChanged,
            staticMassChanged,
            dynamicMassChanged
          )
        end]]--
      end
    end
  end
  
  -- Write no compression bits, marking end of the packet
  Net.Write8(msg,0)
  
  -- Check if networking ID data changed
--[[  if frame.NetIDRefresh and (#frame.NetIDRefresh > 0) then -- FIXME: no check for #frame.NetIDRefresh > 65535
    Net.Write16(msg,#frame.NetIDRefresh)
    for newNetID,oldNetID in pairs(frame.NetIDRefresh) do
      Net.Write32(newNetID)
      Net.Write32(oldNetID)
    end
  else
    Net.Write16(msg,0)
  end ]]--
end


--------------------------------------------------------------------------------
-- Extrapolates frame
--------------------------------------------------------------------------------
function Net.Frame.Extrapolate(old_frame,new_time)

end
