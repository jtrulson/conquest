/* 
 *  gl font handling
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _GLFONT_H
#define _GLFONT_H

#include "TexFont.h"

/* font DL's  */
#ifdef NOEXTERN_GLFONT
GLuint vFontDL = -1;
GLuint vFontW = -1;

GLuint mFontTinyDL = -1;
GLuint mFontTinyW = -1;

/* Texture mapped fonts */
TexFont *fontLargeTxf = NULL;
TexFont *fontFixedTxf = NULL;
TexFont *fontMsgTxf = NULL;
#else

extern GLuint vFontDL;
extern GLuint vFontW;

extern GLuint mFontTinyDL;
extern GLuint mFontTinyW;


extern TexFont *fontLargeTxf;
extern TexFont *fontFixedTxf;
extern TexFont *fontMsgTxf;
#endif /* NOEXTERN_GLFONT */

void glfRender(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
                       TexFont *font, char *str, int color, int scalex,
                       int dofancy);
void drawString(GLfloat x, GLfloat y, GLfloat z, char *str,
                       GLuint DL, int color);
void initFonts(void);
void initTexFonts(void);


#endif /* _GLFONT_H */