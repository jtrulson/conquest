/************************************************************************
 * 
 * defs.h - modify Conquest's behavior - hopefully for the better ;-)
 *
 * Jon Trulson
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

/* Basic information */

#define CONQUEST_USER "root"
#define CONQUEST_GROUP "conquest"

#ifndef CONQHOME
# define CONQHOME "/opt/conquest"
#endif

/* HAS_SETITIMER - Define if your system supports setitimer/getitimer().
 *  Otherwise alarm() is used.  With setitimer() it's possible to get a
 *  faster refresh rate (say 2 updates per second) rather than 1 update
 *  per second as is the case with alarm().
 */
#if defined(HAVE_SETITIMER)
# define HAS_SETITIMER
#endif

/* USE_COMMONMLOCK - Lock the common block into memory via memctl(). 
 *  Requires the plock privilege (unixware).
 */

#if defined(UNIXWARE)
# define USE_COMMONMLOCK 
#endif

/* USE_PVLOCK - Use a locking mechinism to prevent simultanious writes to
 *  the common block.  The method I use only reduces the chance of a
 *  a collision.  A real sync method would use semaphores, (and eventually
 *  conquest will) which will prevent them altogether.
 *  
 * USE_SEMS
 *  Well.. now conquest supports semaphores! Yaay!  #define USE_SEMS to
 *  use them.   If USE_SEMS is undefined, the old locking mechanism will
 *  be used, though you *may* experience random SEGV's...
 */

#define USE_PVLOCK		/* use a locking mechanism for the cmn blk */
				/* RECOMMENDED!!! */
#ifdef USE_PVLOCK
# define USE_SEMS		/* use semephores in PV[UN]LOCK */
#endif

/* WARP0CLOAK - Although the code made it difficult to scan a ship that was
 *  cloaked at warp 0, it was still possible to scan such a ship if it was
 *  within alert range.  Defining this means you CANNOT be detected (even by 
 *  robots) if you are cloaked and at warp 0.0.  An enemy can fly right over
 *  you and won't even know your there... heeheehee
 */
#define WARP0CLOAK 

/* SET_PRIORITY - increase our priority a bit, increase the driver's a bit 
 * more.  Requires the tshar privilege (unixware).
 */

#if defined(UNIXWARE)
# define SET_PRIORITY
#endif
				/* Priorities, if we're using them */
#ifdef SET_PRIORITY
# define CONQUEST_PRI (-4)
# define CONQDRIV_PRI (-7)
#endif

/* ENABLE_MACROS - use the input buffer, and therefore macro F keys 
 */
#define ENABLE_MACROS

#ifdef ENABLE_MACROS
# define MAX_MACROS 24		/* max number of macros supported */
# define MAX_MACRO_LEN 81	/* max length of a macro */
#endif

/* OPER_MSG_BEEP - if defined, beep when a message arrives in conqoper.
 */
/* #define OPER_MSG_BEEP */


/* DEBUG* - debug some things */

#define DEBUG_FLOW		/* trace init flow */
/*#define DEBUGGING		/* general debugging */
/*#define DEBUG_LOCKING		/* debugging common block locking */
/*#define DEBUG_CONFIG		/* debug configuration file processing */
/*#define DEBUG_SEM		/* semaphore debugging */
/*#define DEBUG_MACROS		/* MACRO debugging */
/*#define DEBUG_IO		/* IO debugging */
/*#define DEBUG_SIG		/* debug signal handling */
/*#define DEBUG_AI		/* debug the robots */
/*#define DEBUG_MISC		/* debug other misc stuff */
/*#define DEBUG_RANDOM		/* debug the random number library */
/*#define DEBUG_COLOR		/* debug color processing */

/*#define DEBUG_IOGTIMED   /* debug timed input */
