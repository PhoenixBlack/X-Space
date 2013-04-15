#ifndef LAUNCHPADS_H
#define LAUNCHPADS_H

#define LAUNCHPADS_FLAG_SMALL_ROCKETS	0x00000001
#define LAUNCHPADS_FLAG_MEDIUM_ROCKETS	0x00000002
#define LAUNCHPADS_FLAG_BIG_ROCKETS		0x00000004
#define LAUNCHPADS_FLAG_SHUTTLES		0x00000008
#define LAUNCHPADS_FLAG_KNOWNPOSITION	0x80000000

#ifndef _QUATERNION_TYPE
#define _QUATERNION_TYPE
typedef double quaternion[4];
#endif

struct vessel_tag;

typedef struct launch_pad_tag {
	int index;					//Pad index
	float latitude,longitude;	//Geographic coordinates
	float heading;				//Pad direction
	double elevation;			//Elevation
	int model,planet;			//Indexes
	char pad_name[8];			//Short name (numerical)
	char complex_name[256];		//Space complex name
	int flags;					//LAUNCHPAD_FLAG_*
} launch_pad;

typedef struct launch_pad_model_tag {
	float x,y,z;	//Joint offset
	quaternion q;	//Rotation offset
	int flags;		//Model flags
} launch_pad_model;

void launchpads_initialize();
void launchpads_deinitialize();
void launchpads_draw();
void launchpads_simulate(float dt);
void launchpads_set(struct vessel_tag* v, launch_pad* pad);
launch_pad* launchpads_get_nearest(struct vessel_tag* v);

void launchpads_highlevel_initialize();

extern launch_pad* launchpads;
extern int launchpads_count;

#endif
