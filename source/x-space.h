#ifndef X_SPACE_H
#define X_SPACE_H

#define VERSION				"8"

#define MAX_FILENAME		((32760 + 255 + 1)*2)
#define ARBITRARY_MAX		16384
#ifdef DEDICATED_SERVER
#ifndef ORBITER_MODULE
#define FROM_PLUGINS(file)	"./"file
#else
#define FROM_PLUGINS(file)	"./Modules/x-space/"file
#endif
#else
#define FROM_PLUGINS(file)	"./Resources/plugins/x-space/"file
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifdef _WIN32
#define snprintf _snprintf
#define snscanf _snscanf
#endif

#define PI 3.14159265358979323846264338327950288419716939937510
#define PIf 3.14159265358979323846f
#define RAD(x) (PI*(x)/180.0)
#define DEG(x) (180.0*(x)/PI)
#define RADf(x) ((float)(PIf*(x)/180.0f))
#define DEGf(x) ((float)(180.0f*(x)/PIf))

//Functions defined by simulator
void log_write(char* text, ...);

//Resource management
void xspace_initialize_all();
void xspace_deinitialize_all();
void xspace_reinitialize();
void xspace_reload_aircraft(int index);
void xspace_load_aircraft(int index, char* acfpath, char* acfname);
void xspace_unload_aircraft(int index);
extern int xspace_initialized_all;
extern int xspace_initialized_aircraft;

//Main loops
void xspace_update(float dt);
//void xspace_draw();

//Request for exclusive access over X-Plane aircraft
void xspace_request_models(char** models);
void xspace_models_are_useless();
extern int xspace_controls_models;

#endif
