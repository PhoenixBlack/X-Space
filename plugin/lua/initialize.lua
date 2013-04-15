--=============================================================================-
-- X-Space main initialization script
--=============================================================================-
DEBUG_VSFL = true

-- Some socket service stuff
if socket then
  function socket.connect(address, port, laddress, lport)
      local sock, err = socket.tcp()
      if not sock then return nil, err end
      if laddress then
          local res, err = sock:bind(laddress, lport, -1)
          if not res then return nil, err end
      end
      local res, err = sock:connect(address, port)
      if not res then return nil, err end
      return sock
  end
  
  function socket.bind(host, port, backlog)
      local sock, err = socket.tcp()
      if not sock then return nil, err end
      sock:setoption("reuseaddr", true)
      local res, err = sock:bind(host, port)
      if not res then return nil, err end
      res, err = sock:listen(backlog)
      if not res then return nil, err end
      return sock
  end
  
  function socket.choose(table)
      return function(name, opt1, opt2)
          if base.type(name) ~= "string" then
              name, opt1, opt2 = "default", name, opt1
          end
          local f = table[name or "nil"]
          if not f then base.error("unknown key (".. base.tostring(name) ..")", 3)
          else return f(opt1, opt2) end
      end
  end
  
  function split(str, pat)
     local t = {}
     local fpat = "(.-)" .. pat
     local last_end = 1
     local s, e, cap = str:find(fpat, 1)
     while s do
        if s ~= 1 or cap ~= "" then
  	 table.insert(t,cap)
        end
        last_end = e+1
        s, e, cap = str:find(fpat, last_end)
     end
     if last_end <= #str then
        cap = str:sub(last_end)
        table.insert(t, cap)
     end
     return t
  end
end


--------------------------------------------------------------------------------
FrameCallbacks = {}

function AddCallback(type,name,func)
  if string.lower(type) == "frame" then
    FrameCallbacks[name] = func
  end
end

function DeleteCallback(type,name)
  if string.lower(type) == "frame" then
    FrameCallbacks[name] = nil
  end
end

function InternalCallbacks.OnFrame(dt)
  for k,v in pairs(FrameCallbacks) do v(dt) end
end


--------------------------------------------------------------------------------
-- Initialize aircraft after loading
function InternalCallbacks.OnVesselInitialize(vessel)
  -- Fetch vessel index
  local vesselIdx = vessel.CurrentVessel
  
  -- Process launch pad mount
  if vessel.Mount then
    local nearestPad = LaunchPadsAPI.GetNearest(vesselIdx)
    if nearestPad then
      LaunchPadsAPI.SetVessel(nearestPad,vesselIdx)
      if vessel.Mount.Position then
        VesselAPI.SetMountOffset(vesselIdx,
          vessel.Mount.Position[1] or 0,
          vessel.Mount.Position[2] or 0,
          vessel.Mount.Position[3] or 0)
      end
      if vessel.Mount.Attitude then
        VesselAPI.SetMountAttitude(vesselIdx,
          vessel.Mount.Attitude[1] or 0,
          vessel.Mount.Attitude[2] or 0,
          vessel.Mount.Attitude[3] or 0)
      end
    end
  end
  
  -- Process stages
  if vessel.Stages then
    print("X-Space: Attached stages:")
    local function MountStages(stages,parentIdx)
      for _,vesselStages in ipairs(stages) do
        -- Get name
        local vesselName = vesselStages.Vessel or ""
        local vesselParameters = {}
        
        -- Add all parameters for the body
        for k,v in pairs(vesselStages) do
          if (k ~= "Position") and
             (k ~= "Attitude") and
             (k ~= "Vessel") and
             (not tonumber(k)) then
            vesselParameters[k] = v
          end
        end
        
        -- Create body
        local mountIdx = VesselAPI.Add(vesselName,vesselParameters)
        print("",mountIdx,vesselName,vesselParameters.Index or "")
          
        -- Mount body
        VesselAPI.Mount(parentIdx,mountIdx)
        if vesselStages.Position then
          VesselAPI.SetMountOffset(mountIdx,
            vesselStages.Position[1] or 0,
            vesselStages.Position[2] or 0,
            vesselStages.Position[3] or 0)
        end
        if vesselStages.Attitude then
          VesselAPI.SetMountAttitude(mountIdx,
            vesselStages.Attitude[1] or 0,
            vesselStages.Attitude[2] or 0,
            vesselStages.Attitude[3] or 0)
        end
        
        -- Create and mount any extra bodies
        MountStages(vesselStages,mountIdx)
      end
    end
    
    -- Mount stages
    MountStages(vessel.Stages,vesselIdx)
  end
  
  -- Fuel tanks
  if vessel.FuelTanks then
    for k,v in pairs(vessel.FuelTanks) do
      if type(v) == "number" then
        VesselAPI.SetFuelTank(vesselIdx,k,v)
      else
        --- set offset and size
      end
    end
  end
  
  -- Mass parameters
  if vessel.Mass then
    if vessel.Mass.Total then
      VesselAPI.SetParameter(vesselIdx,17,vessel.Mass.Total*0.9) -- Chassis
      VesselAPI.SetParameter(vesselIdx,18,vessel.Mass.Total*0.1) -- Hull
    end
    if vessel.Mass.Chassis then
      VesselAPI.SetParameter(vesselIdx,17,vessel.Mass.Chassis)
    end
    if vessel.Mass.Hull then
      VesselAPI.SetParameter(vesselIdx,18,vessel.Mass.Hull)
    end
    if vessel.Mass.Jxx or vessel.Mass.Jyy or vessel.Mass.Jzz then
      local Jxx = vessel.Mass.Jxx or 0
      local Jyy = vessel.Mass.Jyy or 0
      local Jzz = vessel.Mass.Jzz or 0
      
      if (Jxx == 0) or (Jyy == 0) or (Jzz == 0) then
        VesselAPI.ComputeMoments(vesselIdx)
        print("X-Space: Radius of gyration (based on drag geometry) for",VesselAPI.GetFilename(vesselIdx))
        print(" ","Jxx",VesselAPI.GetParameter(vesselIdx,10)^0.5,"m")
        print(" ","Jyy",VesselAPI.GetParameter(vesselIdx,11)^0.5,"m")
        print(" ","Jzz",VesselAPI.GetParameter(vesselIdx,12)^0.5,"m")
      else
        VesselAPI.SetParameter(vesselIdx,10,Jxx)
        VesselAPI.SetParameter(vesselIdx,11,Jyy)
        VesselAPI.SetParameter(vesselIdx,12,Jzz)
      end
    end
  end
  
  -- Engine definitions
  for _,engine in pairs(vessel.Engine) do
    -- Check if engine is one of the default types
    if engine.Model then
      if EngineData[engine.Model] then
        for k,v in pairs(EngineData[engine.Model]) do engine[k] = v end
      else
        print("X-Space: Unknown engine type: "..tostring(engine.Type))
      end
    end
    
    -- Create datarefs for engines
    if engine.VerticalAngle   then Dataref(engine.VerticalAngle,0)   end
    if engine.HorizontalAngle then Dataref(engine.HorizontalAngle,0) end
    if engine.Dataref         then Dataref(engine.Dataref,0)         end
    if engine.FuelTankDataref then Dataref(engine.FuelTankDataref,0) end
    if engine.FuelFlowDataref then Dataref(engine.FuelFlowDataref,0) end
  end
  
  -- Network identifier
  if vessel.NetID then
    VesselAPI.SetParameter(vesselIdx,2,vessel.NetID)
    print("X-Space: Network ID for "..VesselAPI.GetFilename(vesselIdx).." is "..tostring(vessel.NetID))
  end
end


--------------------------------------------------------------------------------
-- Initialize engines simulation
Include(PluginsFolder.."lua/engines.lua")

-- VSFL related includes
if DEDICATED_SERVER then Include(PluginsFolder.."lua/vsfl.lua") end
