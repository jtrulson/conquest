#include "c_defs.h"

/************************************************************************
 *
 * packet - packet handling for conquest
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "datatypes.h"
#include "conf.h"
#include "protocol.h"
#define NOPKT_EXTERN
#include "packet.h"
#undef NOPKT_EXTERN
#ifndef HAVE_SELECT
#error "The select() system call is required"
#endif

static struct _packetent clientPackets[] = {
  { CP_NULL, 
    sizeof(cpNull_t), 
    "CP_NULL", 
    pktNotImpl 
  },	/* never used */
  { CP_HELLO, 
    sizeof(cpHello_t), 
    "CP_HELLO", 
    pktNotImpl 
  },
  { CP_ACK, 
    sizeof(cpAck_t), 
    "CP_ACK", 
    pktNotImpl 
  },
  { CP_COMMAND, 
    sizeof(cpCommand_t), 
    "CP_COMMAND", 
    pktNotImpl 
  },
  { CP_FIRETORPS, 
    sizeof(cpFireTorps_t), 
    "CP_FIRETORPS", 
    pktNotImpl 
  },
  { CP_GETSINFO, 
    sizeof(cpGetSInfo_t), 
    "CP_GETSINFO", 
    pktNotImpl 
  },
  { CP_SENDMSG, 
    sizeof(cpSendMsg_t), 
    "CP_SENDMSG", 
    pktNotImpl 
  },
  { CP_SETNAME, 
    sizeof(cpSetName_t), 
    "CP_SETNAME", 
    pktNotImpl 
  },
  { CP_AUTHENTICATE, 
    sizeof(cpAuthenticate_t), 
    "CP_AUTHENTICATE", 
    pktNotImpl 
  },
  { CP_SETCOURSE,
    sizeof(cpSetCourse_t),
    "CP_SETCOURSE",
    pktNotImpl 
  },
  { CP_MESSAGE,
    sizeof(cpMessage_t),
    "CP_MESSAGE",
    pktNotImpl 
  },
  { CP_VARIABLE,
    0,                          /* these are special */
    "CP_VARIABLE",
    pktNotImpl 
  }
};

static struct _packetent serverPackets[] = {
  { SP_NULL, 
    sizeof(spNull_t), 
    "SP_NULL", 
    pktNotImpl 
  },	/* never used */
  { SP_HELLO, 
    sizeof(spHello_t), 
    "SP_HELLO", 
    pktNotImpl 
  },
  { SP_ACK, 
    sizeof(spAck_t), 
    "SP_ACK", 
    pktNotImpl 
  },
  { SP_SERVERSTAT, 
    sizeof(spServerStat_t), 
    "SP_SERVERSTAT", 
    pktNotImpl 
  },
  { SP_CLIENTSTAT, 
    sizeof(spClientStat_t), 
    "SP_CLIENTSTAT", 
    pktNotImpl 
  },
  { SP_SHIP, 
    sizeof(spShip_t), 
    "SP_SHIP", 
    pktNotImpl 
  },
  { SP_SHIPSML, 
    sizeof(spShipSml_t), 
    "SP_SHIPSML", 
    pktNotImpl 
  },
  { SP_SHIPLOC, 
    sizeof(spShipLoc_t), 
    "SP_SHIPLOC", 
    pktNotImpl 
  },
  { SP_PLANET, 
    sizeof(spPlanet_t), 
    "SP_PLANET", 
    pktNotImpl },
  { SP_PLANETSML, 
    sizeof(spPlanetSml_t), 
    "SP_PLANETSML", 
    pktNotImpl },
  { SP_PLANETLOC, 
    sizeof(spPlanetLoc_t), 
    "SP_PLANETLOC", 
    pktNotImpl 
  },
  { SP_MESSAGE, 
    sizeof(spMessage_t), 
    "SP_MESSAGE", 
    pktNotImpl 
  },
  { SP_USER, 
    sizeof(spUser_t), 
    "SP_USER", 
    pktNotImpl 
  },
  { SP_TORP, 
    sizeof(spTorp_t), 
    "SP_TORP", 
    pktNotImpl 
  },
  { SP_ACKMSG, 
    sizeof(spAckMsg_t), 
    "SP_ACKMSG", 
    pktNotImpl 
  },
  { SP_TEAM, 
    sizeof(spTeam_t), 
    "SP_TEAM", 
    pktNotImpl 
  },
  { SP_TORPLOC, 
    sizeof(spTorpLoc_t), 
    "SP_TORPLOC", 
    pktNotImpl 
  },
  { SP_CONQINFO, 
    sizeof(spConqInfo_t), 
    "SP_CONQINFO", 
    pktNotImpl 
  },
  { SP_FRAME, 
    sizeof(spFrame_t), 
    "SP_FRAME", 
    pktNotImpl 
  },
  { SP_HISTORY, 
    sizeof(spHistory_t), 
    "SP_HISTORY", 
    pktNotImpl 
  },
  { SP_DOOMSDAY, 
    sizeof(spDoomsday_t), 
    "SP_DOOMSDAY", 
    pktNotImpl 
  },
  { SP_PLANETINFO, 
    sizeof(spPlanetInfo_t), 
    "SP_PLANETINFO", 
    pktNotImpl 
  },
  { SP_PLANETLOC2, 
    sizeof(spPlanetLoc2_t), 
    "SP_PLANETLOC2", 
    pktNotImpl 
  },
  { SP_TORPEVENT, 
    sizeof(spTorpEvent_t), 
    "SP_TORPEVENT", 
    pktNotImpl 
  },
  { SP_VARIABLE, 
    0,                          /* these are special */
    "SP_VARIABLE", 
    pktNotImpl 
  }
};

#define CLIENTPKTMAX (sizeof(clientPackets) / sizeof(struct _packetent))
#define SERVERPKTMAX (sizeof(serverPackets) / sizeof(struct _packetent))


#define SOCK_TMOUT 15		/* 15 seconds to complete packet */

static int connDead = 0;	/* if we die for some reason */

int isConnDead(void)
{
  return connDead;
}

void pktNotImpl(void *nothing)
{
  clog("packet: NULL/Not Implemented\n");
  return;
}

/* sends acks. to/from client/server. The server (TOCLIENT) can add a
   string message as well. */

int sendAck(int sock, int dir, Unsgn8 severity, Unsgn8 code, Unsgn8 *msg)
{
  cpAck_t cack;
  spAck_t sack;
  spAckMsg_t sackmsg;
  Unsgn8 *buf;

  switch (dir)
    {
    case PKT_TOSERVER:
      cack.type = CP_ACK;
      cack.severity = severity;
      cack.code = code;

      buf = (Unsgn8 *)&cack;

      break;
    case PKT_TOCLIENT:
      if (msg)
	{
	  sackmsg.type = SP_ACKMSG;
	  memset(sackmsg.txt, 0, MESSAGE_SIZE);
	  strncpy(sackmsg.txt, msg, MESSAGE_SIZE - 1);
	  sackmsg.severity = severity;
	  sackmsg.code = code;

	  buf = (Unsgn8 *)&sackmsg;
	}
      else
	{
	  sack.type = SP_ACK;
	  sack.severity = severity;
	  sack.code = code;
	  sack.severity = severity;
	  sack.code = code;
	  buf = (Unsgn8 *)&sack;
	}
      break;

    default:
#if defined(DEBUG_PKT)
      clog("sendAck: invalid dir = %d\n", dir);
#endif
      return -1;
      break;
    }

  return(writePacket(dir, sock, buf));
}

char *psev2String(int psev)
{
  switch (psev)
    {
    case PSEV_INFO:
      return "INFO";
      break;

    case PSEV_WARN:
      return "WARN";
      break;

    case PSEV_ERROR:
      return "ERROR";
      break;

    case PSEV_FATAL:
      return "FATAL";
      break;

    default:
      return "";
      break;
    }

  return ("");			/* NOTREACHED */
}

/* this just inverts the meaning of a packet direction - TOCLIENT becomes
   FROMCLIENT, etc... */
int invertDir(int dir)
{
  switch (dir)
    {
    case PKT_TOCLIENT:
      return PKT_FROMCLIENT;
      break;
   case PKT_TOSERVER:
      return PKT_FROMSERVER;
      break;
   case PKT_FROMCLIENT:
      return PKT_TOCLIENT;
      break;
   case PKT_FROMSERVER:
      return PKT_TOSERVER;
      break;
   default:
      return -1;
      break;
    }
  return -1;			/* NOTREACHED */
}

int waitForPacket(int dir, int sockl[], int type, Unsgn8 *buf, int blen, 
		  int delay, char *nakmsg)
{
  int pkttype;

  while (TRUE)
    {
      errno = 0;		/* be afraid. */
      if ((pkttype = readPacket(dir, sockl, buf, blen, delay)) >= 0)
	{
	  if (pkttype == type || type == PKT_ANYPKT || pkttype == 0)
	    return pkttype;

	  if (pkttype != type && nakmsg) /* we need to use a msg nak */
	    sendAck(sockl[0], invertDir(dir), PSEV_ERROR, PERR_UNSPEC, nakmsg); 
	}

      if (pkttype < 0)
	{
	  if (errno != EINTR)
            {
#if defined(DEBUG_PKT)
              clog("waitForPacket(dir=%d): read error %s\n", dir, strerror(errno));
#endif
              return -1;
            }
	}
    }

  return -1;			/* NOTREACHED */

}



int serverPktSize(int type)
{
  if (type <= 0 || type >= SERVERPKTMAX)
    {
      clog("serverPktSize: invalid packet type %d\n",
	   type);

      /* abort();*/

      return 0;
    }

  return serverPackets[type].size;
}
      
int clientPktSize(int type)
{

  if (type <= 0 || type >= CLIENTPKTMAX)
    {
      clog("clientPktSize: invalid packet type %d\n",
	   type);
      return 0;
    }

  return clientPackets[type].size;
}

/* like iochav(), but for the network connection.  return true if there
   is packet data ready */
int isPacketWaiting(int sock)
{
  struct timeval timeout;
  fd_set readfds;
  
  timeout.tv_sec = 0;		/* no wait */
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);

  if (select(sock+1, &readfds, NULL, NULL, &timeout) > 0)
    return TRUE;
  else
    return FALSE;
}


/* sockl[0] is expected tcp socket, sockl[1] is for udp */
int readPacket(int direction, int sockl[], Unsgn8 *buf, int blen, 
	       unsigned int delay)
{
  Unsgn8 type;
  int len, rlen, left, rv;
  struct timeval timeout;
  fd_set readfds;
  int maxfd;
  int gotudp = FALSE;           
  int vartype;                  /* variable type (SP/CP) */

  if (connDead || direction == -1) 
    return -1;

  timeout.tv_sec = delay;		/* timeout for intial read */
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sockl[0], &readfds);
  maxfd = sockl[0];
  if (sockl[1] >= 0)
    {
      FD_SET(sockl[1], &readfds);
      maxfd = max(sockl[0], sockl[1]);
    }

  if ((rv=select(maxfd+1, &readfds, NULL, NULL, &timeout)) > 0)
    {				/* we have a byte */
      if (FD_ISSET(sockl[0], &readfds))
	{
	  if ((rv = read(sockl[0], &type, 1)) <= 0)
	    {
	      *buf = 0;
	      return -1;
	    }
	}
      else if (sockl[1] >= 0 && FD_ISSET(sockl[1], &readfds))
        {                     /* got a udp, read the whole thing */
          if ((rv = read(sockl[1], buf, blen)) <= 0)
            {
              *buf = 0;
              return -1;
            }
          else
            {
              gotudp = TRUE;
              type = buf[0];
            }
        }
      else
        {
          clog("readPacket: select returned >0, but !FD_ISSET");
          return 0;
        }
    }
  else if (rv == 0)
    {				/* timed out */
      return 0;
    }
  else if (rv < 0)		/* error */
    {
      return -1;
    }

  switch(direction)
    {
    case PKT_FROMSERVER:
      len = serverPktSize(type);
      vartype = SP_VARIABLE;    /* possible variable */
      break;
    case PKT_FROMCLIENT:
      len = clientPktSize(type);
      vartype = CP_VARIABLE;      /* possible variable */
      break;
    default:
#if defined(DEBUG_PKT)
      clog("readPacket: Invalid dir code %s\n", direction);
#endif
      return -1;
      break;
    }

  pktRXBytes += len;

  if (gotudp)
    {                           /* then we already got the whole packet */
      if (rv != len)
        {
          clog("gotudp: rv != len: %d %d", rv, len);
          *buf = 0;
          type = 0;

          return 0;
        }

      if (type == vartype)
        {                       /* if encap packet, do the right thing */
          memcpy(buf, buf + sizeof(struct _generic_var), 
                 rv - sizeof(struct _generic_var));
          type = buf[0];
        }
 
      return type;
    }
        

  if (len)
    {
      if (len >= blen)		/* buf too small */
	{
	  clog("readPacket: buffer too small\n");
	  return -1;
	}
      len = len - sizeof(Unsgn8);
      left = len;

      while (left > 0)
	{
	  timeout.tv_sec = SOCK_TMOUT; /* longest we will wait */
	  timeout.tv_usec = 0;
      
	  FD_ZERO(&readfds);
	  FD_SET(sockl[0], &readfds);
	  
	  if ((rv=select(sockl[0]+1, &readfds, NULL, NULL, &timeout)) > 0)
	    {			/* some data avail */
	      if ((rlen = read(sockl[0], ((buf + 1) + (len - left)), left)) > 0)
		{
		  /* do we have enough? */
		  if ((left - rlen) > 0 /*len != rlen*/)
		    {
#if defined(DEBUG_PKT)
		      clog("readPacket: short packet: type(%d) len = %d, "
                           "rlen = %d left = %d\n",
			   type, len, rlen, left - rlen);
#endif
		      left -= rlen;
		      continue;	/* get rest of packet */
		    }

                                /* we're done... maybe */
                  if (type == vartype)
                    {         /* if encap packet, do the right thing */
                      pktVariable_t *vpkt = (pktVariable_t *)buf;

                      /* read the first byte (type) of new pkt */
                      if ((rv = read(sockl[0], &type, 1)) <= 0)
                        {
                          *buf = 0;
                          return -1;
                        }

                      /* now reset the bytes left so the encapsulated pkt
                         can be read */

                      len = left = (vpkt->len - 1);
                      continue;
                    }

                  /* really done */
		  buf[0] = type;
		  return type;
		}
	      else
		{
		  if (rlen == 0)
		    {
#if defined(DEBUG_PKT)
		      clog("readPacket: read returned 0");
#endif
		      return -1;
		    }

		  if (errno == EINTR)
		    continue;
		  else
		    {
		      clog("readPacket: read returned %d: %s", rlen,
			   strerror(errno));
		      return -1;
		    }
		}
	    }

	  if (rv == 0)		/* timeout */
	    {
	      clog("readPacket: timed out - connDead\n");
	      connDead = 1;
	      return -1;
	    }
	  else if (rv < 0)
	    {
	      if (errno == EINTR)
		continue;
	      clog("readPacket: select error: %s\n", strerror(errno));
	      return -1;
	    }
	}

    }
  else
    clog("readPacket: invalid packet type read %d\n",
	 type);

  return -1;
}


int writePacket(int direction, int sock, Unsgn8 *packet)
{
  int len, wlen, left;
  Unsgn8 type = *packet;	/* first byte is ALWAYS pkt type */

  if (connDead) 
    return -1;

  switch(direction)
    {
    case PKT_TOSERVER:
      len = clientPktSize(type);
      break;
    case PKT_TOCLIENT:
      len = serverPktSize(type);
      break;
    default:
#if defined(DEBUG_PKT)
      clog("writePacket: Invalid dir code %s\n", direction);
#endif
      return -1;
      break;
    }

  if (len)
    {
      left = len;

      while (left > 0)
	{
	  if ((wlen = write(sock, packet, left)) > 0)
	    {
	      if ((left - wlen) > 0)
		{
#if defined(DEBUG_PKT)
		  clog("writePacket: wrote short packet: left = %d, "
                       "wlen = %d, len = %d",
		       left, wlen, len);
#endif
		  left -= wlen;
		  continue;
		}
	      return(TRUE);
	    }
	  else
	    {
	      if (wlen < 0 && errno == EINTR)
		{
		  clog("writePacket: write: Interrupted");
		  continue;
		}

	      if (wlen == 0)
		{
		  clog("writePacket: wrote 0: %s", strerror(errno));
		  continue;
		}

	      clog("writePacket: write (wlen=%d): %s", wlen, strerror(errno));
	      return FALSE;
	    }
	}
    }
  else
    clog("writePacket: invalid packet type %d\n", type);

  return(FALSE);
}  


/* Simply check pkt for non-NULL, and compare pkttype with packet's type */
int validPkt(int pkttype, void *pkt)
{
  Unsgn8 *p = (Unsgn8 *)pkt;

  if (!p)
    return FALSE;

  if (((Unsgn8) *p) != pkttype)
    return FALSE;

  return TRUE;
}

void pktSetNodelay(int sock)
{
  /* turn off TCP delay. */
  int on = 1;
  struct protoent *p = getprotobyname("tcp");

  if (!p)
    {
      clog("INFO: getprotobyname(tcp) == NULL");
      return;
    }

  if (setsockopt(sock, 
                 p->p_proto, TCP_NODELAY, (void *)&on, sizeof(on)) <  0) 
    {
      clog("INFO: setsockopt(TCP_NODELAY) failed: %s",
           strerror(errno));
    }
  
  return;
}

