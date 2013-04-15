--============================================================================--
-- X-Space general GUI (spaceflight menu, dialog boxes)
--============================================================================--
local ScreenWidth,ScreenHeight = GUIAPI.GetScreenSize()




--------------------------------------------------------------------------------
-- Implementation of some menu options
--------------------------------------------------------------------------------
function GUI.ReloadGUICode()
  print("X-Space: Reloading user interface\n")
  InternalCallbacks.OnGUIDeinitialize()
  Include(PluginsFolder.."lua/gui.lua")
end

function GUI.ReloadNetworkCode()
  print("X-Space: Reloading networking code\n")
  if Net.Server and Net.Server.Stop then Net.Server.Stop() end
  if Net.Client and Net.Client.Disconnect then Net.Client.Disconnect() end
  Include(PluginsFolder.."lua/network.lua")
end

function GUI.ReloadEngineCode()
  print("X-Space: Reloading engine code\n")
  Include(PluginsFolder.."lua/engines.lua")
end

function GUI.ReloadIVSSModels()
  print("X-Space: Reloading IVSS models\n")
  for idx=0,VesselAPI.GetCount()-1 do
    local exists = VesselAPI.GetParameter(idx,0)
    if exists == 1 then
      IVSSAPI.ReloadModel(idx)
    end
  end
end




--------------------------------------------------------------------------------
-- Initialize the spaceflight menu
--------------------------------------------------------------------------------
GUI.Menu.AddItem("About...",function() GUI.Dialogs.About:Show() end)
GUI.Menu.AddItem("Configure plugin",function() GUI.Dialogs.Options:Show() end)
GUI.Menu.Debug = GUI.Menu.Add("Debug plugin")
  GUI.Menu.AddItem("Show debug console",GUI.Menu.Debug,function()
    GUI.Dialogs.Console:Show()
  end)
  GUI.Menu.AddSeparator(GUI.Menu.Debug)
  GUI.Menu.AddItem("Reload GUI code",GUI.Menu.Debug,GUI.ReloadGUICode)
  GUI.Menu.AddItem("Reload networking code",GUI.Menu.Debug,GUI.ReloadNetworkCode)
  GUI.Menu.AddItem("Reload engine code",GUI.Menu.Debug,GUI.ReloadEngineCode)
  GUI.Menu.AddItem("Reload IVSS models",GUI.Menu.Debug,GUI.ReloadIVSSModels)
  GUI.Menu.AddItem("Crash simulator",GUI.Menu.Debug,function() end)
GUI.Menu.AddSeparator()
GUI.Menu.AddItem("Start multiplayer session...",function() GUI.Dialogs.Multiplayer:Show() end)
GUI.Menu.AddItem("End multiplayer session",function() end)
GUI.Menu.AddSeparator()
--GUI.Menu.AddItem("Vessels list",function() end)
GUI.Menu.AddItem("General information",function() GUI.Dialogs.VesselInfo:Show() end)
GUI.Menu.AddItem("Geomagnetic information",function() GUI.Dialogs.VesselGeoInfo:Show() end)
GUI.Menu.AddItem("Atmospheric information",function() GUI.Dialogs.VesselAtmInfo:Show() end)
--GUI.Menu.AddItem("Orbital information",function() end)
GUI.Menu.GoToLaunchPad = GUI.Menu.Add("Place vessel at a launch pad")
--[[GUI.Menu.GoToOrbit     = GUI.Menu.Add("Place vessel in orbit")
  GUI.Menu.AddItem("Low-eccentricity orbit",GUI.Menu.GoToOrbit,function() end)
  GUI.Menu.AddItem("High-eccentricity orbit",GUI.Menu.GoToOrbit,function() end)
  GUI.Menu.AddItem("Geo-synchonous orbit",GUI.Menu.GoToOrbit,function() end)
  GUI.Menu.AddItem("Geo-stationary orbit",GUI.Menu.GoToOrbit,function() end)
  GUI.Menu.AddSeparator(GUI.Menu.GoToOrbit)
  GUI.Menu.AddItem("Reentry trajectory (low angle)",GUI.Menu.GoToOrbit,function() end)
  GUI.Menu.AddItem("Reentry trajectory (high angle)",GUI.Menu.GoToOrbit,function() end)
  GUI.Menu.AddItem("Moon return trajectory",GUI.Menu.GoToOrbit,function() end)]]--
GUI.Menu.AddSeparator()
GUI.Menu.AddItem("Edit mount offsets...",function() GUI.Dialogs.MountOffsets:Show() end)
GUI.Menu.AddItem("Edit hull...",function() GUI.Dialogs.HullEditor:Show() end)
GUI.Menu.AddSeparator()
GUI.Menu.AddItem("Heating simulator",function() GUI.Dialogs.HeatingSimulator:Show() end)
GUI.Menu.AddItem("Materials database",function() GUI.Dialogs.MaterialsDatabase:Show() end)
GUI.Menu.AddItem("Unit conversion",function() GUI.Dialogs.UnitConversion:Show() end)
  
  
  
  
--------------------------------------------------------------------------------
-- Quick debug menu dialog
--------------------------------------------------------------------------------
local W,H = 200,120
GUI.Dialogs.QuickDebug = GUI.CreateWindow("Quick debug menu",0,ScreenHeight-2*H,W,H)
GUI.Dialogs.QuickDebug.HasCloseBoxes = 1
local quick1 = GUI.Dialogs.QuickDebug:Add("Button","Reload GUI code",16,24+24*0,W-32,24)
local quick2 = GUI.Dialogs.QuickDebug:Add("Button","Reload network code",16,24+24*1,W-32,24)
local quick3 = GUI.Dialogs.QuickDebug:Add("Button","Reload engine code",16,24+24*2,W-32,24)

quick1.OnPressed = function()
  AddCallback("frame","QuickDebugReloadGUI",function()
    GUI.ReloadGUICode()
    DeleteCallback("frame","QuickDebugReloadGUI")
  end)
end
quick2.OnPressed = GUI.ReloadNetworkCode
quick3.OnPressed = GUI.ReloadEngineCode

--GUI.Dialogs.QuickDebug:Show()




--------------------------------------------------------------------------------
-- Launch pads menu
--------------------------------------------------------------------------------
local launchPadCount = LaunchPadsAPI.GetCount()
local launchPadMenu = {}
for idx=0,launchPadCount-1 do
  local padName,complexName = LaunchPadsAPI.GetName(idx)
--  local planetIndex = LaunchPadsAPI.GetParameter(idx,4)
--  local currentPlanet = 0 --FIXME
  
  if not launchPadMenu[complexName] then
    launchPadMenu[complexName] = GUI.Menu.Add(complexName,GUI.Menu.GoToLaunchPad)
  end
  
  GUI.Menu.AddItem(padName,launchPadMenu[complexName],
    function()
      LaunchPadsAPI.SetVessel(idx,0)
    end)
end





--------------------------------------------------------------------------------
-- About dialog
--------------------------------------------------------------------------------
local labelTexts = {
  "X-Space plugin (C) 2009-2012 by Black Phoenix",
  "",
  "WMM code by Manoj C Nair and Adam Woods",
  "NLMSISE-00 code by Dominik Brodowski",
  "Lua 5.1.4 copyright 1994-2011 PUC-Rio",
  "Simple OGL Image Library by Jonathan Dummer",
  "",
  "Authors of spacecraft included with X-Space are",
  "listed in README.TXT file.",
}

local W,H = 300,32+32+#labelTexts*16
GUI.Dialogs.About = GUI.CreateWindow("About X-Space",nil,nil,W,H)
GUI.Dialogs.About.HasCloseBoxes = 1
local subWindow = GUI.Dialogs.About:Add("SubWindow","",16,32,W-32,H-48)
subWindow.Style = "Screen"
  
for k,v in pairs(labelTexts) do
  local label = GUI.Dialogs.About:Add("Caption",v,20,36+16*(k-1),W-32,16)
  label.Lit = 1
end


--------------------------------------------------------------------------------
-- Configuration dialog
--------------------------------------------------------------------------------
local optionsList = {
  { "Rendering options" },
  {  1, "Use shaders (GLSL v1.20)",
        "Enable shader-based effects (uses GLSL v1.20)",
        "Increases quality of orbital cloud rendering, plasma trails...",
        "" },
  {  2, "Render clouds from orbit",
        "Renders visible cloud layer from orbit. The clouds will be low",
        "quality unless shaders are enabled.",
        "" },
  {  5, "High quality clouds",
        "Uses 7 octaves of procedural noise for generating cloud details.",
        "Might cause noticable FPS drop compared to lower quality clouds.",
        "" },
  {  6, "Clouds cast shadows",
        "Shadows from clouds will be visible on Earth surface. A fairly",
        "cheap and nice looking effect.",
        "" },
  {  3, "Render atmospheric scattering",
        "Draws atmospheric halo around the planet. Only makes sense to",
        "enable this in X-Plane 9 or X-Plane 10 without HDR.",
        "" },
  { 10, "Use particle systems",
        "",
        "",
        "" },
  { 10, "Draw plasma during reentry",
        "",
        "",
        "" },
  { 10, "Draw glow during reentry",
        "Draws glow around very hot parts during reentry.",
        "",
        "" },
  { 10, "Draw volumetric plasma",
        "",
        "",
        "" },

  { "Physics options" },
  {  7, "Use inertial physics",
        "Will use custom high-precision physics for integrating spacecraft",
        "flight in space. It might not support certain X-Plane features,",
        "but provides much higher quality integration." },
  {  8, "Planet rotation",
        "Simulates effects of a rotating planet.",
        "",
        "" },
  { 11, "Use non-spherical gravity",
        "Use EGM96 gravity model",
        "",
        "" },

  { "Debug options" },
  {  4, "Write atmosphere data",
        "Will output atmospheric data (temperatures and pressures) as a",
        "function of altitude. Writes into file located in X-Plane folder",
        "called 'X-Space_Atmosphere.txt'" },
  {  9, "Draw coordinate systems",
        "Draws some global coordinate systems, and displays variables about",
        "current position and state.",
        "" },
  { 12, "Draw engine locations",
        "Draws green trail to indicate where engines are located (for easier",
        "editing).",
        "" },
  { 13, "Draw sensor locations",
        "Draws sensor locations and points on surface they take value from.",
        "",
        "" },
}

local W,H = 400,32+64+20*#optionsList
GUI.Dialogs.Options = GUI.CreateWindow("X-Space configuration",nil,nil,W,H)
GUI.Dialogs.Options.HasCloseBoxes = 1

local subWindow = GUI.Dialogs.Options:Add("SubWindow","",16,H-16-56,W-32,56)
subWindow.Style = "Screen"
local hintCaption1 = GUI.Dialogs.Options:Add("Caption","",16+2,H-16-56+16*0+2,W-32,16)
local hintCaption2 = GUI.Dialogs.Options:Add("Caption","",16+2,H-16-56+16*1+2,W-32,16)
local hintCaption3 = GUI.Dialogs.Options:Add("Caption","",16+2,H-16-56+16*2+2,W-32,16)
hintCaption1.Lit = 1
hintCaption2.Lit = 1
hintCaption3.Lit = 1

for k,v in pairs(optionsList) do
  if v[2] then
    GUI.Dialogs.Options:Add("Caption",v[2],36,26+20*(k-1),W-64,16)
  
    local button = GUI.Dialogs.Options:Add("Button","",16,26+20*(k-1),24,16)
    button.Style = "RadioButton"
    button.Behavior = "CheckBox"
    button.State = Config.Get(v[1])
    button.OnChanged = function()
      hintCaption1.Text = v[3]
      hintCaption2.Text = v[4]
      hintCaption3.Text = v[5]
      Config.Set(v[1],button.State)
      Config.Save()
    end
  else
    GUI.Dialogs.Options:Add("Caption",v[1],16,26+20*(k-1),W-64,16)
  end
end


--------------------------------------------------------------------------------
-- Multiplayer GUI
--------------------------------------------------------------------------------
local W,H = 270,130
GUI.Dialogs.Multiplayer = GUI.CreateWindow("Multiplayer",nil,nil,W,H)
GUI.Dialogs.Multiplayer.HasCloseBoxes = 1
GUI.Dialogs.Multiplayer:Add("Caption","Host:",16,26+16*0,W-32,16)
GUI.Dialogs.Multiplayer:Add("TextField","localhost",16,26+16*1,W-32,16)
GUI.Dialogs.Multiplayer:Add("Caption","Port:",16,26+16*2,W-32,16)
GUI.Dialogs.Multiplayer:Add("TextField","10105",16,26+16*3,W-32,16)

local connectTo = GUI.Dialogs.Multiplayer:Add("Button","Connect to server",32,26+16*5,96,16)
local hostServer = GUI.Dialogs.Multiplayer:Add("Button","Host a server",W-32-96,26+16*5,96,16)
local reloadNet = GUI.Dialogs.Multiplayer:Add("Button","Reload Net Code",W-16-96,26+16*2,96,16)

connectTo.OnPressed = function()
  if DEBUG_VSFL == true
  then Net.Client.Connect("localhost",10205)
  else Net.Client.Connect("localhost",10105)
  end
end
reloadNet.OnPressed = GUI.ReloadNetworkCode

--GUI.Dialogs.Multiplayer:Show()


-- Load additional features
Include(PluginsFolder.."lua/gui_features.lua")


--------------------------------------------------------------------------------
-- Error dialog
--------------------------------------------------------------------------------
local W,H = 300,110
GUI.Dialogs.Error = GUI.CreateWindow("X-Space Error",nil,nil,W,H)
GUI.Dialogs.Error.HasCloseBoxes = 1
GUI.Dialogs.Error:Add("SubWindow","",16,36,W-32,H-40-32)
--GUI.Dialogs.Error:Add("Caption","An error has occured:",16,30+16*0,W-32,16)
local errorText = GUI.Dialogs.Error:Add("Caption","(unknown error)",16,26+16*1,W-32,16)
local okButton = GUI.Dialogs.Error:Add("Button","OK",W/2-96/2,H-24,96,16)
okButton.OnPressed = function()
  GUI.Dialogs.Error:Hide()
end

function InternalCallbacks.OnErrorMessage(text)
  errorText.Text = text or "(unknown error)"
  GUI.Dialogs.Error:Show()
end
