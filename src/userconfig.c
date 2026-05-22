#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "userconfig.h"

#define USERCONFIG_RELATIVE_PATH "/dvbstreamer/userconfig.json"
#define USERCONFIG_MAX_SIZE 8192

static void SetError(char *error, size_t errorSize, const char *message)
{
    if ((error != NULL) && (errorSize > 0))
    {
        snprintf(error, errorSize, "%s", message);
    }
}

static const char *SkipWhitespace(const char *cursor)
{
    while ((*cursor != '\0') && isspace((unsigned char)*cursor))
    {
        cursor++;
    }
    return cursor;
}

static int DecodeJsonString(const char **cursorPtr, const char *key, char **value, char *error, size_t errorSize)
{
    const char *cursor = *cursorPtr;
    char *result;
    size_t resultLen = 0;
    size_t resultCap;

    if (*cursor != '"')
    {
        snprintf(error, errorSize, "Value for \"%s\" must be a JSON string.", key);
        return 1;
    }

    cursor++;
    resultCap = strlen(cursor) + 1;
    result = malloc(resultCap);
    if (result == NULL)
    {
        SetError(error, errorSize, "Failed to allocate memory for auth config.");
        return 1;
    }

    while (*cursor != '\0')
    {
        if (*cursor == '"')
        {
            result[resultLen] = '\0';
            *value = result;
            *cursorPtr = cursor + 1;
            return 0;
        }

        if (*cursor == '\\')
        {
            cursor++;
            if (*cursor == '\0')
            {
                free(result);
                snprintf(error, errorSize, "Invalid escape sequence in \"%s\".", key);
                return 1;
            }

            switch (*cursor)
            {
                case '"':
                case '\\':
                case '/':
                    if (resultLen + 1 >= resultCap)
                    {
                        free(result);
                        snprintf(error, errorSize, "Value for \"%s\" is too long.", key);
                        return 1;
                    }
                    result[resultLen++] = *cursor;
                    break;
                case 'b':
                    if (resultLen + 1 >= resultCap)
                    {
                        free(result);
                        snprintf(error, errorSize, "Value for \"%s\" is too long.", key);
                        return 1;
                    }
                    result[resultLen++] = '\b';
                    break;
                case 'f':
                    if (resultLen + 1 >= resultCap)
                    {
                        free(result);
                        snprintf(error, errorSize, "Value for \"%s\" is too long.", key);
                        return 1;
                    }
                    result[resultLen++] = '\f';
                    break;
                case 'n':
                    if (resultLen + 1 >= resultCap)
                    {
                        free(result);
                        snprintf(error, errorSize, "Value for \"%s\" is too long.", key);
                        return 1;
                    }
                    result[resultLen++] = '\n';
                    break;
                case 'r':
                    if (resultLen + 1 >= resultCap)
                    {
                        free(result);
                        snprintf(error, errorSize, "Value for \"%s\" is too long.", key);
                        return 1;
                    }
                    result[resultLen++] = '\r';
                    break;
                case 't':
                    if (resultLen + 1 >= resultCap)
                    {
                        free(result);
                        snprintf(error, errorSize, "Value for \"%s\" is too long.", key);
                        return 1;
                    }
                    result[resultLen++] = '\t';
                    break;
                default:
                    free(result);
                    snprintf(error, errorSize, "Unsupported escape sequence in \"%s\".", key);
                    return 1;
            }
        }
        else
        {
            if (resultLen + 1 >= resultCap)
            {
                free(result);
                snprintf(error, errorSize, "Value for \"%s\" is too long.", key);
                return 1;
            }
            result[resultLen++] = *cursor;
        }

        cursor++;
    }

    free(result);
    snprintf(error, errorSize, "Unterminated JSON string for \"%s\".", key);
    return 1;
}

static int JsonKeyMatches(const char *keyStart, size_t keyLen, const char *key)
{
    size_t expectedLen = strlen(key);
    return (keyLen == expectedLen) && (strncmp(keyStart, key, keyLen) == 0);
}

static int ExtractJsonString(const char *json, const char *key, char **value, char *error, size_t errorSize)
{
    const char *cursor = json;

    while (*cursor != '\0')
    {
        const char *keyStart;
        const char *keyEnd;

        cursor = SkipWhitespace(cursor);
        if (*cursor == '\0')
        {
            break;
        }

        if (*cursor != '"')
        {
            cursor++;
            continue;
        }

        keyStart = ++cursor;
        while ((*cursor != '\0') && (*cursor != '"'))
        {
            if ((*cursor == '\\') && (cursor[1] != '\0'))
            {
                cursor += 2;
                continue;
            }
            cursor++;
        }

        if (*cursor != '"')
        {
            break;
        }

        keyEnd = cursor;
        cursor++;
        cursor = SkipWhitespace(cursor);
        if (*cursor != ':')
        {
            continue;
        }
        cursor++;
        cursor = SkipWhitespace(cursor);

        if (JsonKeyMatches(keyStart, (size_t)(keyEnd - keyStart), key))
        {
            return DecodeJsonString(&cursor, key, value, error, errorSize);
        }

        if (*cursor == '"')
        {
            char *ignoredValue = NULL;
            if (DecodeJsonString(&cursor, key, &ignoredValue, error, errorSize) != 0)
            {
                return 1;
            }
            free(ignoredValue);
        }
    }

    snprintf(error, errorSize, "Missing \"%s\" in auth config.", key);
    return 1;
}

char *UserConfigAuthPathGet(void)
{
    const char *xdgConfigHome = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    char *path;

    if ((xdgConfigHome != NULL) && (xdgConfigHome[0] != '\0'))
    {
        if (asprintf(&path, "%s%s", xdgConfigHome, USERCONFIG_RELATIVE_PATH) == -1)
        {
            return NULL;
        }
        return path;
    }

    if ((home == NULL) || (home[0] == '\0'))
    {
        return NULL;
    }

    if (asprintf(&path, "%s/.config%s", home, USERCONFIG_RELATIVE_PATH) == -1)
    {
        return NULL;
    }

    return path;
}

int UserConfigAuthLoad(char **username, char **password, char *error, size_t errorSize)
{
    char *path;
    FILE *fp;
    char *buffer;
    size_t bytesRead;
    struct stat pathStat;
    int result = 1;

    *username = NULL;
    *password = NULL;

    path = UserConfigAuthPathGet();
    if (path == NULL)
    {
        SetError(error, errorSize, "Failed to resolve auth config path from HOME.");
        return 1;
    }

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        snprintf(error, errorSize, "Failed to open auth config %s: %s", path, strerror(errno));
        free(path);
        return 1;
    }

    if (stat(path, &pathStat) == 0)
    {
        if (pathStat.st_mode & (S_IRWXG | S_IRWXO))
        {
            snprintf(error, errorSize,
                "Auth config %s has insecure permissions (group/other access).", path);
            fclose(fp);
            free(path);
            return 1;
        }
    }

    buffer = calloc(USERCONFIG_MAX_SIZE + 1, 1);
    if (buffer == NULL)
    {
        fclose(fp);
        free(path);
        SetError(error, errorSize, "Failed to allocate memory for auth config.");
        return 1;
    }

    bytesRead = fread(buffer, 1, USERCONFIG_MAX_SIZE, fp);
    if (ferror(fp))
    {
        snprintf(error, errorSize, "Failed to read auth config %s.", path);
        goto out;
    }
    if ((!feof(fp)) || (bytesRead == USERCONFIG_MAX_SIZE))
    {
        snprintf(error, errorSize, "Auth config %s is too large.", path);
        goto out;
    }

    buffer[bytesRead] = '\0';
    if (ExtractJsonString(buffer, "username", username, error, errorSize) != 0)
    {
        goto out;
    }
    if (ExtractJsonString(buffer, "password", password, error, errorSize) != 0)
    {
        free(*username);
        *username = NULL;
        goto out;
    }

    result = 0;

out:
    free(buffer);
    fclose(fp);
    free(path);
    if (result != 0)
    {
        free(*username);
        free(*password);
        *username = NULL;
        *password = NULL;
    }
    return result;
}