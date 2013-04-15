--=============================================================================-
-- X-Space engines simulation support
--=============================================================================-

-- Engine[1] = {
--   Position              { x, y, z } location of engine
--   VerticalAngle (deg)   Dataref or number for vertical deflection angle
--   HorizontalAngle (deg) Dataref or number for horizontal deflection angle
--   Simulator             Function for physics (default: EngineSimulator)
--   Renderer              Function for rendering (default: EngineRenderer)
--   Dataref               Dataref for command
--   FuelTankDataref       Dataref for fuel tank
--   FuelFlowDataref       Dataref for fuel flow
--   FuelTank              Fuel tank index
--
--   Size (m)              Engine nozzle size (at its largest point)
--   Force (N)             Force at 100% thrust
--   SFC (kg*N-1*s-1)      Specific fuel consumption
--   ISP (s)               Specific impulse
--
--   Type                  Engine type
--   Model                 Engine model
-- }
--
-- Possible engine types:
--  "LO2-LH2" (default)
--  "RCS"




--------------------------------------------------------------------------------
-- Perfomance tables for existing engines
--------------------------------------------------------------------------------
EngineData = {}
local engineDataFile = io.open(PluginsFolder.."engine.dat","r")
local engineDataEntries = 0
if engineDataFile then
  while engineDataFile:read(0) do
    local lineChar = engineDataFile:read(1)
    if (lineChar ~= "\n") and
       (lineChar ~= "\r") and
       (lineChar ~= "") then
      if (lineChar ~= "#") then
        local engineName = ""
        while lineChar ~= " " do
          engineName = engineName .. lineChar
          lineChar = engineDataFile:read(1)
        end
        
        EngineData[engineName] = {
          Force_vac = engineDataFile:read("*n"),
          Force_sl  = engineDataFile:read("*n"),
          ISP_vac   = engineDataFile:read("*n"),
          ISP_sl    = engineDataFile:read("*n"),
          Type      = engineDataFile:read(),
        }
  
        local a,b = string.find(EngineData[engineName].Type, "%S+")
        EngineData[engineName].Type = string.sub(EngineData[engineName].Type,a,b)
        engineDataEntries = engineDataEntries + 1
      else
        engineDataFile:read()
      end
    end
  end
  engineDataFile:close()
end
print("X-Space: Found "..engineDataEntries.." engine data entries")



--------------------------------------------------------------------------------
-- Default engine simulator
--------------------------------------------------------------------------------
local max = math.max
local min = math.min
local clamp = function(x,y,z) return math.min(math.max(x,y),z) end
function EngineSimulator(engine,dt)
  -- Pressure in percent
  local P = max(0,min(1,VesselAPI.GetParameter(CurrentVessel,103)/101325))

  -- Read command
  local datarefCommand = 0
  if engine.Dataref then datarefCommand = Dataref(engine.Dataref) end
  local thrusterCommand = datarefCommand
  
  -- Process throttle limit
  if engine.MaxThrottle then thrusterCommand = min(thrusterCommand,engine.MaxThrottle) end
  
  -- Process finite throttle speed
  if engine.ThrottleSpeed then
    engine.RealThrottle = engine.RealThrottle or datarefCommand

    local requiredDelta = datarefCommand - engine.RealThrottle
    local maxFrameDelta = engine.ThrottleSpeed*dt
    engine.RealThrottle = engine.RealThrottle + clamp(requiredDelta,-maxFrameDelta,maxFrameDelta)
    
    thrusterCommand = engine.RealThrottle
  end
  
  -- Process engine startup and shutdown
  if engine.StartupTime then
    local startupThrottle = (engine.MinThrottle or 0.10)*0.90 -- 90% of min throttle
    engine.OperationTiming = engine.OperationTiming or 0.0
    
    -- Process startup/shutdown timing
    if datarefCommand > startupThrottle then
      if engine.OperationTiming < 1.0 then
        engine.OperationTiming = engine.OperationTiming + (dt/engine.StartupTime)
        if engine.ThrottleSpeed then engine.RealThrottle = thrusterCommand end
      end
    else
      if engine.ShutdownTime and (engine.OperationTiming > 0.0) then
        engine.OperationTiming = engine.OperationTiming - (dt/engine.ShutdownTime)
      else
        engine.OperationTiming = 0
      end
    end
    
    -- Just a sanity thing
    engine.OperationTiming = clamp(engine.OperationTiming,0.0,1.0)
    
    -- Throttle logic
    if engine.OperationTiming < 1.0 then
      thrusterCommand = startupThrottle*(1.0-math.exp(-0.2*engine.OperationTiming))
      if engine.ThrottleSpeed then engine.RealThrottle = thrusterCommand end
    end
  end

  -- Fetch command and force
  local thrusterForce = 0
  if engine.Force then -- Constant force model
    thrusterForce = engine.Force
  elseif engine.Force_vac then -- Force by two keypoints
    local Force_vac = engine.Force_vac
    local Force_sl  = engine.Force_sl or (engine.Force_vac*0.75)
    thrusterForce = Force_sl*P + Force_vac*(1-P)
  end

  -- Get amount of fuel left
  local fuelLeft
  if engine.FuelTankDataref then
    fuelLeft = Dataref(engine.FuelTankDataref)
  elseif engine.FuelTank then
    fuelLeft = VesselAPI.GetFuelTank(CurrentVessel,engine.FuelTank)
  end
  
  -- Calculate fuel consumption
  if fuelLeft then
    local fuelDelta = 0
    if engine.FF then -- Exact fuel flow (for solid engines)
      local FF = Dataref(engine.FF)
      fuelDelta = math.min(FF*thrusterCommand*dt,fuelLeft)
      if engine.FuelFlowDataref then Dataref(engine.FuelFlowDataref,FF*thrusterCommand) end
    else
      -- Compute specific fuel consumption
      local SFC = 0.000600 -- kg/(N*s)
      
      if engine.ISP then -- Use ISP for SFC
        local ISP = Dataref(engine.ISP)
        SFC = 0.101972/ISP
      elseif engine.ISP_vac then -- Use ISP model with two keypoints
        local ISP_vac = engine.ISP_vac
        local ISP_sl  = engine.ISP_sl or (engine.ISP_vac*0.75)
        local ISP = ISP_sl*P + ISP_vac*(1-P)
        
        SFC = 0.101972/ISP
      elseif engine.SFC then -- Fixed SFC model
        SFC = Dataref(engine.SFC)
      end
      
      fuelDelta = math.min(SFC*thrusterCommand*thrusterForce*dt,fuelLeft)
      if engine.FuelFlowDataref then Dataref(engine.FuelFlowDataref,SFC*thrusterCommand*thrusterForce) end
    end

    -- Subtract fuel
    fuelLeft = fuelLeft - fuelDelta
    
    -- Shut down engine if no fuel left
    if fuelLeft == 0 then thrusterCommand = 0 end
    
    -- Write data back
    if engine.FuelTankDataref then
      Dataref(engine.FuelTankDataref,fuelLeft)
    elseif engine.FuelTank then
      VesselAPI.SetFuelTank(CurrentVessel,engine.FuelTank,fuelLeft)
    end
  end

  -- Calculate intensity (RCS firing model)
  if engine.Type == "RCS" then
    local thrusterIntensity = 0
    if thrusterCommand == 0 then
      engine.VisualTiming = 0
    else
      engine.VisualTiming = (engine.VisualTiming or 0) + dt
      thrusterIntensity = 1-2.71^(-64*(engine.VisualTiming or 0))
      if thrusterIntensity < 0.5 then thrusterIntensity = 0.5 end
      if thrusterIntensity > 1.0 then thrusterIntensity = 1.0 end
    end
    engine.Intensity = thrusterIntensity*(1+math.random()*0.04)
  else -- Normal engine model
    engine.Intensity = thrusterCommand*(0.90+math.random()*0.20)
  end

  -- Output force and command
  engine.Command = thrusterCommand
  engine.ActualForce = thrusterForce
  return thrusterCommand*thrusterForce
end




--------------------------------------------------------------------------------
-- Default RCS renderer
--------------------------------------------------------------------------------
local engineRenderParams = {}
function EngineRenderer(engine)
  -- Random noise
  local noise = math.random()-0.5
  
  -- Nozzle size (diameter)
  local nozzleSize = engine.Size or 0.2
  engineRenderParams.NozzleWidth = nozzleSize
  
  -- Exhaust intensity
  local intensity = engine.Intensity or 0.0
  local force = engine.ActualForce
  --(curtime()*0.2) % 1.0 ---math.abs(math.sin(curtime()))

  if intensity <= 0.0 then
    engineRenderParams.ExhaustTrail = nil
    engineRenderParams.ExhaustSmoke = nil
    engineRenderParams.ExhaustCones = nil
    return engineRenderParams
  end
  
  if (engine.Type == "LO2-K") or
     (engine.Type == "LO2-RP1") then
  ------------------------------------------------------------------------------
    local intensity2 = min(0.50,intensity)*(1/0.50)
    local intensity3 = min(0.10,intensity)*(1/0.10)
    local intensity4 = max(0.00,1.0-10.0*intensity2)
    engineRenderParams.ExhaustTrail = {
      Length = nozzleSize*64*(0.8+intensity*0.2),
      Width = nozzleSize,
      A1 = (0.5-(1-intensity2)*math.random()*0.4)*intensity3,
      A2 = (0.1+0.2*intensity)*intensity3,
      
      Texels = 0.013+(intensity2 + 4.0*intensity4)*0.007,
      Velocity = 0.50+0.50*intensity2,
      Intensity = 1.00*(intensity^2),
    }
    engineRenderParams.ExhaustSmoke = nil
    engineRenderParams.ExhaustCones = nil
  elseif engine.Type == "RCS" then
  ------------------------------------------------------------------------------
    engineRenderParams.ExhaustTrail = nil
    engineRenderParams.ExhaustSmoke = nil
    engineRenderParams.ExhaustCones = {
      { NozzleWidth  = nozzleSize,
        NozzleLength = 0.0,
        PlumeWidth   = (nozzleSize*12.0+nozzleSize*noise)*(0.2 + 0.8*intensity),
        PlumeLength  = (nozzleSize*32.0+nozzleSize*noise)*(0.2 + 0.8*intensity),

        R1 = 1.00, G1 = 1.00, B1 = 1.00, A1 = 0.1,
        R2 = 1.00, G2 = 1.00, B2 = 1.00, A2 = 0.0,
      },
      { NozzleWidth  = nozzleSize,
        NozzleLength = 0.0,
        PlumeWidth   = (nozzleSize*18.0+nozzleSize*noise)*(0.2 + 0.8*intensity),
        PlumeLength  = (nozzleSize*20.0+nozzleSize*noise)*(0.2 + 0.8*intensity),

        R1 = 1.00, G1 = 1.00, B1 = 1.00, A1 = 0.05,
        R2 = 1.00, G2 = 1.00, B2 = 1.00, A2 = 0.0,
      },
    }
  elseif engine.Type == "Solid" then
  ------------------------------------------------------------------------------
    local force_clamp = math.max(1.0,force)
    local force_intensity = math.log10(force_clamp)/12
    
    engineRenderParams.ExhaustTrail = {
      Length = nozzleSize*32,
      Width = nozzleSize*0.5,
      A1 = 0.6+0.4*math.random(),
      A2 = 2.0*force_intensity,

      Texels = 0.03,
      Velocity = 0.50,
      Intensity = 1.00,
    }
    engineRenderParams.ExhaustSmoke = {
      Duration = 120*force_intensity,
      MinDistance = 48.0,
      Type = 0,
    }
    engineRenderParams.ExhaustCones = nil
  elseif engine.Type == "SolidJet" then
  ------------------------------------------------------------------------------
    engineRenderParams.ExhaustTrail = nil
    engineRenderParams.ExhaustSmoke = nil
    engineRenderParams.ExhaustCones = {
      { NozzleWidth  = nozzleSize,
        NozzleLength = 0.0,
        PlumeWidth   = (nozzleSize*3.0+nozzleSize*noise*3.0),
        PlumeLength  = (nozzleSize*25.0+nozzleSize*noise*4.0),

        R1 = 1.00*(1+math.random()*0.2),
        G1 = 1.00*(1+math.random()*0.2),
        B1 = 0.60*(1+math.random()*0.2),
        A1 = 0.5*intensity,
        R2 = 1.00*(1+math.random()*0.2),
        G2 = 1.00*(1+math.random()*0.2),
        B2 = 0.60*(1+math.random()*0.2),
        A2 = 0.0,
      },
    }
  else
  ------------------------------------------------------------------------------
    engineRenderParams.ExhaustTrail = nil
    engineRenderParams.ExhaustSmoke = nil
    engineRenderParams.ExhaustCones = {
      { NozzleWidth  = nozzleSize,
        NozzleLength = 0.0,
        PlumeWidth   = (nozzleSize*2.0+nozzleSize*noise)*(0.2 + 0.8*intensity),
        PlumeLength  = (nozzleSize*15.0+nozzleSize*noise)*(0.2 + 0.8*intensity),
        
        R1 = 0.60, G1 = 0.30, B1 = 1.00, A1 = 0.02,
        R2 = 0.60, G2 = 0.30, B2 = 1.00, A2 = 0.0,
      },
      { NozzleWidth  = nozzleSize,
        NozzleLength = 0.0,
        PlumeWidth   = (nozzleSize*2.5+nozzleSize*noise)*(0.2 + 0.8*intensity),
        PlumeLength  = (nozzleSize*7.0+nozzleSize*noise)*(0.2 + 0.8*intensity),

        R1 = 0.60, G1 = 0.50, B1 = 1.00, A1 = 0.03,
        R2 = 0.60, G2 = 0.50, B2 = 1.00, A2 = 0.0,
      },
      { NozzleWidth  = nozzleSize,
        NozzleLength = 0.0,
        PlumeWidth   = (nozzleSize*3.0+nozzleSize*noise)*(0.2 + 0.8*intensity),
        PlumeLength  = (nozzleSize*5.0+nozzleSize*noise)*(0.2 + 0.8*intensity),

        R1 = 0.80, G1 = 0.50, B1 = 1.00, A1 = 0.01,
        R2 = 0.80, G2 = 0.50, B2 = 1.00, A2 = 0.0,
      },
      
      { NozzleWidth  = nozzleSize*0.5+0.05*noise,
        NozzleLength = (nozzleSize*1.3+0.2*noise)*intensity,
        PlumeWidth   = 0.0,
        PlumeLength  = (nozzleSize*3.3+0.2*noise)*intensity,

        R1 = 1.00, G1 = 0.80, B1 = 1.00, A1 = 0.7*intensity^2,
        R1 = 1.00, G1 = 0.80, B1 = 1.00, A1 = 0.7*intensity^2,
      },
    }
  end
  
  return engineRenderParams
end
