#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "x-space.h"
#include "vessel.h"
#include "rendering.h"
#include "config.h"
#include "planet.h"
#include "particles.h"
#include "engine.h"
#include "dragheat.h"

#include <XPLMGraphics.h>

#ifdef WIN32
PFNGLACTIVETEXTUREPROC           glActiveTexture      = NULL;
PFNGLCREATEPROGRAMPROC           glCreateProgram      = NULL;
PFNGLDELETEPROGRAMPROC           glDeleteProgram      = NULL;
PFNGLUSEPROGRAMPROC              glUseProgram         = NULL;
PFNGLCREATESHADERPROC            glCreateShader       = NULL;
PFNGLDELETESHADERPROC            glDeleteShader       = NULL;
PFNGLSHADERSOURCEPROC            glShaderSource       = NULL;
PFNGLCOMPILESHADERPROC           glCompileShader      = NULL;
PFNGLGETSHADERIVPROC             glGetShaderiv        = NULL;
PFNGLGETPROGRAMIVPROC            glGetProgramiv       = NULL;
PFNGLATTACHSHADERPROC            glAttachShader       = NULL;
PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog   = NULL;
PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog  = NULL;
PFNGLLINKPROGRAMPROC             glLinkProgram        = NULL;
PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation = NULL;
PFNGLUNIFORM4FPROC               glUniform4f          = NULL;
PFNGLUNIFORM3FPROC               glUniform3f          = NULL;
PFNGLUNIFORM1FPROC               glUniform1f          = NULL;
PFNGLUNIFORM1IPROC               glUniform1i          = NULL;
PFNGLPOINTPARAMETERFPROC         glPointParameterf    = NULL;
PFNGLPOINTPARAMETERFVPROC        glPointParameterfv   = NULL;
#endif

//==============================================================================
// Initialize resources required for rendering
//==============================================================================
void rendering_initialize()
{
#ifdef WIN32
	//Fetch all the required extension functions
	glActiveTexture                        = (PFNGLACTIVETEXTUREPROC)						wglGetProcAddress("glActiveTexture");
	glCreateProgram                        = (PFNGLCREATEPROGRAMPROC)						wglGetProcAddress("glCreateProgram");
	glDeleteProgram                        = (PFNGLDELETEPROGRAMPROC)						wglGetProcAddress("glDeleteProgram");
	glUseProgram                           = (PFNGLUSEPROGRAMPROC)							wglGetProcAddress("glUseProgram");
	glCreateShader                         = (PFNGLCREATESHADERPROC)						wglGetProcAddress("glCreateShader");
	glDeleteShader                         = (PFNGLDELETESHADERPROC)						wglGetProcAddress("glDeleteShader");
	glShaderSource                         = (PFNGLSHADERSOURCEPROC)						wglGetProcAddress("glShaderSource");
	glCompileShader                        = (PFNGLCOMPILESHADERPROC)						wglGetProcAddress("glCompileShader");
	glGetShaderiv                          = (PFNGLGETSHADERIVPROC)							wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog                     = (PFNGLGETSHADERINFOLOGPROC)					wglGetProcAddress("glGetShaderInfoLog");
	glAttachShader                         = (PFNGLATTACHSHADERPROC)						wglGetProcAddress("glAttachShader");
	glLinkProgram                          = (PFNGLLINKPROGRAMPROC)							wglGetProcAddress("glLinkProgram");
	glGetProgramiv                         = (PFNGLGETPROGRAMIVPROC)						wglGetProcAddress("glGetProgramiv");
	glGetProgramInfoLog                    = (PFNGLGETPROGRAMINFOLOGPROC)					wglGetProcAddress("glGetProgramInfoLog");
	glGetUniformLocation                   = (PFNGLGETUNIFORMLOCATIONPROC)					wglGetProcAddress("glGetUniformLocation");
	glUniform4f                            = (PFNGLUNIFORM4FPROC)							wglGetProcAddress("glUniform4f");
	glUniform3f                            = (PFNGLUNIFORM3FPROC)							wglGetProcAddress("glUniform3f");
	glUniform1f                            = (PFNGLUNIFORM1FPROC)							wglGetProcAddress("glUniform1f");
	glUniform1i                            = (PFNGLUNIFORM1IPROC)							wglGetProcAddress("glUniform1i");
																					
	glPointParameterf                      = (PFNGLPOINTPARAMETERFPROC)						wglGetProcAddress("glPointParameterf");
	glPointParameterfv                     = (PFNGLPOINTPARAMETERFVPROC)					wglGetProcAddress("glPointParameterfv");

	glIsRenderbuffer                       = (PFNGLISRENDERBUFFERPROC)						wglGetProcAddress("glIsRenderbufferEXT");
	glBindRenderbuffer                     = (PFNGLBINDRENDERBUFFERPROC)					wglGetProcAddress("glBindRenderbufferEXT");
	glDeleteRenderbuffers                  = (PFNGLDELETERENDERBUFFERSPROC)					wglGetProcAddress("glDeleteRenderbuffersEXT");
	glGenRenderbuffers                     = (PFNGLGENRENDERBUFFERSPROC)					wglGetProcAddress("glGenRenderbuffersEXT");
	glRenderbufferStorage                  = (PFNGLRENDERBUFFERSTORAGEPROC)					wglGetProcAddress("glRenderbufferStorageEXT");
	glGetRenderbufferParameteriv           = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)			wglGetProcAddress("glGetRenderbufferParameterivEXT");
	glIsFramebuffer                        = (PFNGLISFRAMEBUFFERPROC)						wglGetProcAddress("glIsFramebufferEXT");
	glBindFramebuffer                      = (PFNGLBINDFRAMEBUFFERPROC)						wglGetProcAddress("glBindFramebufferEXT");
	glDeleteFramebuffers                   = (PFNGLDELETEFRAMEBUFFERSPROC)					wglGetProcAddress("glDeleteFramebuffersEXT");
	glGenFramebuffers                      = (PFNGLGENFRAMEBUFFERSPROC)						wglGetProcAddress("glGenFramebuffersEXT");
	glCheckFramebufferStatus               = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)				wglGetProcAddress("glCheckFramebufferStatusEXT");
	glFramebufferTexture1D                 = (PFNGLFRAMEBUFFERTEXTURE1DPROC)				wglGetProcAddress("glFramebufferTexture1DEXT");
	glFramebufferTexture2D                 = (PFNGLFRAMEBUFFERTEXTURE2DPROC)				wglGetProcAddress("glFramebufferTexture2DEXT");
	glFramebufferTexture3D                 = (PFNGLFRAMEBUFFERTEXTURE3DPROC)				wglGetProcAddress("glFramebufferTexture3DEXT");
	glFramebufferRenderbuffer              = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)				wglGetProcAddress("glFramebufferRenderbufferEXT");
	glGetFramebufferAttachmentParameteriv  = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)	wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
	glGenerateMipmap                       = (PFNGLGENERATEMIPMAPPROC)						wglGetProcAddress("glGenerateMipmapEXT");
	glBlitFramebuffer                      = (PFNGLBLITFRAMEBUFFERPROC)						wglGetProcAddress("glBlitFramebufferEXT");
	glRenderbufferStorageMultisample       = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)		wglGetProcAddress("glRenderbufferStorageMultisampleEXT");
	//glFramebufferTextureLayer              = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)				wglGetProcAddress("glFramebufferTextureLayerEXT");

	//Check that all extensions are supported
	if (!glActiveTexture || !glCreateProgram || !glDeleteProgram || !glUseProgram ||
	    !glCreateShader || !glDeleteShader || !glShaderSource || !glCompileShader ||
	    !glGetShaderiv || !glGetShaderInfoLog || !glAttachShader || !glLinkProgram ||
	    !glGetProgramiv || !glGetProgramInfoLog || !glGetUniformLocation ||
	    !glUniform4f || !glUniform3f || !glUniform1f || !glUniform1i || !glPointParameterf || !glPointParameterfv ||
	    !glIsRenderbuffer || !glBindRenderbuffer || !glDeleteRenderbuffers || !glGenRenderbuffers ||
	    !glRenderbufferStorage || !glGetRenderbufferParameteriv || !glIsFramebuffer || !glBindFramebuffer ||
	    !glDeleteFramebuffers || !glGenFramebuffers || !glCheckFramebufferStatus || !glFramebufferTexture1D ||
	    !glFramebufferTexture2D || !glFramebufferTexture3D || !glFramebufferRenderbuffer ||
	    !glGetFramebufferAttachmentParameteriv || !glGenerateMipmap || !glBlitFramebuffer ||
	    !glRenderbufferStorageMultisample) {
		log_write("X-Space: Shaders not supported (win32)");
		config.use_shaders = 0;
		return;
	}
#else
	//Nothing for other platforms yet
#endif

	//Initialize rendering resources
	particles_initialize();
	planet_scattering_initialize();
	planet_clouds_initialize();
	engines_draw_initialize();
	dragheat_draw_initialize();
}


//==============================================================================
// Free up resources
//==============================================================================
void rendering_deinitialize()
{
	//Free up resources
	planet_scattering_deinitialize();
	planet_clouds_deinitialize();
	engines_draw_deinitialize();
	dragheat_draw_deinitialize();
	particles_deinitialize();
}


//==============================================================================
// Compile a shader
//==============================================================================
GLhandleARB rendering_shader_compile(GLenum type, char* filename, int quality)
{
	GLhandleARB handle;
	int source_size,is_compiled;
	char* shader_source;
	FILE* f = fopen(filename, "rb");
	if ((!f) || (quality > 16)) {
		log_write("X-Space: Could not load shader: %s\n",filename);
		return 0;
	} else {
		log_write("X-Space: Loading shader: %s\n",filename);
	}

	//Find file size
	fseek(f,0,SEEK_END);
	source_size = ftell(f);
	fseek(f,0,SEEK_SET);

	//Read it all into memory
	shader_source = (char*)malloc(source_size+1);
	fread(shader_source,1,source_size,f);
	shader_source[source_size] = 0;

	//Create shader
	handle = glCreateShader(type);

	//Generate list of defines for quality and compile shader
	{
		char define_list[ARBITRARY_MAX] = { 0 };
		char* define_ptr;
		char* shader_source_strings[2];
		int i;

		strcpy(define_list,"#version 120\n");
		define_ptr = define_list + strlen(define_list);
		for (i = 0; i < quality; i++) {
			sprintf(define_ptr,"#define QUALITY%d\n",quality);
			define_ptr = define_list + strlen(define_list);
		}

		//Compile shader
		shader_source_strings[0] = define_list;
		shader_source_strings[1] = shader_source;
		glShaderSource(handle,2,shader_source_strings,0);
		glCompileShader(handle);
		free(shader_source);
	}

	//Check if any errors
	glGetShaderiv(handle,GL_COMPILE_STATUS,&is_compiled);
	if (!is_compiled) {
		char buf[ARBITRARY_MAX] = { 0 };
		glGetShaderInfoLog(handle, ARBITRARY_MAX-1, 0, buf);
		log_write("X-Space: Error while compiling shader %s: %s\n",filename,buf);

		//Breakpoint
#ifdef _DEBUG
		_asm {int 3}
#endif
		//FIXME: return invalid handle?
	}

	//Return
	fclose(f);
	return handle;
}


//==============================================================================
// Link two shaders together
//==============================================================================
GLhandleARB rendering_shader_link(GLhandleARB vertex_shader, GLhandleARB fragment_shader)
{
	int shaders_linked;
	GLhandleARB program = glCreateProgram();
	glAttachShader(program,vertex_shader);
	glAttachShader(program,fragment_shader);

	//Link the program
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &shaders_linked);
	if (!shaders_linked) {
		log_write("X-Space: Error linking shaders\n");
	}

	return program;
}


//==============================================================================
// Get camera parameters
//==============================================================================
void rendering_get_camera(double camPos[3], double camUp[3])
{
	double viewMatrix[16];
	double temp[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);

	camPos[0] = -viewMatrix[12];
	camPos[1] = -viewMatrix[13];
	camPos[2] = -viewMatrix[14];
	camUp[0]  = viewMatrix[1];
	camUp[1]  = viewMatrix[5];
	camUp[2]  = viewMatrix[9];

	viewMatrix[12] = viewMatrix[13] = viewMatrix[14] = 0;

	memcpy(temp, viewMatrix, sizeof(double) * 16);
	viewMatrix[1] = temp[4]; viewMatrix[4] = temp[1];
	viewMatrix[2] = temp[8]; viewMatrix[8] = temp[2];
	viewMatrix[6] = temp[9]; viewMatrix[9] = temp[6];

	//camPos = view * camPos;
	temp[0] = viewMatrix[0] * camPos[0] + viewMatrix[4] * camPos[1] +  viewMatrix[8] * camPos[2] + viewMatrix[12];
	temp[1] = viewMatrix[1] * camPos[0] + viewMatrix[5] * camPos[1] +  viewMatrix[9] * camPos[2] + viewMatrix[13];
	temp[2] = viewMatrix[2] * camPos[0] + viewMatrix[6] * camPos[1] + viewMatrix[10] * camPos[2] + viewMatrix[14];

	//Write back result
	camPos[0] = temp[0];
	camPos[1] = temp[1];
	camPos[2] = temp[2];
}


//==============================================================================
// Setup billboarding (around axis)
//==============================================================================
void rendering_billboard_z()
{
	double look[3];
	double up[3];
	double right[3];
	double cam_pos[3];
	double m[16];
	double d;

	rendering_get_camera(cam_pos,up);

	look[0] = cam_pos[0] - 0.0; look[1] = cam_pos[1] - 0.0; look[2] = cam_pos[2] - 0.0;

	//X axis is the top one
	look[0] = 0.0;

	//Normalize look
	d = sqrt(look[0]*look[0]+look[1]*look[1]+look[2]*look[2])+1e-9;
	look[0] /= d; look[1] /= d; look[2] /= d;

	//X axis is still the top one
	up[0] = 1.0; up[1] = 0.0; up[2] = 0.0;

	//right = up x look
	right[0] = up[1]*look[2] - up[2]*look[1];
	right[1] = up[2]*look[0] - up[0]*look[2];
	right[2] = up[0]*look[1] - up[1]*look[0];

	//Generate matrix
	m[ 0] = right[0];	m[ 4] = up[0];	m[ 8] = look[0];	m[12] = 0.0;
	m[ 1] = right[1];	m[ 5] = up[1];	m[ 9] = look[1];	m[13] = 0.0;
	m[ 2] = right[2];	m[ 6] = up[2];	m[10] = look[2];	m[14] = 0.0;
	m[ 3] = 0.0;		m[ 7] = 0.0;	m[11] = 0.0;		m[15] = 1.0;

	glPushMatrix();
	glMultMatrixd(m);
}


//==============================================================================
// Draw a textured sphere
//==============================================================================
void rendering_texture_sphere(float r, int segs)
{
	int i, j;
	double x, y, z, z1, z2, R, R1, R2;

	//Top cap
	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0,0,1);
	glTexCoord2f(0.5f,1.0f); //singularity
	glVertex3d(0,0,r);
	z = cos(PI/segs);
	R = sin(PI/segs);
	for (i = 0; i <= 2*segs; i++) {
		x = R*cos(-i*2.0*PI/(2*segs));
		y = R*sin(-i*2.0*PI/(2*segs));
		glNormal3d(x, y, z);
		glTexCoord2f((float)i/(2*segs), 1.0f-1.0f/segs);
		glVertex3d(r*x, r*y, r*z);
	}
	glEnd();

	//Height segments
	for (j = 1; j < segs-1; j++) {
		z1 = cos(j*PI/segs);
		R1 = sin(j*PI/segs);
		z2 = cos((j+1)*PI/segs);
		R2 = sin((j+1)*PI/segs);
		glBegin(GL_TRIANGLE_STRIP);
		for (i = 0; i <= 2*segs; i++) {
			x = R1*cos(-i*2.0*PI/(2*segs));
			y = R1*sin(-i*2.0*PI/(2*segs));
			glNormal3d(x, y, z1);
			glTexCoord2f((float)i/(2*segs), 1.0f-(float)j/segs);
			glVertex3d(r*x, r*y, r*z1);
			x = R2*cos(-i*2.0*PI/(2*segs));
			y = R2*sin(-i*2.0*PI/(2*segs));
			glNormal3d(x, y, z2);
			glTexCoord2f((float)i/(2*segs), 1.0f-(float)(j+1)/segs);
			glVertex3d(r*x, r*y, r*z2);
		}
		glEnd();
	}

	//Bottom cap
	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0,0,-1);
	glTexCoord2f(0.5f, 1.0f); //This is an ugly (u,v)-mapping singularity
	glVertex3d(0,0,-r);
	z = -cos(PI/segs);
	R = sin(PI/segs);
	for (i = 2*segs; i >= 0; i--) {
		x = R*cos(-i*2.0*PI/(2*segs));
		y = R*sin(-i*2.0*PI/(2*segs));
		glNormal3d(x, y, z);
		glTexCoord2f(1.0f-(float)i/(2*segs), 1.0f/segs);
		glVertex3d(r*x, r*y, r*z);
	}
	glEnd();
}


//==============================================================================
// Initialize FBO
//==============================================================================
int rendering_init_fbo(GLuint* fbo, GLuint* fbo_texture, GLuint *fbo_depth, int width, int height)
{
	XPLMSetGraphicsState(0,1,0,0,0,0,0);
	XPLMGenerateTextureNumbers(fbo_texture,1);
	XPLMBindTexture2d(*fbo_texture,0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	XPLMGenerateTextureNumbers(fbo_depth,1);
	XPLMBindTexture2d(*fbo_depth,0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Create a framebuffer object
	glGenFramebuffers(1, fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

	//Attach the texture to FBO color attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *fbo_texture, 0);
	//Attach the renderbuffer to depth attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *fbo_depth, 0);

	//Check FBO status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return 0;
	}

	//Switch back to window-system-provided framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Return success
	return 1;
}