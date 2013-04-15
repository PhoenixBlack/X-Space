#ifndef ENGINE_H
#define ENGINE_H

struct vessel_tag;

void engines_initialize();
void engines_draw_initialize();
void engines_draw_deinitialize();
void engines_draw();
void engines_simulate(float dt);
void engines_simulate_xplane(float dt);
void engines_addforce(struct vessel_tag* v, double dt, double x, double y, double z, double vert, double horiz, double force);
void engines_update_weights(struct vessel_tag* v); //Update fuel weights by X-Plane variables
void engines_write_weights(struct vessel_tag* v); //Update fuel weights to X-Plane variables

#endif