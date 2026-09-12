#pragma once

#include <SDL2/SDL_opengl.h>

struct GLFuncTable
{
    void ResolveFunctions(bool glesContext);

    void glClearDepthf(GLclampf depth);
    void glDepthRangef(GLclampf near_val, GLclampf far_val);

    void(GLAPIENTRY *glAlphaFunc)(GLenum func, GLclampf ref);
    void(GLAPIENTRY *glBindTexture)(GLenum target, GLuint texture);
    void(GLAPIENTRY *glBlendFunc)(GLenum sfactor, GLenum dfactor);
    void(GLAPIENTRY *glClear)(GLbitfield mask);
    void(GLAPIENTRY *glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void(GLAPIENTRY *glColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
    void(GLAPIENTRY *glDeleteTextures)(GLsizei n, const GLuint *textures);
    void(GLAPIENTRY *glDepthFunc)(GLenum func);
    void(GLAPIENTRY *glDepthMask)(GLboolean flag);
    void(GLAPIENTRY *glDisableClientState)(GLenum cap);
    void(GLAPIENTRY *glDrawArrays)(GLenum mode, GLint first, GLsizei count);
    void(GLAPIENTRY *glEnable)(GLenum cap);
    void(GLAPIENTRY *glEnableClientState)(GLenum cap);
    void(GLAPIENTRY *glFogf)(GLenum pname, GLfloat param);
    void(GLAPIENTRY *glFogfv)(GLenum pname, const GLfloat *params);
    void(GLAPIENTRY *glGenTextures)(GLsizei n, GLuint *textures);
    GLenum(GLAPIENTRY *glGetError)(void);
    void(GLAPIENTRY *glGetFloatv)(GLenum pname, GLfloat *params);
    void(GLAPIENTRY *glGetIntegerv)(GLenum pname, GLint *params);
    void(GLAPIENTRY *glLoadIdentity)(void);
    void(GLAPIENTRY *glLoadMatrixf)(const GLfloat *m);
    void(GLAPIENTRY *glMatrixMode)(GLenum mode);
    void(GLAPIENTRY *glMultMatrixf)(const GLfloat *m);
    void(GLAPIENTRY *glPopMatrix)(void);
    void(GLAPIENTRY *glPushMatrix)(void);
    void(GLAPIENTRY *glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                                   GLvoid *pixels);
    void(GLAPIENTRY *glShadeModel)(GLenum mode);
    void(GLAPIENTRY *glTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
    void(GLAPIENTRY *glTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
    void(GLAPIENTRY *glTexEnvi)(GLenum target, GLenum pname, GLint param);
    void(GLAPIENTRY *glTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                                   GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void(GLAPIENTRY *glTexParameteri)(GLenum target, GLenum pname, GLint param);
    void(GLAPIENTRY *glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                      GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void(GLAPIENTRY *glVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
    void(GLAPIENTRY *glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLUNIFORM1FPROC glUniform1f;
    PFNGLUNIFORM1IPROC glUniform1i;
    PFNGLUNIFORM4FPROC glUniform4f;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

  private:

    void(GLAPIENTRY *glClearDepthf_ptr)(GLclampf depth);
    void(GLAPIENTRY *glDepthRangef_ptr)(GLclampf near_val, GLclampf far_val);

    void(GLAPIENTRY *glClearDepth)(GLclampd depth);
    void(GLAPIENTRY *glDepthRange)(GLclampd near_val, GLclampd far_val);

    bool isGlesContext;
};

extern GLFuncTable g_glFuncTable;
