/* 
 * users.c - user stuff
 *
 * $Id$
 *
 * Copyright 1998 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "color.h"

#define MAX_USERLEN 10		/* only 10 chars for users */

static int checkuname(char *username);


int Logon(char *username, char *password)
{
  int i, col, lin, slin, lenc1, unum;
  char ch;
  char nm[SIZEUSERNAME], pw[SIZEUSERNAME], pwr[SIZEUSERNAME];
  string c1=" CCC    OOO   N   N   QQQ   U   U  EEEEE   SSSS  TTTTT";
  string c2="C   C  O   O  NN  N  Q   Q  U   U  E      S        T";
  string c3="C      O   O  N N N  Q   Q  U   U  EEE     SSS     T";
  string c4="C   C  O   O  N  NN  Q  Q   U   U  E          S    T";
  string c5=" CCC    OOO   N   N   QQ Q   UUU   EEEEE  SSSS     T";

  if (!IsRemoteUser())
    {				/* if local, just get uname */
      glname(username);
      password[0] = EOS;	/* locals don't have passwords */
      return(UT_LOCAL);
    }
				/* if we're here it means we're remote,
				   need to get password and username here */
#if defined DEBUG_SERVER
  clog("Detected a remote user\n");
#endif

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

  lin += 4;
  slin = lin;

  nm[0] = EOS;

  while (TRUE)			/* login loop */
    {
      slin = lin;
      cdclrl( slin, cmaxlin - slin - 1 );
      cprintf( slin, col, ALIGN_CENTER, 
	       "#%dWelome to Conquest, Please login...",
	       SpecialColor);

      slin += 3;
      cdputs("You can use A-Z, a-z, 0-9, '_', or '-'.", MSG_LIN1, 1);
      ch = cdgetx( "Username: ", slin, 1, TERMS, nm, MAX_USERLEN, TRUE );

      if (ch == TERM_ABORT || nm[0] == EOS)
	{			/* if empty user, or abort char, leave */
	  cdend();
	  exit(2);
	}

      if (checkuname(nm) == FALSE)
	{			/* invalid username */
	  cdbeep();
	  cdputs("Invalid character in username.", MSG_LIN2, 1);
	  nm[0] = EOS;
	  continue;
	}
				/* see if remote user exists */
      if (!gunum( &unum, nm, UT_REMOTE ) )
	{			/* nope... */
	  slin++;
	  if (askyn("User doesn't exist. Is this a new user? ", slin, 1))
	    {			/* yep */
	      pw[0] = EOS;
	      cdclrl( MSG_LIN1, 2  );
	      cdputs("Use any printable characters.", MSG_LIN1, 1);
	      ch = cdgetx( "Password: ", slin, 1, 
			   TERMS, pw, SIZEUSERNAME - 1, FALSE );
	      
	      slin++;
	      pwr[0] = EOS;
	      cdclrl( MSG_LIN1, 2  );
	      cdputs("Use any printable characters.", MSG_LIN1, 1);
	      ch = cdgetx( "Retype Password: ", slin, 1, 
			   TERMS, pwr, SIZEUSERNAME - 1, FALSE );
	      
	      if (strcmp(pw, pwr) != 0)
		{			/* pw's don't match, start over */
		  cdbeep();
		  cdclrl( MSG_LIN2, 1  );
		  cdputs("Passwords don't match.", MSG_LIN2, 1);
		  cdrefresh();
		  sleep(2);
		  continue;
		}
				/* if we're here, we have a username
				   and password (new user) - time
				   to rock. */
				/* ideally we should encryt it here */
	      break;
	    }
	  else
	    {
	      continue;		/* restart */
	    }
	}
      else
	{			/* exists, verify that the pw is valid */
	  slin++;
	  pw[0] = EOS;
	  cdclrl( MSG_LIN1, 2  );
	  cdputs("Use any printable characters.", MSG_LIN1, 1);
	  ch = cdgetx( "Password: ", slin, 1, 
		       TERMS, pw, SIZEUSERNAME - 1, FALSE );

	  if (strcmp(pw, Users[unum].pw) != 0)
	    {			/* invalid pw */
		  cdbeep();
		  cdclrl( MSG_LIN2, 1  );
		  cdputs("Invalid Password.", MSG_LIN2, 1);
		  cdrefresh();
		  sleep(2);
		  continue;
	    }
	  
				/* good pw - go for the gusto */
	  break;
	}
    }

				/* if we're here, we're legal */
  strcpy(username, nm);
  strcpy(password, pw);

  return(UT_REMOTE);
}

  
static int checkuname(char *username)
{
  char *s = username;

  while (*s)
    {
      if (!isalnum(*s) && *s != '-' && *s != '_')
	{			/* if invalid character... */
	  return(FALSE);
	}
      s++;
    }

  return(TRUE);
}
	
