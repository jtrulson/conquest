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

#include "c_defs.h"
#include "global.h"
#include "cb.h"
#include "color.h"
#include "ui.h"
#include "rndlb.h"

#include "conqnet.h"
#include "conqutil.h"

#define NOEXTERN_CONF
#include "conf.h"
#undef NOEXTERN_CONF

#include "protocol.h"

/* set default sys config */
static void setSysConfDefaults(void)
{
    SysConf.NoDoomsday = false;
    SysConf.DoRandomRobotKills = false;
    SysConf.AllowVacant = false;
    SysConf.AllowSwitchteams = true;
    SysConf.UserExpiredays = DEFAULT_USEREXPIRE;
    SysConf.LogMessages = false;
    SysConf.AllowRefits = true;
    SysConf.AllowSlingShot = false;
    SysConf.NoTeamWar = false;
    SysConf.NoDrift = false;
    SysConf.Closed = false;

    utStrncpy(SysConf.ServerName, "Generic Conquest Server",
              CONF_SERVER_NAME_SZ);
    utStrncpy(SysConf.ServerMotd, "Keep your shields up in battle.",
              CONF_SERVER_MOTD_SZ);
    utStrncpy(SysConf.ServerContact, "root@localhost",
              META_GEN_STRSIZE);

    // randomly select one. semInit will just recast this to key_t.
    // Use 0x1701XXXX, where XXXX is random.
    SysConf.semKey = 0x17010000 + rndint(0, ((1 << 16) - 1));
    return;
}

/* set default user config */
void setUserConfDefaults(void)
{
    int i, j;

    // conf defaults
    UserConf.DoAlarms = true;
    UserConf.DoIntrudeAlert = true;
    UserConf.DoNumMap = true;
    UserConf.Terse = false;
    UserConf.MessageBell = true;
    UserConf.NoRobotMsgs = false;

    UserConf.UpdatesPerSecond = 10;

    UserConf.DistressToFriendly = false;
    UserConf.AltHUD = false;
    UserConf.hudInfo = true;
    UserConf.DoLocalLRScan = true;
    UserConf.DoETAStats = true;
    UserConf.EnemyShipBox = true;
    UserConf.doVBG = true;

    UserConf.DoTacBkg = false;
    UserConf.DoTacShade = 50;

    UserConf.musicVol = 100;
    UserConf.effectsVol = 100;

    for (i=0; i<MAX_MACROS; i++)
        UserConf.MacrosF[i][0] = 0;

    for (i=0; i<CONF_MAXBUTTONS; i++)
    {
        for (j=0; j<CONF_MAXMODIFIERS; j++)
            UserConf.Mouse[i][j][0] = 0;

        /* set up default mouse macros, '\a' means 'angle' substitution */

        /* fire phaser (left button (0), no modifiers) */
        utStrncpy(UserConf.Mouse[0][0], "f\\a\r", MAX_MACRO_LEN);

        /* set course (middle button (1), no modifiers) */
        utStrncpy(UserConf.Mouse[1][0], "k\\a\r", MAX_MACRO_LEN);

        /* fire torp (right button (2), no modifiers) */
        utStrncpy(UserConf.Mouse[2][0], "p\\a\r", MAX_MACRO_LEN);

        /* zoom in (scroll up, no modifiers) */
        utStrncpy(UserConf.Mouse[3][0], "]", MAX_MACRO_LEN);

        /* zoom out (scroll down, no modifiers) */
        utStrncpy(UserConf.Mouse[4][0], "[", MAX_MACRO_LEN);
    }

    return;
}

/* here we look for a ~/.conquest/ directory, and try to create it if it
   doesn't exist */
static void checkCreateUserConfDir(void)
{
    char buffer[PATH_MAX];
    struct stat sbuf;
    char *home;

    if ((home = getenv(CQ_USERHOMEDIR)) == NULL)
    {
        utLog("checkCreateUserConfDir(): getenv(%s) failed", CQ_USERHOMEDIR);

        return;
    }

    /* start building the filename */
    snprintf(buffer, PATH_MAX, "%s/%s", home, CQ_USERCONFDIR);

    if (stat(buffer, &sbuf) >= 0)
    {
        /* make sure it's a directory */

#ifdef S_ISDIR
        if (!S_ISDIR(sbuf.st_mode))
        {
            utLog("checkCreateUserConfDir(): %s exists, but is not a directory.",
                  buffer);
            return;
        }
#endif

        /* else everythings ok (if S_ISDIR was understood) */
        return;
    }

    /* try to create it */
#if defined(MINGW)
    if (mkdir(buffer) < 0)
#else
        if (mkdir(buffer, (S_IRUSR | S_IWUSR | S_IXUSR |S_IRGRP | S_IXGRP)) < 0)
#endif
        {                             /* not happy */
            utLog("checkCreateUserConfDir(): mkdir(%s) failed: %s.",
                  buffer, strerror(errno));
            return;
        }
        else
        {
            utLog("Created '%s' config directory.",
                  buffer);
        }

    /* we're done */

    return;
}

/* GetSysConf(int checkonly) - load system-wide configuration values */
/*  If checkonly is true, then just load internel defaults, and the existing
    conquesrtc file, if there.  Don't output errmsgs since this will
    only be true when 'priming the pump' in conqoper -C, before writing a
    new conquest.conf file (preserving existing settings if possible).
*/
int GetSysConf(int checkonly)
{
    FILE *conf_fd;
    int i, j;
    char conf_name[PATH_MAX] = {};
    char buffer[BUFFER_SIZE_256] = {};
    int FoundOne = false;
    int buflen;
    char *bufptr;

    /* init some defaults */
    setSysConfDefaults();

    /* start building the filename */
    snprintf(conf_name, PATH_MAX, "%s/%s/%s",
             utGetPath(CONQETC), gameSubdirectory.get().c_str(),
             SYSCONFIG_FILE);

    if ((conf_fd = fopen(conf_name, "r")) == NULL)
    {
        int err;

        err = errno;

        utLog("GetSysConf(): fopen(%s) failed: %s",
              conf_name,
              strerror(err));

        if (checkonly != true)
	{
            fprintf(stderr, "Error opening system-wide config file: %s: %s\n",
                    conf_name,
                    strerror(err));

            fprintf(stderr, "'conqoper -C' needs to be run. Using internal defaults.\n");
	}

        return(-1);
    }

#ifdef DEBUG_CONFIG
    utLog("GetSysConf(): Opened '%s'", conf_name);
#endif

    /* Do it dude... */
    while (fgets(buffer, BUFFER_SIZE_256 - 1, conf_fd) != NULL)
    {				/* get em one at a time */
        buflen = strlen(buffer);

        if (buffer[0] == '#')
            continue;

        if (buffer[0] == '\n')
            continue;

        if (buflen == 0)
            continue;		/* shouldn't happen...but the universe is
				   mysterious... */

        buffer[buflen - 1] = 0;	/* remove trailing LF */
        buflen--;


#ifdef DEBUG_CONFIG
        utLog("GetSysConf(): got '%s'", buffer);
#endif

        /* check for everything else */

        FoundOne = false;

        for (j = 0; (j < SysCfEnd) && (FoundOne == false); j++)
	{
            if (SysConfData[j].ConfName != NULL)
                if (strncmp(SysConfData[j].ConfName,
                            buffer,
                            strlen(SysConfData[j].ConfName)) == 0)
                {			/* we found a valid variable */
                    if (buflen > 0)
                    {		/* split off the value */
                        bufptr = &buffer[strlen(SysConfData[j].ConfName)];

                        switch(SysConfData[j].ConfType)
                        {
                        case CTYPE_NULL:
                            /* special check for version */

                            if (j == SYSCF_VERSION)
                            {
                                if(strncmp((char *)SysConfData[j].ConfValue,
                                           bufptr,
                                           strlen((char *)SysConfData[j].ConfValue)) == 0)
                                {
                                    SysConfData[j].Found = true;
#ifdef DEBUG_CONFIG
                                    utLog("GetSysConf(): got correct version = '%s'", buffer);
#endif
                                }
                            }

                            break;

                        case CTYPE_BOOL:
                            i = process_bool(bufptr);
                            if (i != -1)
                            {
                                *((int *) SysConfData[j].ConfValue) = i;
                                SysConfData[j].Found = true;
                                FoundOne = true;
                            }
                            break;

                        case CTYPE_NUMERIC:
                            if (utIsDigits(bufptr))
                            {
                                int *n = ((int *) SysConfData[j].ConfValue);

                                *n = atoi(bufptr);

                                if (*n < SysConfData[j].min)
                                    *n = SysConfData[j].min;

                                if (*n > SysConfData[j].max)
                                    *n = SysConfData[j].max;

                                SysConfData[j].Found = true;
                                FoundOne = true;
                            }
                            break;

                        case CTYPE_UNUMERIC:
                            if (utIsDigits(bufptr))
                            {
                                uint_t *n = ((uint_t *) SysConfData[j].ConfValue);

                                *n = atoi(bufptr);

                                if (*n < SysConfData[j].min)
                                    *n = SysConfData[j].min;

                                if (*n > SysConfData[j].max)
                                    *n = SysConfData[j].max;

                                SysConfData[j].Found = true;
                                FoundOne = true;
                            }
                            break;

                        case CTYPE_STRING:
                            memset((char *)(SysConfData[j].ConfValue), 0,
                                   SysConfData[j].max);
                            utStrncpy((char *)(SysConfData[j].ConfValue),
                                    bufptr, SysConfData[j].max);
                            ((char *)SysConfData[j].ConfValue)[SysConfData[j].max - 1] = 0;

                            SysConfData[j].Found = true;
                            FoundOne = true;
                            break;


                        } /* switch */
                    } /* if */
                } /* if */
	} /* for */
    } /* while */


    if (!feof(conf_fd))
    {
        fprintf(stderr, "GetSysConf(): Error reading %s: %s\n",
                conf_name,
                strerror(errno));
        fclose(conf_fd);

        return(-1);
    }

    fclose(conf_fd);

    /* Now we check them all to see if one  */
    /* of the options wasn't read in. If one */
    /* is found, notify user that operator
       needs to run conqoper -C . */

    /* see if we found the version */
    if (SysConfData[SYSCF_VERSION].Found == false)
    {				/* conquest.conf version not found */
#ifdef DEBUG_CONFIG
        utLog("GetSysConf(): Incorrect version found.  Update needed.");
#endif

        if (checkonly != true)
	{
            fprintf(stderr, "The %s file needs to be updated by an operator with\n",
                    conf_name);
            fprintf(stderr, "  'conqoper -C'\n");
	}
    }
    else
    {				/* version found. check everything else */
        for (i=0; i<SysCfEnd; i++)
	{
            if (SysConfData[i].ConfType != CTYPE_NULL)
                if (SysConfData[i].Found != true)
                {
#ifdef DEBUG_CONFIG
                    utLog("GetSysConf(): option '%s' not found - Update needed.",
                          SysConfData[i].ConfName);
#endif
                    if (checkonly != true)
                    {
                        fprintf(stderr, "The %s file needs to be updated by an operator with\n",
                                conf_name);
                        fprintf(stderr, "  'conqoper -C'\n");
                    }

                    break;
                }
	}
    }

    return(true);
}

/* can't compile this if there is no GUI */
#if defined(_CQKEYS_H)
/* parse a mouse macro mod/but string */
static int
parseMouseModNum(char *str, uint32_t *mods, uint32_t *button)
{
    int done = false;
    if (!mods || !button || !str)
        return false;

    if (!*str)
        return false;

    *mods = 0;
    *button = 0;
    uint32_t tmpButton = 0;

    while (*str && !done)
    {
        if (!isdigit(*str))
        {
            /* check for 'a', 'c', and 's' */
            switch (*str)
            {
            case 'a':
                *mods |= (CQ_KEY_MOD_ALT >> CQ_MODIFIER_SHIFT) ;
                break;
            case 'c':
                *mods |= (CQ_KEY_MOD_CTRL >> CQ_MODIFIER_SHIFT);
                break;
            case 's':
                *mods |= (CQ_KEY_MOD_SHIFT >> CQ_MODIFIER_SHIFT);
                break;
            default:
                utLog("parseMouseModNum: Invalid modifier char '%c'",
                      *str);
                return false;
                break;
            }
            str++;
        }
        else                      /* the mouse button number, always last */
        {
            tmpButton = atoi(str);
            done = true;
        }
    }

    if (tmpButton >= CONF_MAXBUTTONS)
        return false;

    *button = tmpButton;

    if (*mods >= CONF_MAXMODIFIERS)
        return false;

    return true;
}

#endif /* _CQKEYS_H */
/* get user's configuration */
int GetConf(int usernum)
{
    FILE *conf_fd;
    int i, j, n;
    char conf_name[PATH_MAX] = {};
    char *homevar, *cptr;
    char buffer[BUFFER_SIZE_256];
    int buflen;
    char *bufptr;
    int FoundOne = false;

    /* init some defaults */
    setUserConfDefaults();

    /* check for the user config dir */
    checkCreateUserConfDir();

    /* start building the filename */
    if ((homevar = getenv(CQ_USERHOMEDIR)) == NULL)
    {
        utLog("GetConf(): getenv(%s) failed", CQ_USERHOMEDIR);

        fprintf(stderr, "Can't get %s environment variable. Exiting\n",
                CQ_USERHOMEDIR);
        return(-1);
    }

    snprintf(conf_name, PATH_MAX, "%s/%s/%s",
             homevar, CQ_USERCONFDIR, CONFIG_FILE);

    if ((conf_fd = fopen(conf_name, "r")) == NULL)
    {
        if (errno != ENOENT)
	{
            utLog("GetConf(): fopen(%s) failed: %s, using defaults",
                  conf_name,
                  strerror(errno));

            fprintf(stderr, "Error opening config file: %s: %s, using defaults\n",
                    conf_name,
                    strerror(errno));

            return(-1);
	}

#ifdef DEBUG_CONFIG
        utLog("GetConf(): No config file.");
#endif

        if (MakeConf(conf_name) == -1)
            return(-1);

        return(false);		/* no config file */
    }

#ifdef DEBUG_CONFIG
    utLog("GetConf(): Opened '%s'", conf_name);
#endif


    /* We got it! Now lets process it. */

    while (fgets(buffer, BUFFER_SIZE_256 - 1, conf_fd) != NULL)
    {				/* get em one at a time */
        buflen = strlen(buffer);

        if (buffer[0] == '#')
            continue;

        if (buffer[0] == '\n')
            continue;

        if (buflen == 0)
            continue;		/* shouldn't happen...but the universe is
				   mysterious... */

        buffer[buflen - 1] = 0;	/* remove trailing LF */
        buflen--;

#ifdef DEBUG_CONFIG
        utLog("GetConf(): got '%s'", buffer);
#endif

        FoundOne = false;

        for (j = 0; (j < CfEnd) && (FoundOne == false); j++)
	{
            if (ConfData[j].ConfName != NULL)
                if(strncmp(ConfData[j].ConfName,
                           buffer,
                           strlen(ConfData[j].ConfName)) == 0)
                {			/* we found a valid variable */
                    if (buflen > 0)
                    {		/* split off the value */
                        bufptr = &buffer[strlen(ConfData[j].ConfName)];

                        switch(ConfData[j].ConfType)
                        {
                        case CTYPE_NULL:
                            /* special check for version */
                            if (j == CF_VERSION)
                            {
                                if(strncmp((char *)ConfData[j].ConfValue,
                                           bufptr,
                                           strlen((char *)ConfData[j].ConfValue)) == 0)
                                {
                                    ConfData[j].Found = true;
                                    FoundOne = true;
#ifdef DEBUG_CONFIG
                                    utLog("GetConf(): got correct version = '%s'", buffer);
#endif
                                }
                            }
                            break;

                        case CTYPE_BOOL:
                            i = process_bool(bufptr);
                            if (i != -1)
                            {
                                *((int *) ConfData[j].ConfValue) = i;
                                ConfData[j].Found = true;
                                FoundOne = true;
                            }
                            break;

                        case CTYPE_MACRO:
                            /* need to parse out macro number. */
                            cptr = strchr(bufptr, '=');
                            if (cptr != NULL)
                            {	/* valid entry */
                                *cptr = '\0';
                                n = atoi(bufptr);
                                if (n > 0 && n <= MAX_MACROS)
                                { /* valid macro number */
#ifdef DEBUG_CONFIG
                                    utLog("GetConf(): Macro %d[%d], value '%s' read",
                                          n, n - 1, (char *)cptr + 1);
#endif
                                    /* clean it first... */
                                    memset((char *)(((char *)ConfData[j].ConfValue)
                                                    + ((n - 1) * MAX_MACRO_LEN)),
                                           0,
                                           MAX_MACRO_LEN);
                                    utStrncpy((char *)(((char *)ConfData[j].ConfValue)
                                                       + ((n - 1) * MAX_MACRO_LEN)),
                                              Str2Macro((char *)cptr + 1),
                                              MAX_MACRO_LEN);
                                    ConfData[j].Found = true;
                                    FoundOne = true;
                                }
                            }

                            break;

                            /* no GL, no mouse */
#if defined(_CQKEYS_H)
                        case CTYPE_MOUSE:
                        {
                            uint32_t mods;
                            uint32_t button;

                            /* need to parse out mods/button #. */
                            cptr = strchr(bufptr, '=');

                            if (cptr != NULL)
                            {	/* valid entry */
                                *cptr = '\0';
                                if (parseMouseModNum(bufptr,
                                                     &mods, &button))
                                { /* got a valid button and modifier(s) */

#if defined(DEBUG_CONFIG)
                                    utLog("GetConf(): Mouse %d %d, value '%s' read",
                                          button, mods, (char *)cptr + 1);
#endif
                                    /* clean it first... */
                                    memset(UserConf.Mouse[button][mods],
                                           0,
                                           MAX_MACRO_LEN);
                                    utStrncpy(UserConf.Mouse[button][mods],
                                              Str2Macro((char *)cptr + 1),
                                              MAX_MACRO_LEN);
                                    ConfData[j].Found = true;
                                    FoundOne = true;
                                }
                            }
                        }
			break;
#endif /* _CQKEYS_H */

                        case CTYPE_NUMERIC:
                            if (utIsDigits(bufptr))
                            {
                                int *n = ((int *) ConfData[j].ConfValue);

                                *n = atoi(bufptr);

                                if (*n < ConfData[j].min)
                                    *n = ConfData[j].min;

                                if (*n > ConfData[j].max)
                                    *n = ConfData[j].max;

                                ConfData[j].Found = true;
                                FoundOne = true;
                            }
                            break;

                        case CTYPE_UNUMERIC:
                            if (utIsDigits(bufptr))
                            {
                                uint_t *n = ((uint_t *) ConfData[j].ConfValue);

                                *n = atoi(bufptr);

                                if (*n < ConfData[j].min)
                                    *n = ConfData[j].min;

                                if (*n > ConfData[j].max)
                                    *n = ConfData[j].max;

                                ConfData[j].Found = true;
                                FoundOne = true;
                            }
                            break;

                        } /* switch */
                    } /* if */
                } /* if */
	} /* for */
    } /* while */


    if (!feof(conf_fd))
    {
        fprintf(stderr, "GetConf(): Error reading %s: %s\n",
                conf_name,
                strerror(errno));
        fclose(conf_fd);

        return(-1);
    }

    fclose(conf_fd);

    /* Now we check them all to see if one  */
    /* of the options wasn't read in. If one */
    /* is found, re-write the config file. */

    /* see if we found the version */
    if (ConfData[CF_VERSION].Found == false)
    {				/* conquest.conf version not found */
#ifdef DEBUG_CONFIG
        utLog("GetConf(): Incorrect version found. - rebuilding");
#endif
        MakeConf(conf_name);	/* rebuild */
    }
    else
    {				/* version found. check everything else */
        for (i=0; i<CfEnd; i++)
	{
            if (ConfData[i].ConfType != CTYPE_NULL &&
                ConfData[i].ConfType != CTYPE_MACRO &&
                ConfData[i].ConfType != CTYPE_MOUSE )
                if (ConfData[i].Found != true)
                {
#ifdef DEBUG_CONFIG
                    utLog("GetConf(): option '%s' not found - rebuilding.",
                          ConfData[i].ConfName);
#endif
                    MakeConf(conf_name);	/* rebuild - one not found */
                    break;		/* no need to cont if we rebuilt */
                }
	}
    }
    return(true);
}

/* SaveUserConfig(int unum) - do what the name implies ;-) */
int SaveUserConfig(void)
{
    char conf_name[PATH_MAX] = {};
    char *homevar;

    /* start building the filename */
    if ((homevar = getenv(CQ_USERHOMEDIR)) == NULL)
    {
        utLog("SaveUserConfig(): getenv(%s) failed", CQ_USERHOMEDIR);

        fprintf(stderr,
                "SaveUserConfig(): Can't get %s environment variable. Exiting\n",
                CQ_USERHOMEDIR);
        return(-1);
    }

    snprintf(conf_name, PATH_MAX, "%s/%s/%s",
             homevar, CQ_USERCONFDIR, CONFIG_FILE);

#ifdef DEBUG_OPTIONS
    utLog("SaveUserConfig(): saving user config: conf_name = '%s'", conf_name);
#endif

    return(MakeConf(conf_name));
}

/* SaveSysConfig() - do what the name implies ;-) */
int SaveSysConfig(void)
{
#ifdef DEBUG_OPTIONS
    utLog("SaveSysConfig(): saving system config");
#endif

    return(MakeSysConf());
}

/* process a string value - converts it into a macro string */
const char *Str2Macro(const char *str)
{
    static char retstr[BUFFER_SIZE_256];
    const char *s;
    int i;

    i = 0;
    s = str;
    retstr[0] = 0;

    while (*s && i < (BUFFER_SIZE_256 - 1))
    {
        if (*s == '\\')
	{
            s++;

            if (*s)
	    {
                switch (*s)
		{
		case 'r':
                    retstr[i++] = '\r';
                    s++;
                    break;
		case 't':
                    retstr[i++] = '\t';
                    s++;
                    break;
		case 'n':
                    retstr[i++] = '\n';
                    s++;
                    break;
		case '\\':
                    retstr[i++] = '\\';
                    s++;
                    break;
		default:
                    retstr[i++] = '\\';
                    retstr[i++] = *s;
                    s++;
                    break;
		}
	    }

	}
        else
	{
            retstr[i++] = *s;
            s++;
	}
    }

    if (i < BUFFER_SIZE_256)
        retstr[i] = 0;
    else
        retstr[BUFFER_SIZE_256 - 1] = 0;

    return(retstr);
}

/* process a macro value - converts it back into a printable string */
char *Macro2Str(char *str)
{
    static char retstr[BUFFER_SIZE_256];
    char *s;
    int i;

    i = 0;
    s = str;
    retstr[0] = 0;

#if defined(DEBUG_CONFIG)
    if (str != NULL )
        utLog("Macro2Str('%s')", str);
    else
        utLog("Macro2Str(NULL)");
#endif

    while (*s && i < (BUFFER_SIZE_256 - 1))
    {
        switch (*s)
	{
	case '\r':
            retstr[i++] = '\\';
            retstr[i++] = 'r';
            s++;
            break;
	case '\t':
            retstr[i++] = '\\';
            retstr[i++] = 't';
            s++;
            break;
	case '\n':
            retstr[i++] = '\\';
            retstr[i++] = 'n';
            s++;
            break;
#if 0
	case '\\':
            retstr[i++] = '\\';
            retstr[i++] = '\\';
            s++;
            break;
#endif
	default:
            retstr[i++] = *s;
            s++;
            break;
	}
    }

    if (i < BUFFER_SIZE_256)
        retstr[i] = 0;
    else
        retstr[BUFFER_SIZE_256 - 1] = 0;

    return(retstr);
}


/* process a boolean value */

int process_bool(char *bufptr)
{
    // lower case the string
    char *s = bufptr;

    while(*s)
    {
        *s = (char)tolower(*s);
        s++;
    }

    if ((strstr("false", bufptr) != NULL) ||
        (strstr("no", bufptr) != NULL)    ||
        (strstr("off", bufptr) != NULL))
    {
        return(false);
    }
    else if ((strstr("true", bufptr) != NULL) ||
             (strstr("yes", bufptr) != NULL)  ||
             (strstr("on", bufptr) != NULL))
    {
        return(true);
    }
    else
    {
        fprintf(stderr, "process_bool(): error parsing '%s', \n\t%s\n",
                bufptr,
                "Value must be yes or no or true or false.");
        return(-1);
    }
}

/* MakeConf(filename) - make a fresh, spiffy new conquest.conf file. */

int MakeConf(char *filename)
{
    FILE *conf_fd;
    int i, j, n;

    unlink(filename);

    if ((conf_fd = fopen(filename, "w")) == NULL)
    {
        utLog("Makeconf(): fopen(%s) failed: %s",
              filename,
              strerror(errno));

        fprintf(stderr, "Error creating %s: %s\n",
                filename,
                strerror(errno));

        return(-1);
    }


    for (j=0; j<CfEnd; j++)
    {
        /* option header first */
        i = 0;

        /* check for version - this won't be output in the loop below due to the
           fact that CF_VERSION is of type CTYPE_NULL */

        if (j == CF_VERSION)
	{
            fprintf(conf_fd, "%s%s\n",
                    ConfData[j].ConfName,
                    (char *)ConfData[j].ConfValue);
	}

        while (ConfData[j].ConfComment[i] != NULL)
            fprintf(conf_fd, "%s\n", ConfData[j].ConfComment[i++]);

        /* now write the variable and value if not a
           CTYPE_NULL */
        if (ConfData[j].ConfType != CTYPE_NULL)
            switch (ConfData[j].ConfType)
            {
                /* no opengl, no mouse */
#if defined(_CQKEYS_H)
            case CTYPE_MOUSE:
            {
                int b, m;

                for (b=0; b < CONF_MAXBUTTONS; b++)
                {
                    for (m=0; m<CONF_MAXMODIFIERS; m++)
                    {
                        if (strlen(UserConf.Mouse[b][m]) != 0)
                        {
                            char buffer[16]; /* max 'acs\0' */

                            buffer[0] = 0;
                            if (m & (CQ_KEY_MOD_SHIFT >> CQ_MODIFIER_SHIFT))
                                strcat(buffer, "s");
                            if (m & (CQ_KEY_MOD_CTRL  >> CQ_MODIFIER_SHIFT))
                                strcat(buffer, "c");
                            if (m & (CQ_KEY_MOD_ALT   >> CQ_MODIFIER_SHIFT))
                                strcat(buffer, "a");

                            fprintf(conf_fd, "%s%s%d=%s\n",
                                    ConfData[j].ConfName,
                                    buffer,
                                    b,
                                    Macro2Str(UserConf.Mouse[b][m]));
                        }

                    }
                }
            }
	    break;
#endif /* _CQKEYS_H */

            case CTYPE_MACRO:
                for (n=0; n < MAX_MACROS; n++)
                {
                    if (strlen((char *)(((char *)ConfData[j].ConfValue)
					+ (n * MAX_MACRO_LEN))) != 0)
                    {
                        fprintf(conf_fd, "%s%d=%s\n",
                                ConfData[j].ConfName,
                                n + 1,
                                Macro2Str((char *)(((char *)ConfData[j].ConfValue)
                                                   + (n * MAX_MACRO_LEN))));
                    }
                }
                break;

            case CTYPE_BOOL:
                fprintf(conf_fd, "%s%s\n",
                        ConfData[j].ConfName,
                        (*((int *)ConfData[j].ConfValue) == true) ? "true" : "false");
                break;

            case CTYPE_NUMERIC:
                fprintf(conf_fd, "%s%d\n",
                        ConfData[j].ConfName,
                        *((int *)ConfData[j].ConfValue));
                break;

            case CTYPE_UNUMERIC:
                fprintf(conf_fd, "%s%u\n",
                        ConfData[j].ConfName,
                        *((uint_t *)ConfData[j].ConfValue));
                break;

            }
        /* output a blank line */
        fprintf(conf_fd, "\n");
    }

    fclose(conf_fd);

#ifdef DEBUG_CONFIG
    utLog("MakeConf(%s): Succeeded.", filename);
#endif

    /* that's it! */
    return(true);
}


/* MakeSysConf() - make a fresh, spiffy new sys-wide conquest.conf file.*/

int MakeSysConf()
{
    FILE *sysconf_fd;
    char conf_name[BUFFER_SIZE_256];
    int i, j, n;

    snprintf(conf_name, BUFFER_SIZE_256, "%s/%s/%s", utGetPath(CONQETC),
             gameSubdirectory.get().c_str(), SYSCONFIG_FILE);
    umask(002);
    unlink(conf_name);

    if ((sysconf_fd = fopen(conf_name, "w")) == NULL)
    {
        utLog("MakeSysconf(): fopen(%s) failed: %s",
              conf_name,
              strerror(errno));
        return(-1);
    }

    utLog("OPER: Updating %s file...", conf_name);

    for (j=0; j<SysCfEnd; j++)
    {
        /* option header first */
        i = 0;

        /* check for version - this won't be output in the loop below due to the
           fact that SYSCF_VERSION is of type CTYPE_NULL */

        if (j == SYSCF_VERSION)
	{
            fprintf(sysconf_fd, "%s%s\n",
                    SysConfData[j].ConfName,
                    (char *)SysConfData[j].ConfValue);
	}

        while (SysConfData[j].ConfComment[i] != NULL)
            fprintf(sysconf_fd, "%s\n", SysConfData[j].ConfComment[i++]);

        /* now write the variable and value if not a
           CTYPE_NULL*/
        if (SysConfData[j].ConfType != CTYPE_NULL)
            switch (SysConfData[j].ConfType)
            {
            case CTYPE_STRING:
                fprintf(sysconf_fd, "%s%s\n",
                        SysConfData[j].ConfName,
                        (char *)SysConfData[j].ConfValue);
                break;
            case CTYPE_MACRO:
                for (n=0; n < MAX_MACROS; n++)
                {
                    if (strlen(((char **)SysConfData[j].ConfValue)[n]) != 0)
                    {
                        fprintf(sysconf_fd, "%s%d=%s\n",
                                SysConfData[j].ConfName,
                                n + 1,
                                Macro2Str(((char **)SysConfData[j].ConfValue)[n]));
                    }
                }
                break;

            case CTYPE_BOOL:
                fprintf(sysconf_fd, "%s%s\n",
                        SysConfData[j].ConfName,
                        (*((int *)SysConfData[j].ConfValue) == true) ? "true" : "false");
                break;

            case CTYPE_NUMERIC:
                fprintf(sysconf_fd, "%s%d\n",
                        SysConfData[j].ConfName,
                        *((int *)SysConfData[j].ConfValue));
                break;

            case CTYPE_UNUMERIC:
                fprintf(sysconf_fd, "%s%u\n",
                        SysConfData[j].ConfName,
                        *((uint_t *)SysConfData[j].ConfValue));
                break;
            }
        /* output a blank line */
        fprintf(sysconf_fd, "\n");
    }

    fclose(sysconf_fd);

#ifdef DEBUG_CONFIG
    utLog("MakeSysConf(%s): Succeeded.", conf_name);
#endif

    /* that's it! */
    return(true);
}

uint32_t getServerFlags(void)
{
    uint32_t f = SERVER_F_NONE;

    /* get the current flags */
    if (SysConf.AllowRefits)
        f |= SERVER_F_REFIT;

    if (SysConf.AllowVacant)
        f |= SERVER_F_VACANT;

    if (SysConf.AllowSlingShot)
        f |= SERVER_F_SLINGSHOT;

    if (SysConf.NoDoomsday)
        f |= SERVER_F_NODOOMSDAY;

    if (SysConf.DoRandomRobotKills)
        f |= SERVER_F_KILLBOTS;

    if (SysConf.AllowSwitchteams)
        f |= SERVER_F_SWITCHTEAM;

    if (SysConf.NoTeamWar)
        f |= SERVER_F_NOTEAMWAR;

    if (SysConf.NoDrift)
        f |= SERVER_F_NODRIFT;

    if (SysConf.Closed)
        f |= SERVER_F_CLOSED;

    return f;
}
