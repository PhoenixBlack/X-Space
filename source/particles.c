//==============================================================================
// Particles system
//==============================================================================
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <soil.h>
#include <math.h>

#include "x-space.h"
#include "config.h"
#include "particles.h"
#include "rendering.h"
#include "lookups.h"
#include "vessel.h"
#include "dragheat.h"

#include <XPLMGraphics.h>

//List of particles
particle* particles[PARTICLE_TYPES] = { 0 };
int particles_allocated_count[PARTICLE_TYPES];
int particles_count[PARTICLE_TYPES];

//Misc variables
float particles_last_dt;	//Used for checking for pause
int particles_glow_texture; //Glow texture
int particles_perlin_perm_texture;

double particles_offset_lat;
double particles_offset_lon;
double particles_offset_alt;

//Particle shaders
GLhandleARB particles_program[PARTICLE_TYPES];		//Program object
GLhandleARB particles_vertex[PARTICLE_TYPES];		//Vertex shader for clouds
GLhandleARB particles_fragment[PARTICLE_TYPES];		//Fragment shader 
GLint particles_perm_location[PARTICLE_TYPES];		//Permutations texture

void particles_initialize()
{
#	define GLOW_SIZE 256
	unsigned char glow_texture[GLOW_SIZE*GLOW_SIZE*4];
	char* data;
	int i,j;

	config.max_particles = 1024;

	//Generate glow texture
	for (j = 0; j < GLOW_SIZE; j++) {
		for (i = 0; i < GLOW_SIZE; i++) {
			int dx = i-(GLOW_SIZE/2);
			int dy = j-(GLOW_SIZE/2);
			float d = powf(sqrtf(dx*dx+dy*dy)/132.0f,0.5f);

			glow_texture[(i+j*GLOW_SIZE)*4+0] = (char)max(0,255.0f-255.0f*d);
			glow_texture[(i+j*GLOW_SIZE)*4+1] = (char)max(0,255.0f-255.0f*d);
			glow_texture[(i+j*GLOW_SIZE)*4+2] = (char)max(0,255.0f-255.0f*d);
			glow_texture[(i+j*GLOW_SIZE)*4+3] = (char)max(0,255.0f-255.0f*d);
		}
	}

	XPLMGenerateTextureNumbers(&particles_glow_texture,1);
	XPLMBindTexture2d(particles_glow_texture,0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GLOW_SIZE, GLOW_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, glow_texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	if (!config.use_shaders) return;


	//Generate perlin noise texture
	data = (char*)malloc(256*256*4);
	for (i = 0; i<256; i++) {
		for (j = 0; j<256; j++) {
			int offset = (i*256+j)*4;
			char value = lookup_perlin_perm[(j+lookup_perlin_perm[i]) & 0xFF];
			data[offset+0] = lookup_perlin_grad3[value & 0x0F][0] * 64 + 64; //Gradient x
			data[offset+1] = lookup_perlin_grad3[value & 0x0F][1] * 64 + 64; //Gradient y
			data[offset+2] = lookup_perlin_grad3[value & 0x0F][2] * 64 + 64; //Gradient z
			data[offset+3] = value; //Permuted index
		}
	}

	XPLMGenerateTextureNumbers(&particles_perlin_perm_texture,1);
	XPLMBindTexture2d(particles_perlin_perm_texture,0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //Must use GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	free(data);


	if (!config.use_particles) return;


	//Load shaders for particles
	for (i = 0; i < PARTICLE_TYPES; i++) {
		char filename[ARBITRARY_MAX] = { 0 };

		//Load shaders
		snprintf(filename,ARBITRARY_MAX-1,FROM_PLUGINS("shaders/particle_type%d.vert"),i);
		particles_vertex[i] = rendering_shader_compile(GL_VERTEX_SHADER,filename,1);
		snprintf(filename,ARBITRARY_MAX-1,FROM_PLUGINS("shaders/particle_type%d.frag"),i);
		particles_fragment[i] = rendering_shader_compile(GL_FRAGMENT_SHADER,filename,1);

		particles_program[i] = rendering_shader_link(particles_vertex[i],particles_fragment[i]);
		particles_perm_location[i] = glGetUniformLocation(particles_program[i], "permTexture");

		//Create particles
		if (!particles[i]) {
			particles[i] = (particle*)malloc(config.max_particles*sizeof(particle));
			particles_allocated_count[i] = config.max_particles;
			particles_count[i] = 0;
		}
	}

	//Initialize coordinate systems
	XPLMLocalToWorld(0,0,0,&particles_offset_lat,&particles_offset_lon,&particles_offset_alt);
}

void particles_deinitialize()
{
	int i;
	if (!config.use_shaders) return;
	if (!config.use_particles) return;
	for (i = 0; i < PARTICLE_TYPES; i++) {
		glDeleteProgram(particles_program[i]);
		glDeleteShader(particles_vertex[i]);
		glDeleteShader(particles_fragment[i]);
		//free(particles[i]);
	}
}

void particles_update(float dt)
{
	int i,type;
	double lat,lon,alt;
	if (!config.use_shaders) return;
	if (!config.use_particles) return;
	particles_last_dt = dt;

	XPLMLocalToWorld(0,0,0,&lat,&lon,&alt);
	if ((fabs(particles_offset_lat - lat) > 0.1) ||
		(fabs(particles_offset_lon - lon) > 0.1)) {
		particles_offset_lat = lat;
		particles_offset_lon = lon;
		for (type = 0; type < PARTICLE_TYPES; type++) {
			for (i = 0; i < particles_count[type]; i++) {
				double x,y,z;
				particle* p = &particles[type][i];
				XPLMWorldToLocal(p->lat,p->lon,p->alt,&x,&y,&z);
				p->x = (float)x;
				p->y = (float)y;
				p->z = (float)z;
			}
		}
	}

	for (type = 0; type < PARTICLE_TYPES; type++) {
		for (i = 0; i < particles_count[type]; i++) {
			particle* p = &particles[type][i];

			//Special logic
			//if (type == 0) {
				//
			//}

			//Update particle age
			p->age += p->vel*dt;
			if ((p->age > 0.3f) && (i % 2)) p->age += 4.0f*p->vel*dt;

			//Kill particle
			if (p->age >= 1.0f) {
				if ((particles_count[type] > 0) && (i != particles_count[type]))
					memcpy(&particles[type][i],&particles[type][particles_count[type]-1],sizeof(particle));
				particles_count[type]--;
			}

			//Update global coordinates
			XPLMLocalToWorld(p->x,p->y,p->z,&lat,&lon,&alt);
			p->lat = (float)lat;
			p->lon = (float)lon;
			p->alt = (float)alt;
		}
	}
}

void particles_draw()
{
	int type,i;
	if (!config.use_shaders) return;
	if (!config.use_particles) return;

	for (type = 0; type < PARTICLE_TYPES; type++) {
		switch (type) {
			case 0:
				XPLMSetGraphicsState(0, 1, 1, 0, 1, 1, 0);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
				break;
			case 1:
				if (dragheat_visual_mode != DRAGHEAT_VISUALMODE_NORMAL) continue;
				XPLMSetGraphicsState(0, 1, 0, 0, 1, 1, 0);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE);
				break;
			default:
				break;
		}

		glUseProgram(particles_program[type]);
		glActiveTexture(GL_TEXTURE0);
		XPLMBindTexture2d(particles_perlin_perm_texture,0);
		if (particles_perm_location[type] != -1) glUniform1i(particles_perm_location[type], 0);

		glBegin(GL_QUADS);
		for (i = 0; i < particles_count[type]; i++) {
			particle* p = &particles[type][i];
			float offset = (1.0f*i)/(1.0f*config.max_particles);

			glColor4f(1.0f,p->p,offset,p->age);
			glTexCoord2f(0.0f,0.0f);
			glVertex3f(p->x,p->y,p->z);
			glTexCoord2f(1.0f,0.0f);
			glVertex3f(p->x,p->y,p->z);
			glTexCoord2f(1.0f,1.0f);
			glVertex3f(p->x,p->y,p->z);
			glTexCoord2f(0.0f,1.0f);
			glVertex3f(p->x,p->y,p->z);
		}
		glEnd();

		glUseProgram(0);
		glActiveTexture(GL_TEXTURE0);
	}
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void particles_spawn(double x, double y, double z, double duration, double parameter, int type)
{
	double lat,lon,alt;
	particle* p;
	if (!config.use_shaders) return;
	if (!config.use_particles) return;
	if (particles_last_dt == 0.0) return;

	if (particles_count[type] < particles_allocated_count[type]) {
		p = &particles[type][particles_count[type]];
		memset(p,0,sizeof(particle));
		particles_count[type]++;
	} else {
		int i,imin;
		float min;

		imin = 0;
		min = particles[type][0].age;
		for (i = 0; i < particles_count[type]; i++) {
			if ((particles[type][i].age < 0.8f) &&
				(particles[type][i].age > 0.3f)) {
				//min = particles[type][i].age;
				imin = i;
				break;
			}
		}

		p = &particles[type][imin];
		memset(p,0,sizeof(particle));
	}

	p->x = (float)x;
	p->y = (float)y;
	p->z = (float)z;
	p->p = (float)parameter;
	p->age = 0.0f;
	p->vel = 1.0f/((float)duration);

	XPLMLocalToWorld(p->x,p->y,p->z,&lat,&lon,&alt);
	p->lat = (float)lat;
	p->lon = (float)lon;
	p->alt = (float)alt;
}