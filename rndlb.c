#include "c_defs.h"

/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 07:03:41  jon
 * Initial revision
 *
 * Revision 1.7  1996/11/23  07:03:19  jon
 * - type fixes
 *
 * Revision 1.6  1996/07/02  20:30:17  jon
 * - lots of code cleanup
 *
 * Revision 1.5  1996/03/16  22:11:20  jon
 * cerror() -> clog() changes.
 *
 * Revision 1.4  1995/02/08  05:03:33  jon
 * global change of all _And_'s to &&'s and _Or_ to ||'s.  other minor
 * 'corrections' and cleanups.
 *
 * Revision 1.3  1995/01/14  22:43:53  jon
 * fixed rndini(): wrong seed init function being called - srand(), should
 * have been srand48().
 * fixed rndint(): was returning (low, high], should have been (low, high).
 *
 * Revision 1.2  1995/01/09  00:46:44  jon
 * Added PVLOCK/UNLOCK capability, fixed some minor bugs, changed cloak
 * code so that a cloaked, warp 0 ship is invisible to alert scanners.
 * Increase torp fuse to 50 seconds, thereby increasing range.  Added
 * msync() to implement the upchuck() function.
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 *
 *
 ***********************************************************************/


/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

/*                               R N D L B */
/* Version Date Initials Remarks */
/* ------- ---- -------- ------------------------------------------------------- */
/*   02G  26jul84  JP	Changed use of gtime() to getnow(). */
/*   02F  12jul83  JIC   Ratfixed. */
/*                       Added code so that the "include rndlb.c" could be */
/*                       removed. */
/*   02E  17apr83  JP    Fixed a bug in rndnor introduced in version 02D. */
/*   02D  06apr83  JP    Fixed a bug in rndexp and sped up rndexp, rndnor, */
/*                       and rndgeo. */
/*   02C  19mar82  JP    Fixed three bugs in rndnor. */
/*   02B  08mar82  JP    Got rid of rndmod - plain old mod is used now. */
/*                       Turned integers into integer*4s. */
/*   02A  02mar82  JP    Changed a, c, and m for both sequences. */
/*                       Should now be "portable". */
/*   01B  02feb82  JP    Changed TWOTOTHE31MINUS1 into HALFMODULUS. */
/*                       Changed REALMODULUS to realmodulus. */
/*   01A  18nov80  JP    First written. */
/*  All the algorithms in this package are from "The Art of Computer */
/*  Programming", Vol. 2 (Seminumerical Algorithms), sections 3.2.1 and */
/*  3.4.1. */


#define TABLESIZE 200 

int value1, value2, mult1, mult2, inc1, inc2, modu1, modu2, table[TABLESIZE];


/*##  rndseq - internal routine to compute the next value in a linear */
/*             congruential sequence */
/*  SYNOPSIS */
/*    integer*4 value, multiplier, increment, modulus */
/*    rndseq ( value, multiplier, increment, modulus ) */
/*  DESCRIPTION */
void rndseq ( int value, int multiplier, int increment, int modulus )
{
  value = mod ( value * multiplier + increment, modulus );
  
  return;
}


/*##  rndini - initialize the random number package */
/*  SYNOPSIS */
/*    int seed1, seed2 */
/*    rndini ( seed1, seed2 ) */
/*  DESCRIPTION */
/*      This routine must be called before any of the other routines in */
/*    this package can be called.  The seeds are used to initialize the */
/*    random numbers in a deterministic manner, so that each time you */
/*    initialize with the same seeds you will get the same sequence. */
/*    If you want non-deterministic sequences, use 0 as the seeds, and */
/*    the current time will be used. */
void rndini ( int seed1, int seed2 )
{
  
  int idx;
  int now[8];
  
  /* Here are the multipliers, increments, and moduli for the two sequences. */
  /* Do not change these frivously!  They have been very carefully selected, */
  /* using the "spectral test" and various other empirical tests of */
  /* randomness.  Each sequence, by itself, is better than most random */
  /* sequences currently in use.  Together, they are awesome... */
  mult1 = 1541;	inc1 = 3501;	modu1 = 16384;
  mult2 = 5146;	inc2 = 4100;	modu2 = 19683;
  
  getnow ( now );
  
  if ( seed1 != 0 )
    value1 = seed1;
  else
    value1 = ( ( now[3] * 10 + now[4] ) * 10 + now[5] ) * 10 + now[6];
  value1 = mod ( value1, modu1 );
  
  if ( seed2 != 0 )
    value2 = seed2;
  else
    value2 = ( ( now[6] * 10 + now[5] ) * 10 + now[4] ) * 10 + now[3];
  value2 = mod ( value2, modu2 );
  
  for ( idx=0; idx < TABLESIZE; idx=idx+1 )
    {
      rndseq ( value2, mult2, inc2, modu2 );
      table[idx] = value2;
    }
  
#ifdef DEBUG_RANDOM
  clog("rndini(): seed1 = %d, seed2  = %d, value1 = %d, value2 = %d",
       seed1, seed2, value1, value2);
#endif
  
  
  /*    srand48((long)(value1 * value2)); */
  srand48((long) time(0));
  return;
}


/*##  rnd - random real number in the range [0..1) */
/*  SYNOPSIS */
/*    real r, rnd */
/*    r = rnd(0) */
#ifdef ALT_RND
real rnd ( int dummy )
{
  
  int idx;
  real RET;
  
  rndseq ( value1, mult1, inc1, modu1 );
  idx = ifix ( float(value1) / float(modu1) * TABLESIZE ) + 1;
  RET = float(table[idx]) / float(modu2);
  rndseq ( value2, mult2, inc2, modu2 );
  table[idx] = value2;
  
  return(RET);
}
#else
real rnd ( int dummy )		/* use 48bit linear congruential */
{
  real rc;
  
  rc = (real) drand48();
#ifdef DEBUG_RANDOM
  clog("rnd(): drand48() = %f", rc);
#endif
  return(rc);
}
#endif

/*##  rnduni - random real number in the specified range */
/*  SYNOPSIS */
/*    real rlow, rhigh, r, rnduni */
/*    r = rnduni ( rlow, rhigh ) */
real rnduni ( real rlow, real rhigh )
{
  return(rnd(0) * (rhigh-rlow) + rlow);
  
}


/*##  rndint - random int in the specified range */
/*  SYNOPSIS */
/*    int ilow, ihigh, i, rndint */
/*    i = rndint ( ilow, ihigh ) */
int rndint ( int ilow, int ihigh )
{
  int rc;

  rc = ifix ( rnd(0) * float(ihigh-ilow+1) ) + ilow;
  
#ifdef DEBUG_RANDOM
  clog("rndint(): rc = %d", rc);
#endif
  
  return(rc);
  
}


/*##  rndnor - normally distributed random real number */
/*  SYNOPSIS */
/*    real mean, stddev, r, rndnor */
/*    r = rndnor ( mean, stddev ) */
real rndnor ( real mean, real stddev )
{
  real v1, v2, z;
  
  do            /* repeat*/
    {
      v1 = -alog(1.0-rnd(0));
      v2 = -alog(1.0-rnd(0));
    }
  while ( 2.0 * v1 < powf((real)(v2-1.0), (real)2) );  
  
  if ( rnd(0) > 0.5 )
    z = 1.0;
  else
    z = -1.0;
  
  return(stddev * z * v2 + mean);
  
}


/*##  rndexp - exponentially distributed random real number */
/*  SYNOPSIS */
/*    real mean, r, rndexp */
/*    r = rndexp ( mean ) */
real rndexp ( real mean )
{
  
  return(- alog ( 1.0 - rnd(0) ) * mean);
}


/*##  rndchi - random real number with the chi-square distribution */
/*  SYNOPSIS */
/*    int v */
/*    real r, rndchi */
/*    r = rndchi ( v ) */
/*  DESCRIPTION */
/*      This distribution is also known as the gamma distribution of order v/2*/
real rndchi ( int v )
{
  int k, x;
  real RETVAL;
  
  k = v / 2;
  
  RETVAL = 0.0;
  for ( x=1; x <= k; x=x+1 )
    RETVAL = RETVAL + rndexp ( 1.0 );
  RETVAL = RETVAL * 2.0;
  
  if ( (k * 2 + 1) == v )
    RETVAL = RETVAL + powf((real)rndnor(0.0,1.0), (real) 2);
  
  return(RETVAL);
}


/*##  rndbta - random real number with the beta distribution */
/*  SYNOPSIS */
/*    int v1, v2 */
/*    real r, rndbta */
/*    r = rndbta ( v1, v2 ) */
real rndbta ( int v1, int v2 )
{
  
  real y1, y2;
  
  y1 = rndchi ( v1 );
  y2 = rndchi ( v2 );
  
  return(y1 / (y1+y2));
  
}


/*##  rndF - random real number with the F-distribtion */
/*  SYNOPSIS */
/*    int v1, v2 */
/*    real r, rndF */
/*    r= rndF ( v1, v2 ) */
/*  DESCRIPTION */
/*      This distribution is also known as the variance-ratio distribution. */
real rndF ( int v1, int v2 )
{
  real y1, y2;
  
  y1 = rndchi ( v1 );
  y2 = rndchi ( v2 );
  return((y1*v2) / (y2*v1));
  
}


/*##  rndt - random real number with the t-distribution */
/*  SYNOPSIS */
/*    int v */
/*    real r, rndt */
/*    r = rndt ( v ) */
real rndt ( int v )
{
  real y1, y2;
  
  y1 = rndnor ( 0.0, 1.0 );
  y2 = rndchi ( v );
  return(y1 / sqrt ( y2/v ));
  
}

/*##  rndbin - random int with the binomial distribution */
/*  SYNOPSIS */
/*    int trials, i, rndbin */
/*    real prob */
/*    i = rndbin ( trials, prob ) */
/*  DESCRIPTION */
/*      If some event occurs with probability p and if we carry out t */
/*    independent trials, then the total number of occurrences N equals */
/*    n with probability: */
/*                             t    n     t-n */
/*                            ( )  p  (1-p) */
/*                             n */
int rndbin ( int trials, real prob )
{
  int i;
  int RET;
  
  RET = 0;
  for ( i=1; i <= trials; i=i+1 )
    if ( rnd(0) <= prob )
      RET = RET + 1;
  
  return(RET);
}


/*##  rndpoi - random int with the Poisson distribution */
/*  SYNOPSIS */
/*    real mean */
/*    int i, rndpoi */
/*    i = rndpoi ( mean ) */
/*  DESCRIPTION */
/*      The Poisson distribution is related to the exponential distribution */
/*    as the binomial distribution is related to the geometric: it represents */
/*    the number of occurrences, per unit time, of an event which can occur */
/*    at any instant of time; for example, the number of alpha particles */
/*    emitted by a radioactive substance in a single second has a Poisson */
/*    distribution.  The probability that N = n (u is the mean) is: */
/*                                -u  n */
/*                               e   u  /  n ! */
int rndpoi ( real mean )
{
  
  real p, q;
  int RET;
  
  p = exp ( -mean );
  RET = 0;
  q = 1.0;
  
  while (TRUE) /*repeat forever */
    {
      q = q * rnd(0);
      
      if ( q < p )
	break;
      
      RET = RET + 1;
    }
  
  return(RET);
}
