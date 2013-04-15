#ifndef X_IVSS_H
#define X_IVSS_H

void xivss_highlevel_initialize();
void xivss_initialize_vessel(vessel* v, char* acfpath, char* acfname);
void xivss_load(vessel* v, char* filename);
void xivss_deinitialize(vessel* v);
void xivss_simulate(vessel* v, float dt);
void xivss_draw(vessel* v);

#endif