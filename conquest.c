#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                               C O N Q U E S T */
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
/*                                                                    */
/**********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"
#include "color.h"

static char *conquestId = "$Id$";

/*  conquest - main program */
main(int argc, char *argv[]) 
{
  
  if ((ConquestUID = GetConquestUID()) == ERR)
    {
      fprintf(stderr, "conquest: GetConquestUID() failed\n");
      exit(1);
    }
  
  if ((ConquestGID = GetConquestGID()) == ERR)
    {
      fprintf(stderr, "conquest: GetConquestGID() failed\n");
      exit(1);
    }
  
#ifdef DEBUG_CONFIG
  clog("%s@%d: main() Reading Configuration files.", __FILE__, __LINE__);
#endif
  
  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
    }
  
  if (GetConf() == ERR)		/* do this BEFORE setgid() call! */
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetConf() returned ERR.", __FILE__, __LINE__);
#endif
      exit(1);
    }
  
  
  if (setgid(ConquestGID) == -1)
    {
      clog("conquest: setgid(%d): %s",
           ConquestGID,
           sys_errlist[errno]);
      fprintf(stderr, "conquest: setgid(): failed\n");
      exit(1);
    }
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() *STARTING*", __FILE__, __LINE__);
#endif
  
#ifdef USE_SEMS
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() getting semephores - GetSem()", __FILE__, __LINE__);
#endif
  
  if (GetSem() == ERR)
    {
      fprintf(stderr, "GetSem() failed to get semaphores. exiting.\n");
      exit(1);
    }
#endif
  
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() mapping common block.", __FILE__, __LINE__);
#endif
  
  map_common();
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif
  
  conqinit();			/* machine dependent initialization */
  
  rndini( 0, 0 );			/* initialize random numbers */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting cdinit().", __FILE__, __LINE__);
#endif
  
  
  cdinit();				/* set up display environment */
  
  cmaxlin = cdlins();
  
  cmaxcol = cdcols();
  
  
  
  csnum = 0;				/* force menu to get a new ship */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() welcoming player.", __FILE__, __LINE__);
#endif
  
  if ( welcome( &cunum ) )
    menu();
  
  drpexit();			/* make the driver go away */
  cdend();				/* clean up display environment */
  conqend();			/* machine dependent clean-up */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() *EXITING*", __FILE__, __LINE__);
#endif
  
  exit(0);
  
}


/*  capentry - captured system entry for a new ship */
/*  SYNOPSIS */
/*    int flag, capentry */
/*    int capentry, snum, system */
/*    system = capentry( snum, system ) */
int capentry( int snum, int *system )
{
  int i, j; 
  int ch; 
  int owned[NUMTEAMS]; 
  
  /* First figure out which systems we can enter from. */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      owned[i] = FALSE;
      /* We must own all three planets in a system. */
      for ( j = 0; j < 3; j = j + 1 )
	{
	  if ( pteam[teamplanets[i][j]] != steam[snum] )
	    goto cnext2_1; /* next 2; */
	}
      owned[i] = TRUE;
    cnext2_1:
      ;
    }
  owned[steam[snum]] = TRUE;		/* always can enter in our system */
  
  /* Now count how many systems we can enter from. */
  j = 0;
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( owned[i] )
      j = j + 1;
  
  /* If we can only enter from one, we're done. */
  if ( j <= 1 )
    {
      *system = steam[snum];
      return ( TRUE );
    }
  
  /* Prompt for a decision. */
  c_strcpy( "Enter which system", cbuf );
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( owned[i] )
      {
	appstr( ", ", cbuf );
	appstr( tname[i], cbuf );
      }
  /* Change first comma to a colon. */
  i = c_index( cbuf, ',' );
  if ( i != ERR )
    cbuf[i] = ':';
  
  cdclrl( MSG_LIN1, 2 );
  cdputc( cbuf, MSG_LIN1 );
  cdmove( 1, 1 );
  cdrefresh();
  
  while ( stillalive( csnum ) )
    {
      if ( ! iogtimed( &ch, 1 ) )
	continue; /* next */
      switch  ( ch )
	{
	case TERM_NORMAL:
	case TERM_ABORT:
	  return ( FALSE );
	  break;
	case TERM_EXTRA:
	  /* Enter the home system. */
	  *system = steam[snum];
	  return ( TRUE );
	  break;
	default:
	  for ( i = 0; i < NUMTEAMS; i = i + 1 )
	    if ( chrteams[i] == (char)toupper( ch ) && owned[i] )
	      {
		/* Found a good one. */
		*system = i;
		return ( TRUE );
	      }
	  /* Didn't get a good one; complain and try again. */
	  cdbeep();
	  cdrefresh();
	  break;
	}
    }
  
  return ( FALSE );			/* can get here because of stillalive() */
  
}


/*  command - execute a user's command */
/*  SYNOPSIS */
/*    char ch */
/*    command( ch ) */
void command( int ch )
{
  int i;
  real x;
  
  if (KPAngle(ch, &x) == TRUE)	/* hit a keypad key */
    {				/* change course */
      cdclrl( MSG_LIN1, 1 );
      cdclrl( MSG_LIN2, 1 );
      
      if ( swarp[csnum] < 0.0 ) 
	swarp[csnum] = 0.0; 
      sdhead[csnum] = (real)(x); 
      slock[csnum] = 0; 
      
      return;
    }
  
  switch ( ch )
    {
    case '0':           /* - '9', '=':	/* set warp factor */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '=':
      if ( ch == '=' )
	x = 10.0;
      else
	{
	  i = ch - '0';
	  x = (real) (i); 
	}
      dowarp( csnum, x );
      break;
    case 'a':				/* autopilot */
      if ( uooption[suser[csnum]][ OOPT_AUTOPILOT] )
	{
	  doautopilot( csnum );
	}
      else
	{
	  goto label1;
	}
      break;
    case 'A':				/* change allocation */
      doalloc( csnum );
      stoptimer();
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
    case 'b':				/* beam armies */
      dobeam( csnum );
      break;
    case 'B':				/* bombard a planet */
      dobomb( csnum );
      break;
    case 'C':				/* cloak control */
      docloak( csnum );
      break;
    case 'd':				/* detonate enemy torps */
    case '*':
      dodet( csnum );
      break;
    case 'D':				/* detonate own torps */
      domydet( csnum );
      break;
    case 'E':				/* emergency distress call */
      dodistress( csnum );
      break;
    case 'f':				/* phasers */
      dophase( csnum );
      break;
    case 'F':				/* phasers, same direction */
      dolastphase( csnum );
      break;
    case 'h':
      credraw = TRUE;
      stoptimer();
      dohelp( csubdcl );
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
    case 'H':
      credraw = TRUE;
      stoptimer();
      histlist( FALSE );
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
    case 'i':				/* information */
      doinfo( csnum );
      break;
    case 'I':				/* set user options */
      dooption( csnum, TRUE );
      break;
    case 'k':				/* set course */
      docourse( csnum );
      break;
    case 'K':				/* coup */
      docoup( csnum );
      break;
    case 'L':				/* review old messages */
      doreview( csnum );
      break;
    case 'm':				/* send a message */
      sendmsg( csnum, soption[csnum][OPT_TERSE] );
      break;
    case 'M':				/* strategic/tactical map */
      smap[csnum] = ! smap[csnum];	
      stoptimer();
      display( csnum, FALSE );
      settimer();
      break;
    case 'N':				/* change pseudonym */
      pseudo( cunum, csnum );
      break;
    case 'o':				/* orbit nearby planet */
      doorbit( csnum );
      break;
    case 'P':				/* photon torpedo burst */
      doburst( csnum );
      break;
    case 'p':				/* photon torpedoes */
      dotorp( csnum );
      break;
    case 'Q':				/* self destruct */
      doselfdest( csnum );
      break;
    case 'R':				/* repair mode */
      if ( ! scloaked[csnum] )
	{
	  cdclrl( MSG_LIN1, 2 );
	  srmode[csnum] = TRUE;
	  sdwarp[csnum] = 0.0;
	}
      else
	{
	  cdclrl( MSG_LIN2, 1 );
	  c_putmsg(
		   "You cannot repair while the cloaking device is engaged.",
		   MSG_LIN1 );
	}
      break;
    case 't':				/* tow */
      dotow( csnum );
      break;
    case 'S':				/* more user stats */
      credraw = TRUE;
      stoptimer();
      userstats( FALSE, csnum ); 
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
    case 'T':				/* team list */
      credraw = TRUE;
      stoptimer();
      doteamlist( steam[csnum] );
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
    case 'u':				/* un-tractor */
      dountow( csnum );
      break;
    case 'U':				/* user stats */
      credraw = TRUE;
      stoptimer();
      userlist( FALSE, csnum );
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
    case 'W':				/* war and peace */
      dowar( csnum );
      break;
    case '-':				/* shields down */
      doshields( csnum, FALSE );
      stoptimer();
      display( csnum, FALSE );
      settimer();
      break;
    case '+':				/* shields up */
      doshields( csnum, TRUE );
      stoptimer();
      display( csnum, FALSE );
      settimer();
      break;
    case '/':				/* player list */
      credraw = TRUE;
      stoptimer();
      playlist( FALSE, FALSE, csnum );
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
    case '?':				/* planet list */
      credraw = TRUE;
      stoptimer();
      doplanlist( csnum );
      if ( stillalive( csnum ) )
	display( csnum, FALSE );
      settimer();
      break;
#ifdef NOTUSED
    case '$':				/* spawn to DCL */
      if ( csubdcl )
	{
	  dosubdcl();
	}
      else
	{
	  goto label1;
	}
      break;
#endif
    case '\014':			/* clear and redisplay */
      stoptimer();
      cdredo();
      credraw = TRUE;
      display( csnum, FALSE );
      settimer();
      break;
      
    case TERM_NORMAL:		/* Have [RETURN] act like 'I[RETURN]'  */
    case KEY_ENTER:
    case '\n':
      iBufPut("i\r");		/* (get last info) */
      break;

    case TERM_EXTRA:		/* Have [TAB] act like 'i\t' */
      iBufPut("i\t");		/* (get next last info) */
      break;
      
    case -1:			/* really nothing */
#ifdef DEBUG_IO
      clog("command(): got -1 - ESC?");
#endif
      break;

      /* nothing. */
    default:
    label1:
      /*	    ioeat();*/
      cdbeep();
#ifdef DEBUG_IO
      clog("command(): got 0%o, KEY_A1 =0%o", ch, KEY_A1);
#endif
      c_putmsg( "Type h for help.", MSG_LIN2 );
    }
  
  return;
  
}


/*  conqds - display background for Conquest */
/*  SYNOPSIS */
/*    int multiple, switchteams */
/*    conqds( multiple, switchteams ) */
void conqds( int multiple, int switchteams )
{
  int i, col, lin, lenc1;
  string c1=" CCC    OOO   N   N   QQQ   U   U  EEEEE   SSSS  TTTTT";
  string c2="C   C  O   O  NN  N  Q   Q  U   U  E      S        T";
  string c3="C      O   O  N N N  Q   Q  U   U  EEE     SSS     T";
  string c4="C   C  O   O  N  NN  Q  Q   U   U  E          S    T";
  string c5=" CCC    OOO   N   N   QQ Q   UUU   EEEEE  SSSS     T";
  
  extern char *ConquestVersion;
  extern char *ConquestDate;
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#(#%d#%%c#%d#) - %%s",
	      LabelColor,
	      InfoColor,
	      LabelColor);
	}
  
  /* First clear the display. */
  cdclear();
  
  /* Display the logo. */
  lenc1 = strlen( c1 );
  col = (cmaxcol-lenc1) / 2;
  lin = 2;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c1);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c2);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c3);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c4);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c5);
  
  /* Draw a box around the logo. */
  lin++;
  attrset(A_BOLD);
  cdbox( 1, col-2, lin, col+lenc1+1 );
  attrset(0);
  
  lin++;
  if ( *closed )
    cprintf(lin,0,ALIGN_CENTER,"#%d#%s",RedLevelColor,"The game is closed.");
  else
    cprintf( lin,col,ALIGN_CENTER,"#%d#%s (%s)",YellowLevelColor,
	   ConquestVersion, ConquestDate);
  
  lin++;
  cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Options:");
  
  col = 13;
  lin+=2;
  i = lin;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'e', "enter the game");
  if ( cnewsfile )
    {
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, 'n', "read the news");
    }
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'h', "read the help lesson");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'S', "more user statistics");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'T', "team statistics");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'U', "user statistics");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'L', "review messages");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'W', "set war or peace");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'I', "change user options");
  
  col = 48;
  lin = i;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'N', "change your name");
  if ( ! multiple )
    {
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, 'r', "resign your commission");
    }
  if ( multiple || switchteams )
    {
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, 's', "switch teams");
    }
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'H', "user history");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '/', "player list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '?', "planet list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'q', "exit the program");
  
  return;
  
}


/*  dead - announce to a user that s/he is dead (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int leave */
/*    dead( snum, leave ) */
void dead( int snum, int leave )
{
  int i, j, kb, now, entertime; 
  int ch; 
  string ywkb="You were killed by ";
  char buf[128], junk[128];
  
  /* (Quickly) clear the screen. */
  cdclear();
  cdredo();
  cdrefresh();

  /* If something is wrong, don't do anything. */
  if ( snum < 1 || snum > MAXSHIPS )
    return;
  
  /* If our ships pid is wrong, we are indeed lost. */
  if ( spid[snum] != cpid )
    return;
  
  kb = skilledby[snum];
  
  /* Delay while our torps are exploding. */
  grand( &entertime );
  i = 0;
  while ( dgrand( entertime, &now ) < TORPEDOWAIT_GRAND )
    {
      i = 0;
      for ( j = 0; j < MAXTORPS; j = j + 1 )
	if ( tstatus[snum][j] == TS_DETONATE )
	  i = i + 1;
      if ( i <= 0 )
	break;
      c_sleep( ITER_SECONDS );
    }
  
  /* There aren't supposed to be any torps left. */
  if ( i > 0 )
    {
      c_strcpy( "dead: ", cbuf );
      appship( snum, cbuf );
      appstr( "'s detonating torp count is %d.", cbuf );
      cerror( cbuf, i );
      clog(cbuf, i);
    }
  
  /* Figure out why we died. */
  switch ( kb )
    {
    case KB_SELF:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
		"You scuttled yourself.");

      break;
    case KB_NEGENB:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
		"You were destroyed by the negative energy barrier.");

      break;
    case KB_CONQUER:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
	       "Y O U   C O N Q U E R E D   T H E   U N I V E R S E ! ! !");
      break;
    case KB_NEWGAME:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"N E W   G A M E !");
      break;
    case KB_EVICT:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"Closed for repairs.");
      break;
    case KB_SHIT:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You are no longer allowed to play.");
      break;
    case KB_GOD:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were killed by an act of GOD.");

      break;
    case KB_DOOMSDAY:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were eaten by the doomsday machine.");

      break;
    case KB_GOTDOOMSDAY:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You destroyed the doomsday machine!");
      break;
    case KB_DEATHSTAR:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were vaporized by the Death Star.");

      break;
    case KB_LIGHTNING:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were destroyed by a lightning bolt.");

      break;
    default:
      
	  cbuf[0] = EOS;
	  buf[0] = EOS;
	  junk[0] = EOS;
      if ( kb > 0 && kb <= MAXSHIPS )
	{
	  appship( kb, cbuf );
	  if ( sstatus[kb] != SS_LIVE )
	    appstr( ", who also died.", buf );
	  else
	    appchr( '.', buf );
	  cprintf( 8,0,ALIGN_CENTER, 
		   "#%d#You were kill number #%d#%.1f #%d#for #%d#%s #%d#(#%d#%s#%d#)%s",
		   InfoColor, A_BOLD, skills[kb], 
		   InfoColor, A_BOLD, spname[kb], 
		   InfoColor, A_BOLD, cbuf, 
		   InfoColor, buf );
	}
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	{
	  if ( ptype[-kb] == PLANET_SUN )
	      strcpy(cbuf, "solar radiation.");
	  else
	      strcpy(cbuf, "planetary defenses.");
	  cprintf(8,0,ALIGN_CENTER,"#%d#%s#%d#%s%s#%d#%s", 
		InfoColor, ywkb, A_BOLD, pname[-kb], "'s ",
		InfoColor, cbuf);

	}
      else
	{
	  /* We were unable to determine the cause of death. */
	  buf[0] = EOS;
	  appship( snum, buf );
	  sprintf(cbuf, "dead: %s was killed by %d.", buf, kb);
	  cerror( cbuf );
	  clog(cbuf);
	  
	  cprintf(8,0,ALIGN_CENTER,"#%d#%s%s", 
	  	RedLevelColor, ywkb, "nothing in particular.  (How strange...)");
	}
    }
  
  if ( kb == KB_NEWGAME )
    {
      cprintf( 10,0,ALIGN_CENTER,
		"#%d#Universe conquered by #%d#%s #%d#for the #%d#%s #%d#team.",
		 InfoColor, A_BOLD, conqueror, 
		 InfoColor, A_BOLD, conqteam, LabelColor );
    }
  else if ( kb == KB_SELF )
    {
      i = sarmies[snum];
      if ( i > 0 )
	{
	  junk[0] = EOS; 
	  if ( i == 1 )
	    strcpy( cbuf, "army" );
	  else
	    {
	      if ( i < 100 )
			appnum( i, junk );
	      else
			appint( i, junk );
	      strcpy( cbuf, "armies" );
	    }
	  if ( i == 1 )
	    strcpy( buf, "was" );
	  else
	    strcpy( buf, "were");
	  if ( i == 1 )
		cprintf(10,0,ALIGN_CENTER,
		"#%d#The #%d#%s #%d#you were carrying %s not amused.",
			LabelColor, A_BOLD, cbuf, LabelColor, buf);
	  else
		cprintf(10,0,ALIGN_CENTER,
		"#%d#The #%d#%s %s #%d#you were carrying %s not amused.",
			LabelColor, A_BOLD, junk, cbuf, LabelColor, buf);
	}
    }
  else if ( kb >= 0 )
    {
      if ( sstatus[kb] == SS_LIVE )
	{
	  cprintf( 10,0,ALIGN_CENTER,
		"#%d#He had #%d#%d%% #%d#shields and #%d#%d%% #%d#damage.",
		InfoColor, A_BOLD, round(sshields[kb]), 
		InfoColor, A_BOLD, round(sdamage[kb]),InfoColor );
	}
    }
  cprintf(12,0,ALIGN_CENTER,
	"#%d#You got #%d#%.1f #%d#this time.", 
	InfoColor, A_BOLD, oneplace(skills[snum]), InfoColor );
  cdmove( 1, 1 );
  cdrefresh();

  if ( ! ( leave && kb == KB_SELF ) && kb != KB_SHIT && kb != KB_EVICT )
    c_sleep( 4.0 );
  
  for ( i = 1; i <= 10 && sstatus[snum] == SS_DYING; i++ )
    c_sleep( 1.0 );
  sstatus[snum] = SS_RESERVED;
  ssdfuse[snum] = -TIMEOUT_PLAYER;
  skilledby[snum] = 0;
  
  switch ( kb )
    {
    case KB_CONQUER:
      do
	{
	  cdclear();
	  cdredo();
	  lastwords[0] = EOS;
	  ch = cdgetx( "Any last words? ",
		       14, 1, TERMS, lastwords, MAXLASTWORDS );
	  cdclear();
	  cdredo();
	  if ( lastwords[0] != EOS )
	    {
	      cprintf( 13,0,ALIGN_CENTER, "#%d#%s", 
			InfoColor, "You last words are entered as:");
	      cprintf( 14,0,ALIGN_CENTER, "#%d#%c%s%c", 
			YellowLevelColor, '"', lastwords, '"' );
	    }
	  else
	    cprintf( 14,0,ALIGN_CENTER,"#%d#%s", InfoColor,
		   "You have chosen to NOT leave any last words:" );
	  ch = getcx( "Press TAB to confirm:", 16, 0,
		     TERMS, cbuf, 10 );
	}
      while ( ch != TERM_EXTRA ); /* until -> while */
      break;
    case KB_SELF:
    case KB_EVICT:
    case KB_SHIT:
      /* Do nothing special. */
      break;
    default:
      ioeat();
      putpmt( "--- press space when done ---", MSG_LIN2 );
      cdrefresh();
      while ( ! iogtimed( &ch, 1 ) && stillalive( csnum ) )
	;
      break;
    }
  cdmove( 1, 1 );
  
  /* Turn off sticky war so we can change war settings from menu(). */
  for ( i = 0; i < NUMTEAMS; i++ )
    srwar[snum][i] = FALSE;
  
  return;
  
}


/*  dispoption - display options */
/*  SYNOPSIS */
/*    int op(MAXOPTIONS) */
/*    dispoption( op ) */
void dispoption( int op[] )
{
  string l1="Toggle options, TAB when done: phaser (g)raphics=~ (p)lanet names=~";
  string l2="(a)larm bells=~ (i)ntruder alerts=~ (n)umeric map=~ (t)erse=~ (e)xplosions=~";
  
  cdclrl( MSG_LIN1, 2 );
  cdputs( l1, MSG_LIN1, 1 );
  cdputs( l2, MSG_LIN2, 1 );
  
  if ( op[OPT_PHASERGRAPHICS] )
    cdput( 'T', MSG_LIN1, 50 );
  else
    cdput( 'F', MSG_LIN1, 50 );
  
  if ( op[OPT_PLANETNAMES] )
    cdput( 'T', MSG_LIN1, 67 );
  else
    cdput( 'F', MSG_LIN1, 67 );
  
  if ( op[OPT_ALARMBELL] )
    cdput( 'T', MSG_LIN2, 15 );
  else
    cdput( 'F', MSG_LIN2, 15 );
  
  if ( op[OPT_INTRUDERALERT] )
    cdput( 'T', MSG_LIN2, 35 );
  else
    cdput( 'F', MSG_LIN2, 35 );
  
  if ( op[OPT_NUMERICMAP] )
    cdput( 'T', MSG_LIN2, 51 );
  else
    cdput( 'F', MSG_LIN2, 51 );
  
  if ( op[OPT_TERSE] )
    cdput( 'T', MSG_LIN2, 61 );
  else
    cdput( 'F', MSG_LIN2, 61 );
  
  if ( op[OPT_EXPLOSIONS] )
    cdput( 'T', MSG_LIN2, 76 );
  else
    cdput( 'F', MSG_LIN2, 76 );
  
  return;
  
}


/*  doalloc - change weapon/engine allocations */
/*  SYNOPSIS */
/*    int snum */
/*    doalloc( snum ) */
void doalloc( int snum )
{
  char ch;
  int i, alloc;
  
  string pmt="New weapons allocation: (30-70) ";
  
  cdclrl( MSG_LIN1, 2 );
  cbuf[0] = EOS;
  ch = (char)cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  if ( ch == TERM_EXTRA )
    sweapons[snum] = sengines[snum];
  else if ( ch == TERM_NORMAL )
    {
      i = 0;
      safectoi( &alloc, cbuf, i );			/* ignore status */
      if ( alloc != 0 )
	{
	  if ( alloc < 30 )
	    alloc = 30;
	  else if ( alloc > 70 )
	    alloc = 70;
	  sweapons[snum] = alloc;
	}
    }
  
  sengines[snum] = 100 - sweapons[snum];
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*  doautopilot - handle the autopilot */
/*  SYNOPSIS */
/*    int snum */
/*    doautopilot( snum ) */
void doautopilot( int snum )
{
  int now, laststat; 
  int ch; 
  string conf="Press TAB to engage autopilot:";
  
  cdclrl( MSG_LIN1, 2 );
  cbuf[0] = EOS;
  if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  c_putmsg( "Autopilot activated.", MSG_LIN1 );
  srobot[snum] = TRUE;
  gsecs( &laststat );			/* initialize stat timer */
  while ( stillalive( csnum ) )
    {
      /* Make sure we still control our ship. */
      if ( spid[snum] != cpid )
	break;
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( csnum );
	  laststat = now;
	}
      
      /* Get a character. */
      if ( ! iogtimed( &ch, 1 ) )
	continue;		/* next -> echo */
      cmsgok = FALSE;
      grand( &cmsgrand );
      switch ( ch )
	{
	case TERM_ABORT:
	  break;
	case '\014':	/* ^L */
	  cdredo();
	  break;
	default:
	  c_putmsg( "Press ESCAPE to abort autopilot.", MSG_LIN1 );
	  cdbeep();
	  cdrefresh();
	}
      cmsgok = TRUE;
      if (ch == TERM_ABORT)
	break;
    }
  srobot[snum] = FALSE;
  saction[snum] = 0;
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  dobeam - beam armies up or down (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobeam( snum ) */
void dobeam( int snum )
{
  int pnum, total, num, upmax, downmax, capacity, beamax, i;
  int ototal, entertime, now;
  int oldsshup, dirup, zeroed, conqed;
  int ch; 
  char buf[MSGMAXLINE];
  real rkills;
  int done = FALSE;
  string lastfew="Fleet orders prohibit removing the last three armies.";
  string abt="...aborted...";
  
  srmode[snum] = FALSE;
  
  cdclrl( MSG_LIN1, 2 );
  
  /* Check for allowability. */
  if ( swarp[snum] >= 0.0 )
    {
      c_putmsg( "We must be orbiting a planet to use the transporter.",
	       MSG_LIN1 );
      return;
    }
  pnum = -slock[snum];
  if ( sarmies[snum] > 0 )
    {
      if ( ptype[pnum] == PLANET_SUN )
	{
	  c_putmsg( "Idiot!  Our armies will fry down there!", MSG_LIN1 );
	  return;
	}
      else if ( ptype[pnum] == PLANET_MOON )
	{
	  c_putmsg( "Phoon!  Our armies will suffocate down there!",
		   MSG_LIN1 );
	  return;
	}
      else if ( pteam[pnum] == TEAM_GOD )
	{
	  c_putmsg(
		   "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
		   MSG_LIN1 );
	  return;
	}
    }
  
  i = puninhabtime[pnum];
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minute",
	     i );
      if ( i != 1 )
	appchr( 's', cbuf );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  if ( pteam[pnum] != steam[snum] &&
      pteam[pnum] != TEAM_SELFRULED &&
      pteam[pnum] != TEAM_NOTEAM )
    if ( ! swar[snum][pteam[pnum]] && parmies[pnum] != 0) /* can take empty planets */
      {
	c_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  if ( sarmies[snum] == 0 &&
      pteam[pnum] == steam[snum] && parmies[pnum] <= MIN_BEAM_ARMIES )
    {
      c_putmsg( lastfew, MSG_LIN1 );
      return;
    }
  
  rkills = skills[snum];
#ifdef DEBUG_MISC
  clog("dobeam(): rkills=%f skills[%d]=%f",
       rkills, snum, skills[snum]);
#endif
  if ( rkills < (real)1.0 )
    {
      c_putmsg(
	       "Fleet orders prohibit beaming armies until you have a kill.",
	       MSG_LIN1 );
      return;
    }
  
  /* Figure out what can be beamed. */
  downmax = sarmies[snum];
  if ( spwar(snum,pnum) ||
      pteam[pnum] == TEAM_SELFRULED ||
      pteam[pnum] == TEAM_NOTEAM ||
      pteam[pnum] == TEAM_GOD ||
      parmies[pnum] == 0 )
    {
      upmax = 0;
    }
  else
    {
      capacity = min( ifix( rkills ) * 2, armylim[steam[snum]] );
      upmax = min( parmies[pnum] - MIN_BEAM_ARMIES, capacity-sarmies[snum] );
    }
  
  /* If there are armies to beam but we're selfwar... */
  if ( upmax > 0 && selfwar(snum) && steam[snum] == pteam[pnum] )
    {
      if ( downmax <= 0 )
	{
	  c_strcpy( "The arm", cbuf );
	  if ( upmax == 1 )
	    appstr( "y is", cbuf );
	  else
	    appstr( "ies are", cbuf );
	  appstr( " reluctant to beam aboard a pirate vessel.", cbuf );
	  c_putmsg( cbuf, MSG_LIN1 );
	  return;
	}
      upmax = 0;
    }
  
  /* Figure out which direction to beam. */
  if ( upmax <= 0 && downmax <= 0 )
    {
      c_putmsg( "There is no one to beam.", MSG_LIN1 );
      return;
    }
  if ( upmax <= 0 )
    dirup = FALSE;
  else if ( downmax <= 0 )
    dirup = TRUE;
  else
    {
      c_putmsg( "Beam [up or down] ", MSG_LIN1 );
      cdrefresh();
      done = FALSE;
      while ( stillalive( csnum ) && done == FALSE)
	{
	  if ( ! iogtimed( &ch, 1 ) )
	    {
	      continue;	/* next */
	    }
	  switch ( (char)tolower( ch ) )
	    {
	    case 'u':
	    case 'U':
	      dirup = TRUE;
	      done = TRUE;
	      break;
	    case 'd':
	    case 'D':
	    case TERM_EXTRA:
	      dirup = FALSE;
	      done = TRUE;
	      break;
	    default:
	      c_putmsg( abt, MSG_LIN1 );
	      return;
	    }
	}
    }
  
  if ( dirup )
    beamax = upmax;
  else
    beamax = downmax;
  
  /* Figure out how many armies should be beamed. */
  if ( dirup )
    c_strcpy( "up", buf );
  else
    c_strcpy( "down", buf );
  sprintf( cbuf, "Beam %s [1-%d] ", buf, beamax );
  cdclrl( MSG_LIN1, 1 );
  buf[0] = EOS;
  ch = cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
  if ( ch == TERM_ABORT )
    {
      c_putmsg( abt, MSG_LIN1 );
      return;
    }
  else if ( ch == TERM_EXTRA && buf[0] == EOS )
    num = beamax;
  else
    {
      delblanks( buf );
      if ( alldig( buf ) != TRUE )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  return;
	}
      i = 0;
      safectoi( &num, buf, i );			/* ignore status */
      if ( num < 1 || num > beamax )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  return;
	}
    }
  /* Now we are ready! */
  if ( pteam[pnum] >= NUMTEAMS )
    {
      /* If the planet is not race owned, make it war with us. */
      ssrpwar[snum][pnum] = TRUE;
    }
  else if ( pteam[pnum] != steam[snum] )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      srwar[snum][pteam[pnum]] = TRUE;
      
      /* Chance to create a robot here. */
      intrude( snum, pnum );
    }
  
  /* Lower shields. */
  oldsshup = sshup[snum];
  sshup[snum] = FALSE;
  
  /* Beam. */
  total = 0;
  ototal = -1;				/* force an update the first time */
  zeroed = FALSE;
  conqed = FALSE;
  
  grand( &entertime );
  while(TRUE)			/* repeat infloop */
    {
      if ( ! stillalive( csnum ) )
	return;
      if ( iochav() )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  break;
	}
      
      /* See if it's time to beam again. */
      while ( dgrand( entertime, &now ) >= BEAM_GRAND )
	{
	  /*	      entertime = mod( entertime + BEAM_GRAND, 24*60*60*1000 );*/
	  grand(&entertime);
	  PVLOCK(lockword);
	  if ( dirup )
	    {
	      /* Beam up. */
	      if ( parmies[pnum] <= MIN_BEAM_ARMIES )
		{
		  PVUNLOCK(lockword);
		  c_putmsg( lastfew, MSG_LIN1 );
		  break;
		}
	      sarmies[snum] = sarmies[snum] + 1;
	      parmies[pnum] = parmies[pnum] - 1;
	    }
	  else
	    {
	      /* Beam down. */
	      sarmies[snum] = sarmies[snum] - 1;
	      if ( pteam[pnum] == TEAM_NOTEAM || parmies[pnum] == 0 )
		{
		  takeplanet( pnum, snum );
		  conqed = TRUE;
		}
	      else if ( pteam[pnum] != steam[snum] )
		{
		  parmies[pnum] = parmies[pnum] - 1;
		  if ( parmies[pnum] == 0 )
		    {
		      zeroplanet( pnum, snum );
		      zeroed = TRUE;
		    }
		}
	      else
		parmies[pnum] = parmies[pnum] + 1;
	    }
	  PVUNLOCK(lockword);
	  total = total + 1;
	  
	  if ( total >= num )
	    {
	      /* Done. */
	      cdclrl( MSG_LIN1, 1 );
	      goto cbrk21; /* break 2;*/
	    }
	}
      
      if ( ototal != total )
	{
	  c_strcpy( "Beaming ", cbuf );
	  if ( dirup )
	    appstr( "up from ", cbuf );
	  else
	    appstr( "down to ", cbuf );
	  appstr( pname[pnum], cbuf );
	  appstr( ", ", cbuf );
	  if ( total == 0 )
	    appstr( "no", cbuf );
	  else
	    appint( total, cbuf );
	  appstr( " arm", cbuf );
	  if ( total == 1 )
	    {
	      appchr( 'y', cbuf );
	    }
	  else
	    {
	      appstr( "ies", cbuf );
	    }
	  appstr( " transported, ", cbuf );
	  appint( num - total, cbuf );
	  appstr( " to go.", cbuf );
	  c_putmsg( cbuf, MSG_LIN1 );
	  if ( ototal == -1 )
	    cdrefresh();		/* display the first time */
	  ototal = total;
	}
      
      if ( dirup && parmies[pnum] <= MIN_BEAM_ARMIES )
	{
	  c_putmsg( lastfew, MSG_LIN1 );
	  break;
	}
      
      aston();
      c_sleep( ITER_SECONDS );
      astoff();
    }
 cbrk21:
  
  /* Restore shields. */
  sshup[snum] = oldsshup;
  
  /* Try to display the last bombing message. */
  cdrefresh();
  
  if ( conqed )
    {
      sprintf( cbuf, "You have conquered %s.", pname[pnum] );
      c_putmsg( cbuf, MSG_LIN1 );
    }
  else if ( zeroed )
    c_putmsg( "Sensors show hostile forces eliminated from the planet.",
	     MSG_LIN1 );
  
  return;
  
}


/*  dobomb - bombard a planet (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobomb( snum ) */
void dobomb( int snum )
{
  int pnum, now, entertime, total, ototal, oparmies;
  real x, killprob;
  int oldsshup; 
  char  buf[MSGMAXLINE];
  
  string lastfew="The last few armies are eluding us.";
  string abt="...aborted...";
  
  srmode[snum] = FALSE;
  
  cdclrl( MSG_LIN2, 1 );
  cdclrl(MSG_LIN1, 1);
  
  /* Check for allowability. */
  if ( swarp[snum] >= 0.0 )
    {
      c_putmsg( "We must be orbiting a planet to bombard it.", MSG_LIN1 );
      return;
    }
  pnum = -slock[snum];
  if ( ptype[pnum] == PLANET_SUN || ptype[pnum] == PLANET_MOON ||
      pteam[pnum] == TEAM_NOTEAM || parmies[pnum] == 0 )
    {
      c_putmsg( "There is no one there to bombard.", MSG_LIN1 );
      return;
    }
  if ( pteam[pnum] == steam[snum] )
    {
      c_putmsg( "We can't bomb our own armies!", MSG_LIN1 );
      return;
    }
  if ( pteam[pnum] != TEAM_SELFRULED && pteam[pnum] != TEAM_GOD )
    if ( ! swar[snum][pteam[pnum]] )
      {
	c_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  /* Confirm. */
  sprintf( cbuf, "Press TAB to bombard %s, %d armies:",
	 pname[pnum], parmies[pnum] );
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );
  buf[0] = EOS;
  if ( cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE ) != TERM_EXTRA )
    {
      cdclrl( MSG_LIN1, 1 );
      cdclrl( MSG_LIN2, 1 );
      return;
    }
  
  /* Handle war logic. */
  ssrpwar[snum][pnum] = TRUE;
  if ( pteam[pnum] >= 0 && pteam[pnum] < NUMTEAMS )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      srwar[snum][pteam[pnum]] = TRUE;
      intrude( snum, pnum );
    }
  /* Planets owned by GOD have a special defense system. */
  if ( pteam[pnum] == TEAM_GOD )
    {
      sprintf( cbuf, "That was a bad idea, %s...", spname[snum] );
      c_putmsg( cbuf, MSG_LIN1 );
      damage( snum,  rnduni( 50.0, 100.0 ), KB_LIGHTNING );
      return;
    }
  
  /* Lower shields. */
  oldsshup = sshup[snum];
  sshup[snum] = FALSE;
  
  /* Bombard. */
  total = 0;
  ototal = -1					/* force an update the first time */;
  oparmies = -1;
  grand( &entertime )			/* get start time */;
  while(TRUE)       /*repeat infloop */
    {
      if ( ! stillalive( csnum ) )
	return;
      if ( iochav() )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  break;
	}
      
      cdrefresh();
      
      /* See if it's time to bomb yet. */
      while ((int) fabs (dgrand( (int)entertime, (int *)&now )) >= BOMBARD_GRAND )
	{
	  if ( swfuse[snum] > 0 )
	    {
	      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
	      goto cbrk22; /* break 2;*/
	    }
	  x = BOMBARD_FUEL * (real)(BOMBARD_GRAND / 1000.0);
	  if ( ! usefuel( snum, x, TRUE ) )
	    {
	      c_putmsg( "Not enough fuel to bombard.", MSG_LIN1 );
	      goto cbrk22; /* break 2;*/
	    }
	  /*  entertime = mod( entertime + BOMBARD_GRAND, 24*60*60*1000 );*/
	  grand(&entertime);
	  killprob = (real)((BOMBARD_PROB *
			     ((real) weaeff( snum ) *
			      (real)((real)parmies[pnum]/100.0))) + 0.5 );
	  /*	    cerror(MSG_GOD, "DEBUG: killprob = %d\n", (int) (killprob *10));*/
	  if ( rnd() < killprob )
	    {
	      /*	    cerror(MSG_GOD, "DEBUG: we're in: killprob = %d\n", (int)(killprob * 10));*/
	      PVLOCK(lockword);
	      if ( parmies[pnum] <= MIN_BOMB_ARMIES )
		{
		  /* No more armies left to bomb. */
		  PVUNLOCK(lockword);
		  c_putmsg( lastfew, MSG_LIN1 );
		  goto cbrk22; /* break 2;*/
		}
	      parmies[pnum] = parmies[pnum] - 1;
	      
	      skills[snum] = skills[snum] + BOMBARD_KILLS;
	      ustats[suser[snum]][USTAT_ARMBOMB] =
		ustats[suser[snum]][USTAT_ARMBOMB] + 1;
	      tstats[steam[snum]][TSTAT_ARMBOMB] =
		tstats[steam[snum]][TSTAT_ARMBOMB] + 1;
	      PVUNLOCK(lockword);
	      total = total + 1;
	    }
	  /*	    astservice(0);
		    cdrefresh();
		    c_sleep(ITER_SECONDS);
		    */
	}
      
      if ( parmies[pnum] <= MIN_BOMB_ARMIES )
	{
	  /* No more armies left to bomb. */
	  c_putmsg( lastfew, MSG_LIN1 );
	  break;
	}
      
      if ( parmies[pnum] != oparmies || ototal != total )
	{
	  /* Either our bomb run total or the population changed. */
	  oparmies = parmies[pnum];
	  if ( total == 1 )
	    c_strcpy( "y", buf );
	  else
	    c_strcpy( "ies", buf );
	  sprintf( cbuf, "Bombing %s, %d arm%s killed, %d left.",
		 pname[pnum], total, buf, oparmies );
	  c_putmsg( cbuf, MSG_LIN1 );
	  cdrefresh();
	  if ( ototal == -1 )
	    {
	      cdrefresh();		/* display the first time */
	    }
	  
	  ototal = total;
	}
      
      aston();
      c_sleep( ITER_SECONDS );
      astoff();
    }
 cbrk22:
  ;
  
  /* Restore shields. */
  sshup[snum] = oldsshup;
  
  /* Try to display the last bombing message. */
  cdrefresh();
  
  return;
  
}


/*  doburst - launch a burst of three torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    doburst( snum ) */
void doburst( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < TORPEDO_FUEL )
    {
      c_putmsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
      return;
    }
  
  if ( gettarget( "Torpedo burst: ", MSG_LIN1, 1, &dir, slastblast[snum] ) )
    {
      if ( ! launch( snum, dir, 3, LAUNCH_NORMAL ) )
	c_putmsg( ">TUBES EMPTY<", MSG_LIN2 );
      else
	cdclrl( MSG_LIN1, 1 );
    }
  else
    {
      c_putmsg( "Invalid targeting information.", MSG_LIN1 );
    }

  
  return;
  
}


/*  docloak - cloaking device control */
/*  SYNOPSIS */
/*    int snum */
/*    docloak( snum ) */
void docloak( int snum )
{
  string pmt="Press TAB to engage cloaking device: ";
  string nofuel="Not enough fuel to engage cloaking device.";
  
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );
  
  if ( scloaked[snum] )
    {
      scloaked[snum] = FALSE;
      c_putmsg( "Cloaking device disengaged.", MSG_LIN1 );
      return;
    }
  if ( sefuse[snum] > 0 )
    {
      c_putmsg( "Engines are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < CLOAK_ON_FUEL )
    {
      c_putmsg( nofuel, MSG_LIN1 );
      return;
    }
  
  cdclrl( MSG_LIN1, 1 );
  cbuf[0] = EOS;
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) == TERM_EXTRA )
    {
      if ( cloak( snum ) )
	c_putmsg( "Cloaking device engaged.", MSG_LIN2 );
      else
	c_putmsg( nofuel, MSG_LIN2 );
    }
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*  docoup - attempt to rise from the ashes (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    docoup( snum ) */
void docoup( int snum )
{
  int i, pnum, now, entertime;
  real failprob;
  string nhp="We must be orbiting our home planet to attempt a coup.";
  string conf="Press TAB to try it: ";
  
  cdclrl( MSG_LIN2, 1 );
  
  /* Check for allowability. */
  if ( oneplace( skills[snum] ) < MIN_COUP_KILLS )
    {
      c_putmsg(
	       "Fleet orders require three kills before a coup can be attempted.",
	       MSG_LIN1 );
      return;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( pteam[i] == steam[snum] && parmies[i] > 0 )
      {
	c_putmsg( "We don't need to coup, we still have armies left!",
		 MSG_LIN1 );
	return;
      }
  if ( swarp[snum] >= 0.0 )
    {
      c_putmsg( nhp, MSG_LIN1 );
      return;
    }
  pnum = -slock[snum];
  if ( pnum != homeplanet[steam[snum]] )
    {
      c_putmsg( nhp, MSG_LIN1 );
      return;
    }
  if ( parmies[pnum] > MAX_COUP_ENEMY_ARMIES )
    {
      c_putmsg( "The enemy is still too strong to attempt a coup.",
	       MSG_LIN1 );
      return;
    }
  i = puninhabtime[pnum];
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
	     i );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  /* Now our team can tell coup time for free. */
  tcoupinfo[steam[snum]] = TRUE;
  
  i = couptime[steam[snum]];
  if ( i > 0 )
    {
      sprintf( cbuf, "Our forces need %d more minutes to organize.", i );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  /* Confirm. */
  cdclrl( MSG_LIN1, 1 );
  cbuf[0] = EOS;
  if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
    {
      c_putmsg( "...aborted...", MSG_LIN1 );
      return;
    }
  
  /* Now wait it out... */
  c_putmsg( "Attempting coup...", MSG_LIN1 );
  cdrefresh();
  grand( &entertime );
  while ( dgrand( entertime, &now ) < COUP_GRAND )
    {
      /* See if we're still alive. */
      if ( ! stillalive( csnum ) )
	return;
      
      /* Sleep (and enable asts so the display will work). */
      aston();
      c_sleep( ITER_SECONDS );
      astoff();
    }
  
  cdclrl( MSG_LIN1, 1 );
  PVLOCK(lockword);
  if ( pteam[pnum] == steam[snum] )
    {
      PVUNLOCK(lockword);
      c_putmsg( "Sensors show hostile forces eliminated from the planet.",
	       MSG_LIN2 );
      return;
    }
  
  failprob = parmies[pnum] / MAX_COUP_ENEMY_ARMIES * 0.5 + 0.5;
  if ( rnd() < failprob )
    {
      /* Failed; setup new reorganization time. */
      couptime[steam[snum]] = rndint( 5, 10 );
      PVUNLOCK(lockword);
      c_putmsg( "Coup unsuccessful.", MSG_LIN2 );
      return;
    }
  
  takeplanet( pnum, snum );
  parmies[pnum] = rndint( 10, 20 );		/* create token coup force */
  ustats[suser[snum]][USTAT_COUPS] = ustats[suser[snum]][USTAT_COUPS] + 1;
  tstats[steam[snum]][TSTAT_COUPS] = tstats[steam[snum]][TSTAT_COUPS] + 1;
  PVUNLOCK(lockword);
  c_putmsg( "Coup successful!", MSG_LIN2 );
  
  return;
  
}


/*  docourse - set course */
/*  SYNOPSIS */
/*    int snum */
/*    docourse( snum ) */
void docourse( int snum )
{
  int i, j, what, sorpnum, xsorpnum, newlock, token, count;
  real dir, appx, appy; 
  int ch; 
  
  cdclrl( MSG_LIN1, 2 );

  cbuf[0] = EOS;
  ch = cdgetx( "Come to course: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  delblanks( cbuf );
  if ( ch == TERM_ABORT || cbuf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  newlock = 0;				/* default to no lock */
  fold( cbuf );
  
  what = NEAR_ERROR;
  if ( alldig( cbuf ) == TRUE )
    {
      /* Raw angle. */
      cdclrl( MSG_LIN1, 1 );
      i = 0;
      if ( safectoi( &j, cbuf, i ) )
	{
	  what = NEAR_DIRECTION;
	  dir = (real)mod360( (real)( j ) );
	}
    }
  else if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == TRUE )
    {
      /* Ship. */

      i = 1;
      if ( safectoi( &sorpnum, cbuf, i ) )
	what = NEAR_SHIP;
    }
  else if ( arrows( cbuf, &dir ) )
    what = NEAR_DIRECTION;
  else if ( special( cbuf, &i, &token, &count ) )
    {
      if ( findspecial( snum, token, count, &sorpnum, &xsorpnum ) )
	what = i;
    }
  else if ( planmatch( cbuf, &sorpnum, FALSE ) )
    what = NEAR_PLANET;
  
  switch ( what )
    {
    case NEAR_SHIP:
      if ( sorpnum < 1 || sorpnum > MAXSHIPS )
	{
	  c_putmsg( "No such ship.", MSG_LIN2 );
	  return;
	}
      if ( sorpnum == snum )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
      if ( sstatus[sorpnum] != SS_LIVE )
	{
	  c_putmsg( "Not found.", MSG_LIN2 );
	  return;
	}
      if ( scloaked[sorpnum] )
	{
	  if ( swarp[sorpnum] <= 0.0 )
	    {
	      c_putmsg( "Sensors are unable to lock on.", MSG_LIN2 );
	      return;
	    }
	  appx = rndnor(sx[sorpnum], CLOAK_SMEAR_DIST);
	  appy = rndnor(sy[sorpnum], CLOAK_SMEAR_DIST);
	}
      else
	{
	  appx = sx[sorpnum];
	  appy = sy[sorpnum];
	}
      dir = (real)angle( sx[snum], sy[snum], appx, appy );
      
      /* Give info if he used TAB. */
      if ( ch == TERM_EXTRA )
	infoship( sorpnum, snum );
      else
	cdclrl( MSG_LIN1, 1 );
      break;
    case NEAR_PLANET:
      dir = angle( sx[snum], sy[snum], px[sorpnum], py[sorpnum] );
      if ( ch == TERM_EXTRA )
	{
	  newlock = -sorpnum;
	  infoplanet( "Now locked on to ", sorpnum, snum );
	}
      else
	infoplanet( "Setting course for ", sorpnum, snum );
      break;
    case NEAR_DIRECTION:
      cdclrl( MSG_LIN1, 1 );
      break;
    case NEAR_NONE:
      c_putmsg( "Not found.", MSG_LIN2 );
      return;
      break;
    default:
      /* This includes NEAR_ERROR. */
      c_putmsg( "I don't understand.", MSG_LIN2 );
      return;
      break;
    }
  
  if ( swarp[snum] < 0.0 )		/* if orbitting */
    swarp[snum] = 0.0;		/* break orbit */
  sdhead[snum] = dir;			/* set direction first to avoid */
  slock[snum] = newlock;		/*  a race in display() */
  
  return;
  
}


/*  dodet - detonate enemy torps */
/*  SYNOPSIS */
/*    int snum */
/*    dodet( snum ) */
void dodet( int snum )
{
  cdclrl( MSG_LIN2, 1 );
  
  if ( swfuse[snum] > 0 )
    c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
  else if ( enemydet( snum ) )
    c_putmsg( "detonating...", MSG_LIN1 );
  else
    c_putmsg( "Not enough fuel to fire detonators.", MSG_LIN1 );
  
  return;
  
}


/*  dodistress - send an emergency distress call */
/*  SYNOPSIS */
/*    int snum */
/*    dodistress( snum ) */
void dodistress( int snum )
{
  int i;
  string pmt="Press TAB to send an emergency distress call: ";
  
  cdclrl( MSG_LIN1, 2 );

  cbuf[0] = EOS;
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) == TERM_EXTRA )
    {
      sprintf( cbuf,
	     "sh=%d, dam=%d, fuel=%d, temp=",
	     round(sshields[snum]),
	     round(sdamage[snum]),
	     round(sfuel[snum]) );
      i = round(swtemp[snum]);
      if ( i < 100 )
	appint( i, cbuf );
      else
	appstr( "**", cbuf );
      appchr( '/', cbuf );
      i = round(setemp[snum]);
      if ( i < 100 )
	appint( i, cbuf );
      else
	appstr( "**", cbuf );
      i = sarmies[snum];
      if ( i > 0 )
	{
	  appstr( ", armies=", cbuf );
	  appint( i, cbuf );
	}
      if ( swfuse[snum] > 0 )
	appstr( ", -weapons", cbuf );
      if ( sefuse[snum] > 0 )
	appstr( ", -engines", cbuf );
      
      stormsg( snum, -steam[snum], cbuf );
    }
  
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*  dohelp - display a list of commands */
/*  SYNOPSIS */
/*    int subdcl */
/*    dohelp( subdcl ) */
void dohelp( int subdcl )
{
  int lin, col, tlin;
  int ch;
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#%%-9s#%d#%%s",
	      InfoColor,
	      LabelColor);
	}

  cdclear();
  
  cdclear();
  cprintf(1,0,ALIGN_CENTER, "#%d#%s", LabelColor, "CONQUEST COMMANDS");
  
  lin = 4;
  
  /* Display the left side. */
  tlin = lin;
  col = 4;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "0-9,=", "set warp factor (= is 10)");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "A", "change w/e allocations");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "b", "beam armies");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "B", "bombard a planet");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "C", "cloaking device");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "d,*", "detonate enemy torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "D", "detonate your own torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "E", "send emergency distress call");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "f", "fire phasers");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "F", "fire phasers, same direction");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "h", "this");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "H", "user history");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "i", "information");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "I", "set user options");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "k", "set course");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "K", "try a coup");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "L", "review old messages");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "m", "send a message");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "M", "short/long range sensor toggle");
  
  /* Now do the right side. */
  tlin = lin;
  col = 44;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "N", "change your name");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "o", "come into orbit");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "p", "launch photon torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "P", "launch photon torpedo burst");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "Q", "initiate self-destruct");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "R", "enter repair mode");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "S", "more user statistics");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "t", "engage tractor beams");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "T", "team list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "u", "un-engage tractor beams");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "U", "user statistics");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "W", "set war or peace");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "lower shields");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "raise shields");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "/", "player list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "?", "planet list");
  if ( subdcl )
    {
      tlin++;
	  cprintf(tlin,col,ALIGN_NONE,sfmt, "$", "spawn to DCL");
    }
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "^L", "refresh the screen");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "[RETURN]", "get last info");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "[TAB]", "get next last info");
  
  putpmt( "--- press space when done ---", MSG_LIN2 );
  cdrefresh();
  while ( ! iogtimed( &ch, 1 ) && stillalive( csnum ) )
    ;
  
  return;
  
}


/*  doinfo - do an info command */
/*  SYNOPSIS */
/*    int snum */
/*    doinfo( snum ) */
void doinfo( int snum )
{
  char ch; 
  int i, j, what, sorpnum, xsorpnum, count, token, now[8]; 
  int extra; 
  
  cdclrl( MSG_LIN1, 2 );
  
  cbuf[0] = EOS;
  ch = (char)cdgetx( "Information on: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  extra = ( ch == TERM_EXTRA );
  
  /* Default to what we did last time. */
  delblanks( cbuf );
  fold( cbuf );
  if ( cbuf[0] == EOS )
    {
      c_strcpy( clastinfostr, cbuf );
      if ( cbuf[0] == EOS )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
    }
  else
    c_strcpy( cbuf, clastinfostr );
  
  if ( special( cbuf, &what, &token, &count ) )
    {
      if ( ! findspecial( snum, token, count, &sorpnum, &xsorpnum ) )
	what = NEAR_NONE;
      else if ( extra )
	if ( xsorpnum == 0 )
	  what = NEAR_NONE;
	else
	  sorpnum = xsorpnum;
      
      if ( what == NEAR_SHIP )
	infoship( sorpnum, snum );
      else if ( what == NEAR_PLANET )
	infoplanet( "", sorpnum, snum );
      else
	c_putmsg( "Not found.", MSG_LIN2 );
    }
  else if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == TRUE )
    {
      i = 1;
      safectoi( &j, cbuf, i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( alldig( cbuf ) == TRUE )
    {
      i = 0;
      safectoi( &j, cbuf, i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( planmatch( cbuf, &j, FALSE ) )
    infoplanet( "", j, snum );
  else if ( stmatch( cbuf, "time", FALSE ) )
    {
      getnow( now );
      c_strcpy( "It's ", cbuf );
      appnumtim( now, cbuf );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
    }
  else
    {
      c_putmsg( "I don't understand.", MSG_LIN2 );
      cdmove( MSG_LIN1, 1 );
    }
  
  return;
  
}


/*  dolastphase - do a fire phasers same direction command */
/*  SYNOPSIS */
/*    int snum */
/*    dolastphase( snum ) */
void dolastphase( int snum )
{
  cdclrl( MSG_LIN1, 1 );
  
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN2 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < PHASER_FUEL )
    {
      c_putmsg( "Not enough fuel to fire phasers.", MSG_LIN2 );
      return;
    }
  
  if ( phaser( snum, slastphase[snum] ) )
    cdclrl( MSG_LIN2, 1 );
  else
    c_putmsg( ">PHASERS DRAINED<", MSG_LIN2 );
  
  return;
  
}


/*  domydet - detonate your own torps */
/*  SYNOPSIS */
/*    int snum */
/*    domydet( snum ) */
void domydet( int snum )
{
  int j;
  
  cdclrl( MSG_LIN2, 1 );
  
  c_putmsg( "Detonating...", MSG_LIN1 );
  
  for ( j = 0; j < MAXTORPS; j = j + 1 )
    detonate( snum, j );
  
  return;
  
}


/*  dooption - set user options */
/*  SYNOPSIS */
/*    int snum */
/*    int dodisplay */
/*    dooption( snum, dodisplay ) */
void dooption( int snum, int dodisplay )
{
  int i, tok;
  int ch;
  static int leave = FALSE;
  int top[MAXOPTIONS], sop[MAXOPTIONS]; 
  
  leave = FALSE;
  /* Make some copies of the current ship options. */
  for ( i = 0; i < MAXOPTIONS; i = i + 1)
    {
      sop[i] = soption[snum][i];			/* used in case we abort */
      top[i] = soption[snum][i];			/* used for dispoption() */
    }
  
  while ( stillalive( csnum ) && leave == FALSE)
    {
      /* Display the current options. */
      dispoption( top );
      cdrefresh();
      
      /* Get a character. */
      if ( ! iogtimed( &ch, 1 ) )
	continue; /* next; */
      switch ( ch )
	{
	case TERM_EXTRA:
	  /* Done fooling around, update the user options. */
	  for ( i = 0; i < MAXOPTIONS; i = i + 1)
	    uoption[suser[snum]][i] = top[i];
	  leave = TRUE;
	  break;
	case TERM_ABORT:
	  /* Decided to abort; restore things. */
	  for ( i = 0; i < MAXOPTIONS; i = i + 1)
	    soption[snum][i] = sop[i];
	  if ( dodisplay )
	    {
	      /* Force an update. */
	      stoptimer();
	      display( snum, FALSE );		/* update the display */
	      settimer();
	    }
	  
	  leave = TRUE;
	  break;
	default:
	  if ( getoption( ch, &tok ) )
	    {
	      /* Update temporary. */
	      top[tok] = ! top[tok];
	      
	      /* Copy temporary into ship for display() to use. */
	      soption[snum][tok] = top[tok];
	      
	      if ( dodisplay )
		{
		  /* Force an update. */
		  stoptimer();
		  display( snum, FALSE );
		  settimer();
		}
	    }
	  else
	    cdbeep();
	}
    }
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  doorbit - orbit the ship and print a message */
/*  SYNOPSIS */
/*    int snum */
/*    doorbit( snum ) */
void doorbit( int snum )
{
  int pnum;
  
  if ( ( swarp[snum] == ORBIT_CW ) || ( swarp[snum] == ORBIT_CCW ) )
    infoplanet( "But we are already orbiting ", -slock[snum], snum );
  else if ( ! findorbit( snum, &pnum ) )
    {
      sprintf( cbuf, "We are not close enough to orbit, %s.",
	     spname[snum] );
      c_putmsg( cbuf, MSG_LIN1 );
      cdclrl( MSG_LIN2, 1 );
    }
  else if ( swarp[snum] > MAX_ORBIT_WARP )
    {
      sprintf( cbuf, "We are going to fast to orbit, %s.",
	     spname[snum] );
      c_putmsg( cbuf, MSG_LIN1 );
      sprintf( cbuf, "Maximum orbital insertion velocity is warp %.1f.",
	     oneplace(MAX_ORBIT_WARP) );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  else
    {
      orbit( snum, pnum );
      infoplanet( "Coming into orbit around ", -slock[snum], snum );
    }
  
  return;
  
}


/*  dophase - do a fire phasers command */
/*  SYNOPSIS */
/*    int snum */
/*    dophase( snum ) */
void dophase( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < PHASER_FUEL )
    {
      c_putmsg( "Not enough fuel to fire phasers.", MSG_LIN1 );
      return;
    }
  
  if ( gettarget( "Fire phasers: ", MSG_LIN1, 1, &dir, slastblast[snum] ) )
    {
      if ( phaser( snum, dir ) )
	c_putmsg( "Firing phasers...", MSG_LIN2 );
      else
	c_putmsg( ">PHASERS DRAINED<", MSG_LIN2 );
    }
  else
    {
      c_putmsg( "Invalid targeting information.", MSG_LIN1 );
    }

/*  cdclrl( MSG_LIN1, 1 );*/
  
  return;
  
}


/*  doplanlist - display the planet list for a ship */
/*  SYNOPSIS */
/*    int snum */
/*    doplanlist( snum ) */
void doplanlist( int snum )
{

  if (snum > 0 && snum <= MAXSHIPS)
    planlist( steam[snum], snum );
  else		/* then use user team if user doen't have a ship yet */
    planlist( uteam[cunum], snum );
  
  return;
  
}


/*  doreview - review messages for a ship */
/*  SYNOPSIS */
/*    int snum */
/*    doreview( snum ) */
void doreview( int snum )
{
  int ch;
  int lstmsg;			/* saved last msg in case new ones come in */
  
  if (RMsg_Line == MSG_LIN1)
    {				/* if we don't have an extra msg line,
				   then make sure new msgs don't come
				   in while reviewing */
      
      cmsgok = FALSE;		/* don't want to get msgs when reading
				   old ones.  */
    }

  lstmsg = slastmsg[snum];	/* don't want lstmsg changing while reading old ones. */

  if ( ! review( snum, lstmsg ) )
    {
      c_putmsg( "There are no old messages.", MSG_LIN1 );
      putpmt( "--- press space for more ---", MSG_LIN2 );
      cdrefresh();
      while ( ! iogtimed( &ch, 1 ) && stillalive( csnum ) )
	;
      cdclrl( MSG_LIN1, 2 );
    }

  if (RMsg_Line == MSG_LIN1)
    {
      cmsgok = TRUE;		
    }

  return;
  
}


/*  doselfdest - execute a self-destruct command */
/*  SYNOPSIS */
/*    doselfdest */
void doselfdest(int snum)
{
  
  int entertime, now; 
  string pmt="Press TAB to initiate self-destruct sequence: ";
  
  cdclrl( MSG_LIN1, 2 );

  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
               MSG_LIN1 );
      return;
    }

  cbuf[0] = EOS;
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
    {
      /* Chickened out. */
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  /* See if we want to exit to dcl. */
  /*    cleave = ( 'q' == cbuf[0] || 'Q' == cbuf[0] );*/
  cdclrl( MSG_LIN1, 1 );
  
  /* Set up the destruct fuse. */
  ssdfuse[csnum] = SELFDESTRUCT_FUSE;
  
  gsecs( &entertime );
  
  /* Force a screen update. */
  stoptimer();
  display( csnum, FALSE );
  settimer();
  cmsgok = TRUE;			/* messages are ok in the beginning */
  while ( ssdfuse[csnum] > 0 )
    {
      ssdfuse[csnum] = SELFDESTRUCT_FUSE - dsecs ( entertime, &now );
      /* Display new messages until T-minus 3 seconds. */
      if ( ssdfuse[csnum] < 3 )
	cmsgok = FALSE;
      
      if ( ! stillalive( csnum ) )
	{
	  /* Died in the process. */
	  ssdfuse[csnum] = 0;
	  return;
	}
      
      if ( iochav() )
	{
	  /* Got a new character. */
	  grand( &cmsgrand );
	  cdclrl( MSG_LIN1, 2 );
	  if ( iogchar() == TERM_ABORT )
	    {
	      ssdfuse[csnum] = 0;
	      c_putmsg( "Self destruct has been canceled.", MSG_LIN1 );
	      return;
	    }
	  else
	    {
	      c_putmsg( "Press ESCAPE to abort self destruct.", MSG_LIN1 );
	      cdbeep();
	      cdrefresh();
	    }
	}
      aston();			/* enable asts so the display will work */
      c_sleep( ITER_SECONDS );
      astoff();
    }
  cmsgok = FALSE;			/* turn off messages */
  
  if ( *dstatus == DS_LIVE )
    {
      if ( dist(sx[csnum], sy[csnum], *dx, *dy) <= DOOMSDAY_KILL_DIST )
	{
	  *dstatus = DS_OFF;
	  stormsg( MSG_DOOM, MSG_ALL, "AIEEEEEEEE!" );
	  killship( csnum, KB_GOTDOOMSDAY );
	}
      else
	killship( csnum, KB_SELF );
    }
  else
    killship( csnum, KB_SELF );
  
  return;
  
}


/*  doshields - raise or lower shields */
/*  SYNOPSIS */
/*    int snum */
/*    int up */
/*    doshields( snum, up ) */
void doshields( int snum, int up )
{
  
  sshup[snum] = up;
  if ( up )
    {
      srmode[snum] = FALSE;
      c_putmsg( "Shields raised.", MSG_LIN1 );
    }
  else
    c_putmsg( "Shields lowered.", MSG_LIN1 );
  cdclrl( MSG_LIN2, 1 );
  
  return;
  
}


/*  doteamlist - display the team list for a ship */
/*  SYNOPSIS */
/*    int team */
/*    doteamlist( team ) */
void doteamlist( int team )
{
  int ch;
  
  cdclear();
  while ( stillalive( csnum ) )
    {
      teamlist( team );
      putpmt( "--- press space when done ---", MSG_LIN2 );
      cdrefresh();
      if ( iogtimed( &ch, 1 ) )
	break;
    }
  return;
  
}


/*  dotorp - launch single torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    dotorp( snum ) */
void dotorp( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < TORPEDO_FUEL )
    {
      c_putmsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
      return;
    }
  if ( gettarget( "Launch torpedo: ", MSG_LIN1, 1, &dir, slastblast[snum] ) )
    {
      if ( ! launch( snum, dir, 1, LAUNCH_NORMAL ) )
	c_putmsg( ">TUBES EMPTY<", MSG_LIN2 );
      else
	cdclrl( MSG_LIN1, 1 );
    }
  else
    {
      c_putmsg( "Invalid targeting information.", MSG_LIN1 );
    }
  
  return;
  
}


/*  dotow - attempt to tow another ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dotow( snum ) */
void dotow( int snum )
{
  char ch;
  int i, other;
  
  cdclrl( MSG_LIN1, 2 );
  if ( stowedby[snum] != 0 )
    {
      c_strcpy( "But we are being towed by ", cbuf );
      appship( stowing[snum], cbuf );
      appchr( '!', cbuf );
      return;
    }
  if ( stowing[snum] != 0 )
    {
      c_strcpy( "But we're already towing ", cbuf );
      appship( stowing[snum], cbuf );
      appchr( '.', cbuf );
      return;
    }
  cbuf[0] = EOS;
  ch = (char)cdgetx( "Tow which ship? ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  cdclrl( MSG_LIN1, 1 );
  if ( ch == TERM_ABORT )
    return;
  
  i = 0;
  safectoi( &other, cbuf, i );		/* ignore status */
  cbuf[0] = EOS;
  
  PVLOCK(lockword);
  if ( other < 1 || other > MAXSHIPS )
    c_strcpy( "No such ship.", cbuf );
  else if ( sstatus[other] != SS_LIVE )
    c_strcpy( "Not found.", cbuf );
  else if ( other == snum )
    c_strcpy( "We can't tow ourselves!", cbuf );
  else if ( dist( sx[snum], sy[snum], sx[other], sy[other] ) > TRACTOR_DIST )
    c_strcpy( "That ship is out of tractor range.", cbuf );
  else if ( swarp[other] < 0.0 )
    c_strcpy( "You can't tow a ship out of orbit.", cbuf );
  else if ( sqrt( pow(( (real) (sdx[snum] - sdx[other]) ), (real) 2.0) +
		  pow( (real) ( sdy[snum] - sdy[other] ), (real) 2.0 ) ) / 
	    ( MM_PER_SEC_PER_WARP * ITER_SECONDS ) > MAX_TRACTOR_WARP ) 
    sprintf( cbuf, "That ships relative velocity is higher than %2.1f.",
	     MAX_TRACTOR_WARP );
  else if ( stowing[other] != 0 || stowedby[other] != 0 )
    c_strcpy(
	     "There seems to be some interference with the tractor beams...",
	     cbuf );
  else
    {
      stowedby[other] = snum;
      stowing[snum] = other;
      c_strcpy( "Tractor beams engaged.", cbuf );
    }
  PVUNLOCK(lockword);
  c_putmsg( cbuf, MSG_LIN2 );
  
  return;
  
}


/*  dountow - release a tow (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dountow( snum ) */
void dountow( int snum )
{
  int entertime, now;
  int warsome; 
  
  cdclrl( MSG_LIN1, 2 );
  if ( stowedby[snum] != 0 )
    {
      /* If we're at war with him or he's at war with us, make it */
      /*  hard to break free. */
      warsome = ( satwar( snum, stowedby[snum]) );
      if ( warsome )
	{
	  grand( &entertime );
	  while ( dgrand( entertime, &now ) < BREAKAWAY_GRAND )
	    {
	      if ( ! stillalive( csnum ) )
		return;
	      aston();
	      c_sleep( ITER_SECONDS );
	      astoff();
	    }
	}
      if ( warsome && ( rnd() > BREAKAWAY_PROB ) )
	c_putmsg( "Attempt to break free failed.", MSG_LIN1 );
      else
	{
	  c_strcpy( "Breaking free from ship ", cbuf );
	  appship( stowedby[snum], cbuf );
	  PVLOCK(lockword);
	  if ( stowedby[snum] != 0 )
	    {
	      /* Coast to a stop. */
	      shead[snum] = shead[stowedby[snum]];
	      swarp[snum] = swarp[stowedby[snum]];
	      
	      /* Release the tow. */
	      if ( stowing[stowedby[snum]] != 0 )
		stowing[stowedby[snum]] = 0;
	      stowedby[snum] = 0;
	    }
	  PVUNLOCK(lockword);
	  appchr( '.', cbuf );
	  c_putmsg( cbuf, MSG_LIN1 );
	}
    }
  else if ( stowing[snum] != 0 )
    {
      c_strcpy( "Tow released from ship ", cbuf );
      appship( stowing[snum], cbuf );
      PVLOCK(lockword);
      if ( stowing[snum] != 0 )
	{
	  /* Set other ship coasting. */
	  shead[stowing[snum]] = shead[snum];
				/* only set warp if valid JET - 9/15/97 */
	  if (swarp[snum] >= 0.0)
	    swarp[stowing[snum]] = swarp[snum];
	  
	  /* Release the tow. */
	  if ( stowedby[stowing[snum]] != 0 )
	    stowedby[stowing[snum]] = 0;
	  stowing[snum] = 0;
	}
      PVUNLOCK(lockword);
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
    }
  else
    c_putmsg( "No tractor beam activity detected.", MSG_LIN1 );
  
  return;
  
}


/*  dowar - declare war or peace */
/*  SYNOPSIS */
/*    int snum */
/*    dowar( snum ) */
void dowar( int snum )
{
  int i, entertime, now; 
  int tuwar[NUMTEAMS], dowait;
  int ch;
  const int POffset = 47, WOffset = 61;
  
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    tuwar[i] = swar[snum][i];
  
  cdclrl( MSG_LIN1, 2 );
  
  cdputs(
	 "Press TAB when done, ESCAPE to abort:  Peace: # # # #  War: # # # #", 
	 MSG_LIN1, 1 );
  
  while ( stillalive( csnum ) )
    {
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( tuwar[i] )
	  {
	    cdput( ' ', MSG_LIN1, POffset + (i*2) );
	    if ( srwar[snum][i] )
	      ch = chrteams[i];
	    else
	      ch = (char)tolower(chrteams[i]);
	    cdput( ch, MSG_LIN1, WOffset + (i*2) );
	  }
	else
	  {
	    cdput( (char)tolower(chrteams[i]), MSG_LIN1, POffset + (i*2) );
	    cdput( ' ', MSG_LIN1, WOffset+(i*2) );
	  }
      cdrefresh();
      if ( iogtimed( &ch, 1 ) == FALSE )
	{
	  continue; /* next; */
	}
      
      ch = (char)tolower( ch );
      if ( ch == TERM_ABORT )
	break;
      if ( ch == TERM_EXTRA )
	{
	  /* Now update the war settings. */
	  dowait = FALSE;
	  for ( i = 0; i < NUMTEAMS; i = i + 1 )
	    {
	      if ( tuwar[i] && ! swar[snum][i] )
		dowait = TRUE;
	      uwar[suser[snum]][i] = tuwar[i];
	      swar[snum][i] = tuwar[i];
	    }
	  
	  /* Only check for computer delay when flying. */
	  if ( sstatus[snum] != SS_RESERVED && dowait )
	    {
	      /* We've set war with at least one team, stall a little. */
	      c_putmsg(
		       "Reprogramming the battle computer, please stand by...",
		       MSG_LIN2 );
	      cdrefresh();
	      grand( &entertime );
	      while ( dgrand( entertime, &now ) < REARM_GRAND )
		{
		  /* See if we're still alive. */
		  if ( ! stillalive( csnum ) )
		    return;
		  
		  /* Sleep (and enable asts so the display will work). */
		  aston();
 		  c_sleep( ITER_SECONDS );
		  astoff();
		}
	    }
	  break;
	}
      
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( ch == (char)tolower( chrteams[i] ) )
	  {
	    if ( ! tuwar[i] || ! srwar[snum][i] )
	      {
		tuwar[i] = ! tuwar[i];
		goto ccont1;	/* next 2  */
	      }
	    break;
	  }
      cdbeep();
      
    ccont1:				/* goto  */
      ;
    }
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  dowarp - set warp factor */
/*  SYNOPSIS */
/*    int snum */
/*    real warp */
/*    dowarp( snum, warp ) */
void dowarp( int snum, real warp )
{
  real mw;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( sdwarp[snum] == 0.0 && warp != 0.0 )
    {
      /* See if engines are working. */
      if ( sefuse[snum] > 0 )
	{
	  c_putmsg( "Engines are currently overloaded.", MSG_LIN1 );
	  return;
	}
      
      /* No charge if already warp 0. */
      if ( usefuel( snum, ENGINES_ON_FUEL, FALSE ) == FALSE)
	{
	  c_putmsg( "We don't have enough fuel.", MSG_LIN1 );
	  return;
	}
      
      /* Don't stop repairing if changing to warp 0. */
      srmode[snum] = FALSE;
    }
  
  /* If orbitting, break orbit. */
  if ( swarp[snum] < 0.0 )
    {
      swarp[snum] = 0.0;
      slock[snum] = 0;
      sdhead[snum] = shead[snum];
    }
  
  /* Handle ship limitations. */
  sdwarp[snum] = min( warp, warplim[steam[snum]] );
  
  sprintf( cbuf, "Warp %d.", (int) sdwarp[snum] );
  c_putmsg( cbuf, MSG_LIN1 );
  
  /* Warn about damage limitations. */
  mw = maxwarp( snum );
  if ( around( sdwarp[snum] ) > mw )
    {
      sprintf( cbuf, "(Due to damage, warp is currently limited to %.1f.)",
	     mw );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  
  return;
  
}


/*  getoption - decode char into option */
/*  SYNOPSIS */
/*    int flag, getoption */
/*    char ch */
/*    int tok */
/*    flag = getoption( ch, tok ) */
int getoption( char ch, int *tok )
{
  switch ( ch )
    {
    case 'g':
      *tok = OPT_PHASERGRAPHICS;
      break;
    case 'p':
      *tok = OPT_PLANETNAMES;
      break;
    case 'a':
      *tok = OPT_ALARMBELL;
      break;
    case 'i':
      *tok = OPT_INTRUDERALERT;
      break;
    case 'n':
      *tok = OPT_NUMERICMAP;
      break;
    case 't':
      *tok = OPT_TERSE;
      break;
    case 'e':
      *tok = OPT_EXPLOSIONS;
      break;
    default:
      *tok = 0;
      return ( FALSE );
    }
  return ( TRUE );
  
}


/*  gretds - block letter "greetings..." */
/*  SYNOPSIS */
/*    gretds */
void gretds()
{
  
  int col,lin;
  string g1=" GGG   RRRR   EEEEE  EEEEE  TTTTT   III   N   N   GGG    SSSS";
  string g2="G   G  R   R  E      E        T      I    NN  N  G   G  S";
  string g3="G      RRRR   EEE    EEE      T      I    N N N  G       SSS";
  string g4="G  GG  R  R   E      E        T      I    N  NN  G  GG      S  ..  ..  ..";
  string g5=" GGG   R   R  EEEEE  EEEEE    T     III   N   N   GGG   SSSS   ..  ..  ..";
  
  col = (int)(cmaxcol-strlen(g5)) / (int)2;
  lin = 1;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g1);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g2);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g3);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g4);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g5);
  
  return;
  
}


/*  menu - main user menu (DOES LOCKING) */
/*  SYNOPSIS */
/*    menu */
void menu(void)
{
  
  int i, lin, col, sleepy, countdown;
  int ch;
  int lose, oclosed, switchteams, multiple, redraw;
  int playrv;
  char *if1="Suddenly  a  sinister,  wraithlike  figure appears before you";
  char *if2="seeming to float in the air.  In a low,  sorrowful  voice  he";
  char *if3="says, \"Alas, the very nature of the universe has changed, and";
  char *if4="your ship cannot be found.  All must now pass away.\"  Raising";
  char *if5="his  oaken  staff  in  farewell,  he fades into the spreading";
  char *if6="darkness.  In his place appears a  tastefully  lettered  sign";
  char *if7="reading:";
  char *if8="INITIALIZATION FAILURE";
  char *if9="The darkness becomes all encompassing, and your vision fails.";
  
  EnableSignalHandler();	/* enable trapping of interesting signals */
  
  /* Initialize statistics. */
  initstats( &sctime[csnum], &setime[csnum] );
  
  /* Log this entry into the Game. */
  loghist( cunum );
  
  /* Set up a few ship characteristics here rather than in initship(). */
  suser[csnum] = cunum;
  steam[csnum] = uteam[cunum];
  spid[csnum] = cpid;
  for ( i = 0; i < MAXOPTIONS; i = i + 1 )
    soption[csnum][i] = uoption[cunum][i];
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      srwar[csnum][i] = FALSE;
      swar[csnum][i] = uwar[cunum][i];
    }
  stcpn( upname[cunum], spname[csnum], MAXUSERPNAME );
  
  /* Set up some things for the menu display. */
  switchteams = uooption[cunum][OOPT_SWITCHTEAMS];
  multiple = uooption[cunum][OOPT_MULTIPLE];
  oclosed = *closed;
  cleave = FALSE;
  redraw = TRUE;
  sleepy = 0;
  countdown = 0;
  playrv = FALSE;
  
  do                 
    {
      /* Make sure things are proper. */
      if (playrv == ERR) 
	{
	  if ( csnum < 1 || csnum > MAXSHIPS )
	    lose = TRUE;
	  else if ( spid[csnum] != cpid )
	    lose = TRUE;
	  else if ( sstatus[csnum] != SS_RESERVED )
	    {
	      clog( "menu(): Ship %d no longer reserved.", csnum );
	      lose = TRUE;
	    }
	  else
	    lose = FALSE;
	}
      else
	lose = FALSE;

      if ( lose )				/* again, Jorge? */
	{
	  /* We reincarnated or else something bad happened. */
	  lin = 7;
	  col = 11;
	  cdclear();;
	  cdredo();;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if1);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if2);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if3);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if4);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if5);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if6);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if7);
	  lin+=2;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor | A_BLINK, if8);
	  lin+=2;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if9);
	  ioeat();
	  cdmove( 1, 1 );
	  cdrefresh();
	  return;
	}
      
      /* Some simple housekeeping. */
      if ( multiple != uooption[cunum][OOPT_MULTIPLE] )
	{
	  multiple = ! multiple;
	  redraw = TRUE;
	}
      
      if ( switchteams != uooption[cunum][OOPT_SWITCHTEAMS])
	{
	  switchteams = uooption[cunum][OOPT_SWITCHTEAMS];
	  redraw = TRUE;
	}
      if ( oclosed != *closed )
	{
	  oclosed = ! oclosed;
	  redraw = TRUE;
	}
      if ( redraw )
	{
	  conqds( multiple, switchteams );
	  redraw = FALSE;
	}
      else
	cdclrl( MSG_LIN1, 2 );
      
      userline( -1, -1, cbuf, FALSE, TRUE );
      attrset(LabelColor);
      cdputs( cbuf, MSG_LIN1, 1 );
      userline( cunum, 0, cbuf, FALSE, TRUE );
      attrset(A_BOLD);
      cdputs( cbuf, MSG_LIN2, 1 );
      attrset(0);
      
      cdmove( 1, 1 );
      cdrefresh();
      
      /* Try to kill the driver if we started one the last time */
      /*  we played and we've been in the menu long enough. */
      if ( countdown > 0 )
	{
	  countdown = countdown - 1;
	  if ( countdown <= 0 )
	    drkill();
	}
      
      /* Reset up the destruct fuse. */
      ssdfuse[csnum] = -TIMEOUT_PLAYER;
      
      /* Get a char with timeout. */
      if ( ! iogtimed( &ch, 1 ) )
	{
	  /* We get here if a char hasn't been typed. */
	  sleepy = sleepy + 1;
	  if ( sleepy > 300 )
	    break;
	  continue; /* next */
	}
      
      /* Got a character, zero timeout. */
      sleepy = 0;
      switch ( ch )
	{
	case 'e':
	  playrv = play();
	  if ( childpid != 0 )
	    countdown = 15;
	  else
	    countdown = 0;
	  redraw = TRUE;
	  break;
	case 'h':
	  helplesson();
	  redraw = TRUE;
	  break;
	case 'H':
	  histlist( FALSE );
	  redraw = TRUE;
	  break;
	case 'I':
	  dooption( csnum, FALSE );
	  break;
	case 'L':
	  doreview( csnum );
	  break;
	case 'n':
	  if ( ! cnewsfile )
	    cdbeep();
	  else
	    {
	      news();
	      redraw = TRUE;
	    }
	  break;
	case 'N':
	  pseudo( cunum, csnum );
	  break;
	case 'r':
	  if ( multiple )
	    cdbeep();
	  else
	    {
	      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
		if ( sstatus[i] == SS_LIVE ||
		    sstatus[i] == SS_ENTERING )
		  if ( suser[i] == cunum )
		    break;
	      if ( i <= MAXSHIPS )
		cdbeep();
	      else
		{
		  cdclrl( MSG_LIN1, 2 );
		  cdrefresh();
		  if ( confirm() )
		    {
		      resign( cunum );
		      break;
		    }
		}
	    }
	  break;
	case 's':
	  if ( ! multiple && ! switchteams )
	    cdbeep();
	  else
	    {
	      steam[csnum] = modp1( steam[csnum]+1, NUMTEAMS );
	      uteam[cunum] = steam[csnum];
	      swar[csnum][steam[csnum]] = FALSE;
	      uwar[cunum][uteam[cunum]] = FALSE;
	    }
	  break;
	case 'S':
	  userstats( FALSE, 0 ); /* we're never really neutral ;-) - dwp */
	  redraw = TRUE;
	  break;
	case 'T':
	  doteamlist( steam[csnum] );
	  redraw = TRUE;
	  break;
	case 'U':
	  userlist( FALSE, 0 );
	  redraw = TRUE;
	  break;
	case 'W':
	  dowar( csnum );
	  redraw = TRUE;
	  break;
	case 'q':
	case 'Q':
	  cleave = TRUE;	
	  break;
	case '/':
	  playlist( FALSE, FALSE, 0 );
	  redraw = TRUE;
	  break;
	case '?':
	  doplanlist( 0 );
	  redraw = TRUE;
	  break;
	case '\014':	/* ^L */
	  cdredo();
	  redraw = TRUE;
	  break;
	case ' ':
	case TERM_NORMAL:           
	  /* Do nothing. */
	  break;
	default:
	  cdbeep();
	  break;
	}
    }
  while ( stillalive( csnum ) &&  !cleave );
  
  /* Make our ship available for others to use. */
  if ( sstatus[csnum] == SS_RESERVED )
    {
      conqstats( csnum );
      PVLOCK(lockword);
      ssdfuse[csnum] = 0;
      sstatus[csnum] = SS_OFF;
      PVUNLOCK(lockword);
    }
  
  return;
  
}


/*  newship - create a new ship for a user (DOES LOCKING) */
/*  SYNOPSIS */
/*    int status, newship, unum, snum */
/*    int flag, newship */
/*    flag = newship( unum, snum ) */
int newship( int unum, int *snum )
{
  int i, j, k, l, system; 
  int fresh;
  int selectnum;
  int vec[MAXSHIPS];
  char cbuf[MSGMAXLINE];
  char cbuf2[MSGMAXLINE];
  char selectship[MSGMAXLINE];
  int availlist[MAXSHIPS], numavail;
  int numvec = 0;
  int ch;

  PVLOCK(lockword);
  
  sstatus[*snum] = SS_ENTERING;		/* show intent to fly */

  fresh = TRUE;				/* assume we want a fresh ship*/
  
  /* Count number of his ships flying. */
  j = 0;
  numvec = 0;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] == SS_LIVE || sstatus[i] == SS_ENTERING )
      if ( suser[i] == unum && *snum != i )
	{
	  j++;
	  vec[numvec++] = i;
	}

  PVUNLOCK(lockword);

  if ( ! uooption[unum][OOPT_MULTIPLE] )
    {
      /* Isn't a multiple; see if we need to reincarnate. */
      if ( j > 0 )
	{
	  /* Need to reincarnate. */
	  cdclear();
	  cdredo();

	  i = MSG_LIN2/2;
	  j = 9;

	  if (CheckPid(spid[vec[0]]) == FALSE)
	    {			/* it's available */
	      attrset(InfoColor);
	      cdputs( "You're already playing on another ship." , i, j );
	      cbuf[0] = EOS;
	      if ( cdgetx( "Press TAB to reincarnate to this ship: ",
			   i + 1, j, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
		{
		  sstatus[*snum] = SS_RESERVED;
		  attrset(0);
		  return ( FALSE );
		}
		  attrset(0);
	    }
	  else
	    {
	      sprintf(cbuf, "You're already playing on another ship (pid=%d).",
		      spid[vec[0]]);
	      cprintf(i,j,ALIGN_NONE,"#%d#%s",InfoColor, cbuf);
	      
	      sstatus[*snum] = SS_RESERVED;
	      putpmt( "--- press any key ---", MSG_LIN2 );

	      cdrefresh();
	      iogchar();
	      return ( FALSE );
	    }


	  /* Look for a live ship for us to take. */
	  PVLOCK(lockword);
	  for ( i = 1; i <= MAXSHIPS; i = i + 1)
	    if ( suser[i] == unum && sstatus[i] == SS_LIVE )
	      {
		fresh = FALSE;
		sstatus[*snum] = SS_OFF;
		*snum = i;
		spid[*snum] = cpid;
		sstatus[*snum] = SS_ENTERING;
		break;
	      }
	  PVUNLOCK(lockword);
	}
    }
  else
    {				/* a multiple, so see what's available */
      cdclear();
      cdrefresh();

      while (TRUE)
	{

	  cdclra(0, 0, MSG_LIN1 + 2, cdcols() - 1);

	  PVLOCK(lockword);
	  
	  /* Count number of his ships flying. */
	  j = 0;
	  numvec = 0;
	  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
	    if ( sstatus[i] == SS_LIVE || sstatus[i] == SS_ENTERING )
	      if ( suser[i] == unum && *snum != i )
		{
		  j++;
		  vec[numvec++] = i;
		}
	  
	  PVUNLOCK(lockword);

	  numavail = 0;
	  for (k=0; k < numvec; k++)
	    {
	      if (CheckPid(spid[vec[k]]) == FALSE)
		{
		  /* no pid, so available */
		  availlist[numavail++] = k;
		}
	    }

	  /* Is a multiple, max ships already in and no ships to
	     reincarnate too */
	  if ( j >= umultiple[unum] && numavail == 0)
	    {
	      sstatus[*snum] = SS_RESERVED;
	      cdclear();
	      cdredo();
	      i = MSG_LIN2/2;
	      cdputc(
		     "I'm sorry, but your playing on too many ships right now.", i );
	      i = i + 1;
	      c_strcpy( "You are only allowed to fly ", cbuf );
	      j = umultiple[unum];
	      appint( j, cbuf );
	      appstr( " ship", cbuf );
	      if ( j != 1 )
		appchr( 's', cbuf );
	      appstr( " at one time.", cbuf );
	      cdputc( cbuf, i );
	      cdrefresh();
	      c_sleep( 2.0 );
	      sstatus[*snum] = SS_RESERVED;

	      return ( FALSE );
	    }
	  
	  
	  if (numavail > 0)
	    {
	      /* we need to display a menu allowing the user to reincarnate
		 to an existing (but vacant) ship, or if he/she has slots
		 left, enter a new ship.
		 */
	      
	      
	      i = 3;
	      
	      cprintf(i++, 0, ALIGN_CENTER, 
		      "#%d#The following ship(s) are available for you to reincarnate to.", 
		      InfoColor);
	      
	      cbuf[0] = '\0';
	      for (k=0; k < numavail; k++)
		{
		  sprintf(cbuf2, "%d ", vec[availlist[k]]);
		  strcat(cbuf, cbuf2);
		}

	      if (j < umultiple[unum])
		{
		  cprintf(MSG_LIN1, 0, ALIGN_LEFT, 
			  "#%d#Enter a ship number, or press [TAB] to create a new one.",
			  NoColor);

                  cprintf(MSG_LIN2, 0, ALIGN_LEFT,
                          "#%d#[RETURN] to quit.",
                          NoColor);
		}
	      else
		cprintf(MSG_LIN1, 0, ALIGN_LEFT,
			"#%d#Enter a ship number to reincarnate to.",
			NoColor);

	      /* Now list the ships */

	      i++; i++;
	      cprintf(i++, 0, ALIGN_CENTER,
		      "#%d#Ship Number: #%d#%s",
		      NoColor | A_BOLD,
		      GreenLevelColor,
		      cbuf);

	      cdmove(0, 0);
	      cdrefresh();

	      if (iogtimed(&ch, 1))
		{
		  iBufPutc(ch);	/* stuff the char back in */
		  selectship[0] = EOS;
		  l = cdgetx("Ship Number: ", i, 1, TERMS, selectship, MSGMAXLINE / 2);

		  if (l == TERM_EXTRA || l == TERM_NORMAL)
		    {
		      if (strlen(selectship))
			{
			  selectnum = atoi(selectship);
			  if (selectnum != 0 && selectnum <= MAXSHIPS)
			    { /* See if it's valid */
			      int found = FALSE;
			      
			      for (k=0; k < numavail && found == FALSE ; k++)
				{
				  if (vec[availlist[k]] == selectnum)
				    found = TRUE;
				}
			      if (found  == TRUE)
				{
				  PVLOCK(lockword);
				  sstatus[*snum] = SS_OFF;
				  *snum = selectnum;
				  fresh = FALSE;
				  spid[*snum] = cpid;
				  sstatus[*snum] = SS_ENTERING;
				  PVUNLOCK(lockword);
				  break;
				}
			    }
			}
		      else  /* if strlen(selectship) */ 
			{ 
				/* if selectship was empty and term =
				   TERM_NORMAL, quit */
			  if (l == TERM_NORMAL)
			    {
			      sstatus[*snum] = SS_RESERVED;
			      
			      return(FALSE);
			    }
			  
			  
			  if ( j < umultiple[unum])
			    {
			      fresh = TRUE;
			      break;
			    }
			  else
			    {
			      cdbeep();
			    }
			}
		    }
		}
	    }
	  else
	    {			/* nothing available */
	      fresh = TRUE;
	      break;
	    }

	} /* back to the top... */
	      
    }
  
  /* Figure out which system to enter. */
  if ( fresh )
    {
      system = steam[*snum];
      if ( ! capentry( *snum, &system ) )
	{
	  sstatus[*snum] = SS_RESERVED;
	  return ( ERR );
	}
    }
  
  PVLOCK(lockword);
  
  /* If necessary, initalize the ship */
  if ( fresh )
    {
      initship( *snum, unum );
      
      /* Randomly position the ship near the home sun (or planet). */
      if ( pprimary[homeplanet[system]] == homesun[system] )
	i = homesun[system];
      else
	i = homeplanet[system];
      putship( *snum, px[i], py[i] );
      sdhead[*snum] = rnduni( 0.0, 359.9 );
      shead[*snum] = sdhead[*snum];
      sdwarp[*snum] = (real) rndint( 2, 5 ) ;/* #~~~ this is a kludge*/
      slock[*snum] = -homeplanet[system];
    }
  else
    {				/* if we're reincarnating, skip any
				   messages that might have been sent
				   while we were gone, if enabled */
      if (conf_ClearOldMsgs == TRUE)
	{
	  PVLOCK(lockmesg);
	  slastmsg[*snum] = *lastmsg;
	  salastmsg[*snum] = slastmsg[*snum];
	  PVUNLOCK(lockmesg);
	}
    }
      
  srobot[*snum] = FALSE;
  saction[*snum] = 0;
  
  
  /* Straighten out the ships deltas. */
  fixdeltas( *snum );
  
  /* Finally, turn the ship on. */
  sstatus[*snum] = SS_LIVE;
  
  PVUNLOCK(lockword);
  
  return ( TRUE );
  
}


/*  play - play the game */
/*  SYNOPSIS */
/*    play */
int play()
{
  int laststat, now;
  int ch, rv;
  char msgbuf[128];
  
  /* Can't carry on without a vessel. */
  if ( (rv = newship( cunum, &csnum )) != TRUE)
    return(rv);
  
  drstart();				/* start a driver, if necessary */
  ssdfuse[csnum] = 0;				/* zero self destruct fuse */
  grand( &cmsgrand );			/* initialize message timer */
  cleave = FALSE;				/* assume we won't want to bail */
  credraw = TRUE;				/* want redraw first time */
  cdisplay = TRUE;				/* ok to display */
  cmsgok = TRUE;				/* ok to get messages */
  cdclear();				/* clear the display */
  cdredo();					/*  (quickly) */
  stoptimer();
  display( csnum, FALSE );			/* update the screen manually */
  gsecs( &laststat );			/* initialize stat timer */
  astoff();					/* disable before setting timer */
  settimer();				/* setup for next second */
  
  
  /* Tell everybody, we're here */

  sprintf(msgbuf, "%c%d (%s) has entered the game.",
	  chrteams[steam[csnum]],
	  csnum,
	  spname[csnum]);
  
  stormsg(MSG_COMP, MSG_ALL, msgbuf);
  
  /* While we're alive, field commands and process them. */
  while ( stillalive( csnum ) )
    {
      /* Make sure we still control our ship. */
      if ( spid[csnum] != cpid )
	break;
      
      /* Get a char with one second timeout. */
      if ( iogtimed( &ch, 1 ) )
	{
	  if (RMsg_Line == MSG_LIN1)
	    cmsgok = FALSE;	/* off if we  have no msg line */
	  
#ifdef ENABLE_MACROS
	  if (DoMacro(ch) == TRUE)
	    {
	      while (iBufEmpty() == FALSE)
		{
		  ch = iBufGetCh();
		  command( ch );
		}
	    }
	  else
	    command( ch );
#else
	  command( ch );
#endif
	  
	  grand( &cmsgrand );
	  cmsgok = TRUE;
	  cdrefresh();
	}
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( csnum );
	  laststat = now;
	}
    }
  
  cdisplay = FALSE;
  conqstats( csnum );
  upchuck();
  
  /* Asts are still enabled, simply cancel the next screen update. */
  stoptimer();
  /*    aston();					/* enable asts again */
  
  dead( csnum, cleave );
  
  return(TRUE);
  
}


/*  welcome - entry routine */
/*  SYNOPSIS */
/*    int flag, welcome */
/*    int unum */
/*    flag = welcome( unum ) */
int welcome( int *unum )
{
  int i, team, col; 
  char name[MAXUSERNAME];
  
  string sorry1="I'm sorry, but the game is closed for repairs right now.";
  string sorry2="I'm sorry, but there is no room for a new player right now.";
  string sorry3="I'm sorry, but you are not allowed to play right now.";
  string sorryn="Please try again some other time.  Thank you.";
  char * selected_str="You have been selected to command a";
  char * starship_str=" starship.";
  char * prepare_str="Prepare to be beamed aboard...";
  
  col=0;
  glname( name );
  if ( ! gunum( unum, name ) )
    {
      /* Must be a new player. */
      cdclear();
      cdredo();
      if ( *closed )
	{
	  /* Can't enroll if the game is closed. */
      cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
      cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 2.0 );
	  return ( FALSE );
	}
      team = rndint( 0, NUMTEAMS - 1 );
      cbuf[0] = EOS;
      apptitle( team, cbuf );
      appchr( ' ', cbuf );
      i = strlen( cbuf );
      appstr( name, cbuf );
      cbuf[i] = (char)toupper( cbuf[i] );
      if ( ! c_register( name, cbuf, team, unum ) )
	{
      cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry2 );
      cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 2.0 );
	  return ( FALSE );
	}
      gretds();
      if ( vowel( tname[team][0] ) )
      	cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s%c #%d#%s #%d#%s",
			InfoColor,selected_str,'n',A_BOLD,tname[team],
			InfoColor,starship_str);
	  else
      	cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s #%d#%s #%d#%s",
			InfoColor,selected_str,A_BOLD,tname[team],
			InfoColor,starship_str);
      cprintf(MSG_LIN2/2+1,0,ALIGN_CENTER,"#%d#%s",
		InfoColor, prepare_str );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 3.0 );
    }
  
  /* Must be special to play when closed. */
  if ( *closed && ! uooption[*unum][OOPT_PLAYWHENCLOSED] )
    {
      cdclear();
      cdredo();
      cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
      cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  /* Can't play if on the shit list. */
  if ( uooption[*unum][OOPT_SHITLIST] )
    {
      cdclear();
      cdredo();
      cdputc( sorry3, MSG_LIN2/2 );
      cdputc( sorryn, MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  /* Can't play without a ship. */
  if ( ! findship( &csnum ) )
    {
      cdclear();
      cdredo();
      cdputc( "I'm sorry, but there are no ships available right now.",
	     MSG_LIN2/2 );
      cdputc( sorryn, MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  return ( TRUE );
  
}


