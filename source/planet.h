#ifndef PLANET_H
#define PLANET_H

typedef struct planet {
	double radius;			//Radius (m)
	double mu;				//Gravitational parameter (km^3 s^-2)
	double w;				//Rotation rate (rad/sec)
	double a;				//Current rotation angle (0 deg: 12 am at lon = 0)
	int index;				//Index (0: Earth, 1: Mars)
	double sim_sun_pitch;	//Sun pitch (X-Plane coordinates)
	double sim_sun_heading;	//Sun heading (X-Plane coordinates)
} planet;

planet current_planet;

void planet_initialize();
void planet_update(float dt);

void planet_scattering_initialize();
void planet_scattering_deinitialize();
void planet_scattering_draw();

void planet_clouds_initialize();
void planet_clouds_deinitialize();
void planet_clouds_draw();

#endif