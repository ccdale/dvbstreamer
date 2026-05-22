/*
Copyright (C) 2006  Adam Charrett

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

setup.c

Setups the database for the main application.

*/

#include "config.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "parsezap.h"
#include "dbase.h"
#include "multiplexes.h"
#include "services.h"
#include "main.h"
#include "logging.h"
#include "events.h"
#include "lnb.h"

/*******************************************************************************
* Defines                                                                      *
*******************************************************************************/

#define INIT(_func, _name) \
    do {\
        if (_func) \
        { \
            LogModule(LOG_ERROR, SETUP, "Failed to initialise %s.\n", _name); \
            exit(1);\
        }\
        LogModule(LOG_DEBUGV, SETUP, "Initialised %s.\n", _name);\
    }while(0)

#define DEINIT(_func, _name) \
    do {\
        _func;\
        LogModule(LOG_DEBUGV, SETUP, "Deinitialised %s\n", _name);\
    }while(0)

/*******************************************************************************
* Prototypes                                                                   *
*******************************************************************************/

static void usage(char *appname);
static void version(void);
static int PromptAndWriteRemoteAuthConfig(void);

static int EnsureDirectoryExists(char *path)
{
    char *cursor;

    for (cursor = path + 1; *cursor != '\0'; cursor++)
    {
        if (*cursor == '/')
        {
            *cursor = '\0';
            if ((mkdir(path, S_IRWXU) != 0) && (errno != EEXIST))
            {
                *cursor = '/';
                return 1;
            }
            *cursor = '/';
        }
    }

    if ((mkdir(path, S_IRWXU) != 0) && (errno != EEXIST))
    {
        return 1;
    }
    return 0;
}

static int EscapeJSONString(const char *input, char **output)
{
    size_t i;
    size_t outLen = 0;
    size_t inLen = strlen(input);
    char *escaped = malloc((inLen * 2) + 1);
    if (escaped == NULL)
    {
        return 1;
    }

    for (i = 0; i < inLen; i++)
    {
        if ((input[i] == '"') || (input[i] == '\\'))
        {
            escaped[outLen++] = '\\';
        }
        escaped[outLen++] = input[i];
    }
    escaped[outLen] = '\0';
    *output = escaped;
    return 0;
}

static char *TrimLine(char *line)
{
    char *end;

    while ((*line != '\0') && ((*line == ' ') || (*line == '\t') || (*line == '\n') || (*line == '\r')))
    {
        line++;
    }

    end = line + strlen(line);
    while ((end > line) && ((end[-1] == ' ') || (end[-1] == '\t') || (end[-1] == '\n') || (end[-1] == '\r')))
    {
        end--;
    }
    *end = '\0';
    return line;
}

static char *SetupUserConfigPathGet(void)
{
    const char *xdgConfigHome = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    char *path;

    if ((xdgConfigHome != NULL) && (xdgConfigHome[0] != '\0'))
    {
        if (asprintf(&path, "%s/dvbstreamer/userconfig.json", xdgConfigHome) == -1)
        {
            return NULL;
        }
        return path;
    }

    if ((home == NULL) || (home[0] == '\0'))
    {
        return NULL;
    }

    if (asprintf(&path, "%s/.config/dvbstreamer/userconfig.json", home) == -1)
    {
        return NULL;
    }

    return path;
}

static int PromptAndWriteRemoteAuthConfig(void)
{
    char usernameBuffer[256] = {0};
    char *username;
    char *passwordInput;
    const char *password;
    char *escapedUsername = NULL;
    char *escapedPassword = NULL;
    char *configPath;
    char *configDir;
    char *slash;
    int fd;
    FILE *fp;

    printf("\nRemote control credential setup\n");
    printf("Username [dvbstreamer]: ");
    if (fgets(usernameBuffer, sizeof(usernameBuffer), stdin) == NULL)
    {
        return 1;
    }

    username = TrimLine(usernameBuffer);
    if (username[0] == '\0')
    {
        username = "dvbstreamer";
    }

    passwordInput = getpass("Password [control]: ");
    if ((passwordInput == NULL) || (passwordInput[0] == '\0'))
    {
        password = "control";
    }
    else
    {
        password = passwordInput;
    }

    if (EscapeJSONString(username, &escapedUsername) != 0)
    {
        return 1;
    }
    if (EscapeJSONString(password, &escapedPassword) != 0)
    {
        free(escapedUsername);
        return 1;
    }

    configPath = SetupUserConfigPathGet();
    if (configPath == NULL)
    {
        free(escapedUsername);
        free(escapedPassword);
        return 1;
    }

    configDir = strdup(configPath);
    if (configDir == NULL)
    {
        free(configPath);
        free(escapedUsername);
        free(escapedPassword);
        return 1;
    }

    slash = strrchr(configDir, '/');
    if (slash == NULL)
    {
        free(configDir);
        free(configPath);
        free(escapedUsername);
        free(escapedPassword);
        return 1;
    }
    *slash = '\0';

    if (EnsureDirectoryExists(configDir) != 0)
    {
        free(configDir);
        free(configPath);
        free(escapedUsername);
        free(escapedPassword);
        return 1;
    }

    fd = open(configPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        free(configDir);
        free(configPath);
        free(escapedUsername);
        free(escapedPassword);
        return 1;
    }

    fp = fdopen(fd, "w");
    if (fp == NULL)
    {
        close(fd);
        free(configDir);
        free(configPath);
        free(escapedUsername);
        free(escapedPassword);
        return 1;
    }

    fprintf(fp,
        "{\n"
        "  \"username\": \"%s\",\n"
        "  \"password\": \"%s\"\n"
        "}\n",
        escapedUsername, escapedPassword);

    fclose(fp);
    free(configDir);
    printf("Wrote remote auth config to %s\n", configPath);
    free(configPath);
    free(escapedUsername);
    free(escapedPassword);
    return 0;
}

/*******************************************************************************
* Global variables                                                             *
*******************************************************************************/
static const char SETUP[] = "Setup";

char DataDirectory[PATH_MAX];

/*******************************************************************************
* Global functions                                                             *
*******************************************************************************/

int main(int argc, char *argv[])
{
    DVBDeliverySystem_e channelsFileType = DELSYS_DVBT;
    char *channelsFile = NULL;
    int adapterNumber = 0;
#if defined(ENABLE_DVB)
    LNBInfo_t lnbInfo = {NULL,NULL,0,0,0};
#endif
    int rc;
    int logLevel = 0;
    char logFilename[PATH_MAX] = {0};

    /* Create the data directory */
    sprintf(DataDirectory, "%s/.dvbstreamer", getenv("HOME"));
    mkdir(DataDirectory, S_IRWXU);

    while (TRUE)
    {
        int c;
        c = getopt(argc, argv, "vVdro:a:t:2:s:S:c:A:l:hL:i:");
        if (c == -1)
        {
            break;
        }
        switch (c)
        {
            case 'v':
                logLevel++;
                break;
            case 'L': strcpy(logFilename, optarg);
                break;
            case 'V':
                version();
                exit(0);
                break;
            case 'a':
                adapterNumber = atoi(optarg);
                break;
                /* Database initialisation options*/
#if defined(ENABLE_DVB)
            case 't':
                channelsFile = optarg;
                channelsFileType = DELSYS_DVBT;
                break;
            case '2':
                channelsFile = optarg;
                channelsFileType = DELSYS_DVBT2;
                break;
            case 's':
                channelsFile = optarg;
                channelsFileType = DELSYS_DVBS;
                break;
            case 'S':
                channelsFile = optarg;
                channelsFileType = DELSYS_DVBS2;
                break;
            case 'c':
                channelsFile = optarg;
                channelsFileType = DELSYS_DVBC;
                break;
#endif

#if defined(ENABLE_ATSC)
            case 'A':
                channelsFile = optarg;
                channelsFileType = DELSYS_ATSC;
                break;
#endif
            case 'i':
                channelsFile = optarg;
                channelsFileType = DELSYS_ISDBT;
                break;
                
#if defined(ENABLE_DVB)
            case 'l': /* LNB settings */
                if (LNBDecode(optarg, &lnbInfo))
                {
                    int i = 0;
                    LNBInfo_t *knownLNB;
                    do
                    {
                        knownLNB = LNBEnumerate(i);
                        if (knownLNB)
                        {
                            char **desclines;
                            printf("%s :\n", knownLNB->name);

                            for (desclines = knownLNB->desc; *desclines; desclines ++)
                            {
                                printf("   %s\n", *desclines);
                            }
                            printf("\n");
                            i ++;
                        }
                    }while(knownLNB);
                    exit(1);
                }
                break;
#endif
            case 'h':
            default:
                usage(argv[0]);
                exit(1);
        }
    }

    if (logFilename[0])
    {
        if (LoggingInitFile(logFilename, logLevel))
        {
            perror("Could not open user specified log file:");
            exit(1);
        }
    }
    else
    {
        sprintf(logFilename, "setupdvbstreamer-%d.log", adapterNumber);
        if (LoggingInit(logFilename, logLevel))
        {
            perror("Couldn't initialising logging module:");
            exit(1);
        }
    }


#if defined(ENABLE_DVB)
    if (((channelsFileType == DELSYS_DVBS) || (channelsFileType == DELSYS_DVBS2)) && (lnbInfo.lowFrequency == 0))
    {
        fprintf(stderr, "No LNB information provide for DVB-S channels.conf file!\n");
        exit(1);
    }
#endif

    INIT(ObjectInit(), "objects");
    INIT(EventsInit(), "events");
    INIT(DBaseInit(adapterNumber), "database");
    INIT(MultiplexInit(), "multiplex");
    INIT(ServiceInit(), "service");

    if (!channelsFile)
    {
        usage(argv[0]);
        exit(1);
    }
    rc = DBaseTransactionBegin();
    if (rc != SQLITE_OK)
    {
        LogModule(LOG_ERROR, SETUP, "Begin Transaction failed (%d:%s)\n", rc, sqlite3_errmsg(DBaseConnectionGet()));
    }

    LogModule(LOG_INFO, SETUP, "Importing services from %s\n", channelsFile);
    if (!parsezapfile(channelsFile, channelsFileType))
    {
        exit(1);
    }

#if defined(ENABLE_DVB)
    if ((channelsFileType == DELSYS_DVBS) || (channelsFileType == DELSYS_DVBS2))
    {
        /* Write out LNB settings. */
        DBaseMetadataSetInt(METADATA_NAME_LNB_LOW_FREQ, lnbInfo.lowFrequency);
        DBaseMetadataSetInt(METADATA_NAME_LNB_HIGH_FREQ, lnbInfo.highFrequency);
        DBaseMetadataSetInt(METADATA_NAME_LNB_SWITCH_FREQ, lnbInfo.switchFrequency);
    }
#endif

    DBaseMetadataSetInt(METADATA_NAME_SCAN_ALL, 1);
    rc = 0;
    rc = DBaseTransactionCommit();
    if (rc != SQLITE_OK)
    {
        LogModule(LOG_ERROR, SETUP, "Begin Transaction failed (%d:%s)\n", rc, sqlite3_errmsg(DBaseConnectionGet()));
    }

    printf("%d Services available on %d Multiplexes\n", ServiceCount(), MultiplexCount());

    if (PromptAndWriteRemoteAuthConfig() != 0)
    {
        fprintf(stderr, "Warning: failed to write remote auth config file.\n");
    }

    DEINIT(ServiceDeInit(), "service");
    DEINIT(MultiplexDeInit(), "multiplex");
    DEINIT(DBaseDeInit(), "database");
    DEINIT(EventsDeInit(), "events");
    DEINIT(ObjectDeinit(), "objects");
    LoggingDeInit();
    return 0;
}

/*
 * Output command line usage and help.
 */
static void usage(char *appname)
{
    fprintf(stderr,"Usage:%s <options>\n"
            "      Options:\n"
            "      -v            : Increase the amount of debug output, can be used multiple\n"
            "                      times for more output\n"
            "      -L <file>     : Set the location of the log file.\n"
            "      -V            : Print version information then exit\n"
            "\n"
            "      -a <adapter>  : Use adapter number (ie /dev/dvb/adapter<adapter>/...)\n"
            "\n"
#if defined(ENABLE_DVB)
            "      -t <file>     : Terrestrial channels.conf file to import services and \n"
            "                      multiplexes from. (DVB-T)\n"
            "\n"
            "      -2 <file>     : Terrestrial channels.conf file to import services and \n"
            "                      multiplexes from. (DVB-T2)\n"
            "\n"
            "      -s <file>     : Satellite channels.conf file to import services and \n"
            "                      multiplexes from.(DVB-S)\n"
            "      -S <file>     : DVB-S/S2 Satellite  channels.conf file to import services and \n"
            "                      multiplexes from. NOTE: File must be in VDR format!\n"
            "      -l <LNB Type> : (DVB-S Only) Set LNB type being used\n"
            "                      (Use -l help to print types) or \n"
            "      -l <low>,<high>,<switch> Specify LO frequencies in MHz\n"
            "\n"
            "      -c <file>     : Cable channels.conf file to import services and \n"
            "                      multiplexes from. (DVB-C)\n"
            "\n"
#endif
            "      -i <file>     : ISDB channels.conf file to import services and \n"
            "                      multiplexes from. (ISDB-T)  (EXPERIMENTAL)\n"
            "                      NOTE: The file should be in dvb-t format\n"
            "\n"
#if defined(ENABLE_ATSC)
            "      -A <file>     : ATSC channels.conf file to import services and \n"
            "                      multiplexes from. (ATSC)\n"
#endif
            "\n"
            "      After import, setupdvbstreamer prompts for remote control\n"
            "      username/password and writes userconfig.json.\n"
            ,appname );
}

/*
 * Output version and license conditions
 */
static void version(void)
{
    printf("%s - %s (Compiled %s %s)\n"
           "Written by Adam Charrett (charrea6@users.sourceforge.net).\n"
           "\n"
           "Copyright 2006 Adam Charrett\n"
           "This is free software; see the source for copying conditions.  There is NO\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
           PACKAGE, VERSION, __DATE__, __TIME__);
}

