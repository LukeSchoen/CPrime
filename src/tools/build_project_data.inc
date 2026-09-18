/* Standalone build data support for the native project driver. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>
#include <stdarg.h>
#define CAP 32768
static void die(const char *s) { fprintf(stderr, "%s\n", s); puts("Executable: NOT PRODUCED"); exit(1); }
static void fail(const wchar_t *s) { fwprintf(stderr, L"%ls (Windows error %lu)\n", s, GetLastError()); die("Build tool failed"); }
static void *alloc(size_t n) { void *p = calloc(1,n); if (!p) die("Out of memory"); return p; }

typedef struct Buf
{
    char *s;
    size_t n, cap;
} Buf;
static void bn(Buf *b, const char *s, size_t n)
{
    if (b->n + n + 1 > b->cap)
    {
        b->cap = (b->n + n + 1) * 2;
        b->s = realloc(b->s, b->cap);
        if (!b->s)
            fail(L"Out of memory");
    }
    memcpy(b->s + b->n, s, n);
    b->n += n;
    b->s[b->n] = 0;
}
static void bs(Buf *b, const char *s)
{
    bn(b, s, strlen(s));
}
static char *str(const char *s)
{
    char *p = _strdup(s ? s : "");
    if (!p)
        fail(L"Out of memory");
    return p;
}
static wchar_t *wide(const char *s)
{
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    wchar_t *p = alloc((size_t)n * sizeof(wchar_t));
    if (!n || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, p, n))
        fail(L"Invalid UTF-8");
    return p;
}
static char *utf8(const wchar_t *s)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    char *p = alloc(n);
    if (!n || !WideCharToMultiByte(CP_UTF8, 0, s, -1, p, n, NULL, NULL))
        fail(L"Invalid Unicode");
    return p;
}
static char *cat(const char *a, const char *b) { Buf r={0}; bs(&r,a); bs(&r,b); return r.s; }
static char *absolute(const char *base, const char *s) {
    wchar_t result[CAP], *w;
    char *p = (s[0] && (s[1]==':' || s[0]=='/' || s[0]=='\\')) ? str(s) : cat(cat(base,"/"),s);
    DWORD n; w=wide(p); n=GetFullPathNameW(w,CAP,result,NULL); free(w); free(p);
    if (!n || n>=CAP) die("Invalid path"); return utf8(result);
}
static char *directory(const char *p) { char *r=str(p), *s=strrchr(r,'\\'), *t=strrchr(r,'/'); if(t && (!s || t>s))s=t; if(s)*s=0; return r; }
static int exists(const char *p) { wchar_t *w=wide(p); DWORD a=GetFileAttributesW(w); free(w); return a!=INVALID_FILE_ATTRIBUTES; }
static FILE *file_open(const char *p,const wchar_t *mode) { wchar_t *w=wide(p); FILE *f=_wfopen(w,mode); free(w); return f; }
static char *read_utf8(const char *p) { FILE *f=file_open(p,L"rb"); long n; char *s; if(!f)die(p); if(fseek(f,0,SEEK_END)||(n=ftell(f))<0||fseek(f,0,SEEK_SET))die(p); s=alloc(n+2); if(fread(s,1,n,f)!=(size_t)n)die(p); fclose(f);
    if(n>=2&&(!memcmp(s,"\xff\xfe",2)||!memcmp(s,"\xfe\xff",2))){char *u;long i;if(n&1)die("Truncated UTF-16 text");if((unsigned char)s[0]==0xfe)for(i=2;i<n;i+=2){char c=s[i];s[i]=s[i+1];s[i+1]=c;}u=utf8((wchar_t*)(s+2));free(s);return u;}
    if(n>=3 && !memcmp(s,"\xef\xbb\xbf",3))memmove(s,s+3,n-2); return s; }
static void mkdirs(const char *p) { wchar_t *w=wide(p),*s; for(s=w+3;;++s){wchar_t c=*s; if(!c||c=='/'||c=='\\'){*s=0; if(!CreateDirectoryW(w,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)fail(w); *s=c; if(!c)break;}} free(w); }
static void write_utf8(const char *p,const char *s) { FILE *f; if(exists(p)){char *old=read_utf8(p); int same=!strcmp(old,s); free(old);if(same)return;} mkdirs(directory(p)); f=file_open(p,L"wb");if(!f||fwrite(s,1,strlen(s),f)!=strlen(s)||fclose(f))die(p); }
static void remove_file(const char *p) { wchar_t *w=wide(p); if(!DeleteFileW(w)&&GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)fail(w); free(w); }
typedef struct J
{
    int type;
    char *key, *s;
    struct J *child, *tail, *next;
} J;
enum
{
    JO,
    JA,
    JS,
    JN,
    JB,
    JZ
};
static J *jnew(int type, const char *s)
{
    J *j = alloc(sizeof(*j));
    j->type = type;
    j->s = str(s);
    return j;
}
static J *js(const char *s)
{
    return jnew(JS, s);
}
static void ja(J *j, J *v)
{
    if (j->tail)
        j->tail->next = v;
    else
        j->child = v;
    j->tail = v;
}
static J *get(J *j, const char *key)
{
    J *v;
    for (v = j ? j->child : NULL; v; v = v->next)
        if (v->key && !_stricmp(v->key, key))
            return v;
    return NULL;
}
static const char *val(J *j, const char *key)
{
    J *v = get(j, key);
    return v ? v->s : "";
}
static void put(J *j, const char *key, J *v)
{
    J *old = get(j, key);
    if (old)
    {
        free(old->s);
        old->s = v->s;
        old->type = v->type;
        old->child = v->child;
        old->tail = v->tail;
        free(v);
        return;
    }
    v->key = str(key);
    ja(j, v);
}
static void set(J *j, const char *key, const char *v)
{
    put(j, key, js(v));
}
static void unset(J *j, const char *key)
{
    J **p = &j->child;
    while (*p)
    {
        if ((*p)->key && !_stricmp((*p)->key, key))
        {
            J *v = *p;
            *p = v->next;
            if (j->tail == v)
            {
                j->tail = j->child;
                while (j->tail && j->tail->next)
                    j->tail = j->tail->next;
            }
            return;
        }
        p = &(*p)->next;
    }
}
static J *clone(J *j)
{
    J *r = jnew(j->type, j->s), *v;
    r->key = j->key ? str(j->key) : NULL;
    for (v = j->child; v; v = v->next)
        ja(r, clone(v));
    return r;
}
static void quote(Buf *b, const char *s)
{
    bs(b, "\"");
    while (*s)
    {
        unsigned char c = (unsigned char)*s++;
        char esc[8];
        if (c == '"' || c == '\\')
        {
            bs(b, "\\");
            bn(b, (char *)&c, 1);
        }
        else if (c < 32)
        {
            sprintf_s(esc, sizeof esc, "\\u%04x", c);
            bs(b, esc);
        }
        else
            bn(b, (char *)&c, 1);
    }
    bs(b, "\"");
}
static void json(Buf *b, J *j)
{
    J *v;
    switch (j->type)
    {
    case JO:
    case JA:
        bs(b, j->type == JO ? "{" : "[");
        for (v = j->child; v; v = v->next)
        {
            if (v != j->child)
                bs(b, ",");
            if (j->type == JO)
            {
                quote(b, v->key);
                bs(b, ":");
            }
            json(b, v);
        }
        bs(b, j->type == JO ? "}" : "]");
        break;
    case JS:
        quote(b, j->s);
        break;
    case JZ:
        bs(b, "null");
        break;
    default:
        bs(b, j->s);
    }
}
static void unique(J *a, const char *s)
{
    J *v;
    for (v = a->child; v; v = v->next)
        if (!_stricmp(v->s, s))
            return;
    ja(a, js(s));
}

static void json_space(const char **p)
{
    while (isspace((unsigned char)**p))
        ++*p;
}
static unsigned json_hex(const char **p)
{
    unsigned v = 0;
    int i;
    for (i = 0; i < 4; ++i)
    {
        unsigned char c = (unsigned char)*(*p)++;
        if (!isxdigit(c))
            die("Invalid JSON Unicode escape");
        v = v * 16 + (c <= '9' ? c - '0' : tolower(c) - 'a' + 10);
    }
    return v;
}
static char *json_string(const char **p)
{
    Buf b = {0};
    if (*(*p)++ != '"')
        die("Expected JSON string");
    while (**p && **p != '"')
    {
        unsigned char c = (unsigned char)*(*p)++;
        if (c == '\\')
        {
            c = (unsigned char)*(*p)++;
            switch (c)
            {
            case '"':
            case '\\':
            case '/':
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'u': {
                wchar_t w[3] = {0, 0, 0};
                char *u;
                w[0] = (wchar_t)json_hex(p);
                if (w[0] >= 0xd800 && w[0] <= 0xdbff)
                {
                    if ((*p)[0] != '\\' || (*p)[1] != 'u')
                        die("Invalid JSON surrogate");
                    *p += 2;
                    w[1] = (wchar_t)json_hex(p);
                    if (w[1] < 0xdc00 || w[1] > 0xdfff)
                        die("Invalid JSON surrogate");
                }
                u = utf8(w);
                bs(&b, u);
                free(u);
                continue;
            }
            default:
                die("Invalid JSON escape");
            }
        }
        else if (c < 32)
            die("Invalid JSON string");
        bn(&b, (char *)&c, 1);
    }
    if (*(*p)++ != '"')
        die("Unclosed JSON string");
    return b.s ? b.s : str("");
}
static J *json_value(const char **p, int depth)
{
    J *j;
    json_space(p);
    if (depth > 128)
        die("JSON nesting too deep");
    if (**p == '{' || **p == '[')
    {
        int object = **p == '{';
        char close = object ? '}' : ']';
        ++*p;
        j = jnew(object ? JO : JA, "");
        json_space(p);
        if (**p == close)
        {
            ++*p;
            return j;
        }
        for (;;)
        {
            char *key = NULL;
            J *v;
            if (object)
            {
                key = json_string(p);
                json_space(p);
                if (*(*p)++ != ':')
                    die("Expected JSON colon");
            }
            v = json_value(p, depth + 1);
            if (object)
            {
                put(j, key, v);
                free(key);
            }
            else
                ja(j, v);
            json_space(p);
            if (**p == close)
            {
                ++*p;
                return j;
            }
            if (*(*p)++ != ',')
                die("Expected JSON comma");
            json_space(p);
        }
    }
    if (**p == '"')
    {
        char *s = json_string(p);
        j = js(s);
        free(s);
        return j;
    }
    if (!strncmp(*p, "true", 4))
    {
        *p += 4;
        return jnew(JB, "true");
    }
    if (!strncmp(*p, "false", 5))
    {
        *p += 5;
        return jnew(JB, "false");
    }
    if (!strncmp(*p, "null", 4))
    {
        *p += 4;
        return jnew(JZ, "");
    }
    {
        const char *s = *p;
        char *n;
        while (**p && strchr("-+0123456789.eE", **p))
            ++*p;
        if (s == *p)
            die("Invalid JSON value");
        n = alloc((size_t)(*p - s) + 1);
        memcpy(n, s, (size_t)(*p - s));
        j = jnew(JN, n);
        free(n);
        return j;
    }
}
static J *json_read(const char *filename)
{
    char *s = read_utf8(filename);
    const char *p = s;
    J *j = json_value(&p, 0);
    json_space(&p);
    if (*p)
        die("Trailing JSON data");
    free(s);
    return j;
}
