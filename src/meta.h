//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef META_H_INCLUDED
#define META_H_INCLUDED

#include "conqdef.h"


#define META_VERMAJ 0
#define META_VERMIN 2
#define META_VERSION  (uint16_t)((META_VERMAJ << 8) | META_VERMIN)

#define META_MAXSERVERS   1000  /* max number of servers we will track */
#define BUFFERSZ          (1024 * 64)

#define META_GEN_STRSIZE  256   /* generic meta str size */

/* internal representation of a server record for the meta server */
typedef struct _meta_srec {
    int     valid;
    uint16_t version;
    uint8_t  numactive;
    uint8_t  numvacant;
    uint8_t  numrobot;
    uint8_t  numtotal;
    time_t  lasttime;             /* last contact time */
    uint32_t flags;                /* same as spServerStat_t */
    uint16_t port;
    char    addr[CONF_SERVER_NAME_SZ]; /* server's detected address */
    char    altaddr[CONF_SERVER_NAME_SZ]; /* specified real address */
    char    servername[CONF_SERVER_NAME_SZ];
    char    serverver[CONF_SERVER_NAME_SZ]; /* server's proto version */
    char    motd[CONF_SERVER_MOTD_SZ];

    /* Version 0x0002 */
    uint16_t protovers;
    char    contact[META_GEN_STRSIZE];
    char    walltime[META_GEN_STRSIZE];

} metaSRec_t;

int  metaBuffer2ServerRec(metaSRec_t *srec, char *buf);
void metaServerRec2Buffer(char *buf, metaSRec_t *srec);
int  metaUpdateServer(const char *remotehost, const char *name, int port);
int  metaGetServerList(const char *remotehost, metaSRec_t **srvlist);

#endif /* META_H_INCLUDED */