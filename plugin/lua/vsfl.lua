--------------------------------------------------------------------------------
-- VSFL Network specific code
--------------------------------------------------------------------------------
VSFL = {}

-- Radio->TCP streaming
Include(PluginsFolder.."lua/vsfl_tcp.lua")

-- XSAG groundstations software and list
Include(PluginsFolder.."lua/vsfl_xsag_groundstations.lua")


--------------------------------------------------------------------------------
-- Persisting features
--------------------------------------------------------------------------------
function VSFL.SaveState()
  -- Copy state
  local f_old = io.open(PluginsFolder.."vsfl/state/full.dat","r")
  local f_bak = io.open(PluginsFolder.."vsfl/state/full_backup.dat","w+")
  if f_old and f_bak then
    local data = f_old:read("*all")
    f_bak:write(data)
    f_old:close()
    f_bak:close()
  end
  
  -- Create new state
  local f = io.open(PluginsFolder.."vsfl/state/full.dat","w+")
  if not f then return end
  
  -- Write MJD date
  f:write("MJD  ",DedicatedServerAPI.GetMJD(),"\n")
  
  -- Save existing vessels
  for idx=0,VesselAPI.GetCount()-1 do
    local exists = VesselAPI.GetParameter(idx,0)
    local physics_type = VesselAPI.GetParameter(idx,4)
    local net_id = VesselAPI.GetParameter(idx,2)
    if (exists == 1) and (physics_type == 1) and (net_id < 100000) then
      -- Network ID
      f:write("VESS ",net_id,"\n")
      
      -- Write state vector, attitude
      f:write("X    ",VesselAPI.GetParameter(idx,50),"\n")
      f:write("Y    ",VesselAPI.GetParameter(idx,51),"\n")
      f:write("Z    ",VesselAPI.GetParameter(idx,52),"\n")
      f:write("VX   ",VesselAPI.GetParameter(idx,53),"\n")
      f:write("VY   ",VesselAPI.GetParameter(idx,54),"\n")
      f:write("VZ   ",VesselAPI.GetParameter(idx,55),"\n")

      f:write("Q0   ",VesselAPI.GetParameter(idx,59),"\n")
      f:write("Q1   ",VesselAPI.GetParameter(idx,60),"\n")
      f:write("Q2   ",VesselAPI.GetParameter(idx,61),"\n")
      f:write("Q3   ",VesselAPI.GetParameter(idx,62),"\n")
      f:write("P    ",VesselAPI.GetParameter(idx,63),"\n")
      f:write("Q    ",VesselAPI.GetParameter(idx,64),"\n")
      f:write("R    ",VesselAPI.GetParameter(idx,65),"\n")

      f:write("Jxx  ",VesselAPI.GetParameter(idx,10),"\n")
      f:write("Jyy  ",VesselAPI.GetParameter(idx,11),"\n")
      f:write("Jzz  ",VesselAPI.GetParameter(idx,12),"\n")
      f:write("Mc   ",VesselAPI.GetParameter(idx,17),"\n")
      f:write("Mh   ",VesselAPI.GetParameter(idx,18),"\n")
      f:write("Mf0  ",VesselAPI.GetParameter(idx,19),"\n")
      f:write("Mf1  ",VesselAPI.GetParameter(idx,20),"\n")
      f:write("Mf2  ",VesselAPI.GetParameter(idx,21),"\n")
      f:write("Mf3  ",VesselAPI.GetParameter(idx,22),"\n")
      
      f:write("Stis ",VesselAPI.GetParameter(idx,130),"\n")
      f:write("Sorb ",VesselAPI.GetParameter(idx,131),"\n")
      f:write("Stor ",VesselAPI.GetParameter(idx,132),"\n")
      f:write("Sdis ",VesselAPI.GetParameter(idx,133),"\n")
      
      -- End structure
      f:write("VEND ","\n")
    end
  end
  f:close()
end


-- Compatible with previous versions of VSFL
function VSFL.LoadState_Compat1()
  local f = io.open(PluginsFolder.."vsfl/state/last.dat","r")
  if not f then return end

  -- Load all vessels (append)
  while f:read(0) do
    local netID = (f:read("*number") or 0)
    if netID == 0 then return true end
    print(string.format("VSFL: [compat1] Restored vessel %05d",netID))

    -- Get valid vessel ID
    local vesselID = VesselAPI.GetByNetID(netID)
    if not vesselID then vesselID = VesselAPI.Create({}) end

    VesselAPI.SetParameter(vesselID,0,1) -- Exists?
    VesselAPI.SetParameter(vesselID,4,1) -- Physics type
    VesselAPI.SetParameter(vesselID,2,netID)
    VesselAPI.SetParameter(vesselID,50,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,51,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,52,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,53,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,54,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,55,f:read("*number") or 0)
    
    VesselAPI.SetParameter(vesselID,59,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,60,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,61,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,62,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,63,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,64,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,65,f:read("*number") or 0)
    
    VesselAPI.SetParameter(vesselID,10,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,11,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,12,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,17,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,18,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,19,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,20,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,21,f:read("*number") or 0)
    VesselAPI.SetParameter(vesselID,22,f:read("*number") or 0)
    
    VesselAPI.SetNoninertialCoordinates(vesselID)
  end
  f:close()
  return true
end


-- Load state
function VSFL.LoadState()
  local f = io.open(PluginsFolder.."vsfl/state/full.dat","r")
  if not f then
    if VSFL.LoadState_Compat1() then return end
    return
  end
  
  -- Current vessel index
  local vesselID = nil
  
  -- Read a single variable
  local readVariable = {
    ["MJD "] = function(value)
      DedicatedServerAPI.SetMJD(value)
    end,
    ["VESS"] = function(netID)
      vesselID = VesselAPI.GetByNetID(netID)
      if not vesselID then vesselID = VesselAPI.Create({}) end
      VesselAPI.SetParameter(vesselID,0,1) -- Exists
      VesselAPI.SetParameter(vesselID,4,1) -- Inertial physics
      VesselAPI.SetParameter(vesselID,2,netID)

      print(string.format("VSFL: Restored vessel %05d",netID))
    end,
    ["VEND"] = function()
      if vesselID then VesselAPI.SetNoninertialCoordinates(vesselID) end
    end,
    ["X   "] = function(value) VesselAPI.SetParameter(vesselID,50,value) end,
    ["Y   "] = function(value) VesselAPI.SetParameter(vesselID,51,value) end,
    ["Z   "] = function(value) VesselAPI.SetParameter(vesselID,52,value) end,
    ["VX  "] = function(value) VesselAPI.SetParameter(vesselID,53,value) end,
    ["VY  "] = function(value) VesselAPI.SetParameter(vesselID,54,value) end,
    ["VZ  "] = function(value) VesselAPI.SetParameter(vesselID,55,value) end,

    ["Q0  "] = function(value) VesselAPI.SetParameter(vesselID,59,value) end,
    ["Q1  "] = function(value) VesselAPI.SetParameter(vesselID,60,value) end,
    ["Q2  "] = function(value) VesselAPI.SetParameter(vesselID,61,value) end,
    ["Q3  "] = function(value) VesselAPI.SetParameter(vesselID,62,value) end,
    ["P   "] = function(value) VesselAPI.SetParameter(vesselID,63,value) end,
    ["Q   "] = function(value) VesselAPI.SetParameter(vesselID,64,value) end,
    ["R   "] = function(value) VesselAPI.SetParameter(vesselID,65,value) end,

    ["Jxx "] = function(value) VesselAPI.SetParameter(vesselID,10,value) end,
    ["Jyy "] = function(value) VesselAPI.SetParameter(vesselID,11,value) end,
    ["Jzz "] = function(value) VesselAPI.SetParameter(vesselID,12,value) end,
    ["Mc  "] = function(value) VesselAPI.SetParameter(vesselID,17,value) end,
    ["Mh  "] = function(value) VesselAPI.SetParameter(vesselID,18,value) end,
    ["Mf0 "] = function(value) VesselAPI.SetParameter(vesselID,19,value) end,
    ["Mf1 "] = function(value) VesselAPI.SetParameter(vesselID,20,value) end,
    ["Mf2 "] = function(value) VesselAPI.SetParameter(vesselID,21,value) end,
    ["Mf3 "] = function(value) VesselAPI.SetParameter(vesselID,22,value) end,
    
    ["Stis"] = function(value) VesselAPI.SetParameter(vesselID,130,value) end,
    ["Sorb"] = function(value) VesselAPI.SetParameter(vesselID,131,value) end,
    ["Stor"] = function(value) VesselAPI.SetParameter(vesselID,132,value) end,
    ["Sdis"] = function(value) VesselAPI.SetParameter(vesselID,133,value) end,
  }

  -- Read all data
  while f:read(0) do
    local data = f:read("*line") or ""
    local name  = string.sub(data,1,4)
    local value = tonumber(string.sub(data,6)) or 0

    if readVariable[name] then readVariable[name](value) end
  end
end




--------------------------------------------------------------------------------
-- Store server state every N seconds
--------------------------------------------------------------------------------
local lastStoreTime = curtime()
local stateLoaded = false
local ivssLoaded = {}

AddCallback("frame","VSFLPersistStore",function(dT)
  -- Check if state must be restored
  if not stateLoaded then
    VSFL.LoadState()
    stateLoaded = true
  end
  
  -- Save state
  if curtime() - lastStoreTime > 10.0 then
    lastStoreTime = curtime()
    VSFL.SaveState()
  end
  
  -- Load drag model
  for idx=0,VesselAPI.GetCount()-1 do
    local exists = VesselAPI.GetParameter(idx,0)
    local physics_type = VesselAPI.GetParameter(idx,4)
    local net_id = VesselAPI.GetParameter(idx,2)
    if (exists == 1) and (physics_type == 1) then
      if DragHeatAPI.IsInitialized(idx) == false then
        local pathname = string.format(PluginsFolder.."vsfl/%05d",VesselAPI.GetParameter(idx,2))
        DragHeatAPI.LoadInitialize(idx,pathname)
      end
    end
    
    if (exists == 1) and (not ivssLoaded[net_id]) then
      local filename = string.format(PluginsFolder.."vsfl/%05d/system.ivss",VesselAPI.GetParameter(idx,2))
      IVSSAPI.LoadModel(idx,filename)
      ivssLoaded[net_id] = true
    end
  end
end)




--------------------------------------------------------------------------------
-- Update information in vessel tracker
--------------------------------------------------------------------------------
local prevAltitude1 = {}
local prevAltitude2 = {}
local prevApogee = {}
local prevPerigee = {}

local lastMinorUpdateTime = -1e9
AddCallback("frame","VSFLUpdateMinor",function(dT)
  if DEBUG_VSFL == true then return end

  -- Update server information once per second
  if curtime() - lastMinorUpdateTime < 1.0 then return end
  lastMinorUpdateTime = curtime()

  -- Make a request to the server
  for idx=0,VesselAPI.GetCount()-1 do
    local exists = VesselAPI.GetParameter(idx,0)
    local physics_type = VesselAPI.GetParameter(idx,4)
    if (exists == 1) and
       ((physics_type == 1) or (physics_type == 3)) then
      local net_id = VesselAPI.GetParameter(idx,2)
      local online = 1.0
      local in_space = VesselAPI.GetParameter(idx,130)
      local ap = prevApogee[idx] or 0
      local per = prevPerigee[idx] or 0
      local alt = VesselAPI.GetParameter(idx,16)
      local orb = VesselAPI.GetParameter(idx,131)
      local orbt = VesselAPI.GetParameter(idx,132)
      local dist = VesselAPI.GetParameter(idx,133)

      -- Make a request
      local socket = socket.connect("127.0.0.1",80)
      if socket then
        socket:settimeout(0.01)
        socket:send([[
GET /local/vessel_update.php?online=]]..online..
[[&in_space=]]..in_space..[[&net_id=]]..net_id..
[[&ap=]]..ap..
[[&per=]]..per..
[[&alt=]]..alt..
[[&orb=]]..orb..
[[&orbt=]]..orbt..
[[&dist=]]..dist..[[ HTTP/1.0
Host: vsfl.wireos.com
User-Agent: VSFL

]])
        socket:close()
      end
    end
  end
end)


local lastMajorUpdateTime = -1e9
AddCallback("frame","VSFLUpdateMajor",function(dT)
  if DEBUG_VSFL == true then return end
  
  -- Update server information once per second
  if curtime() - lastMajorUpdateTime < 10.0 then return end
  lastMajorUpdateTime = curtime()

  -- Make a request to the server
  for idx=0,VesselAPI.GetCount()-1 do
    local exists = VesselAPI.GetParameter(idx,0)
    local physics_type = VesselAPI.GetParameter(idx,4)
    if (exists == 1) and
       ((physics_type == 1) or (physics_type == 3)) then
      local net_id = VesselAPI.GetParameter(idx,2)
      local mjd = DedicatedServerAPI.GetMJD()
      local current_date = os.date("*t",86400.0 * (mjd + 2400000.5 - 2440587.5))
      local ep = (current_date.year-2012)*366 + current_date.yday +
        ((current_date.hour*60*60 + current_date.min*60 + current_date.sec - 2*60*60)/86400)
      local sma = VesselAPI.GetParameter(idx,120)
      local e = VesselAPI.GetParameter(idx,121)
      local i = math.deg(VesselAPI.GetParameter(idx,122))
      local mna = math.deg(VesselAPI.GetParameter(idx,123))
      local agp = math.deg(VesselAPI.GetParameter(idx,124))
      local asn = math.deg(VesselAPI.GetParameter(idx,125))
      local bst = VesselAPI.GetParameter(idx,126)
      local per = VesselAPI.GetParameter(idx,127)
      local lat = VesselAPI.GetParameter(idx,14)
      local lon = VesselAPI.GetParameter(idx,15)

      -- Make a request
      local socket = socket.connect("127.0.0.1",80)
      if socket then
        socket:settimeout(0.01)
        socket:send([[
GET /local/vessel_orbital.php?]]..
[[net_id=]]..net_id..
[[&ep=]]..ep..
[[&e=]]..e..
[[&i=]]..i..
[[&asn=]]..asn..
[[&sma=]]..sma..
[[&mna=]]..mna..
[[&agp=]]..agp..
[[&bst=]]..bst..
[[&per=]]..per..
[[&lat=]]..lat..
[[&lon=]]..lon..
[[ HTTP/1.0
Host: vsfl.wireos.com
User-Agent: VSFL

]])
        socket:close()
      end
    end
  end
end)


AddCallback("frame","VSFLUpdateStatus",function(dT)
  -- Do constant tracking of apogee/perigee
  for idx=0,VesselAPI.GetCount()-1 do
    local exists = VesselAPI.GetParameter(idx,0)
    local physics_type = VesselAPI.GetParameter(idx,4)
    if (exists == 1) and
       ((physics_type == 1) or (physics_type == 3)) then
      local elevation = VesselAPI.GetParameter(idx,16)
      prevAltitude1[idx] = prevAltitude1[idx] or elevation
      prevAltitude2[idx] = prevAltitude2[idx] or elevation
      
      local pathname = string.format(PluginsFolder.."vsfl/%05d/radar_tracking.txt",VesselAPI.GetParameter(idx,2))
      local time = VesselAPI.GetParameter(idx,130) / 86400
      
      if (prevAltitude2[idx] < prevAltitude1[idx]) and
         (elevation < prevAltitude1[idx]) then
        prevApogee[idx] = elevation
        
        if prevApogee[idx] and prevPerigee[idx] then
          local f = io.open(pathname,"a+")
          if f then
            f:write(time,"\t",prevApogee[idx],"\t",prevPerigee[idx],"\n")
            f:close()
          end
        end
      end
      if (prevAltitude2[idx] > prevAltitude1[idx]) and
         (elevation > prevAltitude1[idx]) then
        prevPerigee[idx] = elevation

        if prevApogee[idx] and prevPerigee[idx] then
          local f = io.open(pathname,"a+")
          if f then
            f:write(time,"\t",prevApogee[idx],"\t",prevPerigee[idx],"\n")
            f:close()
          end
        end
      end
      
      prevAltitude2[idx] = prevAltitude1[idx]
      prevAltitude1[idx] = elevation
    end
  end
end)
