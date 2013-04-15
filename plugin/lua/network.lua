--============================================================================--
-- X-Space highlevel networking
--------------------------------------------------------------------------------
--
--============================================================================--

-- Setup networking API
Net = {}

-- Protocol version
Net.ProtocolVersion = 4

-- Event types
Net.EventType = {
  None = 0,
  Connected = 1,
  Disconnected = 2,
  Message = 3,
}

-- Message ID's
Net.Message = {
  World = 1,        -- Message containing state of the world
  Features = 2,     -- Description of features client requests/server gives
  TimeSync = 3,     -- Time synchronization
  Transmission = 4, -- Radio transmission
}

-- Invalid data cycle
Net.InvalidCycle = 0

-- Functions for working with messages
function Net.Write8(msg,value)       NetAPI.MessageWrite(msg,0,value) end
function Net.Write16(msg,value)      NetAPI.MessageWrite(msg,1,value) end
function Net.Write32(msg,value)      NetAPI.MessageWrite(msg,2,value) end
function Net.WriteSingle(msg,value)  NetAPI.MessageWrite(msg,3,value) end
function Net.WriteDouble(msg,value)  NetAPI.MessageWrite(msg,4,value) end
function Net.Read8(msg,value)        return NetAPI.MessageRead(msg,0) end
function Net.Read16(msg,value)       return NetAPI.MessageRead(msg,1) end
function Net.Read32(msg,value)       return NetAPI.MessageRead(msg,2) end
function Net.ReadSingle(msg,value)   return NetAPI.MessageRead(msg,3) end
function Net.ReadDouble(msg,value)   return NetAPI.MessageRead(msg,4) end

-- Load client and server code
Include(PluginsFolder.."lua/network_frame.lua")
Include(PluginsFolder.."lua/network_client.lua")
Include(PluginsFolder.."lua/network_server.lua")
if DEDICATED_SERVER then Include(PluginsFolder.."lua/network_http.lua") end
Include(PluginsFolder.."lua/network_radio.lua")
