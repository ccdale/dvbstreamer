#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "userconfig.h"

#define USERCONFIG_RELATIVE_PATH "/.config/dvbstreamer/userconfig.json"
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

static int ExtractJsonString(const char *json, const char *key, char **value, char *error, size_t errorSize)
{
    char pattern[64];
    const char *cursor;
    char *result;
    size_t resultLen = 0;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    cursor = strstr(json, pattern);
    if (cursor == NULL)
    {
        snprintf(error, errorSize, "Missing \"%s\" in auth config.", key);
        return 1;
    }

    cursor += strlen(pattern);
    cursor = SkipWhitespace(cursor);
    if (*cursor != ':')
    {
        snprintf(error, errorSize, "Invalid JSON near \"%s\".", key);
        return 1;
    }

    cursor++;
    cursor = SkipWhitespace(cursor);
    if (*cursor != '"')
    {
        snprintf(error, errorSize, "Value for \"%s\" must be a JSON string.", key);
        return 1;
    }

    cursor++;
    result = malloc(strlen(cursor) + 1);
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
                    result[resultLen++] = *cursor;
                    break;
                case 'b':
                    result[resultLen++] = '\b';
                    break;
                case 'f':
                    result[resultLen++] = '\f';
                    break;
                case 'n':
                    result[resultLen++] = '\n';
                    break;
                case 'r':
                    result[resultLen++] = '\r';
                    break;
                case 't':
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
            result[resultLen++] = *cursor;
        }

        cursor++;
    }

    free(result);
    snprintf(error, errorSize, "Unterminated JSON string for \"%s\".", key);
    return 1;
}

char *UserConfigAuthPathGet(void)
{
    const char *home = getenv("HOME");
    char *path;

    if (home == NULL)
    {
        return NULL;
    }

    if (asprintf(&path, "%s%s", home, USERCONFIG_RELATIVE_PATH) == -1)
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