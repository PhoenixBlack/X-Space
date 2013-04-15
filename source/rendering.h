#ifndef RENDERING_H
#define RENDERING_H

#if IBM
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <glext.h>
#elif LIN
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <glext.h>
#else
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#if APL
#define GL_GLEXT_PROTOTYPES
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#if (GL_GLEXT_VERSION < 44)
#undef __glext_h_
#endif
#include <glext.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <gl.h>
#include <glu.h>
#include <glext.h>
#endif
#endif


#ifdef WIN32
PFNGLACTIVETEXTUREPROC                        glActiveTexture;
PFNGLCREATEPROGRAMPROC                        glCreateProgram;
PFNGLDELETEPROGRAMPROC                        glDeleteProgram;
PFNGLUSEPROGRAMPROC                           glUseProgram;
PFNGLCREATESHADERPROC                         glCreateShader;
PFNGLDELETESHADERPROC                         glDeleteShader;
PFNGLSHADERSOURCEPROC                         glShaderSource;
PFNGLCOMPILESHADERPROC                        glCompileShader;
PFNGLGETSHADERIVPROC                          glGetShaderiv;
PFNGLGETPROGRAMIVPROC                         glGetProgramiv;
PFNGLATTACHSHADERPROC                         glAttachShader;
PFNGLGETSHADERINFOLOGPROC                     glGetShaderInfoLog;
PFNGLGETPROGRAMINFOLOGPROC                    glGetProgramInfoLog;
PFNGLLINKPROGRAMPROC                          glLinkProgram;
PFNGLGETUNIFORMLOCATIONPROC                   glGetUniformLocation;
PFNGLUNIFORM4FPROC                            glUniform4f;
PFNGLUNIFORM3FPROC                            glUniform3f;
PFNGLUNIFORM1FPROC                            glUniform1f;
PFNGLUNIFORM1IPROC                            glUniform1i;

PFNGLPOINTPARAMETERFPROC                      glPointParameterf;
PFNGLPOINTPARAMETERFVPROC                     glPointParameterfv;

PFNGLISRENDERBUFFERPROC                       glIsRenderbuffer;
PFNGLBINDRENDERBUFFERPROC                     glBindRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC                  glDeleteRenderbuffers;
PFNGLGENRENDERBUFFERSPROC                     glGenRenderbuffers;
PFNGLRENDERBUFFERSTORAGEPROC                  glRenderbufferStorage;
PFNGLGETRENDERBUFFERPARAMETERIVPROC           glGetRenderbufferParameteriv;
PFNGLISFRAMEBUFFERPROC                        glIsFramebuffer;
PFNGLBINDFRAMEBUFFERPROC                      glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC                   glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC                      glGenFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC               glCheckFramebufferStatus;
PFNGLFRAMEBUFFERTEXTURE1DPROC                 glFramebufferTexture1D;
PFNGLFRAMEBUFFERTEXTURE2DPROC                 glFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTURE3DPROC                 glFramebufferTexture3D;
PFNGLFRAMEBUFFERRENDERBUFFERPROC              glFramebufferRenderbuffer;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC  glGetFramebufferAttachmentParameteriv;
PFNGLGENERATEMIPMAPPROC                       glGenerateMipmap;
PFNGLBLITFRAMEBUFFERPROC                      glBlitFramebuffer;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC       glRenderbufferStorageMultisample;
//PFNGLFRAMEBUFFERTEXTURELAYERPROC              glFramebufferTextureLayer;
#endif

void rendering_initialize();
void rendering_deinitialize();
GLhandleARB rendering_shader_compile(GLenum type, char* filename, int quality);
GLhandleARB rendering_shader_link(GLhandleARB vertex_shader, GLhandleARB fragment_shader);
int rendering_init_fbo(GLuint* fbo, GLuint* fbo_texture, GLuint *fbo_depth, int width, int height);

void rendering_get_camera(double camPos[3], double camUp[3]);
void rendering_billboard_z();
void rendering_texture_sphere(float r, int segs);

#endif
