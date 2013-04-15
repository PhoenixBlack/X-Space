#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
//#include "server.h"
//#include "../xtime.h"
//#include "frame.h"

//#include <enet/enet.h>
#include "x-space.h"
#include "highlevel.h"
#include "curtime.h"

#include "vessel.h"


//==============================================================================
//Clear Windows console screen
void cls(HANDLE hConsole)
{
	COORD coordScreen = { 0, 0 };
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	GetConsoleScreenBufferInfo(hConsole, &csbi);
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	FillConsoleOutputCharacter(hConsole, (TCHAR) ' ',
	                           dwConSize, coordScreen, &cCharsWritten);
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
	                           dwConSize, coordScreen, &cCharsWritten);
	SetConsoleCursorPosition(hConsole, coordScreen);
	return;
}


//==============================================================================
//Server timekeeping
double current_mjd;

//Starts server with specific time
int highlevel_setmjd(lua_State* L)
{
	current_mjd = lua_tonumber(L,1);
	return 0;
}

//Get MJD time
int highlevel_getmjd(lua_State* L)
{
	lua_pushnumber(L,current_mjd);
	return 1;
}

//Get true MJD time
int highlevel_gettruemjd(lua_State* L)
{
	lua_pushnumber(L,curtime_mjd());
	return 1;
}


//==============================================================================
// Dedicated server main routine
//==============================================================================
void main()
{
	double notify_time = -1e9;

	//Initialize server
	log_write("X-Space: Dedicated server starting up (Win32)\n");
	xspace_initialize_all();
	vessels[0].exists = 0;

	//Register server API
	lua_createtable(L,0,32);
	lua_setglobal(L,"DedicatedServerAPI");
	highlevel_addfunction("DedicatedServerAPI","GetMJD",highlevel_getmjd);
	highlevel_addfunction("DedicatedServerAPI","GetTrueMJD",highlevel_gettruemjd);
	highlevel_addfunction("DedicatedServerAPI","SetMJD",highlevel_setmjd);

	//Initialize timing
	current_mjd = curtime_mjd();

	//Start dedicated server
	if (highlevel_pushcallback("OnDedicatedServer")) highlevel_call(0,0);

	//Enter main loop
	while (1) {
		double dt,dmjd;
		double mjd = curtime_mjd();
		double date_offset = mjd - current_mjd;

		if (date_offset < 1.0*60.0/86400.0) { //30 FPS, normal
			dt = 1.0/30.0;
			dmjd = dt/86400.0;
		} else { //10 FPS
			dt = 1.0/10.0;
			dmjd = dt/86400.0;
		}

		if (mjd > current_mjd + dmjd) {
			current_mjd += dmjd;
			xspace_update((float)dt);

			if (date_offset > 60.0/86400.0) {
				if (curtime() - notify_time > 60.0*60.0) {
					int late_seconds = (int)(date_offset*86400.0);
					notify_time = curtime();
					log_write("X-Space: Server time is %2d days, %02d:%02d late\n",
						late_seconds/86400,
						(late_seconds/3600)%24,
						(late_seconds/60)%60);
				}
			}
		} else {
			Sleep(10);
		}
		//double newtime = curtime();
		//double dt = newtime - prevtime;
		//prevtime = newtime;
		//xspace_update((float)dt);
		//Sleep(20); //50 FPS
	}

	xspace_deinitialize_all();
}