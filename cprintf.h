/*
 * cprintf defines.  SHOULD BE UI AGNOSTIC!
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _CPRINTF_H
#define _CPRINTF_H

/* cprintf align values */
#define ALIGN_CENTER 3
#define ALIGN_LEFT   2
#define ALIGN_RIGHT  1
#define ALIGN_NONE   0

void cprintf(int lin, int col, int align, char *fmt, ...);

#endif /* _CPRINTF_H */
