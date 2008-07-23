/* 
 * packet routines
 * 
 * Some ideas borrowed from netrek.
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED

#include "datatypes.h"
#include "protocol.h"

#define PKT_MAXSIZE     1024	/* no packet should ever be this large. gulp.*/

void pktNotImpl(void *);	/* a no-show */

struct _packetent {
  Unsgn32  pktid;
  Unsgn32  size;
  char    *name;
  void     (*handler)();
};


/* directions from client/server */
#define PKT_TOCLIENT 0
#define PKT_TOSERVER 1
#define PKT_FROMCLIENT 2
#define PKT_FROMSERVER 3

/* error/severity codes for Acks, should make sure these sync to
   pktSeverity2String(int psev) */
#define PSEV_INFO     0	
#define PSEV_WARN     1
#define PSEV_ERROR    2
#define PSEV_FATAL    3

/* error codes */
#define PERR_OK           0	/* no error */
#define PERR_UNSPEC       1	/* unspecified */
#define PERR_BADPROTO     2	/* bad protocol */
#define PERR_BADCMN       3	/* common block mismatch */
#define PERR_INVUSER      4 	/* invalid username */
#define PERR_NOUSER       5 	/* no such user */
#define PERR_INVPWD       6 	/* invalid password */
#define PERR_BADPWD       7 	/* wrong password */
#define PERR_CLOSED       8 	/* game closed */
#define PERR_REGISTER     9 	/* register failed */
#define PERR_NOSHIP       10 	/* no slots available */
#define PERR_LOSE         11 	/* lose in menu() */
#define PERR_FLYING       12 	/* already flying a ship (newship()) */
#define PERR_TOOMANYSHIPS 13	/* you are flying too many ships (newship()) */
#define PERR_CANCELED     14	/* an operation (bombing, etc) was cancelled
				   for some reason. */
#define PERR_DONE         15    /* finished something - like beaming */
#define PERR_DOUDP        16	/* used in hello to tell server udp is ok */
#define PERR_PINGRESP     17	/* a ping reponse for nCP */

#ifdef NOEXTERN_PACKET
int            pktRXBytes = 0;
Unsgn32        pktPingAvgMS = 0;
#else
extern int     pktRXBytes;
extern Unsgn32 pktPingAvgMS;
#endif

int   pktSendAck(int sock, int dir, Unsgn8 severity, Unsgn8 code, char *msg);
int   pktIsConnDead(void);
void  pktNotImpl(void *nothing);
void  pktSetNodelay(int sock);
char *pktSeverity2String(int psev);
int   pktInvertDirection(int dir);

int   pktWaitForPacket(int dir, int sockl[], int type, char *buf, int blen, 
                    int delay, char *nakmsg);

int   pktClientPacketSize(int type);
int   pktServerPacketSize(int type);

int   pktIsPacketWaiting(int sock);
int   pktWrite(int direction, int sock, void *data);
int   pktRead(int direction, int sockl[], char *buf, int len, 
                 unsigned int delay);
int   pktIsValid(int pkttype, void *pkt);

#endif /* PACKET_H_INCLUDED */
