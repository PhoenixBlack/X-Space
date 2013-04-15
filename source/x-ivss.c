//==============================================================================
// IVSS implementation in X-Space
//==============================================================================
#include <ivss.h>
#include <ivss_interface_x-plane.h>
#include <ivss_sim_xgdc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <glfont.h>

#include "x-space.h"
#include "vessel.h"
#include "x-ivss.h"
#include "threading.h"
#include "highlevel.h"
#include "radiosys.h"

#if (defined(DEDICATED_SERVER))
#include "x-ivss_vsfl.h"
#endif

#ifdef FFMPEG_SUPPORT
#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"

unsigned char av_picture_data[352*288*3];
unsigned char av_picture_yuv[(352*288*3) / 2]; //YUV 420
AVCodec *av_codec = 0;
AVCodecContext *av_context = 0;
AVFrame *av_picture = 0;
#endif

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "rendering.h"

#include <XPLMGraphics.h>
#include <XPLMDisplay.h>
#include <XPLMPlanes.h>
#include <XPLMUtilities.h>

char* xivss_debug_buf[80] = { 0 };
#endif
lockID xivss_debug_lock = BAD_ID;

//Just a small hack
extern int thread_initialized_global;


//==============================================================================
#define IVSS_TYPE_XSPACE_VARIABLE 0
#define IVSS_TYPE_XSPACE_RADIO 1
#define IVSS_TYPE_XSPACE_VIDEO 2

int IVSS_Interface_XSpace_RegisterUnit(IVSS_SYSTEM* system, IVSS_SIMULATOR* simulator, IVSS_UNIT* unit)
{
	if (strcmp(unit->model_name,"Variable") == 0) {
		IVSS_VARIABLE* variable;
		unit->type = IVSS_TYPE_XSPACE_VARIABLE;
		IVSS_Unit_GetNode(system,unit,"Value",&variable);
		if (!variable->invalid) variable->value[0] = 0.0;
		return IVSS_CLAIM_UNIT;
	}
	if (strcmp(unit->model_name,"Radio") == 0) {
		unit->type = IVSS_TYPE_XSPACE_RADIO;
		return IVSS_CLAIM_UNIT;
	}
#ifdef FFMPEG_SUPPORT
	if (strcmp(unit->model_name,"VideoCamera") == 0) {
		unit->type = IVSS_TYPE_XSPACE_VIDEO;
		return IVSS_CLAIM_UNIT;
	}
#endif
	return IVSS_IGNORE_UNIT;
}

//==============================================================================
int IVSS_Interface_XSpace_Simulation(IVSS_SYSTEM* system, IVSS_SIMULATOR* simulator, IVSS_UNIT* unit)
{
	if (unit->type == IVSS_TYPE_XSPACE_VIDEO) {
#ifdef FFMPEG_SUPPORT
		IVSS_VARIABLE* variable;
		unsigned char out_buf[128000];
		int size, out_size, i;

		IVSS_Unit_GetNode(system,unit,"VideoSignal",&variable);

		/*int i, out_size, size, x, y, outbuf_size;
		FILE *f;
		
		uint8_t *outbuf, *picture_buf;
		//double error_probability = 0.0;

		printf("Video encoding\n");*/

		//First-time initialization (FIXME)
		if ((!av_codec) && (!av_context)) {
			av_codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
			av_context = avcodec_alloc_context();
			av_picture = avcodec_alloc_frame();

			av_context->bit_rate = 100000;
			av_context->width = 352;
			av_context->height = 288;
			av_context->time_base.den = 25; //25 FPS
			av_context->time_base.num = 1;
			av_context->gop_size = 10; //I-frame period
			av_context->max_b_frames = 1;
			av_context->pix_fmt = PIX_FMT_YUV420P;

			if (avcodec_open(av_context, av_codec) < 0) {
				av_codec = 0;
			}

			size = av_context->width * av_context->height;
			av_picture->data[0] = av_picture_yuv;
			av_picture->data[1] = av_picture->data[0] + size;
			av_picture->data[2] = av_picture->data[1] + size / 4;
			av_picture->linesize[0] = av_context->width;
			av_picture->linesize[1] = av_context->width / 2;
			av_picture->linesize[2] = av_context->width / 2;
		}

		if (av_codec && (!system->is_paused)) {
			int x,y;

			//Convert frame to YUV
			//Y  =      (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
			//Cr = V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
			//Cb = U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128

			//Y
			for(y = 0; y < av_context->height; y++) {
				for(x = 0; x < av_context->width; x++) {
					av_picture->data[0][y * av_picture->linesize[0] + x] = 16
						+0.257*av_picture_data[(y*352+x)*3+0]
						+0.504*av_picture_data[(y*352+x)*3+1]
						+0.098*av_picture_data[(y*352+x)*3+2];//+x+y*2;
				}
			}
			//Cb and Cr
			for(y = 0; y < av_context->height/2; y++) {
				for(x = 0; x < av_context->width/2; x++) {
					av_picture->data[1][y * av_picture->linesize[1] + x] = 128
						+0.439*av_picture_data[(2*y*352+2*x)*3+0]+
						-0.368*av_picture_data[(2*y*352+2*x)*3+1]+
						-0.071*av_picture_data[(2*y*352+2*x)*3+2];
					av_picture->data[2][y * av_picture->linesize[2] + x] = 128
						-0.148*av_picture_data[(2*y*352+2*x)*3+0]
						-0.291*av_picture_data[(2*y*352+2*x)*3+1]
						+0.439*av_picture_data[(2*y*352+2*x)*3+2];
				}
			}

			//Encode current frame
			out_size = avcodec_encode_video(av_context, out_buf, 128000, av_picture);
			for (i = 0; i < out_size; i++) IVSS_Variable_WriteBuffer(system,variable,out_buf[i]);

			//Encode delayed frames
			while (out_size) {
				out_size = avcodec_encode_video(av_context, out_buf, 128000, 0);
				for (i = 0; i < out_size; i++) IVSS_Variable_WriteBuffer(system,variable,out_buf[i]);
			}
		}

		/*avcodec_close(c);
		av_free(c);
		av_free(picture);*/
		thread_sleep(1.0/60.0);
#endif
	} else {
		thread_sleep(1000.0);
	}
	return IVSS_OK;
}

int IVSS_Interface_XSpace_Frame(IVSS_SYSTEM* system, IVSS_UNIT* unit)
{
	IVSS_VARIABLE* variable;
	IVSS_VARIABLE* variable_aux;
	vessel* v = system->userdata;
	if (unit->type == IVSS_TYPE_XSPACE_RADIO) {
		double value;
		IVSS_Unit_GetNode(system,unit,"DataBus",&variable);
		IVSS_Unit_GetParameter(system,unit,"Channel",&variable_aux);
		while (IVSS_Variable_ReadBuffer(system,variable,&value) == IVSS_OK) {
			radiosys_transmit(v,(int)variable_aux->value[0],(char)(value));
			//if ((int)variable_aux->value[0] == 46) {
				//FILE* f = fopen(FROM_PLUGINS("v.mpeg1"),"ab+")
			//}
		}
	} else {
		int index,arr_index,vidx,is_attached;

		IVSS_Unit_GetParameter(system,unit,"Index",&variable_aux);
		index = (int)(variable_aux->value[0]);

		IVSS_Unit_GetParameter(system,unit,"ArrayIndex",&variable_aux);
		arr_index = (int)(variable_aux->value[0]);

		if (IVSS_Unit_GetParameter(system,unit,"Vessel",&variable_aux) != IVSS_ERROR_NOT_FOUND) {
			vidx = (int)(variable_aux->value[0]);
			if ((vidx >= 0) && (vidx < vessel_count)) {
				v = &vessels[vidx];
			}
		}

		is_attached = 1;
		if (IVSS_Unit_GetParameter(system,unit,"CheckAttached",&variable_aux) != IVSS_ERROR_NOT_FOUND) {
			if ((int)(variable_aux->value[0])) {
				is_attached = v->attached;
			}
		}

		//Read or write variable
		if (IVSS_Unit_GetNode(system,unit,"Set",&variable) != IVSS_ERROR_NOT_FOUND) {
			if (is_attached) {
				vessels_set_parameter(v,index,arr_index,variable->value[0]);
			}
		} else {
			IVSS_Unit_GetNode(system,unit,"Value",&variable);
			if (!variable->invalid) {
				if (is_attached) {
					variable->value[0] = vessels_get_parameter(v,index,arr_index);
				} else {
					variable->value[0] = 0.0;
				}
			}
		}
	}
	return IVSS_OK;
}

IVSS_SIMULATOR IVSS_Interface_XSpace = {
	"X-Space",
	IVSS_Interface_XSpace_RegisterUnit,
	0,
	IVSS_Interface_XSpace_Simulation,
	0,
	0
};


int IVSS_Interface_XSpace_Register(IVSS_SYSTEM* system) {
	return IVSS_Simulator_Register(system,&IVSS_Interface_XSpace);
}


//==============================================================================
void xivss_initialize_vessel(vessel* v, char* acfpath, char* acfname)
{
	FILE* f;
	char filename[MAX_FILENAME] = { 0 };
	char buf[ARBITRARY_MAX] = { 0 };

	if (xivss_debug_lock == BAD_ID) xivss_debug_lock = lock_create();

	//Get filename
	snprintf(filename,MAX_FILENAME-1,"%s/%s.ivss",acfpath,acfname);
	f = fopen(filename,"r");
	if (!f) {
		/*snprintf(filename,MAX_FILENAME-1,"%s/systems.ivss",acfpath);
		f = fopen(filename,"r");
		if (!f) {
			return;
		}*/
		return;
	}
	fclose(f);

	//Generate prefix for the system
	snprintf(buf,ARBITRARY_MAX-1,"%s/",acfpath);

	//Load the system
	IVSS_Simulator_XGDC_TranslatePrefix = buf;
	xivss_load(v,filename);
	IVSS_Simulator_XGDC_TranslatePrefix = 0;
}

void xivss_load(vessel* v, char* filename){
	int error_code;
	xivss_deinitialize(v);

	//Try to initialize system
	thread_initialized_global = 1;
	IVSS_System_Create(&v->ivss_system);
	IVSS_Simulator_XGDC_Register(v->ivss_system);
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	IVSS_Interface_XPlane_Register(v->ivss_system);
#endif
	IVSS_Interface_XSpace_Register(v->ivss_system);
#if (defined(DEDICATED_SERVER))
	xivss_initialize_vsfl(v->ivss_system);
#endif
	if (error_code = IVSS_System_LoadFromFile(v->ivss_system,filename)) {
		if (error_code == IVSS_ERROR_SYNTAX) log_write("X-IVSS: Loading error: %s\n",IVSS_System_GetError(v->ivss_system));
		thread_initialized_global = 2;
		IVSS_System_Destroy(&v->ivss_system);
	} else {
		log_write("X-Space: Loading IVSS description from %s...\n",filename);

		v->ivss_system->is_paused = 2;
		v->ivss_system->userdata = v;
		IVSS_System_Reset(v->ivss_system);
		//IVSS_System_GetUnitsByClassType(v->ivss_system,"XGDC",IVSS_TYPE_XGDC_VIDEO_PROCESSOR,&video_processors);
		//IVSS_System_GetUnitsByClassType(v->ivss_system,"XGDC",IVSS_TYPE_XGDC_MDM_PROCESSOR,&sensor_processors);
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
		IVSS_System_GetUnitsByClassType(v->ivss_system,"X-Plane",IVSS_TYPE_XPLANE_DATAREF,&v->ivss_datarefs);
#endif
		IVSS_System_GetUnitsByClassType(v->ivss_system,"X-Space",IVSS_TYPE_XSPACE_VARIABLE,&v->ivss_variables);
		IVSS_System_GetUnitsByClassType(v->ivss_system,"X-Space",IVSS_TYPE_XSPACE_RADIO,&v->ivss_radios);
	}
}

void xivss_deinitialize(vessel* v)
{
	if (v->ivss_system) {
		//Make sure IVSS knows that threading is initialized, and doesn't even try to de-initialize it
		thread_initialized_global = 2;
		IVSS_System_Destroy(&v->ivss_system);
	}
}

void xivss_simulate(vessel* v, float dt)
{
	if (v->ivss_system) {
		int i;
		v->ivss_system->is_paused = dt == 0.0;

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
		for (i = 0; i < v->ivss_datarefs->count; i++) {
			IVSS_Interface_XPlane_Frame(v->ivss_system,v->ivss_datarefs->units[i]);
		}
#endif
		for (i = 0; i < v->ivss_variables->count; i++) {
			IVSS_Interface_XSpace_Frame(v->ivss_system,v->ivss_variables->units[i]);
		}
		for (i = 0; i < v->ivss_radios->count; i++) {
			IVSS_Interface_XSpace_Frame(v->ivss_system,v->ivss_radios->units[i]);
		}
	}
}

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void xivss_draw(vessel* v)
{
	int i;//,w,h;
	/*XPLMGetScreenSize(&w,&h);

	for (i = 0; i < 80; i++) {
		float RGB[4] = { 0.75f,1.00f,1.00f,1.0f };
		if (xivss_debug_buf[i]) XPLMDrawString(RGB,12*2,12*(i+2),xivss_debug_buf[i],0,xplmFont_Basic);
	}*/

	lock_enter(xivss_debug_lock);
	for (i = 0; i < 80; i++) {
		if (xivss_debug_buf[i]) {
			if (highlevel_pushcallback("OnIVSSMessage")) {
				lua_pushstring(L,xivss_debug_buf[i]);
				highlevel_call(1,0);
			}
			free(xivss_debug_buf[i]);
			xivss_debug_buf[i] = 0;
		}
	}
	lock_leave(xivss_debug_lock);

	//Read as fast as possible?
#ifdef FFMPEG_SUPPORT
	glReadPixels((1024/2)-(352/2), (768/2)-(288/2), 352, 288, GL_BGR, GL_UNSIGNED_BYTE, av_picture_data);
#endif
}
#endif

//==============================================================================
// IVSS interface stuff
//==============================================================================
int IVSS_Simulator_XGDC_Video_Initialize(IVSS_SYSTEM* system, IVSS_UNIT* unit) { return IVSS_OK; }
int IVSS_Simulator_XGDC_Video_Deinitialize(IVSS_SYSTEM* system, IVSS_UNIT* unit) { return IVSS_OK; }
int IVSS_Simulator_XGDC_Video_BeginFrame(IVSS_SYSTEM* system, IVSS_UNIT* unit, IVSS_UNIT* disp_unit) { return IVSS_OK; }
int IVSS_Simulator_XGDC_Video_EndFrame(IVSS_SYSTEM* system, IVSS_UNIT* unit, IVSS_UNIT* disp_unit) { return IVSS_OK; }
int IVSS_Simulator_XGDC_Video_BeginDisplays(IVSS_SYSTEM* system, IVSS_UNIT* unit) { return IVSS_OK; }
int IVSS_Simulator_XGDC_Video_EndDisplays(IVSS_SYSTEM* system, IVSS_UNIT* unit) { return IVSS_OK; }
void add_debug_log(const char* message) {
	int i;
	lock_enter(xivss_debug_lock);
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	if (xivss_debug_buf[79]) free(xivss_debug_buf[79]);
	for (i = 79; i > 0; i--) {
		xivss_debug_buf[i] = xivss_debug_buf[i-1];
	}
	xivss_debug_buf[0] = malloc(strlen(message)+1);
	strcpy(xivss_debug_buf[0],message);
#else
	printf("%s\n",message);
#endif
	lock_leave(xivss_debug_lock);
}
void IVSS_Simulator_XGDC_Error(const char* message) {
	log_write("AVCL Error: %s\n",message);
	add_debug_log(message);
//#ifdef _DEBUG
	//_asm {int 3}
//#endif
}
void IVSS_Simulator_XGDC_Message(const char* message) {
	log_write("AVCL: %s\n",message);
	add_debug_log(message);
}


//==============================================================================
// Highlevel interface
//==============================================================================
int xivss_highlevel_loadmodel(lua_State* L)
{
	vessel* v = 0;
	const char* filename = luaL_checkstring(L,2);
	int idx = luaL_checkint(L,1);
	if ((idx >= 0) && (idx < vessel_count)) {
		v = &vessels[idx];
	}

	if (v) {
		xivss_load(v,filename);
	}
	return 0;
}

int xivss_highlevel_reloadmodel(lua_State* L)
{
	vessel* v = 0;
	int idx = luaL_checkint(L,1);
	if ((idx >= 0) && (idx < vessel_count)) {
		v = &vessels[idx];
	}

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	if (v) {
		char path[MAX_FILENAME] = { 0 }, model[MAX_FILENAME] = { 0 };
		char buf[ARBITRARY_MAX] = { 0 };
		char* acfpath;

		XPLMGetNthAircraftModel(v->plane_index, model, path);
		XPLMExtractFileAndPath(path);
	
		//acfpath = strstr(path,"Aircraft");
		//if (acfpath) snprintf(path,MAX_FILENAME-1,"./%s",acfpath);
#ifdef APL
		{
			char* tpath;
			while (tpath = strchr(path,':')) *tpath = '/';
			strncpy(model,path,MAX_FILENAME-1);
			strncpy(path,"/Volumes/",MAX_FILENAME-1);
			strncat(path,model,MAX_FILENAME-1);
		}
#endif

		//Fetch only aircraft model name
		acfpath = strstr(model,".acf");
		if (acfpath) *acfpath = 0;

		xivss_initialize_vessel(v,path,model);
	}
#endif
	return 1;
}

void xivss_highlevel_initialize()
{
	//Register API
	lua_createtable(L,0,32);
	lua_setglobal(L,"IVSSAPI");

	highlevel_addfunction("IVSSAPI","LoadModel",xivss_highlevel_loadmodel);
	highlevel_addfunction("IVSSAPI","ReloadModel",xivss_highlevel_reloadmodel);

#ifdef FFMPEG_SUPPORT
	avcodec_register_all();
	//av_log_set_level(AV_LOG_QUIET);
#endif
}