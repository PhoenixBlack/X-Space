#ifndef PARTICLES_H
#define PARTICLES_H

#define PARTICLE_TEXTURES		2
#define PARTICLE_TYPES			2

typedef struct particle_tag {
	float lat,lon,alt;	//Base lon/lat
	float x,y,z;		//Coordinates
	float age;			//Age of particle
	float vel;			//Velocity
	float p;			//Parameter
} particle;

void particles_initialize();
void particles_deinitialize();
void particles_update(float dt);
void particles_draw();
void particles_spawn(double x, double y, double z, double duration, double parameter, int type);

extern int particles_perlin_perm_texture; //Perlin noise permutation texture
extern int particles_glow_texture;

#endif