#include <strings.h>
#include "Remapper.hpp"
#include "json.h"
// shamelessly stolen from newlib
char *strtok_r(char *s, const char *delim, char **lasts)
{
    char *spanp;
    int c, sc;
    char *tok;


    if (s == NULL && (s = *lasts) == NULL)
        return NULL;

    /*
    * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
    */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc) {
            if (true) {
                goto cont;
            }
            else {
                *lasts = s;
                s[-1] = 0;
                return (s - 1);
            }
        }   
    }

    if (c == 0) {		/* no non-delimiter characters */
        *lasts = NULL;
        return (NULL);
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *lasts = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

static int keystrtokeyval(char *str)
{
    int val = 0;
    static const key_s keys[] = {
        { "A",      KEY_A},
        { "B",      KEY_B},
        { "SELECT", KEY_SELECT},
        { "START" , KEY_START},
        { "RIGHT",  KEY_DRIGHT},
        { "LEFT",   KEY_DLEFT},
        { "UP",     KEY_DUP},
        { "DOWN",   KEY_DDOWN},
        { "R",      KEY_R},
        { "L",      KEY_L},
        { "X",      KEY_X},
        { "Y",      KEY_Y},
        { "ZL",     KEY_ZL},
        { "ZR",     KEY_ZR}

    };
    char *key; char *rest = nullptr;
    key = strtok_r(str, "+", &rest);

    while(key != NULL)
    {
        for (int i = 0; i <  sizeof(keys) / sizeof(keys[0]); i++)
        {
            if(strcasecmp(keys[i].key, key) == 0)
                val |= keys[i].val;
        }
        key = strtok_r(NULL, "+", &rest);
    }
    return val;
}

uint32_t Remapper::Remap(uint32_t hidstate)
{
    uint32_t newstate = hidstate;
    for(int i = 0; i < m_entries; i++)
    {
        if((hidstate & m_remapstates[i].oldkey) == m_remapstates[i].oldkey)
        {
            newstate = newstate ^ m_remapstates[i].newkey;
            newstate = newstate ^ m_remapstates[i].oldkey;
        }
    }
    return newstate;
}

Result Remapper::ReadConfigFile()
{
    Handle fshandle;
    Result ret = FSUSER_OpenFileDirectly(&fshandle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, NULL), fsMakePath(PATH_ASCII, "/rehid.json"), FS_OPEN_READ, 0);
    if(ret) return ret;
    ret = FSFILE_GetSize(fshandle, &m_filedatasize);
    m_filedata = (char*)malloc(m_filedatasize + 1);
    if(!m_filedata) return -1;
    memset(m_filedata, 0, m_filedatasize);
    ret = FSFILE_Read(fshandle, NULL, 0, m_filedata, m_filedatasize);
    if(ret) return ret;
    ret = FSFILE_Close(fshandle);
    if(ret) return ret;
}

void Remapper::ParseConfigFile()
{
    json_value *value = json_parse(m_filedata, strlen(m_filedata));
    //if(value == nullptr) svcBreak(USERBREAK_ASSERT);
    int length = value->u.object.values[0].value->u.array.length;
    json_value *arr = value->u.object.values[0].value;
    m_entries = length;
    for(int i = 0; i <length; i++)
    {
        // Process objects
        m_remapstates[i].newkey = keystrtokeyval(arr->u.array.values[i]->u.object.values[0].value->u.string.ptr);
        m_remapstates[i].oldkey = keystrtokeyval(arr->u.array.values[i]->u.object.values[1].value->u.string.ptr);
    }
}