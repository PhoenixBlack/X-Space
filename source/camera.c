#include <math.h>
#include <stdio.h>

#include "x-space.h"
#include "vessel.h"
#include "quaternion.h"
#include "coordsys.h"
#include "rendering.h"
#include "curtime.h"

#include <XPLMCamera.h>
#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMUtilities.h>


/*******************************************************************************
 * Various variables
 ******************************************************************************/
#define KEY_UP_LIFT				0
#define KEY_UP_PUSH				1
#define KEY_DOWN_LIFT			2
#define KEY_DOWN_PUSH			3
#define KEY_LEFT_LIFT			4
#define KEY_LEFT_PUSH			5
#define KEY_RIGHT_LIFT			6
#define KEY_RIGHT_PUSH			7
#define KEY_PLUS_LIFT			8
#define KEY_PLUS_PUSH			9
#define KEY_MINUS_LIFT			10
#define KEY_MINUS_PUSH			11
#define KEY_FAST_UP_LIFT		12
#define KEY_FAST_UP_PUSH		13
#define KEY_FAST_DOWN_LIFT		14
#define KEY_FAST_DOWN_PUSH		15
#define KEY_FAST_LEFT_LIFT		16
#define KEY_FAST_LEFT_PUSH		17
#define KEY_FAST_RIGHT_LIFT		18
#define KEY_FAST_RIGHT_PUSH		19
#define KEY_FAST_PLUS_LIFT		20
#define KEY_FAST_PLUS_PUSH		21
#define KEY_FAST_MINUS_LIFT		22
#define KEY_FAST_MINUS_PUSH		23
#define KEY_NEXT_PUSH			24
#define KEY_PREV_PUSH			25
#define KEY_MODE1_PUSH			26
#define KEY_MODE2_PUSH			27
#define KEY_COUNT				28

//Camera structure
struct {
	//Keyboard inputs
	float kbX[2],kbY[2],kbZ[2];

	//Camera position and angles
	double x,y,z,zoom,pitch,yaw,roll;
	quaternion q;

	//Camera mode
	// 00 No camera
	// 01 Locked spot
	// 02 Earth-relative spot
	int mode;

	//Current vessel index
	int vessel;

	//Camera hotkeys
	XPLMHotKeyID hotkeys[KEY_COUNT];
} camera;

//Forward declaration for camera handler
extern int camera_handler(XPLMCameraPosition_t* outCameraPosition, int inIsLosingControl, void* inRefcon);
extern void camera_key_handler(void* inRefcon);

//Hotkeys for modes
XPLMHotKeyID camera_mode1_hotkey;
XPLMHotKeyID camera_mode2_hotkey;


/*******************************************************************************
 * Set camera mode
 ******************************************************************************/
void camera_set_mode(int mode)
{
	if (mode) {
		camera_set_mode(0);

		XPLMCommandButtonPress(xplm_joy_v_fr1);
		XPLMCommandButtonRelease(xplm_joy_v_fr1);
		XPLMControlCamera(xplm_ControlCameraUntilViewChanges,camera_handler,0);

		camera.hotkeys[0]	= XPLMRegisterHotKey(XPLM_VK_UP,xplm_UpFlag,							"",camera_key_handler,(void*)KEY_UP_LIFT);
		camera.hotkeys[1]	= XPLMRegisterHotKey(XPLM_VK_UP,xplm_DownFlag,							"",camera_key_handler,(void*)KEY_UP_PUSH);
		camera.hotkeys[2]	= XPLMRegisterHotKey(XPLM_VK_DOWN,xplm_UpFlag,							"",camera_key_handler,(void*)KEY_DOWN_LIFT);
		camera.hotkeys[3]	= XPLMRegisterHotKey(XPLM_VK_DOWN,xplm_DownFlag,						"",camera_key_handler,(void*)KEY_DOWN_PUSH);
		camera.hotkeys[4]	= XPLMRegisterHotKey(XPLM_VK_LEFT,xplm_UpFlag,							"",camera_key_handler,(void*)KEY_LEFT_LIFT);
		camera.hotkeys[5]	= XPLMRegisterHotKey(XPLM_VK_LEFT,xplm_DownFlag,						"",camera_key_handler,(void*)KEY_LEFT_PUSH);
		camera.hotkeys[6]	= XPLMRegisterHotKey(XPLM_VK_RIGHT,xplm_UpFlag,							"",camera_key_handler,(void*)KEY_RIGHT_LIFT);
		camera.hotkeys[7]	= XPLMRegisterHotKey(XPLM_VK_RIGHT,xplm_DownFlag,						"",camera_key_handler,(void*)KEY_RIGHT_PUSH);
		camera.hotkeys[8]	= XPLMRegisterHotKey(XPLM_VK_RIGHT,xplm_ControlFlag | xplm_DownFlag,	"",camera_key_handler,(void*)KEY_NEXT_PUSH);
		camera.hotkeys[9]	= XPLMRegisterHotKey(XPLM_VK_LEFT,xplm_ControlFlag | xplm_DownFlag,		"",camera_key_handler,(void*)KEY_PREV_PUSH);
		camera.hotkeys[10]	= XPLMRegisterHotKey(XPLM_VK_EQUAL,xplm_UpFlag,							"",camera_key_handler,(void*)KEY_PLUS_LIFT);
		camera.hotkeys[11]	= XPLMRegisterHotKey(XPLM_VK_EQUAL,xplm_DownFlag,						"",camera_key_handler,(void*)KEY_PLUS_PUSH);
		camera.hotkeys[12]	= XPLMRegisterHotKey(XPLM_VK_MINUS,xplm_UpFlag,							"",camera_key_handler,(void*)KEY_MINUS_LIFT);
		camera.hotkeys[13]	= XPLMRegisterHotKey(XPLM_VK_MINUS,xplm_DownFlag,						"",camera_key_handler,(void*)KEY_MINUS_PUSH);
		camera.hotkeys[14]	= XPLMRegisterHotKey(XPLM_VK_UP,xplm_ShiftFlag | xplm_UpFlag,			"",camera_key_handler,(void*)KEY_FAST_UP_LIFT);
		camera.hotkeys[15]	= XPLMRegisterHotKey(XPLM_VK_UP,xplm_ShiftFlag | xplm_DownFlag,			"",camera_key_handler,(void*)KEY_FAST_UP_PUSH);
		camera.hotkeys[16]	= XPLMRegisterHotKey(XPLM_VK_DOWN,xplm_ShiftFlag | xplm_UpFlag,			"",camera_key_handler,(void*)KEY_FAST_DOWN_LIFT);
		camera.hotkeys[17]	= XPLMRegisterHotKey(XPLM_VK_DOWN,xplm_ShiftFlag | xplm_DownFlag,		"",camera_key_handler,(void*)KEY_FAST_DOWN_PUSH);
		camera.hotkeys[18]	= XPLMRegisterHotKey(XPLM_VK_LEFT,xplm_ShiftFlag | xplm_UpFlag,			"",camera_key_handler,(void*)KEY_FAST_LEFT_LIFT);
		camera.hotkeys[19]	= XPLMRegisterHotKey(XPLM_VK_LEFT,xplm_ShiftFlag | xplm_DownFlag,		"",camera_key_handler,(void*)KEY_FAST_LEFT_PUSH);
		camera.hotkeys[20]	= XPLMRegisterHotKey(XPLM_VK_RIGHT,xplm_ShiftFlag | xplm_UpFlag,		"",camera_key_handler,(void*)KEY_FAST_RIGHT_LIFT);
		camera.hotkeys[21]	= XPLMRegisterHotKey(XPLM_VK_RIGHT,xplm_ShiftFlag | xplm_DownFlag,		"",camera_key_handler,(void*)KEY_FAST_RIGHT_PUSH);
		camera.hotkeys[22]	= XPLMRegisterHotKey(XPLM_VK_EQUAL,xplm_ShiftFlag | xplm_UpFlag,		"",camera_key_handler,(void*)KEY_FAST_PLUS_LIFT);
		camera.hotkeys[23]	= XPLMRegisterHotKey(XPLM_VK_EQUAL,xplm_ShiftFlag | xplm_DownFlag,		"",camera_key_handler,(void*)KEY_FAST_PLUS_PUSH);
		camera.hotkeys[24]	= XPLMRegisterHotKey(XPLM_VK_MINUS,xplm_ShiftFlag | xplm_UpFlag,		"",camera_key_handler,(void*)KEY_FAST_MINUS_LIFT);
		camera.hotkeys[25]	= XPLMRegisterHotKey(XPLM_VK_MINUS,xplm_ShiftFlag | xplm_DownFlag,		"",camera_key_handler,(void*)KEY_FAST_MINUS_PUSH);
	} else {																							 
		int i;																							 
		for (i = 0; i < KEY_COUNT; i++) {
			if (camera.hotkeys[i]) XPLMUnregisterHotKey(camera.hotkeys[i]);
			camera.hotkeys[i] = 0;
		}
	}
	camera.mode = mode;
}


/*******************************************************************************
 * Set current vessel
 ******************************************************************************/
void camera_set_vessel(int idx)
{
	if (idx < 0) idx = vessel_count-1;
	if (idx >= vessel_count) idx = 0;
	if (idx > camera.vessel) {
		while ((!vessels[idx].exists) && (idx > 0)) {
			idx++;
			if (idx >= vessel_count) idx = 0;
		}
	} else if (idx < camera.vessel) {
		while ((!vessels[idx].exists) && (idx > 0)) {
			idx--;
		}
	}
	camera.vessel = idx;
}


/*******************************************************************************
 * Key handler
 ******************************************************************************/
void camera_key_handler(void* inRefcon)
{
	switch ((int)inRefcon) {
		case KEY_UP_PUSH:			camera.kbY[0] =  1.0f; break;
		case KEY_DOWN_PUSH:			camera.kbY[1] = -1.0f; break;
		case KEY_LEFT_PUSH:			camera.kbX[0] = -1.0f; break;
		case KEY_RIGHT_PUSH:		camera.kbX[1] =  1.0f; break;
		case KEY_PLUS_PUSH:			camera.kbZ[0] =  1.0f; break;
		case KEY_MINUS_PUSH:		camera.kbZ[1] = -1.0f; break;
		case KEY_UP_LIFT:			camera.kbY[0] =  0.0f; break;
		case KEY_DOWN_LIFT:			camera.kbY[1] =  0.0f; break;
		case KEY_LEFT_LIFT:			camera.kbX[0] =  0.0f; break;
		case KEY_RIGHT_LIFT:		camera.kbX[1] =  0.0f; break;
		case KEY_PLUS_LIFT:			camera.kbZ[0] =  0.0f; break;
		case KEY_MINUS_LIFT:		camera.kbZ[1] =  0.0f; break;

		case KEY_FAST_UP_PUSH:		camera.kbY[0] =  8.0f; break;
		case KEY_FAST_DOWN_PUSH:	camera.kbY[1] = -8.0f; break;
		case KEY_FAST_LEFT_PUSH:	camera.kbX[0] = -8.0f; break;
		case KEY_FAST_RIGHT_PUSH:	camera.kbX[1] =  8.0f; break;
		case KEY_FAST_PLUS_PUSH:	camera.kbZ[0] =  8.0f; break;
		case KEY_FAST_MINUS_PUSH:	camera.kbZ[1] = -8.0f; break;
		case KEY_FAST_UP_LIFT:		camera.kbY[0] =  0.0f; break;
		case KEY_FAST_DOWN_LIFT:	camera.kbY[1] =  0.0f; break;
		case KEY_FAST_LEFT_LIFT:	camera.kbX[0] =  0.0f; break;
		case KEY_FAST_RIGHT_LIFT:	camera.kbX[1] =  0.0f; break;
		case KEY_FAST_PLUS_LIFT:	camera.kbZ[0] =  0.0f; break;
		case KEY_FAST_MINUS_LIFT:	camera.kbZ[1] =  0.0f; break;

		case KEY_MODE1_PUSH:		camera_set_mode(1); break;
		case KEY_MODE2_PUSH:		camera_set_mode(2); break;

		case KEY_NEXT_PUSH:			camera_set_vessel(camera.vessel+1); break;
		case KEY_PREV_PUSH:			camera_set_vessel(camera.vessel-1); break;
		default: break;
	}
}


/*******************************************************************************
 * Initialize camera
 ******************************************************************************/
void camera_initialize()
{
	int i;
	for (i = 0; i < KEY_COUNT; i++) camera.hotkeys[i] = 0;

	camera_mode1_hotkey = XPLMRegisterHotKey(XPLM_VK_RBRACE,xplm_ShiftFlag | xplm_DownFlag,"",camera_key_handler,(void*)KEY_MODE1_PUSH);
	camera_mode2_hotkey = XPLMRegisterHotKey(XPLM_VK_LBRACE,xplm_ShiftFlag | xplm_DownFlag,"",camera_key_handler,(void*)KEY_MODE2_PUSH);
}

void camera_deinitialize()
{
	camera_set_mode(0);
	XPLMUnregisterHotKey(camera_mode1_hotkey);
	XPLMUnregisterHotKey(camera_mode2_hotkey);
}


/*******************************************************************************
 * Simulate camera
 ******************************************************************************/
void camera_simulate()
{
	static double camera_previous_time = 0;
	double camera_new_time = curtime();
	float dt = (float)(camera_new_time - camera_previous_time);
	camera_previous_time = camera_new_time;

	switch (camera.mode) {
		case 1: //Locked spot
		case 2: //Free spot
			//Angles
			camera.roll = 0;
			camera.pitch	-= 45.0f * (camera.kbY[0] + camera.kbY[1]) * dt;
			camera.yaw		-= 45.0f * (camera.kbX[0] + camera.kbX[1]) * dt;
			camera.zoom		-= 80.0f * (camera.kbZ[0] + camera.kbZ[1]) * dt;
			if (camera.zoom < 5.0f) camera.zoom = 5.0f;

			//Camera quaternion
			qeuler_from(camera.q,RAD(camera.roll),RAD(camera.pitch),RAD(camera.yaw));

			//Location
			if (camera.mode == 1) {
				camera.x =  camera.zoom*cos(RAD(camera.yaw))*cos(RAD(-camera.pitch));
				camera.y =  camera.zoom*sin(RAD(camera.yaw))*cos(RAD(-camera.pitch));
				camera.z =  camera.zoom*sin(RAD(-camera.pitch));
			} else {
				camera.z =  camera.zoom*cos(RAD(-camera.yaw))*sin(-RAD(camera.pitch-90));
				camera.y = -camera.zoom*cos(RAD(camera.pitch-90));
				camera.x =  camera.zoom*sin(RAD(-camera.yaw))*sin(-RAD(camera.pitch-90));
			}
			break;
	}
}


/*******************************************************************************
 * Camera
 ******************************************************************************/
int camera_handler(XPLMCameraPosition_t* outCameraPosition, int inIsLosingControl, void* inRefcon)
{
	if (outCameraPosition) {
		extern XPLMDataRef dataref_vessel_x;
		extern XPLMDataRef dataref_vessel_y;
		extern XPLMDataRef dataref_vessel_z;
		double gx,gy,gz,rx,ry,rz,dx,dy,dz;
		quaternion q;

		//Fetch up-to-date vessel information
		//vessels_read(&vessels[camera.vessel],0);

		if (vessels[camera.vessel].physics_type == VESSEL_PHYSICS_SIM) {
			//Workaround for issues with setting aircraft position
			dx = XPLMGetDatad(dataref_vessel_x)-vessels[0].sim.x;
			dy = XPLMGetDatad(dataref_vessel_y)-vessels[0].sim.y;
			dz = XPLMGetDatad(dataref_vessel_z)-vessels[0].sim.z;
		} else {
			dx = 0;
			dy = 0;
			dz = 0;
		}

		//Apply camera rotation
		if (camera.mode == 1) {
			qmul(q,vessels[camera.vessel].sim.q,camera.q);

			//Turn local coordinate system coords into global
			coord_l2sim(&vessels[camera.vessel],camera.x,camera.y,camera.z,&gx,&gy,&gz);
		} else {
			quaternion ground;
			qeuler_from(ground,0,0,0);
			qmul(q,ground,camera.q);

			//Use camera coords
			gx = vessels[camera.vessel].sim.x+camera.x;
			gy = vessels[camera.vessel].sim.y+camera.y;
			gz = vessels[camera.vessel].sim.z+camera.z;
		}
		qeuler_to(q,&rx,&ry,&rz);

		//Set position
		outCameraPosition->x = (float)(gx+dx);
		outCameraPosition->y = (float)(gy+dy);
		outCameraPosition->z = (float)(gz+dz);
		outCameraPosition->roll = DEGf(rx);
		outCameraPosition->pitch = DEGf(ry);
		outCameraPosition->heading = DEGf(rz);
	}
	if (inIsLosingControl) camera_set_mode(0);
	return camera.mode > 0;
}


/*******************************************************************************
 * Camera
 ******************************************************************************/
void camera_draw2d() {
	if (camera.mode) {
		char buf[ARBITRARY_MAX] = { 0 };
		float RGB[4] = { 0.60f,0.90f,0.75f,1.0f };

		snprintf(buf,ARBITRARY_MAX-1,"Altitude: %.3f km   Inertial velocity: %.3f km/sec   Non-inertial velocity: %.3f km/sec   Mass: %.0f kg",
			vessels[camera.vessel].elevation*1e-3,
			sqrt(vessels[camera.vessel].inertial.vx*vessels[camera.vessel].inertial.vx+
				 vessels[camera.vessel].inertial.vy*vessels[camera.vessel].inertial.vy+
				 vessels[camera.vessel].inertial.vz*vessels[camera.vessel].inertial.vz)*1e-3,
			sqrt(vessels[camera.vessel].noninertial.nvx*vessels[camera.vessel].noninertial.nvx+
				 vessels[camera.vessel].noninertial.nvy*vessels[camera.vessel].noninertial.nvy+
				 vessels[camera.vessel].noninertial.nvz*vessels[camera.vessel].noninertial.nvz)*1e-3,
			vessels[camera.vessel].mass);
		XPLMDrawString(RGB,4,4,buf,0,xplmFont_Basic);
	}
}