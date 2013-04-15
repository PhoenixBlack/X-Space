//==============================================================================
// Highlevel GUI
//==============================================================================
#include "x-space.h"
#include "highlevel.h"

//Include X-Plane SDK
#include <XPLMDisplay.h>
#include <XPLMMenus.h>
#include <XPWidgets.h>
#include <XPStandardWidgets.h>


//==============================================================================
// Spaceflight menu handler
//==============================================================================
static void gui_menu_handler(void* ref, void* param)
{
	int function_ref = (int)param;
	lua_rawgeti(L,LUA_REGISTRYINDEX,function_ref);
	highlevel_call(0,0);
}

static int gui_widget_handler(XPWidgetMessage message, XPWidgetID widget, long param1, long param2)
{
	if (highlevel_pushcallback("OnWidgetMessage")) {
		lua_pushnumber(L,message);
		highlevel_newptr("Widget",widget);
		lua_pushnumber(L,param1);
		lua_pushnumber(L,param2);
		//lua_pushvalue(L,-5);
		highlevel_call(4,0);
	}
	return 0;
}


//==============================================================================
// Interface to X-Plane GUI API
//==============================================================================
int gui_highlevel_addmenu(lua_State* L)
{
	XPLMMenuID menu_id;
	luaL_checktype(L,1,LUA_TSTRING); //Name
	luaL_checkudata(L,2,"Menu");
	luaL_checkudata(L,3,"Item");

	//Create menu
	menu_id = XPLMCreateMenu(
	              lua_tostring(L,1),
	              highlevel_getptr(L,2),
	              (int)highlevel_getptr(L,3),
	              gui_menu_handler,0);

	//Return
	highlevel_newptr("Menu",(void*)menu_id);
	return 1;
}

int gui_highlevel_addseparator(lua_State* L)
{
	luaL_checkudata(L,1,"Menu"); //Menu
	XPLMAppendMenuSeparator(highlevel_getptr(L,1));
	return 0;
}

int gui_highlevel_additem(lua_State* L)
{
	int menu_item,function_ref;
	luaL_checktype(L,1,LUA_TSTRING); //Name
	luaL_checkudata(L,2,"Menu"); //Parent
	luaL_checktype(L,3,LUA_TFUNCTION); //Handler

	//Remember function
	lua_pushvalue(L,3);
	function_ref = luaL_ref(L,LUA_REGISTRYINDEX);

	//Append item
	menu_item = XPLMAppendMenuItem(
	                highlevel_getptr(L,2),
	                lua_tostring(L,1),
	                (void*)function_ref,1);

	//Return
	highlevel_newptr("Item",(void*)menu_item);
	return 1;
}

int gui_highlevel_destroymenu(lua_State* L)
{
	luaL_checkudata(L,1,"Menu");
	XPLMDestroyMenu(highlevel_getptr(L,1));
	return 0;
}

int gui_highlevel_clearmenu(lua_State* L)
{
	luaL_checkudata(L,1,"Menu");
	XPLMClearAllMenuItems(highlevel_getptr(L,1));
	return 0;
}

int gui_highlevel_createwidget(lua_State* L)
{
	XPWidgetID widget;
	int is_visible,is_root,w,h;
	luaL_checkudata(L,1,"Widget"); //parent
	luaL_checktype(L,2,LUA_TNUMBER); //type
	luaL_checktype(L,3,LUA_TSTRING); //name
	luaL_checktype(L,4,LUA_TNUMBER); //x
	luaL_checktype(L,5,LUA_TNUMBER); //y
	luaL_checktype(L,6,LUA_TNUMBER); //w
	luaL_checktype(L,7,LUA_TNUMBER); //h

	is_visible = 0;
	is_root = 0;
	if (lua_isboolean(L,9)) {
		if (lua_isboolean(L,8)) is_visible = lua_toboolean(L,8);
		if (lua_isboolean(L,9)) is_root = lua_toboolean(L,9);
	} else {
		if (lua_isboolean(L,8)) is_visible = lua_toboolean(L,8);
	}

	//Convert coordinates from X-Planes messed up system to something sane
	XPLMGetScreenSize(&w,&h);

	widget = XPCreateWidget(
	             lua_tointeger(L,4),
	             h-lua_tointeger(L,5),
	             lua_tointeger(L,4)+lua_tointeger(L,6),
	             h-lua_tointeger(L,5)-lua_tointeger(L,7),
	             is_visible,
	             lua_tostring(L,3),
	             is_root,
	             highlevel_getptr(L,1),
	             lua_tointeger(L,2));
	XPAddWidgetCallback(widget, gui_widget_handler);

	highlevel_newptr("Widget",widget);
	return 1;
}

int gui_highlevel_setwidgetproperty(lua_State* L)
{
	luaL_checkudata(L,1,"Widget"); //widget
	luaL_checktype(L,2,LUA_TNUMBER); //index
	luaL_checktype(L,3,LUA_TNUMBER); //value
	XPSetWidgetProperty(
	    highlevel_getptr(L,1),
	    lua_tointeger(L,2),
	    lua_tointeger(L,3)
	);
	return 0;
}

int gui_highlevel_getwidgetproperty(lua_State* L)
{
	int exists = 0;
	luaL_checkudata(L,1,"Widget"); //widget
	luaL_checktype(L,2,LUA_TNUMBER); //index
	lua_pushnumber(L,XPGetWidgetProperty(
	                   highlevel_getptr(L,1),
	                   lua_tointeger(L,2),
	                   &exists
	               ));
	lua_pushboolean(L,exists);
	return 2;
}

int gui_highlevel_setwidgettext(lua_State* L)
{
	luaL_checkudata(L,1,"Widget"); //widget
	luaL_checktype(L,2,LUA_TSTRING); //value
	XPSetWidgetDescriptor(
	    highlevel_getptr(L,1),
	    lua_tostring(L,2)
	);
	return 0;
}

int gui_highlevel_getwidgettext(lua_State* L)
{
	char buf[ARBITRARY_MAX-1] = { 0 };
	luaL_checkudata(L,1,"Widget"); //widget
	XPGetWidgetDescriptor(
	    highlevel_getptr(L,1),
	    buf,ARBITRARY_MAX-1);
	lua_pushstring(L,buf);
	return 1;
}

int gui_highlevel_showwidget(lua_State* L)
{
	luaL_checkudata(L,1,"Widget"); //widget
	XPShowWidget(highlevel_getptr(L,1));
	return 0;
}

int gui_highlevel_hidewidget(lua_State* L)
{
	luaL_checkudata(L,1,"Widget"); //widget
	XPHideWidget(highlevel_getptr(L,1));
	return 0;
}

int gui_highlevel_destroywidget(lua_State* L)
{
	luaL_checkudata(L,1,"Widget"); //widget
	XPDestroyWidget(highlevel_getptr(L,1),0);
	return 0;
}

int gui_highlevel_getwidgetltrb(lua_State* L)
{
	int l,t,r,b;
	luaL_checkudata(L,1,"Widget"); //widget
	XPGetWidgetGeometry(highlevel_getptr(L,1),&l,&t,&r,&b);
	lua_pushnumber(L,l);
	lua_pushnumber(L,t);
	lua_pushnumber(L,r);
	lua_pushnumber(L,b);
	return 4;
}

int gui_highlevel_widgetsequal(lua_State* L)
{
	XPWidgetID w1;
	XPWidgetID w2;
	luaL_checkudata(L,1,"Widget");
	luaL_checkudata(L,2,"Widget");

	w1 = highlevel_getptr(L,1);
	w2 = highlevel_getptr(L,2);
	lua_pushboolean(L,w1 == w2);
	return 1;
}

int gui_highlevel_getscreensize(lua_State* L)
{
	int w,h;
	XPLMGetScreenSize(&w,&h);
	lua_pushnumber(L,w);
	lua_pushnumber(L,h);
	return 2;
}


//==============================================================================
// Loads all GUI
//==============================================================================
void gui_initialize()
{
	//Register API
	lua_createtable(L,0,32);
	highlevel_newptr("Menu",0);
	lua_setfield(L,-2,"BadMenu");
	highlevel_newptr("Item",0);
	lua_setfield(L,-2,"BadItem");
	highlevel_newptr("Widget",0);
	lua_setfield(L,-2,"BadWidget");
	lua_setglobal(L,"GUIAPI");

	highlevel_addfunction("GUIAPI","AddMenu",gui_highlevel_addmenu);
	highlevel_addfunction("GUIAPI","AddMenuSeparator",gui_highlevel_addseparator);
	highlevel_addfunction("GUIAPI","AddMenuItem",gui_highlevel_additem);
	highlevel_addfunction("GUIAPI","DestroyMenu",gui_highlevel_destroymenu);
	highlevel_addfunction("GUIAPI","ClearMenu",gui_highlevel_clearmenu);
	highlevel_addfunction("GUIAPI","CreateWidget",gui_highlevel_createwidget);
	highlevel_addfunction("GUIAPI","SetWidgetProperty",gui_highlevel_setwidgetproperty);
	highlevel_addfunction("GUIAPI","SetWidgetText",gui_highlevel_setwidgettext);
	highlevel_addfunction("GUIAPI","GetWidgetProperty",gui_highlevel_getwidgetproperty);
	highlevel_addfunction("GUIAPI","GetWidgetText",gui_highlevel_getwidgettext);
	highlevel_addfunction("GUIAPI","ShowWidget",gui_highlevel_showwidget);
	highlevel_addfunction("GUIAPI","HideWidget",gui_highlevel_hidewidget);
	highlevel_addfunction("GUIAPI","DestroyWidget",gui_highlevel_destroywidget);
	highlevel_addfunction("GUIAPI","GetWidgetLTRB",gui_highlevel_getwidgetltrb);
	highlevel_addfunction("GUIAPI","WidgetsEqual",gui_highlevel_widgetsequal);
	highlevel_addfunction("GUIAPI","GetScreenSize",gui_highlevel_getscreensize);

	//Initialize X-Net GUI
	highlevel_load(FROM_PLUGINS("lua/gui.lua"));
}


//==============================================================================
// Unloads all GUI
//==============================================================================
void gui_deinitialize()
{
	if (highlevel_pushcallback("OnGUIDeinitialize")) {
		highlevel_call(0,0);
	}
}