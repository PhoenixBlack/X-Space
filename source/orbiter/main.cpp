#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

//Orbiter SDK
#include <OrbiterSDK.h>

extern "C" {
	#include "x-space.h"
	#include "highlevel.h"
}

//==============================================================================
// Initialize Orbiter module
//==============================================================================
DLLCLBK void InitModule(HINSTANCE hDLL)
{
	//Clear log
	fclose(fopen("x-orbiter_log.txt","w+"));

	//Start up
	log_write("X-Space: Orbiter plugin starting up\n");
	xspace_initialize_all();

	//Start dedicated server
	if (highlevel_pushcallback("OnDedicatedServer")) highlevel_call(0,0);

	//Enter main loop
	/*while (1) {
		xspace_update(0.0);
		Sleep(1);
	}

	xspace_deinitialize_all();*/
}