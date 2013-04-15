#ifndef PHYSICS_H
#define PHYSICS_H

typedef struct vessel;

typedef struct orbit {
	double count; //Number of orbits around planet

	//Previous coordinates (for orbit counter)
	double previous_latitude,previous_longitude;
} orbit;

orbit current_orbit;

void physics_initialize();
void physics_update(float dt);
void physics_integrate(float dt, vessel* v);

#endif
