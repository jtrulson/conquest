#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"
#include "color.h"


#define GREEN_ALERT 0
#define YELLOW_ALERT 1
#define RED_ALERT 2

#define DS_LIVE_STR "DS_LIVE"
#define DS_OFF_STR  "DS_OFF"

/* Global to this module */

static int AlertLevel = GREEN_ALERT;
extern real LastPhasDist;	/* defined in conqlb.c */


void do_bottomborder(void)
{
  int lin;

  lin = DISPLAY_LINS + 1;

  cdline( lin, 1, lin, CqContext.maxcol );
  mvaddch(lin - 1, STAT_COLS - 1, ACS_BTEE);
}

void do_border(void)
{
  int lin;
  
  lin = DISPLAY_LINS + 1;
  
  
  cdline( 1, STAT_COLS, lin, STAT_COLS );
  do_bottomborder();
  
  return;
}

int alertcolor(int alert)
{
  int theattrib;
  
  switch (alert)
    {
    case GREEN_ALERT:
      theattrib = GreenLevelColor;
      break;
    case YELLOW_ALERT:
      theattrib = YellowLevelColor;
      break;
    case RED_ALERT:
      theattrib = RedLevelColor;
      break;
    default:
      clog("alertcolor(): invalid alert level: %d", alert);
      break;
    }

  return(theattrib);
}

void draw_alertborder(int alert)
{
  
  attrset(alertcolor(alert));
  do_border();
  attrset(0);
  
  /*  cdrefresh(); */
  
  return;
}

/*  display - do one update of a ships screen */
/*  SYNOPSIS */
/*    int snum, display_info */
/*    display( snum, display_info ) */
void display( int snum, int display_info )
{
  int i, j, k, l, m, idx, lin, col, dcol, datacol, minenemy, minsenemy;
  int linofs[8] = {0, -1, -1, -1, 0, 1, 1, 1};
  int colofs[8] = {1, 1, 0, -1, -1, -1, 0, 1};
  int outattr;
  static int OldAlert = 0;
  static string dirch="-/|\\-/|\\";
  char ch, buf[MSGMAXLINE];
  int dobeep, lsmap;
  int palertcol;
  real x, scale, cenx, ceny, dis, mindis, minsdis, fl, cd, sd;
  static real zzskills, zzswarp;
  static char zzbuf[MSGMAXLINE];
  static int zzsshields, zzcshields, zzshead, zzsfuel, zzcfuel;
  static int zzsweapons, zzsengines, zzsdamage, zzcdamage, zzsarmies;
  static int zzsetemp, zzswtemp, zzctemp, zzstowedby, zzssdfuse;
  static real prevsh = 0.0 , prevdam = 100.0 ;

  static int ShieldAttrib = 0;
  static int FuelAttrib = 0;
  static int WeapAttrib = 0;
  static int EngAttrib = 0;
  static int DamageAttrib = 0;
  
  static char zbuf[MSGMAXLINE];
  static char *dstatstr;

  if ( CqContext.redraw )
    {
      cdclear();
      lin = DISPLAY_LINS + 1;
      draw_alertborder(AlertLevel);
    }
  else
    {
      cdclra( 1, STAT_COLS  + 1, DISPLAY_LINS, CqContext.maxcol + 1 );
    }

  dobeep = FALSE;
  mindis = 1.0e6;
  minsdis = 1.0e6;
  minenemy = 0;
  minsenemy = 0;

  if (snum > 0)
    lsmap = Ships[snum].map;
  else
    lsmap = FALSE;
  
  if ( lsmap )
    {
      scale = MAP_FAC;
      
      if (sysconf_DoLocalLRScan)
	{
	  cenx = Ships[snum].x;
	  ceny = Ships[snum].y;
	}
      else
	{
	  cenx = 0.0;
	  ceny = 0.0;
	}
    }
  else
    {
      scale = SCALE_FAC;
      if (snum == DISPLAY_DOOMSDAY) { /* dwp */
	cenx = Doomsday->x;
	ceny = Doomsday->y;
      }
      else {
	cenx = Ships[snum].x;
	ceny = Ships[snum].y;
      }
    }
  
  /* Display the planets and suns. */
  for ( i = NUMPLANETS; i > 0; i = i - 1 )
    {
      if ( ! Planets[i].real )
	continue; /*next;*/
      if ( ! cvtcoords( cenx, ceny, Planets[i].x, Planets[i].y, scale, &lin, &col ) )
	continue; /* next;*/

      palertcol = 0;
				/* determine alertlevel for object */
      if (snum > 0 && spwar( snum, i ) && Planets[i].scanned[Ships[snum].team])
	{
	  palertcol = RedLevelColor;
	}
      else if (snum > 0 && Planets[i].team == Ships[snum].team && !selfwar(snum))
	{
	  palertcol = GreenLevelColor;
	}
      else
	palertcol = YellowLevelColor;

				/* suns are always yellow level material */
      if (Planets[i].type == PLANET_SUN)
	palertcol = YellowLevelColor;

      if ( lsmap )
	{
	  /* Strategic map. */
	  /* Can't see moons. */
	  if ( Planets[i].type == PLANET_MOON )
	    continue; /* next;*/
	  /* If it's a sun or we any planet we haven't scanned... */
	  if ( Planets[i].type == PLANET_SUN || ! Planets[i].scanned[Ships[snum].team] )
	    {
	      if (Planets[i].type == PLANET_SUN)
		attrset(RedLevelColor); /* suns have a red core */
	      else
		attrset(palertcol);

	      cdput( ConqInfo->chrplanets[Planets[i].type], lin, col );
	      attrset(0);
	    }
	  else
	    {
	      /* Pick a planet owner character. */
	      if ( Planets[i].armies <= 0 || Planets[i].team < 0 || Planets[i].team >= NUMPLAYERTEAMS )
		ch = '-';
	      else
		ch = Teams[Planets[i].team].torpchar;
	      
	      /* Display the planet; either it's numeric or it's not. */
	      if ( Ships[snum].options[OPT_NUMERICMAP] )
		{
		  sprintf( buf, "%d", Planets[i].armies);
		  l = strlen(buf);
		  
		  m = (col + 2 - (l + 2));

		  if (m > STAT_COLS)
		    {
		      attrset(palertcol);
		      cdput( ch, lin, m++);
		      attrset(0);

		      attrset(InfoColor);
		      cdputs( buf, lin, m);
		      m += l;
		      attrset(0);

		      attrset(palertcol);
                      cdput( ch, lin, m);
		      attrset(0);
		    }

		}
	      else if ( Planets[i].scanned[Ships[snum].team] )
		{
		  l = 3;		/* strlen */
		  m = (col + 2 - l);

		  if (m > STAT_COLS)
		    {
                      attrset(palertcol);
                      cdput( ch, lin, m++);
                      attrset(0);

                      attrset(InfoColor);
                      cdput( ConqInfo->chrplanets[Planets[i].type], lin, m++);
                      attrset(0);

                      attrset(palertcol);
                      cdput( ch, lin, m++);
		      attrset(0);
		    }
		}
	    }
	  
	  /* If necessary, display the planet name. */
	  if ( snum < 0 || (snum > 0 && 
			    Ships[snum].options[OPT_PLANETNAMES]) ) /* dwp */
	    {
	      sprintf(buf, "%c%c%c", Planets[i].name[0], Planets[i].name[1], Planets[i].name[2]);
	      attrset(palertcol);
	      cdputs( buf, lin, col+2 );
	      attrset(0);
	    }
	}
      else
	{
	  /* Tactical map. */
	  attrset(palertcol);
	  puthing( Planets[i].type, lin, col );
	  attrset(0);

	  if (col - 3 >= STAT_COLS - 1)
	    {
	      if (lin <= DISPLAY_LINS && lin > 0 )
		{
		  if (snum > 0 && !Planets[i].scanned[Ships[snum].team])
		    attrset(palertcol);	/* default to yellow for unscanned */
		  else
		    attrset(InfoColor);	/* scanned (known) value */

		  if (Planets[i].type == PLANET_SUN)
		    attrset(RedLevelColor); /* suns have a red core */

		  cdput( ConqInfo->chrplanets[Planets[i].type], lin, col + 1);
		  attrset(0);
		}
	      if ( snum < 0 || (snum > 0 && Ships[snum].options[OPT_PLANETNAMES]) ) /* dwp */
		if ( (lin + 1 <= DISPLAY_LINS) && col + 1< cdcols() )
		  {
		    attrset(palertcol);
		    cdputs( Planets[i].name, lin + 1, col + 2 );
		    attrset(0);
		  }
	    }
	}
    }
  
  /* Display the planet eater. */
  if ( Doomsday->status == DS_LIVE )
    if ( ! lsmap )
      if ( cvtcoords( cenx, ceny, Doomsday->x, Doomsday->y, scale, &lin, &col ) )
	{
	  dobeep = TRUE;
	  sd = sind(Doomsday->heading);
	  cd = cosd(Doomsday->heading);
	  /* Draw the body. */
	  attrset(COLOR_PAIR(COL_BLUEBLACK));
	  for ( fl = -DOOMSDAY_LENGTH/2.0;
	       fl < DOOMSDAY_LENGTH/2.0;
	       fl = fl + 50.0 )
	    if ( cvtcoords( cenx, ceny, Doomsday->x+fl*cd, Doomsday->y+fl*sd, scale, &lin, &col ) )
	      cdput( '#', lin, col );
	  attrset(0);
	  /* Draw the head. */
	  if ( cvtcoords( cenx, ceny, Doomsday->x+DOOMSDAY_LENGTH/2.0*cd,
			 Doomsday->y+DOOMSDAY_LENGTH/2.0*sd,
			 scale, &lin, &col ) )
	    {
	      attrset(RedLevelColor);
	      cdput( '*', lin, col );
	      attrset(0);
	    }
	}
  if ( snum > 0)
    {
      /* Display phaser graphics. */
      if ( ! lsmap && Ships[snum].pfuse > 0 )
	if ( Ships[snum].options[ OPT_PHASERGRAPHICS] )
	  {
	    sd = sind(Ships[snum].lastphase);
	    cd = cosd(Ships[snum].lastphase);
	    ch = dirch[mod( (round( Ships[snum].lastphase + 22.5 ) / 45), 7 )];
	    attrset(InfoColor);
	    for ( fl = 0; fl <= LastPhasDist; fl = fl + 50.0 )
	      if ( cvtcoords( cenx, ceny,
			      Ships[snum].x+fl*cd, Ships[snum].y+fl*sd,
			      scale, &lin, &col ) )
		cdput( ch, lin, col );
	    attrset(0);
	  }
    }
  
  /* Display the ships. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status != SS_OFF )
      {
	if (sysconf_DoLRTorpScan)
	  {
	    /* Display the torps on a LR scan if it's a friend. */
	    if (lsmap)
	      {
		if (snum > 0 && Ships[snum].war[Ships[i].team] == FALSE &&
		    Ships[i].war[Ships[snum].team] == FALSE)
		  {
		    if (i == snum) /* if it's your torps - */
		      attrset(A_BOLD);
		    else
		      attrset(YellowLevelColor);

		    for ( j = 0; j < MAXTORPS; j = j + 1 )
		      if ( Ships[i].torps[j].status == TS_LIVE 
			  || Ships[i].torps[j].status == TS_DETONATE )
			if ( cvtcoords( cenx, ceny, Ships[i].torps[j].x, Ships[i].torps[j].y,
				       scale, &lin, &col ) )
			  cdput( Teams[Ships[i].team].torpchar, lin, col );
		    attrset(0);
		  }
	      }
	  }
	
	if ( ! lsmap )
	  {
	    /* First display exploding torps. */
	    if ( snum < 0 || (snum > 0 && Ships[snum].options[ OPT_EXPLOSIONS]) ) /* dwp */
	      for ( j = 0; j < MAXTORPS; j = j + 1 )
		if ( Ships[i].torps[j].status == TS_FIREBALL )
		  if ( cvtcoords( cenx, ceny, Ships[i].torps[j].x, Ships[i].torps[j].y,
				  scale, &lin, &col) )
		  { /* colorize torp explosions */
		    if (i != snum && 
			(satwar(i, snum) || Ships[i].torps[j].war[Ships[snum].team]))
		      attrset(RedLevelColor);
		    else if (i != snum )
		      attrset(GreenLevelColor);
		    else
		      attrset(0);
		    puthing( THING_EXPLOSION, lin, col );
		    attrset(0);
		  }
	    /* Now display the live torps. */

	    if (snum > 0)
	      {			/* a ship */
		if (i == snum) /* if it's your torps and you're a ship */
		  attrset(0);
		else if (i != snum && satwar(i, snum))
		  attrset(RedLevelColor);
		else
		  attrset(GreenLevelColor);
	      }
	    else		/* a special */
	      attrset(YellowLevelColor);
		
	    for ( j = 0; j < MAXTORPS; j = j + 1 )
	      if ( Ships[i].status != SS_DYING && Ships[i].status != SS_DEAD && 
		   (Ships[i].torps[j].status == TS_LIVE || Ships[i].torps[j].status == TS_DETONATE) )
		if ( cvtcoords( cenx, ceny, Ships[i].torps[j].x, Ships[i].torps[j].y,
				scale, &lin, &col ) )
		  cdput( Teams[Ships[i].team].torpchar, lin, col );

	    attrset(0);
	  }

	/* Display the ships. */
	if ( Ships[i].status == SS_LIVE )
	  {
	    /* It's alive. */
	    if ( snum > 0)	/* it's a ship view */
	      {
		dis = (real) dist(Ships[snum].x, Ships[snum].y, Ships[i].x, Ships[i].y );
	    
		/* Here's where ship to ship accurate information is "gathered". */
		if ( dis < ACCINFO_DIST && ! Ships[i].cloaked && ! selfwar( snum ) )
		  Ships[i].scanned[Ships[snum].team] = SCANNED_FUSE;
	    
		/* Check for nearest enemy and nearest scanned enemy. */
		if ( satwar( i, snum ) )
		  if ( i != snum )
		    {
		  
#ifdef WARP0CLOAK
		      /* 1/6/94 */
		      /* we want any cloaked ship at warp 0.0 */
		      /* to be invisible. */
		      if (Ships[i].cloaked && Ships[i].warp == 0.0)
			{
			  /* skip to next ship. this one isn't here */
			  /* ;-} */
			  continue;	/* RESTART FOR */
			}
#endif /* WARP0CLOAK */
		  
		      if ( dis < mindis )
			{
			  /* New nearest enemy. */
			  mindis = dis;
			  minenemy = i;
			}
		      if ( dis < minsdis )
			if ( ! selfwar( snum ) )
			  if ( Ships[i].scanned[Ships[snum].team] > 0 )
			    {
			      /* New nearest scanned enemy. */
			      minsdis = dis;
			      minsenemy = i;
			    }
		      
		    }
	      }	/* if a ship view (snum > 0) */

	    /* There is potential for un-cloaked ships and ourselves. */
	    if ( ! Ships[i].cloaked || i == snum )
	      {
		/* ... especially if he's in the bounds of our current */
		/*  display (either tactical or strategic map) */
		if ( cvtcoords( cenx, ceny, Ships[i].x, Ships[i].y,
			       scale, &lin, &col ) )
		  {
		    /* He's on the screen. */
		    /* We can see him if one of the following is true: */
#
		    /*  - We are not looking at our strategic map */
		    /*  - We're mutually at peace */
		    /*  - Our team has scanned him and we're not self-war */
		    /*  - He's within accurate scanning range */
		    
		    if ( ( ! lsmap ) ||
			( snum > 0 && !satwar(i, snum) ) ||
			( snum > 0 && Ships[i].scanned[Ships[snum].team] && 
			  !selfwar(snum) ) ||
			( dis <= ACCINFO_DIST ) )
		      {
			if ( snum > 0 && ( i == snum ) && Ships[snum].cloaked )
			  ch = CHAR_CLOAKED;
			else
			  ch = Teams[Ships[i].team].teamchar;
			
				/* determine color */
			if (snum > 0)
			  {
			    if (i == snum)    /* it's ours */
			      attrset(A_BOLD);
			    else if (satwar(i, snum)) /* we're at war with it */
			      attrset(RedLevelColor);
			    else if (Ships[i].team == Ships[snum].team && !selfwar(snum))
			      attrset(GreenLevelColor); /* it's a team ship */
			    else
			      attrset(YellowLevelColor);
			  }
			else
			  attrset(YellowLevelColor); /* special view */
			    
				 
			cdput( ch, lin, col );
			attrset(0); attrset(A_BOLD);
			cdputn( i, 0, lin, col + 2 );
			attrset(0);

			idx = (int)mod( round((((real)Ships[i].head + 22.5) / 45.0) + 0.5) - 1, 8);
			/* JET 9/28/94 */
			/* Very strange -mod keep returning 8 = 8 % 8*/
			/* which aint right... mod seems to behave */
			/* itself elsewhere... anyway, a kludge: */
			idx = ((idx == 8) ? 0 : idx);
			
			/*			    cerror("idx = %d", idx);*/
			
			j = lin+linofs[idx];
			k = col+colofs[idx];

			attrset(InfoColor);
			if ( j >= 0 && j < DISPLAY_LINS && 
			    k > STAT_COLS && k < CqContext.maxcol )
			  cdput( dirch[idx], j, k );
			attrset(0);
		      }
		  }
		if ( snum > 0 && snum == i )
		  {
		    /* If it's our ship and we're off the screen, fake it. */
		    if ( lin < 1 )
		      lin = 1;
		    else
		      lin = min( lin, DISPLAY_LINS );
		    if ( col < STAT_COLS + 1 )
		      col = STAT_COLS + 1;
		    else
		      col = min( col, CqContext.maxcol );
		    cdmove( lin, col );
		  }
	      }
	  } /* it's alive */
      } /* for each ship */
  
  /* Construct alert status line. */
  if ( CqContext.redraw )
    zzbuf[0] = EOS;
  buf[0] = EOS;

  if (snum > 0)
    {				/* if a ship view */
      if ( minenemy != 0 || Ships[snum].talert )
	{
	  if ( mindis <= PHASER_DIST )
	    {
	      /* Nearest enemy is very close. */
	      outattr = COLOR_PAIR(COL_REDBLACK);
	      AlertLevel = RED_ALERT;
	      c_strcpy( "RED ALERT ", buf );
	      dobeep = TRUE;
	    }
	  else if ( mindis < ALERT_DIST )
	    {
	      /* Nearest enemy is close. */
	      outattr = COLOR_PAIR(COL_REDBLACK);
	      AlertLevel = RED_ALERT;
	      c_strcpy( "Alert ", buf );
	      dobeep = TRUE;
	    }
	  else if ( Ships[snum].talert )
	    {
	      /* Nearby torpedos. */
	      outattr = COLOR_PAIR(COL_YELLOWBLACK);
	      AlertLevel = YELLOW_ALERT;
	      c_strcpy( "Torp alert", buf );
	      minenemy = 0;			/* disable nearby enemy code */
	      dobeep = TRUE;
	    }
	  else if ( mindis < YELLOW_DIST )
	    {
	      /* Near an enemy. */
	      outattr = COLOR_PAIR(COL_YELLOWBLACK);
	      AlertLevel = YELLOW_ALERT;
	      c_strcpy( "Yellow alert ", buf );
	    }
	  else if ( minsenemy != 0 )
	    {
	      /* An enemy near one of our ships or planets. */
	      outattr = COLOR_PAIR(COL_YELLOWBLACK);
	      minenemy = minsenemy;		/* for cloaking code below */
	      AlertLevel = YELLOW_ALERT;
	      c_strcpy( "Proximity Alert ", buf );
	    }
	  else
	    {
	      outattr = COLOR_PAIR(COL_GREENBLACK);
	      AlertLevel = GREEN_ALERT;
	      minenemy = 0;
	    }
	  
	  if ( minenemy != 0 )
	    {
	      appship( minenemy, buf );
	      if ( Ships[minenemy].cloaked )
		appstr( " (CLOAKED)", buf );
	    }
	}
      else
	AlertLevel = GREEN_ALERT;
    }
  else /* if snum < 0 */
    AlertLevel = GREEN_ALERT;	/* for a special */

  if (OldAlert != AlertLevel)
    {
      draw_alertborder(AlertLevel);
      OldAlert = AlertLevel;
    }
  
  if ( strcmp( buf, zzbuf ) != 0 )
    {
      lin = DISPLAY_LINS + 1;
      attrset(alertcolor(AlertLevel));
      do_bottomborder();
      attrset(0);

      if ( buf[0] != EOS )
	{
	  col = (int) (CqContext.maxcol - STAT_COLS - strlen(buf)) / (int)2 + STAT_COLS + 1;
	  if (HasColors)
	    {
	      attrset(outattr);
	    }
	  else
	    {
	      if (outattr == COLOR_PAIR(COL_REDBLACK))
		attrset(A_BOLD | A_BLINK);
	      else if (outattr == COLOR_PAIR(COL_YELLOWBLACK))
		attrset(A_BOLD);
	    }
	  cdputs( buf, DISPLAY_LINS+1, col );
	  attrset(0);
	}
      c_strcpy( buf, zzbuf );
    }
  
  /* Build and display the status info as necessary. */
  if (snum > 0) 
    { /* we're watching a ship */  /* dwp */
      lin = 1;
      col = 1;
      datacol = col + 14;
    
    /* Shields. */
    if ( Ships[snum].shields < prevsh )
      dobeep = TRUE;
    prevsh = Ships[snum].shields;
    
    if ( CqContext.redraw )
      {
	zzsshields = -9;
	zzcshields = ' ';
      }
    i = round( Ships[snum].shields );
    if ( ! Ships[snum].shup || Ships[snum].rmode )
      i = -1;
    if ( i != zzsshields || i == -1)
      {
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	if ( i == -1 )
	  {
	    if (AlertLevel == YELLOW_ALERT) 
	      attrset(YellowLevelColor);
	    else if (AlertLevel == RED_ALERT)
	      attrset(RedLevelColor | A_BLINK);
	    else
	      attrset(GreenLevelColor);
	    
	    cdputs( "DOWN", lin, datacol );
	    attrset(0);
	  }
	else
	  {
	    if (i >= 0 && i <= 50)
	      ShieldAttrib = RedLevelColor;
	    else if (i >=51 && i <=80)
	      ShieldAttrib = YellowLevelColor;
	    else if (i >= 81)
	      ShieldAttrib = GreenLevelColor;
	    
	    sprintf( buf, "%d", i );
	    attrset(ShieldAttrib);
	    cdputs( buf, lin, datacol );
	  attrset(0);
	  }
	zzsshields = i;
      }
    
    if ( i < 60 )
      j = 'S';
    else
      j = 's';
    if ( j != zzcshields )
      {
	attrset(LabelColor);
	if ( j == 'S' )
	  cdputs( "SHIELDS =", lin, col );
	else
	  cdputs( "shields =", lin, col );
	attrset(0);
	zzcshields = j;
      }
    
    /* Kills. */
    lin = lin + 2;
    if ( CqContext.redraw )
      {
	attrset(LabelColor);
	cdputs( "kills =", lin, col );
	attrset(0);
	zzskills = -20.0;
      }
    x = (Ships[snum].kills + Ships[snum].strkills);
    if ( x != zzskills )
      {
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	sprintf( buf, "%0.1f", oneplace(x) );
	
	attrset(InfoColor);
	cdputs( buf, lin, datacol );
	attrset(0);
	
	zzskills = x;
      }
    
    /* Warp. */
    lin = lin + 2;
    if ( CqContext.redraw )
      {
	attrset(LabelColor);
	cdputs( "warp =", lin, col );
	attrset(0);
	zzswarp = 92.0;			/* "Warp 92 Mr. Sulu." */
      }
    x = Ships[snum].warp;
    if ( x != zzswarp )
      {
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	
	attrset(InfoColor);
	if ( x >= 0.0 )
	  {
	    sprintf( buf, "%.1f", x );
	    cdputs( buf, lin, datacol );
	  }
	else
	  cdput( 'o', lin, datacol );
	attrset(0);
	
	zzswarp = x;
      }
    
    /* Heading. */
    lin = lin + 2;
    if ( CqContext.redraw )
      {
	attrset(LabelColor);
	cdputs( "heading =", lin, col );
	attrset(0);
	zzshead = 999;
      }
    i = Ships[snum].lock;
    if ( i >= 0 || Ships[snum].warp < 0.0)
      i = round( Ships[snum].head );
    if ( -i != zzshead)
      {
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	
	attrset(InfoColor);
	if ( -i > 0 && -i <= NUMPLANETS)
	  sprintf( buf, "%.3s", Planets[-i].name );
	else
	  sprintf( buf, "%d", i );
	cdputs( buf, lin, datacol );
	attrset(0);
	zzshead = i;
      }
    
    /* Fuel. */
    lin = lin + 2;
    if ( CqContext.redraw )
      {
	zzsfuel = -99;
	zzcfuel = ' ';
      }
    i = round( Ships[snum].fuel );
    if ( i != zzsfuel )
      {
	if (i >= 0 && i <= 200)
	  FuelAttrib = RedLevelColor;
	else if (i >=201 && i <=500)
	  FuelAttrib = YellowLevelColor;
	else if (i >= 501)
	  FuelAttrib = GreenLevelColor;
	
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	sprintf( buf, "%d", i );
	
	attrset(FuelAttrib);
	cdputs( buf, lin, datacol );
	attrset(0);
	
	zzsfuel = i;
      }
    
    if ( i < 200 )
      j = 'F';
    else
      j = 'f';
    if ( j != zzcfuel )
      {
	attrset(LabelColor);
	if ( j == 'F' )
	  cdputs( "FUEL =", lin, col );
	else if ( j == 'f' )
	  cdputs( "fuel =", lin, col );
	attrset(0);
	
	zzcfuel = j;
      }
    
    /* Allocation. */
    lin = lin + 2;
    if ( CqContext.redraw )
      {
	attrset(LabelColor);
	cdputs( "w/e =", lin, col );
	attrset(0);
	zzsweapons = -9;
	zzsengines = -9;
      }
    i = Ships[snum].weapalloc;
    j = Ships[snum].engalloc;
    if ( Ships[snum].wfuse > 0 )
      i = 0;
    if ( Ships[snum].efuse > 0 )
      j = 0;
    if ( i != zzsweapons || j != zzsengines )
      {
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	buf[0] = EOS;
	if ( i == 0 )
	  appstr( "**", buf );
	else
	  appint( i, buf );
	appchr( '/', buf );
	if ( j == 0 )
	  appstr( "**", buf );
	else
	  appint( j, buf );
	attrset(InfoColor);
	cdputs( buf, lin, datacol );
	attrset(0);
	zzsweapons = i;
      }
    
    /* Temperature. */
    lin = lin + 2;
    if ( CqContext.redraw )
      {
	zzswtemp = 0;
	zzsetemp = 0;
	zzctemp = ' ';
      }
    i = round( Ships[snum].wtemp );
    j = round( Ships[snum].etemp );
    if ( i > 100 )
      i = 100;
    if ( j > 100 )
      j = 100;
    if ( i != zzswtemp || j != zzsetemp )
      {
	static char wtbuf[16];
	static char etbuf[16];
	
	wtbuf[0] = '\0';
	etbuf[0] = '\0';
	
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	if ( i != 0 || j != 0 )
	  {
	    buf[0] = EOS;
	    
	    if ( i >= 100 )
	      strcpy(wtbuf, "**");
	    else
	      sprintf(wtbuf, "%02d", i);
	    
	    if ( j >= 100 )
	      strcpy(etbuf, "**");
	    else
	      sprintf(etbuf, "%02d", j);
	    
	    if (i >= 0 && i <= 50)
	      WeapAttrib = GreenLevelColor;
	    else if (i >=51 && i <=75)
	      WeapAttrib = YellowLevelColor;
	    else if (i >= 76)
	      WeapAttrib = RedLevelColor;
	    
	    attrset(WeapAttrib);
	    cdputs(wtbuf, lin, datacol);
	    attrset(0);
	  
	    attrset(InfoColor);
	    cdputs("/", lin, datacol + 2);
	    attrset(0);
	  
	    if (j >= 0 && j <= 50)
	      EngAttrib = GreenLevelColor;
	    else if (j >=51 && j <=80)
	      EngAttrib = YellowLevelColor;
	    else if (j >= 81)
	      EngAttrib = RedLevelColor;
	    
	    attrset(EngAttrib);
	    cdputs(etbuf, lin, datacol + 3);
	    attrset(0);
	    
	  }
	zzswtemp = i;
	zzsetemp = j;
      }
    
    if ( i == 0 && j == 0 )
      j = ' ';
    else if ( i >= 80 || j >= 80 )
      j = 'T';
    else
      j = 't';
    if ( j != zzctemp )
      {
	cdclra( lin, col, lin, datacol-1 );
	if ( j == 't' )
	  {
	    attrset(LabelColor);
	    cdputs( "temp =", lin, col );
	    attrset(0);
	  }
	else if ( j == 'T' )
	  {
	    attrset(LabelColor);
	    cdputs( "TEMP =", lin, col );
	    attrset(0);
	  }
	zzctemp = j;
      }
    
    /* Damage/repair. */
    if ( Ships[snum].damage > prevdam )
      dobeep = TRUE;
    prevdam = Ships[snum].damage;
    
    lin = lin + 2;
    if ( CqContext.redraw )
      {
	zzsdamage = -9;
	zzcdamage = ' ';
      }
    i = round( Ships[snum].damage );
    if ( i != zzsdamage )
      {
	cdclra( lin, datacol, lin, STAT_COLS-1 );
	if ( i > 0 )
	  {
	    sprintf( buf, "%d", i );
	    if (i >= 0 && i <= 10)
	      DamageAttrib = GreenLevelColor;
	    else if (i >=11 && i <=65)
	      DamageAttrib = YellowLevelColor;
	    else if (i >= 66)
	      DamageAttrib = RedLevelColor;
	  
	    attrset(DamageAttrib);
	    cdputs( buf, lin, datacol );
	    attrset(0);
	  }
	zzsdamage = i;
      }
  
    if ( Ships[snum].rmode )
      j = 'r';
    else if ( i >= 50 )
      j = 'D';
    else if ( i > 0 )
      j = 'd';
    else
      j = ' ';
    if ( j != zzcdamage )
      {
	cdclra( lin, col, lin, datacol-1 );
      
	if ( j == 'r' )
	  {
	    attrset(GreenLevelColor);
	    cdputs( "REPAIR, dmg =", lin, col );
	  }
	else if ( j == 'd' )
	  {
	    attrset(YellowLevelColor);
	    cdputs( "damage =", lin, col );
	  }
	else if ( j == 'D' )
	  {
	    attrset(RedLevelColor);
	    cdputs( "DAMAGE =", lin, col );
	  }
	attrset(0);
	zzcdamage = j;
      }
  
    /* Armies. */
    lin = lin + 2;
    if ( CqContext.redraw )
      zzsarmies = -666;
    i = Ships[snum].armies;
    if ( i == 0 )
      i = -Ships[snum].action;
    if ( i != zzsarmies )
      {
	cdclra( lin, col, lin, STAT_COLS-1 );
	if ( i > 0 )
	  {
	    attrset(InfoColor);
	    cdputs( "armies =", lin, col );
	    sprintf( buf, "%d", i );
	    cdputs( buf, lin, datacol );
	    attrset(0);
	  }
	else if ( i < 0 )
	  {
	    attrset(InfoColor);
	    cdputs( "action =", lin, col );
	    robstr( -i, buf );
	    cdputs( buf, lin, datacol );
	    attrset(0);
	  }
	zzsarmies = i;
      }
  
    /* Tractor beams. */
    lin = lin + 2;
    if ( CqContext.redraw )
      zzstowedby = 0;
    i = Ships[snum].towedby;
    if ( i == 0 )
      i = -Ships[snum].towing;
    if ( i != zzstowedby )
      {
	cdclra( lin, col, lin, datacol-1 );
	if ( i == 0 )
	  buf[0] = EOS;
	else if ( i < 0 )
	  {
	    c_strcpy( "towing ", buf );
	    appship( -i, buf );
	  }
	else if ( i > 0 )
	  {
	    c_strcpy( "towed by ", buf );
	    appship( i, buf );
	  }
	attrset(InfoColor);
	cdputs( buf, lin, col );
	attrset(0);
	zzstowedby = i;
      }
  
    /* Self destruct fuse. */
    lin = lin + 2;
    if ( CqContext.redraw )
      zzssdfuse = -9;
    if ( Ships[snum].cloaked )
      i = -1;
    else
      i = max( 0, Ships[snum].sdfuse );
    if ( i != zzssdfuse )
      {
	cdclra( lin, col, lin, STAT_COLS-1 );
	if ( i > 0 )
	  {
	    sprintf( buf, "DESTRUCT MINUS %-3d", i );
	    attrset(RedLevelColor);
	    cdputs( buf, lin, col );
	    attrset(0);
	  }
	else if ( i == -1 )
	  {
	    attrset(RedLevelColor);
	    cdputs( "CLOAKED", lin, col );
	    attrset(0);
	  }
	else 
	  cdputs( "       ", lin, col );
	zzssdfuse = i;
      }
  
    if ( dobeep )
      if ( Ships[snum].options[OPT_ALARMBELL] )
	cdbeep();
  
  } /* end of ship stats display */
  /* 
   * The following stat code for doomsday machine previously 
   * lived in doomdisplay() - dwp 
   */
  else				/* snum <= 0 meaning a planet of special */
    {  /* we're watching a special */ 
      if ( snum == DISPLAY_DOOMSDAY)
	{
	  lin = 1;
	  col = 1;

	  cdclra( lin, col, DISPLAY_LINS, STAT_COLS - 1 );

	  c_strcpy( Doomsday->name, buf );
	  /* put dtype in stats, dstatus next to name  - dwp */
	  if ( Doomsday->status == DS_LIVE  )
	    appstr( "  (live)", buf);
	  else if ( Doomsday->status == DS_OFF )
	    appstr( "  (off)", buf);
	  else
	    appstr( "  (unknown)", buf);
	  cdputs( buf, 1, STAT_COLS + 
		  (int)(CqContext.maxcol - STAT_COLS - (strlen(buf))) / (int)2 + 1);
      
	  lin = lin + 2;
	  dcol = col + 11;
      
	  attrset(LabelColor);
	  cdputs( "  dstatus:", lin, col );
	  attrset(InfoColor);
	  if ( Doomsday->status == DS_LIVE)
	    dstatstr = DS_LIVE_STR;
	  else if (Doomsday->status == DS_OFF)
	    dstatstr = DS_OFF_STR;
	  else
	    dstatstr = "";
	  sprintf(buf, "%d (%s)", Doomsday->status, dstatstr);
	  cdputs(buf, lin, dcol);
	  attrset(0);
      
	  lin = lin + 1;
	  attrset(LabelColor);
	  cdputs( "       dx:", lin, col );
	  attrset(0);
	  attrset(InfoColor);
	  cdputr( oneplace(Doomsday->x), 0, lin, dcol );
	  attrset(0);
      
	  lin = lin + 1;
	  attrset(LabelColor);
	  cdputs( "       dy:", lin, col );
	  attrset(0);
	  attrset(InfoColor);
	  cdputr( oneplace(Doomsday->y), 0, lin, dcol );
	  attrset(0);
      
	  lin = lin + 1;
	  attrset(LabelColor);
	  cdputs( "      ddx:", lin, col );
	  attrset(0);
	  attrset(InfoColor);
	  cdputr( oneplace(Doomsday->dx), 0, lin, dcol );
	  attrset(0);

	  lin = lin + 1;
	  attrset(LabelColor);
	  cdputs( "      ddy:", lin, col );
	  attrset(0);
	  attrset(InfoColor);
	  cdputr( oneplace(Doomsday->dy), 0, lin, dcol );
	  attrset(0);
      
	  lin = lin + 1;
	  attrset(LabelColor);
	  cdputs( "    dhead:", lin, col );
	  attrset(0);
	  attrset(InfoColor);
	  cdputn( round(Doomsday->heading), 0, lin, dcol );
	  attrset(0);
      
	  lin = lin + 1;
	  attrset(LabelColor);
	  cdputs( "    dlock:", lin, col );
	  cdputs( "      ddt:", lin+1, col );
	  attrset(0);
      
	  for (i=0;i<MAXPLANETNAME;i++)
	    zbuf[i] = ' ';
	  zbuf[MAXPLANETNAME-1] = EOS;
	  cdputs( zbuf, lin, dcol );	/* clean up status line */
      
	  i = Doomsday->lock;
	  if ( -i > 0 && -i <= NUMPLANETS )
	    {
	      attrset(RedLevelColor);
	      cdputs( Planets[-i].name, lin, dcol );	/* -[] */
	      attrset(0);
	      attrset(InfoColor);
	      cdputn( round( dist( Doomsday->x, Doomsday->y, Planets[-i].x, Planets[-i].y ) ),
		      0, lin + 1, dcol );
	      attrset(0);
	    }
	  else if ( i > 0 && i <= MAXSHIPS )
	    {
	      buf[0] = EOS;
	      appship( i, buf );
	      attrset(RedLevelColor);
	      cdputs( buf, lin, dcol );
	      attrset(0);
	      attrset(InfoColor);
	      cdputn( round( dist( Doomsday->x, Doomsday->y, Ships[i].x, Ships[i].y ) ),
		      0, lin + 1, dcol );
	      attrset(0);
	    }
	  else
	    cdputn( i, 0, lin + 1, dcol );
      
	  attrset(0);
      
	  lin = lin + 2;
	}
      else
	{			/* not the doomsday, nothing yet... */
	  clog("display(): got invalid ship number %d", snum);
	}
      
    }
  
  if (snum > 0)			/* a ship */
    if (display_info)
      display_headers(snum);

  cdrefresh();
  CqContext.redraw = FALSE;
  
  return;
  
}

void display_headers(int snum)
{

  char *heading_fmt = "%s %c%d (%s)%s";
  char *closed_str1 = "GAME CLOSED -";
  char *robo_str1 = "ROBOT (external)";
  char *robo_str2 = "ROBOT";
  char *ship_str1 = "SHIP";
  char hbuf[MSGMAXLINE];
  char ssbuf[MSGMAXLINE];
  
  hbuf[0] = EOS;
  ssbuf[0] = EOS;
  
  appstr( ", ", ssbuf );
  appsstatus( Ships[snum].status, ssbuf);
  
  if ( ConqInfo->closed) 
    {
      sprintf(hbuf, heading_fmt, closed_str1, 
	      Teams[Ships[snum].team].teamchar, 
	      snum,
	      Ships[snum].alias, ssbuf); 
      attrset(A_BOLD);
      cdputs( hbuf, 1, STAT_COLS + 
	      (int)(CqContext.maxcol - STAT_COLS - (strlen(hbuf))) / (int)2 + 1);
      attrset(0);
    }
  else if ( Ships[snum].robot )
    {
      if (ConqInfo->externrobots == TRUE) 
	{
	  sprintf(hbuf, heading_fmt, robo_str1, 
		  Teams[Ships[snum].team].teamchar, snum,
		  Ships[snum].alias, ssbuf); 
	  attrset(A_BOLD);
	  cdputs( hbuf, 1, STAT_COLS + 
		  (int)(CqContext.maxcol - STAT_COLS - (strlen(hbuf))) / (int)2 + 1);
	  attrset(0);
	}
      else 
	{
	  sprintf(hbuf, heading_fmt, robo_str2, 
		  Teams[Ships[snum].team].teamchar, snum,
		  Ships[snum].alias, ssbuf);
	  attrset(A_BOLD);
	  cdputs( hbuf, 1, STAT_COLS + 
		  (int)(CqContext.maxcol - STAT_COLS - (strlen(hbuf))) / (int)2 + 1);
	  attrset(0);
	}
    }
  else 
    {
      sprintf(hbuf, heading_fmt, ship_str1, Teams[Ships[snum].team].teamchar,
	      snum,
	      Ships[snum].alias, ssbuf);
      attrset(A_BOLD);
      cdputs( hbuf, 1, STAT_COLS + 
	      (int)(CqContext.maxcol - STAT_COLS - (strlen(hbuf))) / (int)2 + 1);
      attrset(0);
    }
  
  cdrefresh();

}
