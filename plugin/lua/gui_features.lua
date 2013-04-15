--============================================================================--
-- X-Space features GUI
--============================================================================--




--------------------------------------------------------------------------------
-- Local vessel information dialog
--------------------------------------------------------------------------------
local W,H = 425,500
GUI.Dialogs.VesselInfo = GUI.CreateWindow("Vessel information",nil,nil,W,H)
GUI.Dialogs.VesselInfo.HasCloseBoxes = 1

GUI.Dialogs.VesselInfo:Add("Caption","Vessel index:",16,24+16*0,W-2,16)
local vesselSelector = GUI.Dialogs.VesselInfo:Add("ScrollBar","",16+96,24+16*0,W-32-96,16)

GUI.Dialogs.VesselInfo:Add("Caption","Vessel inertial coordinates",16,24+16*1-4,W-2,16)
local subWindow = GUI.Dialogs.VesselInfo:Add("SubWindow","",8,24+16*2,200,16*15+4)
      subWindow.Style = "Screen"
GUI.Dialogs.VesselInfo:Add("Caption","Vessel non-inertial coordinates",16+208,24+16*1-4,W-2,16)
local subWindow = GUI.Dialogs.VesselInfo:Add("SubWindow","",8+208,24+16*2,200,16*15+4)
      subWindow.Style = "Screen"
      
GUI.Dialogs.VesselInfo:Add("Caption","General variables",16,24+16*18-4,W-2,16)
local subWindow = GUI.Dialogs.VesselInfo:Add("SubWindow","",8,24+16*19,200,16*10+4)
      subWindow.Style = "Screen"
      
GUI.Dialogs.VesselInfo:Add("Caption","Physics variables",16+208,24+16*18-4,W-2,16)
local subWindow = GUI.Dialogs.VesselInfo:Add("SubWindow","",8+208,24+16*19,200,16*10+4)
      subWindow.Style = "Screen"
      
local physicsType = {
  [0] = "Simulator",
  [1] = "Inertial",
  [2] = "Inertial transfer",
  [3] = "Non-inertial",
}
local boolStr = {
  [0] = "No",
  [1] = "Yes",
}

local parameterLabels = {
  {  2, 0, 30, " X",    "%.3f m" },
  {  3, 0, 31, " Y",    "%.3f m" },
  {  4, 0, 32, " Z",    "%.3f m" },
  {  5, 0, 33, "VX",    "%.6f m/s" },
  {  6, 0, 34, "VY",    "%.6f m/s" },
  {  7, 0, 35, "VZ",    "%.6f m/s" },
  {  8, 0, 36, "AX",    "%.9f m/s2" },
  {  9, 0, 37, "AY",    "%.9f m/s2" },
  { 10, 0, 38, "AZ",    "%.9f m/s2" },
  { 11, 0,  0, "Pitch", "ERROR" },
  { 12, 0,  0, "Yaw",   "ERROR" },
  { 13, 0,  0, "Roll",  "ERROR" },
  { 14, 0, 43, "P",     "%.5f deg/s" },
  { 15, 0, 44, "Q",     "%.5f deg/s" },
  { 16, 0, 45, "R",     "%.5f deg/s" },

  { 19, 0,  0, "Index",       function(n,idx) return tostring(idx) end },
  { 20, 0,  1, "Attached",    function(n) return boolStr[n] or "Unknown" end },
  { 21, 0,  2, "NetID",       "%d" },
  { 22, 0,  3, "Networked",   function(n) return boolStr[n] or "Unknown" end },
  { 23, 0,  4, "PhysicsType", function(n) return physicsType[n] or "Unknown" end },
  { 24, 0,  5, "Aircraft",    function(n) return boolStr[n] or "Unknown" end },
  { 25, 0,  6, "PlaneID",     "%d" },

  { 26, 0, 14, "Latitude", "%.6f deg" },
  { 27, 0, 15, "Longitude", "%.6f deg" },
  { 28, 0, 16, "Elevation", "%.0f m" },

  
  {  2, 1, 50, " X",    "%.3f m" },
  {  3, 1, 51, " Y",    "%.3f m" },
  {  4, 1, 52, " Z",    "%.3f m" },
  {  5, 1, 53, "VX",    "%.6f m/s" },
  {  6, 1, 54, "VY",    "%.6f m/s" },
  {  7, 1, 55, "VZ",    "%.6f m/s" },
  {  8, 1, 56, "AX",    "%.6f m/s2" },
  {  9, 1, 57, "AY",    "%.6f m/s2" },
  { 10, 1, 58, "AZ",    "%.6f m/s2" },
  { 11, 1,  0, "Pitch", "ERROR" },
  { 12, 1,  0, "Yaw",   "ERROR" },
  { 13, 1,  0, "Roll",  "ERROR" },
  { 14, 1, 63, "P",     "%.5f deg/s" },
  { 15, 1, 64, "Q",     "%.5f deg/s" },
  { 16, 1, 65, "R",     "%.5f deg/s" },
  
  { 19, 1, 10, "jxx", "%.3f m2" },
  { 20, 1, 11, "jyy", "%.3f m2" },
  { 21, 1, 12, "jzz", "%.3f m2" },
  { 22, 1,  7, "Ixx", "%.0f kg m2" },
  { 23, 1,  8, "Iyy", "%.0f kg m2" },
  { 24, 1,  9, "Izz", "%.0f kg m2" },
  { 25, 1, 13, "Mass", "%.3f kg" },
}

for k,v in pairs(parameterLabels) do
  v[6] = GUI.Dialogs.VesselInfo:Add("Caption","ERROR", 80+208*v[2],24+16*v[1],W-32,16)
  v[6].Lit = 1
  v[7] = GUI.Dialogs.VesselInfo:Add("Caption",v[4]..":",8+4+208*v[2],24+16*v[1],W-32,16)
  v[7].Lit = 1
end

local stateUpdateLabel = GUI.Dialogs.VesselInfo:Add("Caption","",0,0,1,1)
stateUpdateLabel.OnPaint = function(self)
  local idx = vesselSelector.SliderPosition
  vesselSelector.Min = 0
  vesselSelector.Max = VesselAPI.GetCount()-1
  
  for k,v in pairs(parameterLabels) do
    if type(v[5]) == "string" then
      v[6].Text = string.format(v[5],VesselAPI.GetParameter(idx,v[3]) or 0)
    else
      v[6].Text = v[5](VesselAPI.GetParameter(idx,v[3]) or 0,idx)
    end
  end
end




--------------------------------------------------------------------------------
-- Local vessel geomagnetic information dialog
--------------------------------------------------------------------------------
local W,H = 225,280
GUI.Dialogs.VesselGeoInfo = GUI.CreateWindow("Vessel geomagnetic information",nil,nil,W,H)
GUI.Dialogs.VesselGeoInfo.HasCloseBoxes = 1

GUI.Dialogs.VesselGeoInfo:Add("Caption","Vessel index:",16,24+16*0,W-2,16)
local vesselSelector = GUI.Dialogs.VesselGeoInfo:Add("ScrollBar","",16+96,24+16*0,W-32-96,16)

GUI.Dialogs.VesselGeoInfo:Add("Caption","Geomagnetic variables:",16,24+16*1-4,W-2,16)
local subWindow = GUI.Dialogs.VesselGeoInfo:Add("SubWindow","",8,24+16*2,W-16,16*13+4)
      subWindow.Style = "Screen"

local parameterLabels = {
  {  2, 0, 70, "Incl", "%.3f deg" },
  {  3, 0, 71, "Decl", "%.3f deg" },
  {  4, 0, 72, "F",    "%.3f nT" },
  {  5, 0, 73, "H",    "%.3f nT" },
  {  6, 0, 74, "MX",   "%.3f nT" },
  {  7, 0, 75, "MY",   "%.3f nT" },
  {  8, 0, 76, "MZ",   "%.3f nT" },
  {  9, 0, 77, "LX",   "%.3f nT" },
  { 10, 0, 78, "LY",   "%.3f nT" },
  { 11, 0, 79, "LZ",   "%.3f nT" },
  { 12, 0, 80, "GV",   "%.3f deg" },
  { 13, 0, 81, "g",    "%.7f m/s2" },
  { 14, 0, 82, "V",    "%.0f m2/s2" },
}

for k,v in pairs(parameterLabels) do
  v[6] = GUI.Dialogs.VesselGeoInfo:Add("Caption","ERROR", 80+208*v[2],24+16*v[1],W-32,16)
  v[6].Lit = 1
  v[7] = GUI.Dialogs.VesselGeoInfo:Add("Caption",v[4]..":",8+4+208*v[2],24+16*v[1],W-32,16)
  v[7].Lit = 1
end

local stateUpdateLabel = GUI.Dialogs.VesselGeoInfo:Add("Caption","",0,0,1,1)
stateUpdateLabel.OnPaint = function(self)
  local idx = vesselSelector.SliderPosition
  vesselSelector.Min = 0
  vesselSelector.Max = VesselAPI.GetCount()-1

  for k,v in pairs(parameterLabels) do
    v[6].Text = string.format(v[5],VesselAPI.GetParameter(idx,v[3]) or 0)
  end
end




--------------------------------------------------------------------------------
-- Local vessel atmospheric information dialog
--------------------------------------------------------------------------------
local W,H = 425,280
GUI.Dialogs.VesselAtmInfo = GUI.CreateWindow("Vessel atmospheric information",nil,nil,W,H)
GUI.Dialogs.VesselAtmInfo.HasCloseBoxes = 1

GUI.Dialogs.VesselAtmInfo:Add("Caption","Vessel index:",16,24+16*0,W-2,16)
local vesselSelector = GUI.Dialogs.VesselAtmInfo:Add("ScrollBar","",16+96,24+16*0,W-32-96,16)

GUI.Dialogs.VesselAtmInfo:Add("Caption","Atmospheric variables:",16,24+16*1-4,W-2,16)
local subWindow = GUI.Dialogs.VesselAtmInfo:Add("SubWindow","",8,24+16*2,200,16*13+4)
      subWindow.Style = "Screen"
GUI.Dialogs.VesselAtmInfo:Add("Caption","Partial concentration:",16+208,24+16*1-4,W-2,16)
local subWindow = GUI.Dialogs.VesselAtmInfo:Add("SubWindow","",8+208,24+16*2,200,16*13+4)
      subWindow.Style = "Screen"

local parameterLabels = {
  {  2, 0, 100, "rho",  "%.3E kg/m3" },
  {  3, 0, 100, "rho",  function(n) return string.format("%.2f%%",100*n/1.24) end },
  {  4, 0, 101, "n",    "%.3E m-1" },
  {  5, 0, 102, "T",    "%.3f K"  },
  {  6, 0, 103, "P",    "%.1f Pa" },
  {  7, 0, 104, "Q",    "%.3E Pa" },
  
  {  2, 1, 106, "He",   "%.3E m-1" },
  {  3, 1, 107, "O",    "%.3E m-1" },
  {  4, 1, 108, "N2",   "%.3E m-1" },
  {  5, 1, 109, "O2",   "%.3E m-1" },
  {  6, 1, 110, "Ar",   "%.3E m-1" },
  {  7, 1, 111, "H",    "%.3E m-1" },
  {  8, 1, 112, "N",    "%.3E m-1" },
}

for k,v in pairs(parameterLabels) do
  v[6] = GUI.Dialogs.VesselAtmInfo:Add("Caption","ERROR", 80+208*v[2],24+16*v[1],W-32,16)
  v[6].Lit = 1
  v[7] = GUI.Dialogs.VesselAtmInfo:Add("Caption",v[4]..":",8+4+208*v[2],24+16*v[1],W-32,16)
  v[7].Lit = 1
end

local stateUpdateLabel = GUI.Dialogs.VesselAtmInfo:Add("Caption","",0,0,1,1)
stateUpdateLabel.OnPaint = function(self)
  local idx = vesselSelector.SliderPosition
  vesselSelector.Min = 0
  vesselSelector.Max = VesselAPI.GetCount()-1

  for k,v in pairs(parameterLabels) do
    if type(v[5]) == "string" then
      v[6].Text = string.format(v[5],VesselAPI.GetParameter(idx,v[3]) or 0)
    else
      v[6].Text = v[5](VesselAPI.GetParameter(idx,v[3]) or 0,idx)
    end
  end
end




--------------------------------------------------------------------------------
-- Heating simulator
--------------------------------------------------------------------------------
local W,H = 400,404
GUI.Dialogs.HeatingSimulator = GUI.CreateWindow("Heating simulator",nil,nil,W,H)
GUI.Dialogs.HeatingSimulator.HasCloseBoxes = 1
GUI.Dialogs.HeatingSimulator:Add("SubWindow","",10,24+16*1,W-20,16*2)
GUI.Dialogs.HeatingSimulator:Add("SubWindow","",10,24+16*4,W-20,16*9)
GUI.Dialogs.HeatingSimulator:Add("SubWindow","",10,24+16*14,W-200-32-8,16*9)
GUI.Dialogs.HeatingSimulator:Add("SubWindow","",10+W-32-200,24+16*14,216,16*9)

GUI.Dialogs.HeatingSimulator:Add("Caption","Heating simulator allows testing vessel under various thermal",16,28+16*0,W-32,16)
GUI.Dialogs.HeatingSimulator:Add("Caption","conditions, velocities, pressures.",16,28+16*1,W-32,16)
local altitudeCaption  = GUI.Dialogs.HeatingSimulator:Add("Caption","",  16,28+16*4, W-32,16)
local altitudeSelector = GUI.Dialogs.HeatingSimulator:Add("ScrollBar","",16,28+16*5, W-32,16)
local alphaCaption     = GUI.Dialogs.HeatingSimulator:Add("Caption","",  16,28+16*6, W-32,16)
local alphaSelector    = GUI.Dialogs.HeatingSimulator:Add("ScrollBar","",16,28+16*7, W-32,16)
local betaCaption      = GUI.Dialogs.HeatingSimulator:Add("Caption","",  16,28+16*8, W-32,16)
local betaSelector     = GUI.Dialogs.HeatingSimulator:Add("ScrollBar","",16,28+16*9, W-32,16)
local velocityCaption  = GUI.Dialogs.HeatingSimulator:Add("Caption","",  16,28+16*10,W-32,16)
local velocitySelector = GUI.Dialogs.HeatingSimulator:Add("ScrollBar","",16,28+16*11,W-32,16)

local resetButton = GUI.Dialogs.HeatingSimulator:Add("Button","Reset simulation",16,28+16*15,W-32-200-20,16)
resetButton.OnPressed = function() DragHeatAPI.ResetSimulation() end
GUI.Dialogs.HeatingSimulator:Add("Caption","Enable simulation",  16+16+2,30+16*16,W-32-200-20,16)
local enabledBox = GUI.Dialogs.HeatingSimulator:Add("Button","",16,30+16*16,16,16)
enabledBox.Style = "RadioButton"
enabledBox.Behavior = "CheckBox"
GUI.Dialogs.HeatingSimulator:Add("Caption","Interpolate values",  16+16+2,30+16*17,W-32-200-20,16)
local interpolateBox = GUI.Dialogs.HeatingSimulator:Add("Button","",16,30+16*17,16,16)
interpolateBox.Style = "RadioButton"
interpolateBox.Behavior = "CheckBox"
interpolateBox.State = 1
local tpsWeight = GUI.Dialogs.HeatingSimulator:Add("Caption","TPS weight: 0 kg",16,30+16*19,W-32-200-20,16)
local hullWeight = GUI.Dialogs.HeatingSimulator:Add("Caption","Hull weight: 0 kg",16,30+16*20,W-32-200-20,16)
local chassisWeight = GUI.Dialogs.HeatingSimulator:Add("Caption","Chassis weight: 0 kg",16,30+16*21,W-32-200-20,16)

GUI.Dialogs.HeatingSimulator:Add("Caption","Select display mode:",16+W-32-200,28+16*14,W-32,16)
local visualButtons = {
  "Normal view",
  "Heat flux",
  "Surface temperature",
  "Hull temperature",
  "Dynamic pressure",
  "FEM",
  "Materials",
  "Shockwaves",
}
for k,v in pairs(visualButtons) do
  local button = GUI.Dialogs.HeatingSimulator:Add("Button",v,16+W-32-200,28+16*(14+k),200,16)
  button.OnPressed = function() DragHeatAPI.SetVisualMode(k-1) end
end

altitudeSelector.Min = 0 -- x 100
altitudeSelector.Max = 2000 -- x 100
altitudeSelector.SliderPosition = 10
alphaSelector.Min = -1800
alphaSelector.Max = 1800
betaSelector.Min = -900
betaSelector.Max = 900
velocitySelector.Min = 0
velocitySelector.Max = 10000

altitudeCaption.OnPaint = function()
  DragHeatAPI.SetSimulationState(enabledBox.State,
    altitudeSelector.SliderPosition*100,
    alphaSelector.SliderPosition*0.1,
    betaSelector.SliderPosition*0.1,
    velocitySelector.SliderPosition)
  DragHeatAPI.SetVisualInterpolation(interpolateBox.State)
    
  altitudeCaption.Text = string.format("Altitude (%.0f m):",  altitudeSelector.SliderPosition*100)
  alphaCaption.Text    = string.format("Alpha (%.2f deg):",   alphaSelector.SliderPosition*0.1)
  betaCaption.Text     = string.format("Beta (%.2f deg):",    betaSelector.SliderPosition*0.1)
  velocityCaption.Text = string.format("Velocity (%.0f m/s):",velocitySelector.SliderPosition)
  
  tpsWeight.Text       = string.format("TPS weight: %.0f kg",VesselAPI.GetParameter(0,94))
  hullWeight.Text      = string.format("Hull weight: %.0f kg",VesselAPI.GetParameter(0,18))
  chassisWeight.Text   = string.format("Chassis weight: %.0f kg",VesselAPI.GetParameter(0,17))
end

--GUI.Dialogs.HeatingSimulator:Show()




--------------------------------------------------------------------------------
-- Hull editor
--------------------------------------------------------------------------------
local W,H = 560,548
GUI.Dialogs.HullEditor = GUI.CreateWindow("Hull editor",nil,nil,W,H)
GUI.Dialogs.HullEditor.HasCloseBoxes = 1
GUI.Dialogs.HullEditor:Add("SubWindow","",10,24+16*0,W-20,16*6)
GUI.Dialogs.HullEditor:Add("SubWindow","",10,24+16*7,W-20,16*4)
GUI.Dialogs.HullEditor:Add("SubWindow","",10,24+16*12,215,16*9)
GUI.Dialogs.HullEditor:Add("SubWindow","",245,24+16*12,W-215-40,16*9)
GUI.Dialogs.HullEditor:Add("SubWindow","",10,24+16*22,W-20-200-20,16*10)
GUI.Dialogs.HullEditor:Add("SubWindow","",10+W-20-200,24+16*22,200,16*10)

GUI.Dialogs.HullEditor:Add("Caption","Select material and click on the spacecraft mesh to set its material",16,28+16*0,W-32,16)
local paintCaption1 = GUI.Dialogs.HullEditor:Add("Caption","Paint material (none):",16,28+16*1,W-32,16)
local materialSelector = GUI.Dialogs.HullEditor:Add("ScrollBar","",16,28+16*2,W-32,16)

local paintCaption2 = GUI.Dialogs.HullEditor:Add("Caption","Paint thickness (error):",16,28+16*3,W-32,16)
local thicknessSelector = GUI.Dialogs.HullEditor:Add("ScrollBar","",16,28+16*4,W-32,16)
thicknessSelector.Min = 1
thicknessSelector.Max = 300
thicknessSelector.SliderPosition = 50

GUI.Dialogs.HullEditor:Add("Caption","Instructions:",245+8,28+16*12,W-32,16)
GUI.Dialogs.HullEditor:Add("Caption","1. Open aircraft and all stages in Plane Maker",245+8,28+16*13,W-32,16)
GUI.Dialogs.HullEditor:Add("Caption","2. Go to menu \"Special\"",245+8,28+16*14,W-32,16)
GUI.Dialogs.HullEditor:Add("Caption","3. Select \"Generate OBJs from Aircraft, 1 OBJ per part\"",245+8,28+16*15,W-32,16)
GUI.Dialogs.HullEditor:Add("Caption","4. Load OBJ files into X-Space, and adjust offsets",245+8,28+16*16,W-32,16)
GUI.Dialogs.HullEditor:Add("Caption","5. Save drag model",245+8,28+16*17,W-32,16)
GUI.Dialogs.HullEditor:Add("Caption","6. Reload drag model (to reinitialize simulation)",245+8,28+16*18,W-32,16)

GUI.Dialogs.HullEditor:Add("Caption","Importing/saving model:",16,28+16*7,W-32,16)
local loadOBJ = GUI.Dialogs.HullEditor:Add("Button","Load OBJ files",16,28+16*8,(W-32)/2-16,16)
local saveDrag = GUI.Dialogs.HullEditor:Add("Button","Save drag model",16+(W-32)/2+16,28+16*8,(W-32)/2-16,16)
local reloadDrag = GUI.Dialogs.HullEditor:Add("Button","Reload drag model",16+(W-32)/2+16,28+16*9,(W-32)/2-16,16)

loadOBJ.OnPressed = function() DragHeatAPI.SetVisualMode(6) for i=0,VesselAPI.GetCount()-1 do DragHeatAPI.LoadOBJ(i) end end
saveDrag.OnPressed = function() for i=0,VesselAPI.GetCount()-1 do DragHeatAPI.SaveModel(i) end end
reloadDrag.OnPressed = function() for i=0,VesselAPI.GetCount()-1 do DragHeatAPI.ReloadModel(i) end end

GUI.Dialogs.HullEditor:Add("Caption","Select display mode:",16,28+16*12,W-32,16)
for k,v in pairs(visualButtons) do -- See heating simulation
  local button = GUI.Dialogs.HullEditor:Add("Button",v,16,28+16*(12+k),200,16)
  button.OnPressed = function() DragHeatAPI.SetVisualMode(k-1) end
end

local vesselCaption = GUI.Dialogs.HullEditor:Add("Caption","Select vessel to adjust offsets (none):",16,28+16*22,W-32-220,16)
local vesselSelector = GUI.Dialogs.HullEditor:Add("ScrollBar","",16,28+16*23,W-32-220,16)

GUI.Dialogs.HullEditor:Add("Caption","X offset (forward/aft):",16,28+16*25,W-32-220,16)
local Xoffset = GUI.Dialogs.HullEditor:Add("ScrollBar","",     16,28+16*26,W-32-220,16)
GUI.Dialogs.HullEditor:Add("Caption","Y offset (left/right):", 16,28+16*27,W-32-220,16)
local Yoffset = GUI.Dialogs.HullEditor:Add("ScrollBar","",     16,28+16*28,W-32-220,16)
GUI.Dialogs.HullEditor:Add("Caption","Z offset (up/down):",    16,28+16*29,W-32-220,16)
local Zoffset = GUI.Dialogs.HullEditor:Add("ScrollBar","",     16,28+16*30,W-32-220,16)
Xoffset.Min = -10000
Xoffset.Max =  10000
Xoffset.SliderPosition = 0
Yoffset.Min = -1000
Yoffset.Max =  1000
Yoffset.SliderPosition = 0
Zoffset.Min = -1000
Zoffset.Max =  1000
Zoffset.SliderPosition = 0
local previousVessel = -1

GUI.Dialogs.HullEditor:Add("Caption","Interpolate values",  W-32-170+16,30+16*22,200-16-4,16)
local interpolateBox = GUI.Dialogs.HullEditor:Add("Button","",W-32-170,30+16*22,16,16)
interpolateBox.Style = "RadioButton"
interpolateBox.Behavior = "CheckBox"
interpolateBox.State = 1
local tpsWeight = GUI.Dialogs.HullEditor:Add("Caption","TPS weight: 0 kg",W-32-170,30+16*24,200,16)
local hullWeight = GUI.Dialogs.HullEditor:Add("Caption","Hull weight: 0 kg",W-32-170,30+16*25,200,16)
local chassisWeight = GUI.Dialogs.HullEditor:Add("Caption","Chassis weight: 0 kg",W-32-170,30+16*26,200,16)

paintCaption1.OnPaint = function()
  -- Update limits
  materialSelector.Min = 0
  materialSelector.Max = MaterialAPI.GetCount() - 1
  vesselSelector.Min = 0
  vesselSelector.Max = VesselAPI.GetCount()-1

  -- Update material
  local materialName = MaterialAPI.GetName(materialSelector.SliderPosition)
  local materialThickness = 10^(thicknessSelector.SliderPosition*0.01-1)
  paintCaption1.Text = "Paint material ("..materialName.."):"
  paintCaption2.Text = string.format("Paint thickness (%.1f cm):",materialThickness)
  DragHeatAPI.SetPaintMode(materialSelector.SliderPosition)
  DragHeatAPI.SetPaintThickness(materialThickness*0.01)
  
  -- Update vessel
  if vesselSelector.SliderPosition == 0 then
    vesselCaption.Text = "Select vessel to adjust offsets (local vessel):"
  else
    local filename = VesselAPI.GetFilename(vesselSelector.SliderPosition)
    vesselCaption.Text = string.format("Select vessel to adjust offsets (%s):",
      string.sub(filename,(string.find(filename,"/",-#filename) or 0)+1))
  end
  
  -- Update offsets
  if previousVessel ~= vesselSelector.SliderPosition then
    previousVessel = vesselSelector.SliderPosition
    Xoffset.SliderPosition = VesselAPI.GetParameter(previousVessel,23)*100
    Yoffset.SliderPosition = VesselAPI.GetParameter(previousVessel,24)*100
    Zoffset.SliderPosition = VesselAPI.GetParameter(previousVessel,25)*100
  else
    VesselAPI.SetParameter(previousVessel,23,Xoffset.SliderPosition/100)
    VesselAPI.SetParameter(previousVessel,24,Yoffset.SliderPosition/100)
    VesselAPI.SetParameter(previousVessel,25,Zoffset.SliderPosition/100)
  end
  
  -- Update weights data
  tpsWeight.Text       = string.format("TPS weight: %.0f kg",VesselAPI.GetParameter(0,94))
  hullWeight.Text      = string.format("Hull weight: %.0f kg",VesselAPI.GetParameter(0,18))
  chassisWeight.Text   = string.format("Chassis weight: %.0f kg",VesselAPI.GetParameter(0,17))
end

--GUI.Dialogs.HullEditor:Show()




--------------------------------------------------------------------------------
-- Materials database
--------------------------------------------------------------------------------
local W,H = 600,478
GUI.Dialogs.MaterialsDatabase = GUI.CreateWindow("Materials database",nil,nil,W,H)
GUI.Dialogs.MaterialsDatabase.HasCloseBoxes = 1
GUI.Dialogs.MaterialsDatabase:Add("SubWindow","",10,256+35+16*0,W-20,16*5)
GUI.Dialogs.MaterialsDatabase:Add("SubWindow","",10,256+35+16*6,W-20,16*5)

local subWindow = GUI.Dialogs.MaterialsDatabase:Add("SubWindow","",8,24,W-16,256)
      subWindow.Style = "Screen"
local materialGraph = GUI.Dialogs.MaterialsDatabase:Add("Caption","",8+8,24+8,W-16-16,256-16)

local selectM1Caption = GUI.Dialogs.MaterialsDatabase:Add("Caption",  "",16,256+40+16*0,W-32,16)
local selectM1Slider  = GUI.Dialogs.MaterialsDatabase:Add("ScrollBar","",16,256+40+16*1,W-32,16)
local selectM2Caption = GUI.Dialogs.MaterialsDatabase:Add("Caption",  "",16,256+40+16*2,W-32,16)
local selectM2Slider  = GUI.Dialogs.MaterialsDatabase:Add("ScrollBar","",16,256+40+16*3,W-32,16)
local M1Line1 = GUI.Dialogs.MaterialsDatabase:Add("Caption",  "",16,256+40+16*6,W-32,16)
local M1Line2 = GUI.Dialogs.MaterialsDatabase:Add("Caption",  "",32,256+40+16*7,W-32,16)
local M2Line1 = GUI.Dialogs.MaterialsDatabase:Add("Caption",  "",16,256+40+16*8,W-32,16)
local M2Line2 = GUI.Dialogs.MaterialsDatabase:Add("Caption",  "",32,256+40+16*9,W-32,16)
selectM1Slider.SliderPosition = 1
selectM2Slider.SliderPosition = 1
      
materialGraph.OnPaint = function(self)
  selectM1Slider.Min = 1
  selectM1Slider.Max = MaterialAPI.GetCount() - 1
  selectM2Slider.Min = 1
  selectM2Slider.Max = MaterialAPI.GetCount() - 1

  local L,T,R,B = GUIAPI.GetWidgetLTRB(self.WidgetRef)
  MaterialAPI.DrawGUIGraph1(selectM1Slider.SliderPosition,
                            selectM2Slider.SliderPosition,0,L,T-0.9*(T-B)/2,R-L,0.9*(T-B)/2)
  MaterialAPI.DrawGUIGraph1(selectM1Slider.SliderPosition,
                            selectM2Slider.SliderPosition,1,L,B,R-L,0.9*(T-B)/2)
                            
  local M1 = MaterialAPI.GetName(selectM1Slider.SliderPosition)
  local M2 = MaterialAPI.GetName(selectM2Slider.SliderPosition)
                            
  selectM1Caption.Text = "Select material 1 ("..M1.."):"
  selectM2Caption.Text = "Select material 2 ("..M2.."):"
  
  local Rho1,Cp1,k1 = MaterialAPI.GetParameters(selectM1Slider.SliderPosition,273.15+25)
  local Rho2,Cp2,k2 = MaterialAPI.GetParameters(selectM2Slider.SliderPosition,273.15+25)
  M1Line1.Text = string.format("%s (blue line) has density of %.0f kg/m3",M1,Rho1)
  M2Line1.Text = string.format("%s (yellow line) has density of %.0f kg/m3",M2,Rho2)
  M1Line2.Text = string.format("1 kg needs %.0f KJ of heat to reach 500 C. At 500 C it conducts %.3f KW to hull (~5 cm thick)",
    Cp1*1.0*(500+25)*1e-3,
    1e-3*k1*1.0*(500)/0.05)
  M2Line2.Text = string.format("1 kg needs %.0f KJ of heat to reach 500 C. At 500 C it conducts %.3f KW to hull (~5 cm thick)",
    Cp2*1.0*(500+25)*1e-3,
    1e-3*k2*1.0*(500)/0.05)
end

--GUI.Dialogs.MaterialsDatabase:Show()




--------------------------------------------------------------------------------
-- Mount offset editing
--------------------------------------------------------------------------------
local W,H = 400,210
GUI.Dialogs.MountOffsets = GUI.CreateWindow("Edit mount offsets",nil,nil,W,H)
GUI.Dialogs.MountOffsets.HasCloseBoxes = 1
GUI.Dialogs.MountOffsets:Add("SubWindow","",10,24+16*1,W-20,16*2)
GUI.Dialogs.MountOffsets:Add("SubWindow","",10,24+16*4,W-20,16*7)

--GUI.Dialogs.MountOffsets:Show()

local vesselCaption = GUI.Dialogs.MountOffsets:Add("Caption","Select vessel to adjust offsets (none):",16,28+16*0,W-32,16)
local vesselSelector = GUI.Dialogs.MountOffsets:Add("ScrollBar","",16,28+16*1,W-32,16)

local Xcaption = GUI.Dialogs.MountOffsets:Add("Caption","X offset (forward/aft):",16,28+16*4,W-32,16)
local Xoffset = GUI.Dialogs.MountOffsets:Add("ScrollBar","",16,28+16*5,W-32,16)
local Ycaption = GUI.Dialogs.MountOffsets:Add("Caption","Y offset (left/right):",16,28+16*6,W-32,16)
local Yoffset = GUI.Dialogs.MountOffsets:Add("ScrollBar","",16,28+16*7,W-32,16)
local Zcaption = GUI.Dialogs.MountOffsets:Add("Caption","Z offset (up/down):",16,28+16*8,W-32,16)
local Zoffset = GUI.Dialogs.MountOffsets:Add("ScrollBar","",16,28+16*9,W-32,16)
Xoffset.Min = -20000
Xoffset.Max =  20000
Xoffset.SliderPosition = 0
Yoffset.Min = -20000
Yoffset.Max =  20000
Yoffset.SliderPosition = 0
Zoffset.Min = -20000
Zoffset.Max =  20000
Zoffset.SliderPosition = 0
local previousVessel = -1

vesselCaption.OnPaint = function()
  -- Update limits
  vesselSelector.Min = 0
  vesselSelector.Max = VesselAPI.GetCount()-1
  
  -- Update vessel
  if vesselSelector.SliderPosition == 0 then
    vesselCaption.Text = "Select vessel to adjust offsets (local vessel):"
  else
    local filename = VesselAPI.GetFilename(vesselSelector.SliderPosition)
    if not filename then return end
    vesselCaption.Text = string.format("Select vessel to adjust offsets (%s):",
      string.sub(filename,(string.find(filename,"/",-#filename) or 0)+1))
  end

  -- Update offsets
  if previousVessel ~= vesselSelector.SliderPosition then
    previousVessel = vesselSelector.SliderPosition
    Xoffset.SliderPosition = VesselAPI.GetParameter(previousVessel,91)*100
    Yoffset.SliderPosition = VesselAPI.GetParameter(previousVessel,92)*100
    Zoffset.SliderPosition = VesselAPI.GetParameter(previousVessel,93)*100
  else
    VesselAPI.SetParameter(previousVessel,91,Xoffset.SliderPosition/100)
    VesselAPI.SetParameter(previousVessel,92,Yoffset.SliderPosition/100)
    VesselAPI.SetParameter(previousVessel,93,Zoffset.SliderPosition/100)
  end
  
  -- Update text
  Xcaption.Text = string.format("X offset (forward/aft): %.2f",Xoffset.SliderPosition/100)
  Ycaption.Text = string.format("Y offset (left/right): %.2f", Yoffset.SliderPosition/100)
  Zcaption.Text = string.format("Z offset (up/down): %.2f",    Zoffset.SliderPosition/100)
end



-------------------------------------------------------------------------------
-- Unit conversion
--------------------------------------------------------------------------------
local W,H = 250,18*19
GUI.Dialogs.UnitConversion = GUI.CreateWindow("Unit conversion",nil,nil,W,H)
GUI.Dialogs.UnitConversion.HasCloseBoxes = 1
local conversionUnits = {
  ["Distance"] = {
    { "Meters",         1.0             },
--    { "Kilometers",     1e-3            },
    { "Feet",           3.2808399       },
    { "Statute Miles",  0.000621371192  },
    { "Nautical Miles", 0.000539956803  },
  },
  ["Velocity"] = {
    { "Meters/Second",  1.0             },
    { "Kilometers/Hour",3.6             },
    { "Feet/Second",    3.2808399       },
    { "Miles/Hour",     2.23693629      },
    { "Knots",          1.94384449      },
  },
--  ["Thermal energy"] = {
--    { "Joules",         1.0             },
--    { "BTU",            1/1055.056      },
--  },
  ["Force"] = {
    { "Newtons",        1.0             },
    { "Kilogram-force", 0.101971621     },
    { "Pound-force",    0.224808943     },
  },
}

-- Formatting function
local function format_value(x)
  if (x >= 0.001) and (x <   1e6) then return string.format("%.3f", x) end
  return string.format("%e",x)
end

-- Add all conversions
local y = 0
for k,v in pairs(conversionUnits) do
  GUI.Dialogs.UnitConversion:Add("SubWindow","",10,24+18*y,W-20,18*(#v+1))
  
  for idx,unit in ipairs(v) do
    local ratio = format_value(unit[2])
    GUI.Dialogs.UnitConversion:Add("Caption",  unit[1],24,   32+18*(y-0),W-20,      18)
    unit[3] = GUI.Dialogs.UnitConversion:Add("TextField",ratio,  24+96,33+18*(y-0),W-20-96-24,18)
    y = y + 1
  end
  
  y = y + 2
end

-- Add all functions
for k,v in pairs(conversionUnits) do
  for idx,unit in ipairs(v) do
    unit[3].OnKeyPress = function()
      -- Get current
      local currentValue = tonumber(unit[3].Text) or 0.0
      -- Update all others
      for idx2,unit2 in ipairs(v) do
        if idx2 ~= idx then
          local ratio = unit2[2]/unit[2]
          unit2[3].Text = format_value(currentValue*ratio)
        end
      end
    end
  end
end


--GUI.Dialogs.UnitConversion:Show()
