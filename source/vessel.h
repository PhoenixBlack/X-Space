#ifndef VESSEL_H
#define VESSEL_H

#ifndef _QUATERNION_TYPE
#define _QUATERNION_TYPE
typedef double quaternion[4];
#endif

#ifndef __THREAD_ID
	#define BAD_ID 0xFFFFFFFF
	typedef unsigned int genericID;
	typedef genericID threadID;
	typedef genericID lockID;
	#define __THREAD_ID
#endif

#define VESSEL_PHYSICS_SIM				0
#define VESSEL_PHYSICS_INERTIAL			1
#define VESSEL_PHYSICS_INERTIAL_TO_SIM	2
#define VESSEL_PHYSICS_NONINERTIAL		3
#define VESSEL_PHYSICS_DISABLED			4

#define VESSEL_MOUNT_LAUNCHPAD			-1
#define VESSEL_MAX_FUELTANKS			50
#define VESSEL_MAX_SOLAR_PANELS			100
#define VESSEL_MAX_ENGINES				512

#define FACE_LAYER_TPS					0
#define FACE_LAYER_HULL					1

//------------------------------------------------------------------------------
// Single geometric face of the mesh (used for heating, drag computations)
// Read/write flags show which variables can be read/written at any time
//------------------------------------------------------------------------------
typedef struct face_tag {
	//Thermal parameters (must be first for alignment)
	double heat_flux;			//Heat flux, w/m^2
	double temperature[2];		//Layer temperature, K
	double surface_temperature;	//Surface temperature, K
	double shockwaves;			//Number of shockwaves intersecting with given point

	//Location and size
	double x[3],y[3],z[3];		//Vertices
	double nx,ny,nz;			//Normal to triangle
	double ax,ay,az;			//Aerodynamic center
	double vnx[3],vny[3],vnz[3];//Normal in vertex

	//Information about boundary faces
	int boundary_faces[3*8];	//Max 8 boundary faces for each vertex
	int boundary_num_faces1;	//Vertex 0
	int boundary_num_faces2;	//Vertex 1
	int boundary_num_faces3;	//Vertex 2

	int ow_boundary_faces[3*8];	//One-way list of boundary faces
	int ow_boundary_num_faces1;	//Faces in this list do not have THIS face in their lists
	int ow_boundary_num_faces2;	//Used for drag/heating computations
	int ow_boundary_num_faces3;	//

	int boundary_edge[3];		//Triangle at edge 0,1,2

	//Geometric parameters
	double area;				//Total area, m^2
	double thickness;			//Total thickness, m
	double mass;				//Total TPS mass, kg
	double hull_mass;			//Total hull mass, kg
	int creates_shockwave;		//Does this element create a shockwave?

	//Temporary variables
	double _Q;					//Change in energy, J
	double _Cp[2],_k[2];		//Material properties
	double _temperature[2];		//Layer temperature, K
	double _dot;				//Used for rendering
	double _dot0,_dot1,_dot2;	//Used for rendering, dot of triangles with shared edge
	double _shockwaves;			//Accumulated number of shockwaves (used in parallel thread)

	//Computed information
	double fx,fy,fz;			//Force
	double Q;					//Dynamic pressure, Pa
	double pressure;			//Static pressure, Pa

	//Material parameters
	int m;						//Surface material, -1 if none
	int creates_drag;			//Does this face create drag
} face;


//------------------------------------------------------------------------------
// Surface sensor (measures something off vessels surface)
//------------------------------------------------------------------------------
typedef struct surface_sensor_tag {
	//Coordinates/face lookup
	double x,y,z;			//Coordinates
	double d0,d1,d2;		//Distances to vertexes of given face
	int face;				//Face that this sensor belongs to

	//Measured parameters
	double temperature;		//K
	double heat_flux;		//W/m2
	//double Q; //Pa
} surface_sensor;


typedef struct radio_buffers_tag {
	int num_channels;				//Number of channels allocated (typically 128)
	int buffer_size;				//Total size of data buffer (typically 256 KB)
	int* channels_recv_used;		//Is this channel can be received by this vessel?
	int** send_buffer;				//Data buffers for every channel
	int** recv_buffer;				//
	int* send_buffer_user_position;	//Index of last sent byte
	int* recv_buffer_user_position;	//Index of last read byte
	int* send_buffer_sys_position;	//Index of byte being sent at the moment
	int* recv_buffer_sys_position;	//Index of byte last byte that can be received
} radio_buffers;


typedef struct solar_panel_tag {
	double bx,by,bz;	//Base location
	double x,y,z;		//Solar panel location
	double w,h;			//Dimensions
	quaternion q;		//Attitude

	double Cd;			//Drag coefficient
	double A;			//Total area
} solar_panel;


//------------------------------------------------------------------------------
// Vessel (any sort of object that is located in the world)
//------------------------------------------------------------------------------
typedef struct vessel_tag {
	//Vessel parameters
	double cx,cy,cz;		//Center of mass (in local coordinates)
	double jxx,jyy,jzz;		//Radius of gyration squared
	double ixx,iyy,izz;		//[Calculated] Moment of inertia. Used in actual computations, accounts for attached bodies
	double mass;			//[Calculated] Total mass. Used in computations, includes mass of attached bodies
	double attached_mass;	//[Calculated] Mass attached to this vessel. Used for X-Plane physics interface
	int index;				//0: main vessel

	//Vessel weights
	struct {
		double chassis;		//Mass of the structure itself
		double hull;		//Mass of the hull
		double tps;			//Mass of thermal protection system

		struct {
			double x,y,z;
		} fuel_location[VESSEL_MAX_FUELTANKS];
		double fuel[VESSEL_MAX_FUELTANKS];	//Fuel tanks (for vessel 0 they map to X-Plane fuel)
	} weight;

	//Vessel parameters
	int physics_type;		//Should inertial physics be calculated for it
	double detach_time;		//Time at which last object was detached. X-Plane specific hack
	//int network_override;	//Vessel overriden by networking

	//Existance parameters
	int exists;				//Does the vessel exist now
	int attached;			//Is vessel attached to other vessel (see mount)

	//Plane parameters (if loaded as an AI plane)
	int is_plane;			//Is this weapon represented as X-Plane aircraft
	int plane_index;		//Index for AI plane
	char plane_filename[1024]; //Filename for AI plane

	//Networking data
	char net_model[1024];	//Network model name (CSL ID or obj filename). First byte \0 if not networked
	char net_signature[16]; //128-bit network signature
	int net_id;				//Unique networked ID (network hash)
	int client_id;			//Networked client ID
	int networked;			//Is this vessel from cyberspace (used on X-Space clients to disable physics)
	double last_update;		//Time of the last networking update

	//Inertial coordinate system position variables
	struct {
		double x,y,z;		//Position
		double vx,vy,vz;	//Velocity
		quaternion q;		//Attitude
		double P,Q,R;		//Rates (in sim coordinate system, not inertial!)

		//For information only
		double ax,ay,az;	//Linear acceleration
		double Pd,Qd,Rd;	//Angular acceleration
	} inertial;

	//Simulator coordinate system position variables
	struct {
		double x,y,z;		//Position
		double vx,vy,vz;	//Velocity
		double ivx,ivy,ivz; //Inertial velocity
		quaternion q;		//Attitude
		double P,Q,R;		//Rates

		//For information only
		double ax,ay,az;	//Linear acceleration
		double Pd,Qd,Rd;	//Angular acceleration
	} sim;

	//Local coordinate system variables
	struct {
		double  x, y, z;	//Position
		double vx,vy,vz;	//Velocity
		double ax,ay,az;	//Acceleration
	} local;

	//Non-inertial coordinate system variables (info only)
	struct {
		double  x, y, z;	//Position
		double vx,vy,vz;	//Velocity (actually still inertial velocity! just in non-inertial coordinates!)
		double ax,ay,az;	//Acceleration
		quaternion q;		//Attitude
		double P,Q,R;		//Rates
		double nvx,nvy,nvz;	//Non-inertial velocity (this is the correct velocity accounting for non-inertial effects)

		double cx,cy,cz;	//Coordinate offset (for networking)
	} noninertial;

	//Relative to earth coordinate system (info only)
	struct {
		quaternion q;
		double roll,pitch,yaw,heading,hpath,vpath; //degrees
		double P,Q,R; //degrees/second
		double airspeed,vv;
		double gx,gy,gz; //accelerometer values
	} relative;

	//Orbital elements
	struct {
		//State vector
		double smA;		//Semi-major axis
		double e;		//Eccentricity
		double i;		//Inclination
		double MnA;		//Mean anomaly
		double AgP;		//Periapsis argument
		double AsN;		//Ascending node

		//Additional parameters
		//double TrA;		//True anomaly
		double BSTAR;	//BSTAR drag term (for NORAD two-line sets)
		double period;	//Orbital period in seconds
	} orbit;

	//Forces acting over center of mass (in global coordinate system)
	struct {
		double ax,ay,az;	//Linear acceleration
		double Pd,Qd,Rd;	//Angular acceleration
	} accumulated;

	//Geographical position
	double latitude,longitude,elevation;

	//Mount information. Body 0 mounts to launch pad, other bodies mount to it
	struct {
		int body;				//Body this is mounted to, or launch pad index
		double x,y,z;			//Attachment offset (local coords offset from root body)
		quaternion attitude;	//Mount attitude

		int checked;			//Mounting stuff checked (reset before doing vessel_update_physics)
		int root_body;			//Body the physics must be applied to
		double rx,ry,rz;		//Attachment offset relative to root body
		quaternion rattitude;	//Attitude relative to root body
	} mount;

	//Geometric information
	struct {
		//Geometric data
		int num_faces;
		face* faces;

		//Aerodynamic data
		double Cd;				//Drag coefficient (for all faces)
		double K;				//High-velocity heating coefficient
		double trqx,trqy,trqz;	//Torque coefficients (for physics)
		double dx,dy,dz;		//Offset of 3D model (used only by the editor)
		double mx,my,mz;		//Visual mass-center (used only for rendering)
		double total_area;		//Total area exposed to air
		int shockwave_heating;	//Should shockwave heating be accounted for? (slow)

		//Material data
		int hull;				//Hull material
		int invalid;			//Is geometry invalid (cannot perform calculations based on it)?

		//Misc data
		void* obj_ref;			//XPLMObjectRef reference to visual model
		threadID heat_thread;	//Heat simulation thread ID
		threadID shockwave_thread; //Shockwave simulation thread ID
		lockID heat_data_lock;	//Heat data lock (heat-flux is updated)
		double heat_fps;		//Heat simulation FPS
		double effective_M;		//Mach number used in computations

		//Even more misc data
		double vx,vy,vz,vmag;	//Velocity direction, magnitude in local coordinates
	} geometry;

	//Atmosphere information at altitude
	struct {
		//Passive
		double density;
		double concentration;
		double temperature;
		double pressure; //Static pressure
		double Q; //Dynamic pressure

		//Partial
		double density_He;
		double density_O;
		double density_N2;
		double density_O2;
		double density_Ar;
		double density_H;
		double density_N;
		double concentration_He;
		double concentration_O;
		double concentration_N2;
		double concentration_O2;
		double concentration_Ar;
		double concentration_H;
		double concentration_N;
		double exospheric_temperature;
	} air;

	//Geomagnetic data
	struct {
		double declination; //Angle between magfield vector and true north, positive east
		double inclination; //Angle between magfield vector and the horizontal plane, positive down
		double F; //Magnetic field strength
		double H; //Horizontal magnetic field strength
		struct { double x,y,z; } noninertial;
		struct { double x,y,z; } local;
		double GV; //Grid variation

		double V; //Gravity potential
		double g; //Freefall acceleration
	} geomagnetic;

	//Surface sensors
	struct {
		surface_sensor* surface;
		int surface_count;
	} sensors;

	//Radio transmission simulation
	struct {
		radio_buffers buffers;

		//Transmission variables
		int channel;
	} radiosys;

	//Statistics
	struct {
		double time_in_space;
		double total_orbits;
		double total_true_orbits;
		double total_distance;

		double prev_inertial_longitude;
		double prev_longitude;
		double prev_latitude;
	} statistics;

	//Solar panels
	solar_panel solar_panels[VESSEL_MAX_SOLAR_PANELS];

	//Internal systems
	struct IVSS_SYSTEM_TAG* ivss_system;
	struct IVSS_UNITS_LIST_TAG* ivss_datarefs;
	struct IVSS_UNITS_LIST_TAG* ivss_variables;
	struct IVSS_UNITS_LIST_TAG* ivss_radios;
} vessel;

//Iteraction with X-Plane
void vessels_read(vessel* v, int update_inertial); //Read from simulator
void vessels_write(vessel* v); //Write to simulator
void vessels_update_coordinates(vessel* v); //Updates coordinates (synchronizes sim, inertial)

void vessels_set_ni(vessel* v); //Set by non-inertial coordinates
void vessels_set_i(vessel* v); //Set by inertial coordinates
void vessels_set_local(vessel* v); //Set by local coordinates
void vessels_get_ni(vessel* v); //Get non-inertial coordinates from the vessel

//Vessel management
int vessels_add(); //Adds new vessel, returns its index
int vessels_load(char* filename); //Loads vessel and return index
void vessels_set_parameter(vessel* v, int index, int array_index, double value); //Write parameter
double vessels_get_parameter(vessel* v, int index, int array_index); //Read parameter

//Add force on a vessel (in local coordinates)
void vessels_addforce(vessel* v, double dt, double lx, double ly, double lz, double fx, double fy, double fz);
void vessels_update_mount_physics(vessel* v); //Updates forces and behaviour of mount joints
void vessels_update_physics(vessel* v); //Recalculate total mass, moments of inertia
void vessels_reset_physics(vessel* v); //Reset physics calculations
void vessels_compute_moments(vessel* v); //Computes Jxx, Jyy, Jzz based on geometry, weight, etc

//General functions
void vessels_initialize(); //Initialize vessels system
void vessels_reinitialize();
void vessels_deinitialize(); //Free up resources
void vessels_draw(); //Draw all vessels
void vessels_update_aircraft(); //Update X-Plane aircraft

//Highlevel functions
void vessels_highlevel_initialize();
void vessels_highlevel_logic();

//List of all vessels
extern int vessel_count;
extern int vessel_alloc_count;
extern vessel* vessels;

#endif