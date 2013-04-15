--============================================================================--
-- X-Space console GUI
--============================================================================--



local consoleLog = {
  "","","","","","","","",
  "","","","","","","",""
}

local consoleLabels = {
}

local W,H = 400,32+32+24+#consoleLog*16
GUI.Dialogs.Console = GUI.CreateWindow("Debug console",300,100,W,H)
GUI.Dialogs.Console.HasCloseBoxes = 1
local subWindow = GUI.Dialogs.Console:Add("SubWindow","",16,32,W-32,H-48-24)
subWindow.Style = "Screen"
local textEntry = GUI.Dialogs.Console:Add("TextField","",16,H-30,W-32-64,20)
local textAccept = GUI.Dialogs.Console:Add("Button","Run",W-16-64,H-30,64,20)
textAccept.OnPressed = function()
  -- Get command sourcecode
  local lua_source = "Console.Write(\"%s\",tostring("..textEntry.Text.." or nil))"
  textEntry.Text = ""
  
  -- Run command
  local lua_command,message = loadstring(lua_source,"")
  if lua_command then
    local result,message = pcall(lua_command)
    if not result then
      Console.Write("%s",tostring(message))
    end
  else
    Console.Write("%s",tostring(message))
  end
end
  
for k,v in pairs(consoleLog) do
  consoleLabels[k] = GUI.Dialogs.Console:Add("Caption",v,20,36+16*(k-1),W-32,16)
  consoleLabels[k].Lit = 1
end


-- Console API
Console = {}

function Console.Write(...)
  local outString = string.format(...)
  
  table.remove(consoleLog,1)
  table.insert(consoleLog,outString)
  
  for k,v in pairs(consoleLog) do
    consoleLabels[k].Text = v
  end
end

function Console.WriteDebug(...)
  Console.Write(...)
  GUI.Dialogs.Console:Show()
end

function InternalCallbacks.OnIVSSMessage(message)
  Console.Write("IVSS: %s",message or "")
end

Console.Write("X-Space debug console initialized")
