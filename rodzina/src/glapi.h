/* Minimalny, własny loader OpenGL 3.3 core (bez zewnętrznych nagłówków).
 * Funkcje ładowane w gl_load(): Windows — wglGetProcAddress/opengl32.dll,
 * Linux (testy) — OSMesaGetProcAddress. */
#ifndef GLAPI_H
#define GLAPI_H
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define GLAPIENTRY __stdcall
#else
#define GLAPIENTRY
#endif

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned char GLboolean;
typedef char GLchar;
typedef unsigned int GLbitfield;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef unsigned char GLubyte;
typedef void GLvoid;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_TRIANGLES 0x0004
#define GL_LINES 0x0001
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_LEQUAL 0x0203
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 1
#define GL_ZERO 0
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_HALF_FLOAT 0x140B
#define GL_RED 0x1903
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_STREAM_DRAW 0x88E0
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_DEPTH_COMPONENT 0x1902
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_RGBA16F 0x881A
#define GL_RGB16F 0x881B
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_RGBA8 0x8058
#define GL_R8 0x8229
#define GL_MAX_SAMPLES 0x8D57
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_MULTISAMPLE 0x809D
#define GL_POLYGON_OFFSET_FILL 0x8037

#define GLFN(ret, name, args) typedef ret(GLAPIENTRY *PFN_##name) args; extern PFN_##name name;
#define GL_FUNCS(X) \
  X(void, glClear, (GLbitfield)) \
  X(void, glClearColor, (GLfloat, GLfloat, GLfloat, GLfloat)) \
  X(void, glViewport, (GLint, GLint, GLsizei, GLsizei)) \
  X(void, glScissor, (GLint, GLint, GLsizei, GLsizei)) \
  X(void, glEnable, (GLenum)) \
  X(void, glDisable, (GLenum)) \
  X(void, glBlendFunc, (GLenum, GLenum)) \
  X(void, glDepthFunc, (GLenum)) \
  X(void, glDepthMask, (GLboolean)) \
  X(void, glColorMask, (GLboolean, GLboolean, GLboolean, GLboolean)) \
  X(void, glCullFace, (GLenum)) \
  X(void, glPolygonOffset, (GLfloat, GLfloat)) \
  X(void, glGenTextures, (GLsizei, GLuint *)) \
  X(void, glDeleteTextures, (GLsizei, const GLuint *)) \
  X(void, glBindTexture, (GLenum, GLuint)) \
  X(void, glTexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *)) \
  X(void, glTexSubImage2D, (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *)) \
  X(void, glTexImage3D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *)) \
  X(void, glTexSubImage3D, (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void *)) \
  X(void, glTexParameteri, (GLenum, GLenum, GLint)) \
  X(void, glTexParameterf, (GLenum, GLenum, GLfloat)) \
  X(void, glGenerateMipmap, (GLenum)) \
  X(void, glActiveTexture, (GLenum)) \
  X(void, glPixelStorei, (GLenum, GLint)) \
  X(void, glReadPixels, (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *)) \
  X(const GLubyte *, glGetString, (GLenum)) \
  X(void, glGetIntegerv, (GLenum, GLint *)) \
  X(void, glGetFloatv, (GLenum, GLfloat *)) \
  X(GLenum, glGetError, (void)) \
  X(void, glDrawArrays, (GLenum, GLint, GLsizei)) \
  X(void, glDrawElements, (GLenum, GLsizei, GLenum, const void *)) \
  X(GLuint, glCreateShader, (GLenum)) \
  X(void, glDeleteShader, (GLuint)) \
  X(void, glShaderSource, (GLuint, GLsizei, const GLchar *const *, const GLint *)) \
  X(void, glCompileShader, (GLuint)) \
  X(void, glGetShaderiv, (GLuint, GLenum, GLint *)) \
  X(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
  X(GLuint, glCreateProgram, (void)) \
  X(void, glAttachShader, (GLuint, GLuint)) \
  X(void, glBindAttribLocation, (GLuint, GLuint, const GLchar *)) \
  X(void, glLinkProgram, (GLuint)) \
  X(void, glGetProgramiv, (GLuint, GLenum, GLint *)) \
  X(void, glGetProgramInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) \
  X(void, glUseProgram, (GLuint)) \
  X(GLint, glGetUniformLocation, (GLuint, const GLchar *)) \
  X(void, glUniform1i, (GLint, GLint)) \
  X(void, glUniform1f, (GLint, GLfloat)) \
  X(void, glUniform2f, (GLint, GLfloat, GLfloat)) \
  X(void, glUniform3f, (GLint, GLfloat, GLfloat, GLfloat)) \
  X(void, glUniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
  X(void, glUniform3fv, (GLint, GLsizei, const GLfloat *)) \
  X(void, glUniform4fv, (GLint, GLsizei, const GLfloat *)) \
  X(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat *)) \
  X(void, glGenVertexArrays, (GLsizei, GLuint *)) \
  X(void, glDeleteVertexArrays, (GLsizei, const GLuint *)) \
  X(void, glBindVertexArray, (GLuint)) \
  X(void, glGenBuffers, (GLsizei, GLuint *)) \
  X(void, glDeleteBuffers, (GLsizei, const GLuint *)) \
  X(void, glBindBuffer, (GLenum, GLuint)) \
  X(void, glBufferData, (GLenum, GLsizeiptr, const void *, GLenum)) \
  X(void, glBufferSubData, (GLenum, GLintptr, GLsizeiptr, const void *)) \
  X(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void *)) \
  X(void, glEnableVertexAttribArray, (GLuint)) \
  X(void, glVertexAttribDivisor, (GLuint, GLuint)) \
  X(void, glDrawArraysInstanced, (GLenum, GLint, GLsizei, GLsizei)) \
  X(void, glGenFramebuffers, (GLsizei, GLuint *)) \
  X(void, glDeleteFramebuffers, (GLsizei, const GLuint *)) \
  X(void, glBindFramebuffer, (GLenum, GLuint)) \
  X(void, glFramebufferTexture2D, (GLenum, GLenum, GLenum, GLuint, GLint)) \
  X(void, glGenRenderbuffers, (GLsizei, GLuint *)) \
  X(void, glDeleteRenderbuffers, (GLsizei, const GLuint *)) \
  X(void, glBindRenderbuffer, (GLenum, GLuint)) \
  X(void, glRenderbufferStorage, (GLenum, GLenum, GLsizei, GLsizei)) \
  X(void, glRenderbufferStorageMultisample, (GLenum, GLsizei, GLenum, GLsizei, GLsizei)) \
  X(void, glFramebufferRenderbuffer, (GLenum, GLenum, GLenum, GLuint)) \
  X(void, glBlitFramebuffer, (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)) \
  X(GLenum, glCheckFramebufferStatus, (GLenum)) \
  X(void, glFinish, (void))

GL_FUNCS(GLFN)
#undef GLFN

/* zwraca 0 przy powodzeniu, inaczej nazwę brakującej funkcji przez *missing */
int gl_load(void *(*getproc)(const char *), const char **missing);

#endif
