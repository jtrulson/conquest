#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                               C O N Q V M S */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"


/* int GetConquestUID(void) - return conquest's User ID */
int GetConquestUID(void)
{
  struct passwd *conq_pwd;
  static int theuid;
  
  if ((conq_pwd = getpwnam(CONQUEST_USER)) == NULL)
    {
      fprintf(stderr, "conqsvr42: GetConquestUID(%s): can't get user: %s",
	      CONQUEST_USER,
	      sys_errlist[errno]);
      
      return(ERR);
    }
  
  theuid = conq_pwd->pw_uid;
  
  return(theuid);
}


/* int GetConquestGID(void) - return conquest's Group ID */
int GetConquestGID(void)
{
  struct group *conq_grp;
  static int thegid;
  
  if ((conq_grp = getgrnam(CONQUEST_GROUP)) == NULL)
    {
      fprintf(stderr, "conqsvr42: GetConquestGID(%s): can't get group: %s",
	      CONQUEST_GROUP,
	      sys_errlist[errno]);
      
      return(ERR);
    }
  
  thegid = conq_grp->gr_gid;
  
  return(thegid);
}




/*##  astoff - disable asts */
/*  SYNOPSIS */
/*    astoff */
void astoff(void)
{
  
  /*    sys__setast( %val(0) )		/* disable asts */
    /*  JET_ASTOFF();			/* block sigs? */
  return;
  
}


/*##  aston - enable asts */
/*  SYNOPSIS */
/*    aston */
void aston(void)
{
  
  /*    sys$setast( %val(1) )		/* enable asts */
    /*  JET_ASTON();			/* enable sigs? */
  return;
  
}

void EnableSignalHandler(void)
{
  int i;

#ifdef DEBUG_SIG
  clog("EnableSignalHandler() ENABLED");
#endif
  
  signal(SIGHUP, (void (*)(int))DoSig);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTERM, (void (*)(int))DoSig);  
  signal(SIGINT, SIG_IGN);

  if (isagod(NULL))
    {
      signal(SIGQUIT, (void (*)(int))DoSig);
    }
  else if (sysconf_AllowSigquit == TRUE)
    {
      signal(SIGQUIT, (void (*)(int))DoSig);
    }
  else
    {
      signal(SIGQUIT, SIG_IGN);
    }
  
  return;
}

void DoSig(int sig)
{
  
#ifdef DEBUG_SIG
  clog("DoSig() got SIG %d", sig);
#endif
  
  switch(sig)
    {
    case SIGQUIT:
      stoptimer();
      drpexit();
      cdclear();
      cdrefresh(FALSE);
      conqstats(csnum);
      conqend();
      cdend();
      
      exit(0);
      break;
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
      cdend();
      exit(0);
      break;
    default:
      break;
    }

  EnableSignalHandler();	/* reset */
  return;
}


/*##  astservice - ast service routine for conquest */
/*  SYNOPSIS */
/*    astservice */
/* This routine gets called from a sys$setimr ast. Normally, it outputs */
/* one screen update and then sets up another timer request. */
void astservice(int sig)
{
  int now, msg;
  int readone;
  
  /* Don't do anything if we're not supposed to. */
  if ( ! cdisplay )
    return;
  
  /* Don't do anything if we're dead. */
  if ( ! stillalive( csnum ) )
    return;
  stoptimer();
  drcheck();				/* handle driver logic */
  
  /* See if we can display a new message. */
  readone = FALSE;
  if ( cmsgok )
    if ( dgrand( cmsgrand, &now ) >= NEWMSG_GRAND )
      if ( getamsg( csnum, &slastmsg[csnum] ) )
	{
	  if (readmsg( csnum, slastmsg[csnum], RMsg_Line ) == TRUE)
	    {
	      if (msgfrom[slastmsg[csnum]] != csnum)
		if (conf_MessageBell == TRUE)
		  cdbeep();
	      cmsgrand = now;
	      readone = TRUE;
	    }
	}
  
  /* Perform one ship display update. */
  display( csnum );
  
  
  /* Un-read the message if there's a chance it got garbaged. */
  /* JET 3/24/96 - another try with curses timer disabled */
  if ( readone )
    if ( iochav( 0 ) )
      slastmsg[csnum] = modp1( slastmsg[csnum] - 1, MAXMESSAGES );
  
  /* Schedule for next time. */
  settimer();
  
  return;
  
}


/*##  comsize - return size of the common block (in bytes) */
/*  SYNOPSIS */
/*    int size */
/*    comsize( size ) */
void comsize( unsigned long *size )
{
  long val;
  
  val = ((char *)glastmsg - (char *)commonrev); 
  *size = (val < 0) ? (val * -1) + sizeof(int) : val + sizeof(int); 
  
  
  return;
  
}


/*##  conqend - machine dependent clean-up */
/*  SYNOPSIS */
/*    conqend */
void conqend(void)
{
  
  gamend();					/* clean up game environment */
  
  return;
  
}


/*##  conqinit - machine dependent initialization */
/*  SYNOPSIS */
/*    conqinit */
void conqinit(void)
{
  
  int i;
  int gdial, gcron, gprio, gdespri;
  extern int c_conq_fdial, c_conq_fprio, c_conq_despri;
  extern char *c_conq_badttys, *c_conq_antigods, *c_conq_conquest;
  extern char *c_conq_newsfile; 
  extern int c_conq_fsubdcl;
  char gamcron[FILENAMESIZE];
  
  /* First things first. */
  if ( *commonrev != COMMONSTAMP )
    error( "conquest: Common block ident mismatch.  \nInitialize the Universe via conqoper." );
  
  
  /* Get priority for use when spawning. */
  /*    if ( t_getbpri( cpriority ) != OK )
	error( "conqinit: Failed to get base priority" );
	*/
  /* Get an event flag for the ast timer. */
  /*    if ( lib__get_ef( ctimflag ) != OK )
	error( "conqinit: Failed to allocate event flag" );
	*/
  
#ifdef SET_PRIORITY
  /* Increase our priority a bit */
  
  if (nice(CONQUEST_PRI) == -1)
    {
      clog("conqinit(): nice(CONQUEST_PRI (%d)): failed: %s",
	   CONQUEST_PRI,
	   sys_errlist[errno]);
    }
  else
    clog("conqinit(): nice(CONQUEST_PRI (%d)): succeeded.",
	 CONQUEST_PRI);
#endif
  
  /* Set up game environment. */
  gdial = ( c_conq_fdial == YES );
  gprio = ( c_conq_fprio == YES );
  gdespri = (c_conq_despri);
  
  /* Figure out which gamcron file to use (and if we're gonna use one). */
  gcron = FALSE;
  
  gamlinit( gdial, gprio, gcron, gdespri, c_conq_antigods,
	   c_conq_badttys, c_conq_conquest, gamcron );
  
  /* Other house keeping. */
  cpid = getpid();		
  csubdcl = ( c_conq_fsubdcl == YES );
  cnewsfile = ( strcmp( c_conq_newsfile, "" ) != 0 );
  
  /* Zero process id of our child (since we don't have one yet). */
  childpid = 0;
  
  /* Zero last time drcheck() was called. */
  clastime = 0;
  
  /* Haven't scanned anything yet. */
  clastinfostr[0] = EOS;
  
  return;
  
}


/*##  conqstats - handle cpu and elapsed statistics (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    conqstats( snum ) */
void conqstats( int snum )
{
  int unum, team, cadd, eadd;
  
  cadd = 0;
  eadd = 0;
  
  upstats( &sctime[snum], &setime[snum], &scacc[snum], &seacc[snum],
	  &cadd, &eadd );
  
  /* Add in the new amounts. */
  PVLOCK(lockword);
  if ( spid[snum] != 0 )
    {
      /* Update stats for a humanoid ship. */
      unum = suser[snum];
      ustats[unum][USTAT_CPUSECONDS] += cadd;
      ustats[unum][USTAT_SECONDS] += eadd;
      team = uteam[unum];
      tstats[team][TSTAT_CPUSECONDS] += cadd;
      tstats[team][TSTAT_SECONDS] += eadd;
      *ccpuseconds += cadd;
      *celapsedseconds += eadd;
    }
  PVUNLOCK(lockword);
  
  return;
  
}


/*##  drcheck - make sure the driver is still around (DOES LOCKING) */
/*  SYNOPSIS */
/*    drcheck */
void drcheck(void)
{
  int ppid;
  
  /* If we haven't been getting cpu time in recent history, do no-thing. */
  /*  gsecs(playtime);*/
  if ( dsecs( clastime, &clastime ) > TIMEOUT_DRCHECK )
    return;
  
  if ( dsecs( *drivtime, playtime ) > TIMEOUT_DRIVER )
    {
      if ( childpid != 0 )
	{
	  /* We own the driver. See if it's still there. */
	  ppid = childpid;
	  if ( kill(childpid, 0) != -1 )
	    {
	      /* He's still alive and belongs to us. */
	      gsecs( drivtime );
	      return;
	    }
	  else
	    clog( "drcheck(): Wrong ppid %d.", ppid );
	  
	  /* If we got here, something was wrong; disown the child. */
	  childpid = 0;
	}
      
      PVLOCK(lockword);
      if ( dsecs( *drivtime, playtime ) > TIMEOUT_DRIVER )
	{
	  drcreate();
	  *drivcnt = modp1( *drivcnt + 1, 1000 );
	  clog( "Driver timeout #%d.", *drivcnt );
	}
      PVUNLOCK(lockword);
    }
  drstart();
  
  return;
  
}


/*##  drcreate - create a new driver process */
/*  SYNOPSIS */
/*    drcreate */
void drcreate(void)
{
  int pid;
  char drivcmd[BUFFER_SIZE];
  extern char c_conq_conqdriv[];
  
  gsecs( drivtime );			/* prevent driver timeout */
  *drivpid = 0;			/* zero current driver pid */
  *drivstat = DRS_RESTART;		/* driver state to restart */
  
  /* fork the child - mmap()'s should remain */
  /*  intact */
  if ((pid = fork()) == -1)
    {				/* error */
      *drivstat = DRS_OFF;
      clog( "drcreate(): fork(): %s", sys_errlist[errno]);
      return;
    }
  
  if (pid == 0)
    {				/* The child: aka "The Driver" */
      sprintf(drivcmd, "%s/%s", CONQHOME, c_conq_conqdriv);
      execl(drivcmd, drivcmd, NULL);
      clog("drcreate(): exec(): %s", sys_errlist[errno]);
      perror("exec");		/* shouldn't be reached */
      exit(1);
      /* NOTREACHED */
    }
  else
    {				/* We're the parent, store pid */
      childpid = pid;	
    }
  
  return;
  
}


/*##  drkill - make the driver go away if we started it (DOES LOCKING) */
/*  SYNOPSIS */
/*    drkill */
void drkill(void)
{
  
  int i;
  
  if ( childpid != 0 )
    if ( childpid == *drivpid && *drivstat == DRS_RUNNING )
      {
	PVLOCK(lockword);
	if ( childpid == *drivpid && *drivstat == DRS_RUNNING )
	  *drivstat = DRS_KAMIKAZE;
	PVUNLOCK(lockword);
      }
  
  return;
  
}


/*##  drpexit - make the driver go away if we started it */
/*  SYNOPSIS */
/*    drpexit */
void drpexit(void)
{
  
  int i;
  
  if ( childpid != 0 )
    {
      /* We may well have started the driver. */
      drkill();
      for ( i = 1; childpid == *drivpid && i <= 50; i = i + 1 )
	c_sleep( 0.1 );
      if ( childpid == *drivpid )
	clog("drpexit(): Driver didn't exit; pid = %08x", childpid );
    }
  
  return;
  
}


/*##  drstart - Start a new driver if necessary (DOES LOCKING) */
/*  SYNOPSIS */
/*    drstart */
void drstart(void)
{
  
  if ( *drivstat == DRS_OFF )
    {
      PVLOCK(lockword);
      if ( *drivstat == DRS_OFF )
	drcreate();
      PVUNLOCK(lockword);
    }
  return;
  
}


/*##  gcputime - get cpu time */
/*  SYNOPSIS */
/*    int cpu */
/*    gcputime( cpu ) */
/*  DESCRIPTION */
/*    The total cpu time (in hundreths) for the current process is returned. */
void gcputime( int *cpu )
{
  static struct tms Ptimes;
  
  times(&Ptimes);
  
  *cpu = round( ((real)(Ptimes.tms_stime + Ptimes.tms_utime) / 
		 (real)CLK_TCK) * 
	       100.0);
  
  /* clog("gcputime() - *cpu = %d", *cpu); */
  
  return;
  
}


/*##  helplesson - verbose help */
/*  SYNOPSIS */
/*    helplesson */
void helplesson(void)
{
  
  char buf[MSGMAXLINE];
  char helpfile[BUFFER_SIZE];
  extern char *c_conq_helpfile;
  
  sprintf(helpfile, "%s/%s", CONQHOME, c_conq_helpfile);
  sprintf( buf, "%s: Can't open.", helpfile );
  pagefile( helpfile, buf, TRUE, TRUE );
  
  return;
  
}


/*##  initstats - statistics setup */
/*  SYNOPSIS */
/*    int ctemp, etemp */
/*    initstats( ctemp, etemp ) */
void initstats( int *ctemp, int *etemp )
{
  
  gcputime( ctemp );
  grand( etemp );
  
  return;
  
}


/*##  isagod - determine if a user is a god or not - NULL means current user */
/*  SYNOPSIS */
/*    int flag, isagod */
/*    flag = isagod() */
int isagod( char *name )
{
  static struct group *grp = NULL;
  static int god = FALSE;
  static char myname[128];
  int i;
  
  god = FALSE;
  
  if (name == NULL)		/* get god status for current user */
    {				/* now find out whether we're in it */
      strcpy(myname, cuserid(NULL));
    }
  else
    {
      strcpy(myname, name);
    }
  
  if (grp == NULL)
    {				/* first time */
      grp = getgrnam(CONQUEST_GROUP);
      
      if (grp == NULL)
	{
	  clog("isagod(%s): getgrnam(%s) failed: %s",
	       myname,
	       CONQUEST_GROUP,
	       sys_errlist[errno]);
	  
	  god = FALSE;
	  return(FALSE);
	}
    }
  
  /* root is always god */
  if (strcmp(myname, "root") == 0)
    god = TRUE;
  
  i = 0;
  
  if (grp->gr_mem != NULL)
    {
      while (grp->gr_mem[i] != NULL)
	{
	  if (strcmp(myname, grp->gr_mem[i]) == 0)
	    {		/* a match */
	      god = TRUE;
	      break;
	    }
	  
	  i++;
	}
    }
  
  endgrent();
  
  return(god);
  
}


/*##  mail - send a one liner mail message (TOOLS mail version) */
/*  SYNOPSIS */
/*    int sendok, mail */
/*    char names(), subject(), msg() */
/*    sendok = mail( names, subject, msg ) */
/* Note: The buffer msg() will contain an error message if FALSE is */
/* returned by this routine. */
int mail( char names[], char subject[], char msg[] )
{
  c_strcpy( "Mail not available", msg );
  
  return ( FALSE );
  
}


/*##  mailimps - send a one liner mail message to the Implementors */
/*  SYNOPSIS */
/*    int sendok, mailimps */
/*    char subject(), msg() */
/*    sendok = mailimps( subject, msg ) */
/* Note: The buffer msg() will contain an error message if FALSE is */
/* returned by this routine. */
int mailimps( char subject[], char msg[] )
{
  
  string mailaddr=MAILADDR;
  
  /*    if ( mailaddr[0] == EOS )
	{
	c_strcpy( "It is not possible to contact the Implementors.", msg );
	return ( FALSE );
	}
	return ( mail( MAILADDR, subject, msg ) );
	*/
  
  return(FALSE);
}


/*##  news - list current happenings */
/*  SYNOPSIS */
/*    news */
void news(void)
{
  char newsfile[BUFFER_SIZE];
  extern char *c_conq_newsfile;
  
  sprintf(newsfile, "%s/%s", CONQHOME, c_conq_newsfile);
  
  pagefile( newsfile, "No news is good news.", TRUE, TRUE );
  
  return;
  
}


/*##  settimer - set timer to display() */
/*  SYNOPSIS */
/*    csetimer */
void settimer(void)
{
  static struct sigaction Sig;
  static char errstr[128];
  
#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  Sig.sa_handler = (void (*)(int))astservice;
  
/*  Sig.sa_flags = SA_RESTART;*/	/* restart syscalls! */

  Sig.sa_flags = 0;

  if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
      clog("settimer():sigaction(): %s\n", sys_errlist[errno]);
      exit(errno);
    }
  
#ifdef HAS_SETITIMER

  if (sysconf_AllowFastUpdate == TRUE && conf_DoFastUpdate == TRUE)
    {
      
      /* 2 updates per sec */
      itimer.it_value.tv_sec = 0;
      itimer.it_value.tv_usec = 500000;
      
      itimer.it_interval.tv_sec = 0;
      itimer.it_interval.tv_usec = 500000;
    }
  else
    {

      /* 1 update per second */
      itimer.it_value.tv_sec = 1;
      itimer.it_value.tv_usec = 0;
      
      itimer.it_interval.tv_sec = 1;
      itimer.it_interval.tv_usec = 0;
    }

  setitimer(ITIMER_REAL, &itimer, NULL);
#else
  alarm(1);			/* set alarm() */
#endif  
  return;
  
}


/*##  stoptimer - cancel timer */
/*  SYNOPSIS */
/*    stoptimer */
void stoptimer(void)
{
#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  cdisplay = FALSE;
  
  signal(SIGALRM, SIG_IGN);
  
#ifdef HAS_SETITIMER
  itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
  itimer.it_value.tv_usec = itimer.it_interval.tv_usec = 0;
  
  setitimer(ITIMER_REAL, &itimer, NULL);
#else
  alarm(0);
#endif
  
  cdisplay = TRUE;
  
  return;
  
}


/*##  upchuck - update the common block to disk. */
/*  SYNOPSIS */
/*    upchuck */
void upchuck(void)
{
  
  PVLOCK(lockword);
  
  flush_common();
  getdandt( lastupchuck );
  
  PVUNLOCK(lockword);
  
  return;
  
}


/*##  upstats - update statistics */
/*  SYNOPSIS */
/*    int ctemp, etemp, caccum, eaccum, ctime, etime */
/*    upstats( ctemp, etemp, caccum, eaccum, ctime, etime ) */
void upstats( int *ctemp, int *etemp, int *caccum, int *eaccum, int *ctime, int *etime )
{
  
  int i, j, now;
  
  /* Update cpu time. */
  gcputime( &i );
  
  if (i >= *ctemp )		/* prevent oddities with timing - JET */
    {				/* - for multple godlike exits/entries */
      *caccum = *caccum + (i - *ctemp);
    }
  *ctemp = i;			/* if oddity above, this will self-correct */
  
  if ( *caccum > 100 )
    {
      /* Accumulated a cpu second. */
      *ctime = *ctime + (*caccum / 100);
      *caccum = mod( *caccum, 100 );
    }
  
  /* Update elapsed time. */
  *eaccum = *eaccum + dgrand( *etemp, &now );
  if ( *eaccum > 1000 )
    {
      /* A second elapsed. */
      *etemp = now;
      *etime = *etime + (*eaccum / 1000);
      *eaccum = mod( *eaccum, 1000 );
    }
  
  return;
  
}
