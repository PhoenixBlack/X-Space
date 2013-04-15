#ifndef CONFIG_H
#define CONFIG_H

typedef struct global_config {
	//Resource settings
	int max_vessels;

	//Debug settings
	int write_atmosphere;
	int draw_coordsys;

	//Rendering settings
	int use_shaders;			//Use shaders in various drawing routines
	int use_clouds;				//Load clouds texture, draw clouds layer from orbit
	int use_scattering;			//Enables scattering
	int use_hq_clouds;			//Use high quality clouds (shaders)
	int cloud_shadows;			//Clouds cast shadows
	int max_particles;			//Maximum number of particles
	int reentry_glow;			//Draw reentry glow
	int engine_debug_draw;		//Draw engine positions
	int sensor_debug_draw;		//Draw sensor positions
	int use_particles;			//Draw particles for exhaust

	//Physics settings
	int use_inertial_physics;	//Use custom physics engine above 385,000 ft
	int nonspherical_gravity;	//Use non-spherical gravity
	int planet_rotation;		//Simulate planet rotation
	double staging_wait_time;	//Time during which inertial physics must be enabled after staging
} global_config;

extern global_config config;

void config_initialize();
void config_load();
void config_save();

#endif