--============================================================================--
-- X-Space highlevel GUI
--------------------------------------------------------------------------------
-- C interface:
--  GUIAPI.AddMenu(name,parentMenu,parentItem)
--  GUIAPI.AddMenuSeparator(parentMenu)
--  GUIAPI.AddItem(name,parentMenu,handlerFunction)
--  GUIAPI.DestroyMenu(menu)
--  GUIAPI.ClearMenu(menu)
--  GUIAPI.CreateWidget(parent,id,text,x,y,w,h,isVisible,isRoot)
--  GUIAPI.SetWidgetProperty(widget,id,value)
--  GUIAPI.SetWidgetText(widget,text)
--  GUIAPI.GetWidgetProperty(widget,id)
--  GUIAPI.GetWidgetText(widget)
--  GUIAPI.ShowWidget(widget)
--  GUIAPI.HideWidget(widget)
--  GUIAPI.DestroyWidget(widget)
--  GUIAPI.WidgetsEqual(widget1,widget2)
--  GUIAPI.GetScreenSize()
--============================================================================--




--------------------------------------------------------------------------------
-- Create menu, unless it already exists
if not GUIAPI.ParentMenuRef then
  GUIAPI.ParentMenuRef = GUIAPI.AddMenu("Spaceflight",GUIAPI.BadMenu,GUIAPI.BadItem)
end

-- Initialize GUI interface
GUI = {}
GUI.Menus = {}
GUI.Items = {}

-- Sub-modules of GUI
GUI.Menu = {}


-- Adds a new menu
function GUI.Menu.Add(name,parentItem)
  -- Fetch parent menu
  local parentMenuRef = GUIAPI.ParentMenuRef
  if parentItem then parentMenuRef = parentItem.MenuRef end

  -- Create item and menu
  local newItem = {
    ItemRef = GUIAPI.AddMenuItem(name,parentMenuRef,function() end),
    ParentMenuRef = parentMenuRef,
  }
  local newMenu = {
    MenuRef = GUIAPI.AddMenu(name,parentMenuRef,newItem.ItemRef),
  }
  newItem.MenuRef = newMenu.MenuRef

  -- Add to table
  table.insert(GUI.Items,newItem)
  table.insert(GUI.Menus,newMenu)

  return newItem
end


-- Adds a new item
function GUI.Menu.AddItem(name,param1,param2)
  local handlerFunction,parentMenuRef
  if param2 then
    handlerFunction = param2
    parentMenuRef = param1.MenuRef or GUIAPI.ParentMenuRef
  else
    handlerFunction = param1
    parentMenuRef = GUIAPI.ParentMenuRef
  end
  
  -- Add the item
  local newItem = {
    ItemRef = GUIAPI.AddMenuItem(name,parentMenuRef,handlerFunction),
    MenuRef = parentMenuRef,
    ParentMenuRef = parentMenuRef,
  }
  table.insert(GUI.Items,newItem)
  
  return newItem
end

-- Adds a new separator
function GUI.Menu.AddSeparator(parentMenu)
  local parentMenuRef = GUIAPI.ParentMenuRef
  if parentMenu then parentMenuRef = parentMenu.MenuRef or GUIAPI.ParentMenuRef end
  GUIAPI.AddMenuSeparator(parentMenuRef)
end




--------------------------------------------------------------------------------
-- Widgets API interface
GUI.WidgetInformation = {
  MainWindow = { ID = 1,
    Style = {
      Normal = 0,
      Transculent = 1,
    },
    Properties = {
      Style = 1100,
      HasCloseBoxes = 1200,
    },
    Messages = {
      OnClose = 1200,
    },
  },
  ---------------------------------------------------------------------------
  SubWindow = { ID = 2,
    Style = {
      Normal = 0,
      Screen = 2,
      ListView = 3,
    },
    Properties = {
      Style = 1200,
    },
    Messages = {
    },
  },
  ---------------------------------------------------------------------------
  Button = { ID = 3,
    Style = {
      PushButton = 0,
      RadioButton = 1,
      WindowCloseBox = 3,
      LittleDownArrow = 5,
      LittleUpArrow = 6,
    },
    Behavior = {
      PushButton = 0,
      CheckBox = 1,
      RadioButton = 2,
    },
    Properties = {
      Style = 1300,
      Behavior = 1301,
      State = 1302,
    },
    Messages = {
      OnPressed = 1300,
      OnChanged = 1301,
    },
  },
  ---------------------------------------------------------------------------
  TextField = { ID = 4,
    Style = {
      EntryField = 0,
      Transparent = 3,
      Transculent = 4,
    },
    Properties = {
      EditFieldSelStart = 1400,
      EditFieldSelEnd  = 1401,
      EditFieldSelDragStart = 1402,
      TextFieldType = 1403,
      PasswordMode = 1404,
      MaxCharacters = 1405,
      ScrollPosition = 1406,
      Font = 1407,
      ActiveEditSide = 1408,
    },
    Messages = {
      OnChanged = 1400,
    },
  },
  ---------------------------------------------------------------------------
  ScrollBar = { ID = 5,
    Style = {
      Normal = 0,
      Slider = 1,
    },
    Properties = {
      SliderPosition = 1500,
      Min = 1501,
      Max = 1502,
      PageAmount = 1503,
      Style = 1504,
      Slop = 1505,
    },
    Messages = {
      OnChanged = 1500,
    },
  },
  ---------------------------------------------------------------------------
  Caption = { ID = 6,
    Properties = {
      Lit = 1600,
    },
    Messages = {
    },
  },
  ---------------------------------------------------------------------------
  GeneralGraphics = { ID = 7,
    Type = {
      Ship = 4,
      ILSGlideScope = 5,
      MarkerLeft = 6,
      Airport = 7,
      NDB = 8,
      VOR = 9,
      RadioTower = 10,
      AircraftCarrier = 11,
      Fire = 12,
      MarkerRight = 13,
      CustomObject = 14,
      CoolingTower = 15,
      SmokeStack = 16,
      Building = 17,
      PowerLine = 18,
      VORWithCompassRose = 19,
      OilPlatform = 21,
      OilPlatformSmall = 22,
      WayPoint = 23,
    },
    Properties = {
      Type = 1700,
    },
    Messages = {
    },
  },
  ---------------------------------------------------------------------------
  Progress = { ID = 8,
    Properties = {
      Position = 1800,
      Min = 1801,
      Max = 1802,
    },
    Messages = {
    },
  },
}

-- Add some common messages to all widgets
for k,v in pairs(GUI.WidgetInformation) do
  v.Messages.OnCreate            = 1
  v.Messages.OnDestroy           = 2
  v.Messages.OnPaint             = 3
  v.Messages.OnDraw              = 4
  v.Messages.OnKeyPress          = 5
  v.Messages.OnKeyTakeFocus      = 6
  v.Messages.OnKeyLoseFocus      = 7
  v.Messages.OnMouseDown         = 8
  v.Messages.OnMouseDrag         = 9
  v.Messages.OnMouseUp           = 10
  v.Messages.OnReshape           = 11
  v.Messages.OnExposedChanged    = 12
  v.Messages.OnAcceptChild       = 13
  v.Messages.OnLoseChild         = 14
  v.Messages.OnAcceptParent      = 15
  v.Messages.OnShown             = 16
  v.Messages.OnHidden            = 17
  v.Messages.OnDescriptorChanged = 18
  v.Messages.OnPropertyChanged   = 19
  v.Messages.OnMouseWheel        = 20
  v.Messages.OnCursorAdjust      = 21
end

-- Widgets created right now
GUI.Dialogs = {}
GUI.Widgets = {}
GUI.WidgetPrototype = { WidgetRef = GUIAPI.BadWidget }

-- Add a widget to this one
function GUI.WidgetPrototype:Add(type,text,x,y,w,h,isRoot)
  if not GUI.WidgetInformation[type] then
    error("Unknown widget: "..tostring(type))
  end
  
  -- Compute delta offset
  local dX,dY = 0,0
  if self.WidgetRef ~= GUIAPI.BadWidget then
    dX = self.X
    dY = self.Y
  end
  
  if (not x) and (not y) then
    local sw,sh = GUIAPI.GetScreenSize()
    x = sw*0.5-w*0.5
    y = sh*0.5-h*0.5
  end

  -- Create widget
  local widget = GUIAPI.CreateWidget(
    self.WidgetRef,
    GUI.WidgetInformation[type].ID,
    text or "",x+dX,y+dY,w,h,true,isRoot)

  -- Create object
  local widgetObject = {
    Parent = self,
    X = x+dX, Y = y+dY,
    W = w, H = h,
    Info = GUI.WidgetInformation[type],
    WidgetRef = widget
  }
  
  -- Add callbacks
  for k,v in pairs(GUI.WidgetInformation[type].Messages) do
    widgetObject[k] = function() end
  end

  -- Set metatable
  setmetatable(widgetObject,GUI.WidgetPrototypeMetatable)
  
  -- Remember this object for cleanup, return it
  table.insert(GUI.Widgets,{ WidgetRef = widget, WidgetObject = widgetObject})
  return widgetObject
end

-- Show widget
function GUI.WidgetPrototype:Show()
  GUIAPI.ShowWidget(self.WidgetRef)
end

-- Hide widget
function GUI.WidgetPrototype:Hide()
  GUIAPI.HideWidget(self.WidgetRef)
end

-- Create new window
function GUI.CreateWindow(title,x,y,w,h)
  local window = GUI.WidgetPrototype:Add("MainWindow",title,x,y,w,h,true)
  window.OnClose = function(self) self:Hide() end
  window:Hide()
  return window
end

-- Metatable for widgets
GUI.WidgetPrototypeMetatable = {
  __index = function(table, key)
    if GUI.WidgetPrototype[key] then return GUI.WidgetPrototype[key] end
    
    local info = rawget(table,"Info")
    local widgetRef = rawget(table,"WidgetRef")
    if (not info) or (not widgetRef) then return end
    
    if info.Properties[key] then
      if info[key] then -- Object.Style = ...
        return 0 -- FIXME
      else
        return GUIAPI.GetWidgetProperty(widgetRef,info.Properties[key])
      end
    elseif key == "Text" then
      return GUIAPI.GetWidgetText(widgetRef)
    else
      return rawget(table,key)
    end
  end,
  __newindex = function(table,key,value)
    local info = rawget(table,"Info")
    local widgetRef = rawget(table,"WidgetRef")
    if (not info) or (not widgetRef) then return end

    if info.Properties[key] then
      if info[key] then -- Object.Style = ...
        if info[key][value] then
          GUIAPI.SetWidgetProperty(widgetRef,info.Properties[key],info[key][value])
        end
      else
        GUIAPI.SetWidgetProperty(widgetRef,info.Properties[key],value)
      end
    elseif key == "Text" then
      GUIAPI.SetWidgetText(widgetRef,value)
    else
      rawset(table,key,value)
    end
  end,
}




--------------------------------------------------------------------------------
-- GUIAPI special call
function InternalCallbacks.OnGUIDeinitialize()
  -- Clear spaceflight menu
  for k,v in pairs(GUI.Menus) do
    GUIAPI.DestroyMenu(v.MenuRef)
    v.MenuRef = nil
    for k2,v2 in pairs(GUI.Items) do
      if v2.MenuRef == v.MenuRef then v2.MenuRef = nil end
      if v2.ParentMenuRef == v.MenuRef then v2.ParentMenuRef = nil end
    end
  end
  GUIAPI.ClearMenu(GUIAPI.ParentMenuRef)
  
  -- Clear widgets
  for k,v in pairs(GUI.Widgets) do
    GUIAPI.DestroyWidget(v.WidgetRef)
    v.WidgetRef = nil
    v.WidgetObject.WidgetRef = nil
  end
end

-- GUIAPI special call for receiving message
function InternalCallbacks.OnWidgetMessage(message,widget,param1,param2)
  for k,v in pairs(GUI.Widgets) do
    if v.WidgetRef and GUIAPI.WidgetsEqual(widget,v.WidgetRef) then
      local info = rawget(v.WidgetObject,"Info")
      if info and info.Messages then
        for k2,v2 in pairs(info.Messages) do
          if v2 == message then
            v.WidgetObject[k2](v.WidgetObject,message,param1,param2)
          end
        end
      end
    end
  end
end




--------------------------------------------------------------------------------
-- Initialize the menu
Include(PluginsFolder.."lua/gui_main.lua")
Include(PluginsFolder.."lua/gui_console.lua")
