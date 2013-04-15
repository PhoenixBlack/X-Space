#ifndef DRAGHEAT_H
#define DRAGHEAT_H

#define DRAGHEAT_VISUALMODE_NORMAL				0
#define DRAGHEAT_VISUALMODE_FLUX				1
#define DRAGHEAT_VISUALMODE_TEMPERATURE			2
#define DRAGHEAT_VISUALMODE_HULL_TEMPERATURE	3
#define DRAGHEAT_VISUALMODE_DYNAMIC_PRESSURE	4
#define DRAGHEAT_VISUALMODE_FEM					5
#define DRAGHEAT_VISUALMODE_MATERIAL			6
#define DRAGHEAT_VISUALMODE_SHOCKWAVES			7

void dragheat_initialize(vessel* v, char* acfpath, char* acfname);
void dragheat_initialize_highlevel(vessel* v);
void dragheat_deinitialize(vessel* v);
void dragheat_simulate(float dt);
void dragheat_simulate_vessel(vessel* v, float dt);
void dragheat_simulate_vessel_heat(vessel* v);
void dragheat_simulate_vessel_shockwaves(vessel* v);
void dragheat_reset();

void dragheat_draw_initialize();
void dragheat_draw_deinitialize();
void dragheat_draw();
void dragheat_draw2d();
void dragheat_draw_vessel(vessel* v);

void dragheat_model_loadobj_file(char* filename, int creates_drag);
int dragheat_model_loadobj(vessel* v);
int dragheat_model_save(vessel* v);

void dragheat_highlevel_initialize();

extern int dragheat_visual_mode;

#endif