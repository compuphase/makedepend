
/* $Xorg: main.c,v 1.0.10 2001/02/09 02:03:16 xorgcvs Exp $ */

/*

Copyright (c) 1993, 1994, 1998 The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

/* $XFree86: xc/config/makedepend/main.c,v 3.33tsi Exp $ */

#include "def.h"
#ifdef hpux
#   define sigvec sigvector
#endif /* hpux */

#ifdef X_POSIX_C_SOURCE
#   define _POSIX_C_SOURCE X_POSIX_C_SOURCE
#   include <signal.h>
#   undef _POSIX_C_SOURCE
#else
#   if defined(X_NOT_POSIX) || defined(_POSIX_SOURCE)
#       include <signal.h>
#   else
#       define _POSIX_SOURCE
#       include <signal.h>
#       undef _POSIX_SOURCE
#   endif
#endif

#if (defined _WIN32 || defined __WIN32__) && !defined WIN32
#   define WIN32
#endif

#if defined WIN32
#   include <io.h>
#   define open(name, method)           _open((name), (method))
#   define close(handle)                _close(handle)
#   define read(handle, buffer, size)   _read((handle), (buffer), (size))
#   define unlink(name)                 _unlink(name)
#   if !defined __MINGW32__
#       define strcasecmp(s1, s2)       _stricmp((s1), (s2))
#   endif
#endif

#include <stdarg.h>

#ifdef MINIX
#   define USE_CHMOD   1
#endif

#ifdef DEBUG
    int _debugmask;
#endif

/* #define DEBUG_DUMP */
#ifdef DEBUG_DUMP
#   define DBG_PRINT(file, fmt, args)   fprintf(file, fmt, args)
#else
#   define DBG_PRINT(file, fmt, args)  /* empty */
#endif

#define DASH_INC_PRE    "#include \""
#define DASH_INC_POST   "\""

const char *ProgramName;

const char * const directives[] = { /* the order of these strings must match with the constants in def.h */
    "if",
    "ifdef",
    "ifndef",
    "else",
    "endif",
    "define",
    "undef",
    "include",
    "line",
    "pragma",
    "error",
    "ident",
    "sccs",
    "elif",
    "eject",
    "warning",
    "include_next",
    NULL
};

#define  OBJSUFFIX   ".o"
#define  INCLUDEDIR  ""

#define MAKEDEPEND
#   include "imakemdep.h"         /* from config sources */
#undef MAKEDEPEND

#if defined WIN32 || defined _WIN32 || defined __WIN32__
#   define OPEN_READ_TEXT  "rt"
#   define OPEN_WRITE_TEXT "wt"
#else
#   define OPEN_READ_TEXT  "r"
#   define OPEN_WRITE_TEXT "w"
#endif

struct inclist inclist[MAXFILES],
    *inclistp = inclist,
    *inclistnext = inclist,
    maininclist;

static char *filelist[MAXFILES];
const char  *includedirs[ MAXDIRS + 1 ];
const char **includedirsnext = includedirs;
char *notdotdot[MAXDIRS];
static int cmdinc_count = 0;
static char *cmdinc_list[2 * MAXINCFILES];
const char *objprefix = "";
const char *objsuffix = OBJSUFFIX;
static const char *startat = "# GENERATED DEPENDENCIES. DO NOT DELETE.";
int width = 78;
static boolean make_backup = TRUE;
static boolean include_cfile = FALSE;
boolean exclude_sysincludes = FALSE;
boolean ignore_missing = FALSE;     /* do not warn on include files that cannot be found */
boolean printed = FALSE;
boolean verbose = FALSE;
boolean warn_multiple = FALSE;      /* warn on multiple includes of same file */
boolean show_where_not = FALSE;

static void setfile_vars (const char *name, struct inclist *file);
static void setfile_cmdinc (struct filepointer *filep, long count, char **list);
static void redirect(const char *line, const char *makefile, const char *filelist[]);

static
#ifdef SIGNALRETURNSINT
    int
#else
    void
#endif
_catch (int sig)
{
    fflush (stdout);
    fatalerr ("got signal %d\n", sig);
}

#if defined(USG) || (defined(i386) && defined(SYSV)) || defined(WIN32) || defined(__UNIXOS2__) || defined(Lynx_22) || defined(__CYGWIN__)
#   define USGISH
#endif

#ifndef USGISH
#   ifdef X_NOT_POSIX
#       define sigaction sigvec
#       define sa_handler sv_handler
#       define sa_mask sv_mask
#       define sa_flags sv_flags
#   endif
    static struct sigaction sig_act;
#endif /* USGISH */

int main (int argc, char *argv[])
{
    char **fltail = filelist;
    const char **incp = includedirs;
    char *p;
    struct inclist *ip;
    char *makefile = NULL;
    struct filepointer *filecontent;
    const struct symtab *psymp = predefs;
    const char *endmarker = NULL;
    char **undeflist = NULL;
    int numundefs = 0;
    char **searchpathlist = NULL;
    int numsearchpaths = 0;
    int i;
    boolean accumulate = FALSE;
    boolean systeminclude = TRUE;
    boolean sort_targets = FALSE;
    boolean showhelp = FALSE;

    memset (filelist, 0, sizeof (filelist));
    ProgramName = argv[0];

    while (psymp->s_name) {
        define2 (psymp->s_name, NULL, psymp->s_value, &maininclist);
        psymp++;
    }
    if (argc == 2 && argv[1][0] == '@') {
        struct stat ast;
        int afd;
        char *args;
        char **nargv;
        int nargc;
        char quotechar = '\0';

        nargc = 1;
        if ((afd = open (argv[1] + 1, O_RDONLY)) < 0)
            fatalerr ("cannot open \"%s\"\n", argv[1] + 1);
        fstat (afd, &ast);
        args = (char *) malloc (ast.st_size + 1);
        if ((ast.st_size = read (afd, args, ast.st_size)) < 0)
            fatalerr ("failed to read %s\n", argv[1] + 1);
        args[ast.st_size] = '\0';
        close (afd);
        for (p = args; *p; p++) {
            if (quotechar) {
                if (quotechar == '\\' || (*p == quotechar && p[-1] != '\\'))
                    quotechar = '\0';
                continue;
            }
            switch (*p) {
                case '\\':
                case '"':
                case '\'':
                    quotechar = *p;
                    break;
                case ' ':
                case '\n':
                    *p = '\0';
                    if (p > args && p[-1])
                        nargc++;
                    break;
            }
        }
        if (p[-1])
            nargc++;
        nargv = (char **) malloc (nargc * sizeof (char *));
        nargv[0] = argv[0];
        argc = 1;
        for (p = args; argc < nargc; p += strlen (p) + 1)
            if (*p)
                nargv[argc++] = p;
        argv = nargv;
    }
    for (argc--, argv++; argc; argc--, argv++) {
        /* if looking for endmarker then check before parsing */
        if (endmarker && strcmp (endmarker, *argv) == 0) {
            endmarker = NULL;
            continue;
        }
        if (**argv != '-') {
            char **pp;
            /* treat +thing as an option for C++ */
            if (endmarker && **argv == '+')
                continue;
            /* check for a file appearing twice on the command line */
            for (pp = filelist; pp != fltail && (strcmp (*pp, argv[0]) != 0); pp++)
                {}
            if (pp == fltail)   /* reached the end of the list, so this is a new file */
                *fltail++ = argv[0];
            continue;
        }
        switch (argv[0][1]) {
            case '-':
                endmarker = &argv[0][2];
                if (endmarker[0] == '\0')
                    endmarker = "--";
                break;
            case 'D':
                i = 2;
                if (argv[0][i] == '\0') {
                    argv++;
                    argc--;
                    i = 0;
                }
                for (p = argv[0] + i; *p; p++)
                    if (*p == '=') {
                        *p = ' ';
                        break;
                    }
                define (argv[0] + i, &maininclist);
                break;
            case 'I':
                if (strcmp(argv[0] + 2, "-") == 0) {
                    systeminclude = FALSE;
                } else {
                    if (incp >= includedirs + MAXDIRS)
                        fatalerr ("Too many -I flags.\n");
                    /* allow both syntax -Ipath and -I path */
                    *incp++ = argv[0] + 2;
                    if (**(incp - 1) == '\0') {
                        *(incp - 1) = *(++argv);
                        argc--;
                    }
                }
                break;
            case 'U':
                /* Undef's override all -D's so save them up */
                numundefs++;
                if (numundefs == 1) {
                    undeflist = (char**)malloc (sizeof (char *));
                } else {
                    char **old = undeflist;
                    undeflist = (char**)realloc (undeflist, numundefs * sizeof (char *));
                    if (!undeflist) {
                        free (old);
                        memoryerr ();
                    }
                }
                i = 2;
                if (argv[0][i] == '\0') {
                    argv++;
                    argc--;
                    i = 0;
                }
                undeflist[numundefs - 1] = argv[0] + i;
                break;
                /* do not handle options below if endmarker has been set */
            case 'L':
                if (endmarker)
                    break;
                numsearchpaths++;
                if (numsearchpaths == 1) {
                    searchpathlist = (char**)malloc (sizeof (char *));
                } else {
                    char **old = searchpathlist;
                    searchpathlist = (char**)realloc (searchpathlist, numsearchpaths * sizeof (char *));
                    if (!searchpathlist) {
                        free (old);
                        memoryerr ();
                    }
                }
                /* allow both syntax -Lpath and -L path */
                i = 2;
                if (argv[0][i] == '\0') {
                    argv++;
                    argc--;
                    i = 0;
                }
                searchpathlist[numsearchpaths - 1] = argv[0] + i;
                break;
            case 'a':
                if (endmarker)
                    break;
                if (argv[0][2])
                    goto badopt;
                accumulate = TRUE;
                break;
            case 'b':
                if (endmarker)
                    break;
                if (argv[0][2])
                    goto badopt;
                make_backup = FALSE;
                break;
            case 'c':
                if (endmarker)
                    break;
                if (argv[0][2])
                    goto badopt;
                include_cfile = TRUE;
                break;
            case 'e':
                if (endmarker)
                    break;
                if (argv[0][2])
                    goto badopt;
                exclude_sysincludes = TRUE;
                break;
            case 'f':
                if (endmarker)
                    break;
                makefile = argv[0] + 2;
                if (*makefile == '\0') {
                    makefile = *(++argv);
                    argc--;
                }
                break;
            case '?':
            case 'h':
                if (endmarker)
                    break;
                if (argv[0][2])
                    goto badopt;
                showhelp = TRUE;
                break;
            case 'i':
                if (endmarker)
                    break;
                if (strcmp (&argv[0][1], "include") == 0) {
                    char *buf;
                    if (argc < 2)
                        fatalerr ("option -include is a missing its parameter\n");
                    if (cmdinc_count >= MAXINCFILES)
                        fatalerr ("Too many -include flags.\n");
                    argc--;
                    argv++;
                    buf = (char*)malloc (strlen (DASH_INC_PRE) +
                                         strlen (argv[0]) +
                                         strlen (DASH_INC_POST) + 1);
                    if (!buf)
                        fatalerr ("out of memory at " "-include string\n");
                    cmdinc_list[2 * cmdinc_count + 0] = argv[0];
                    cmdinc_list[2 * cmdinc_count + 1] = buf;
                    cmdinc_count++;
                    break;
                } else {
                    if (argv[0][2])
                        goto badopt;
                    ignore_missing = TRUE;
                }
                break;
            case 'm':
                if (endmarker)
                    break;
                if (argv[0][2])
                    goto badopt;
                warn_multiple = TRUE;
                break;
            case 'o':
                if (endmarker)
                    break;
                if (argv[0][2] == '\0') {
                    /* syntax -o .ext */
                    argv++;
                    argc--;
                    objsuffix = argv[0];
                } else {
                    /* syntax -o.ext */
                    objsuffix = argv[0] + 2;
                }
                break;
            case 'p':
                if (endmarker)
                    break;
                if (argv[0][2] == '\0') {
                    /* syntax -p prefix */
                    argv++;
                    argc--;
                    objprefix = argv[0];
                } else {
                    /* syntax -pprefix */
                    objprefix = argv[0] + 2;
                }
                break;
            case 's':
                if (endmarker)
                    break;
                if (strcmp(argv[0] + 1, "sort") == 0) {
                    sort_targets = TRUE;
                } else {
                    startat = argv[0] + 2;
                    if (*startat == '\0') {
                        startat = *(++argv);
                        argc--;
                    }
                    if (*startat != '#')
                        fatalerr ("-s flag's value should start %s\n", "with '#'.");
                }
                break;
            case 'v':
                if (endmarker)
                    break;
                verbose = TRUE;
#ifdef DEBUG
                if (argv[0][2])
                    _debugmask = atoi (argv[0] + 2);
#endif
                break;
            case 'w':
                if (endmarker)
                    break;
                if (argv[0][2] == '\0') {
                    /* syntax -w 99 */
                    argv++;
                    argc--;
                    if (argv[0])
                        width = atoi (argv[0]);
                } else {
                    /* syntax -w99 */
                    width = atoi (argv[0] + 2);
                }
                break;

            default:
                if (endmarker)
                    break;
            badopt:
                warning ("ignoring option %s\n", argv[0]);
                break;
        }
    }

    if (showhelp)
        showusage();

    /* Now do the undefs from the command line */
    for (i = 0; i < numundefs; i++)
        undefine (undeflist[i], &maininclist);
    if (numundefs > 0)
        free (undeflist);

    if (systeminclude) {
        #ifdef PREINCDIR
            if (incp >= includedirs + MAXDIRS)
                fatalerr ("Too many -I flags.\n");
            *incp++ = PREINCDIR;
        #endif
        #if defined __UNIXOS2__  || defined WIN32
            {
                #if defined __UNIXOS2__
                char *emxinc = getenv("C_INCLUDE_PATH");
                #else
                char *emxinc = getenv("INCLUDE");
                #endif
                /* can have more than one component */
                if (emxinc) {
                    char *inc, *buffer;
                    buffer = copy(emxinc);  /* make a copy so that we can tokenize it */
                    for (inc = strtok(buffer, ";"); inc!=NULL; inc = strtok(NULL, ";")) {
                        if (incp >= includedirs + MAXDIRS)
                            fatalerr("Too many include dirs\n");
                        *incp++ = copy(inc);    /* make a copy, because buffer will get freed */
                    }
                    free(buffer);
                }
            }
        #else
            /* not Linux/Unix, OS2 or Windows, use fixed setting or nothing
             * at all
             */
            if (incp >= includedirs + MAXDIRS)
                fatalerr ("Too many -I flags.\n");
            *incp++ = INCLUDEDIR;
        #endif

        #ifdef EXTRAINCDIR
            if (incp >= includedirs + MAXDIRS)
                fatalerr ("Too many -I flags.\n");
            *incp++ = EXTRAINCDIR;
        #endif

        #ifdef POSTINCDIR
            if (incp >= includedirs + MAXDIRS)
                fatalerr ("Too many -I flags.\n");
            *incp++ = POSTINCDIR;
        #endif
    }

    redirect (startat, makefile, (accumulate ? (const char**)filelist : NULL));

    /*
     * catch signals.
     */
#ifdef USGISH

/*  should really reset SIGINT to SIG_IGN if it was.  */
#ifdef SIGHUP
    signal (SIGHUP, _catch);
#endif
    signal (SIGINT, _catch);
#ifdef SIGQUIT
    signal (SIGQUIT, _catch);
#endif
    signal (SIGILL, _catch);
#ifdef SIGBUS
    signal (SIGBUS, _catch);
#endif
    signal (SIGSEGV, _catch);
#ifdef SIGSYS
    signal (SIGSYS, _catch);
#endif
#else
    sig_act.sa_handler = _catch;
#if defined(_POSIX_SOURCE) || !defined(X_NOT_POSIX)
    sigemptyset (&sig_act.sa_mask);
    sigaddset (&sig_act.sa_mask, SIGINT);
    sigaddset (&sig_act.sa_mask, SIGQUIT);
#ifdef SIGBUS
    sigaddset (&sig_act.sa_mask, SIGBUS);
#endif
    sigaddset (&sig_act.sa_mask, SIGILL);
    sigaddset (&sig_act.sa_mask, SIGSEGV);
    sigaddset (&sig_act.sa_mask, SIGHUP);
    sigaddset (&sig_act.sa_mask, SIGPIPE);
#ifdef SIGSYS
    sigaddset (&sig_act.sa_mask, SIGSYS);
#endif
#else
    sig_act.sa_mask = ((1 << (SIGINT - 1))
        | (1 << (SIGQUIT - 1))
#ifdef SIGBUS
        | (1 << (SIGBUS - 1))
#endif
        | (1 << (SIGILL - 1))
        | (1 << (SIGSEGV - 1))
        | (1 << (SIGHUP - 1))
        | (1 << (SIGPIPE - 1))
#ifdef SIGSYS
        | (1 << (SIGSYS - 1))
#endif
        );
#endif /* _POSIX_SOURCE */
    sig_act.sa_flags = 0;
    sigaction (SIGHUP, &sig_act, (struct sigaction *) 0);
    sigaction (SIGINT, &sig_act, (struct sigaction *) 0);
    sigaction (SIGQUIT, &sig_act, (struct sigaction *) 0);
    sigaction (SIGILL, &sig_act, (struct sigaction *) 0);
#ifdef SIGBUS
    sigaction (SIGBUS, &sig_act, (struct sigaction *) 0);
#endif
    sigaction (SIGSEGV, &sig_act, (struct sigaction *) 0);
#ifdef SIGSYS
    sigaction (SIGSYS, &sig_act, (struct sigaction *) 0);
#endif
#endif /* USGISH */

    /* sort the file list, to make look-up easier */
    if (sort_targets && filelist[0] && filelist[1]) {
        int i;
        for (i = 1; filelist[i]; i++) {
            char *key = filelist[i];
            int j;
            for (j = i; j > 0 && strcmp(filelist[j-1], key) > 0; j--)
                filelist[j] = filelist[j-1];
            filelist[j] = key;
        }
    }

    /*
     * now peruse through the list of files.
     */
    for (fltail = filelist; *fltail; fltail++) {
        char *path = locatefile (*fltail, (const char**)searchpathlist, numsearchpaths);
        filecontent = getfile (path);
        setfile_cmdinc (filecontent, cmdinc_count, cmdinc_list);
        ip = newinclude (path, (char *) NULL);
        setfile_vars (path, ip);
        find_includes (filecontent, ip, ip, 0, TRUE);
        freefile (filecontent);
        if (include_cfile)
            ip->i_flags |= FORCED_DEP;
        recursive_pr_include (ip, ip->i_file);
        inc_clean ();
    }
    if (printed)
        printf ("\n");

    if (searchpathlist)
        free (searchpathlist);

    return 0;
}

#ifdef __UNIXOS2__
/*
 * eliminate \r chars from file
 */
static int elim_cr (char *buf, int sz)
{
    int i, wp;
    for (i = wp = 0; i < sz; i++) {
        if (buf[i] != '\r')
            buf[wp++] = buf[i];
    }
    return wp;
}
#endif

static void convert_slashes (char *path)
{
#ifdef WIN32
    char *ptr;
    assert(path);
    while ((ptr = strchr (path, '/')))
        *ptr = '\\';
#else
    (void)path;
#endif
}

char *locatefile (char *filename, const char **searchpaths, int searchentries)
{
    /* Note: this function is not re-entrant, because of this static buffer */
    static char localpath[2048];
    int i;

    assert(filename);
    assert(searchentries >= 0);
    if (!searchpaths || searchentries == 0)
        return filename;    /* no search paths set, use given file */
    if (strlen (filename) >= sizeof localpath)
        return filename;    /* the filename is so long, we cannot prefix a local path anyway */
    strcpy (localpath, filename);
    convert_slashes (localpath);
    if (localpath[0] == '/' || localpath[0] == '\\' || (strlen(localpath) > 3 && localpath[1] == ':' && localpath[2] == '\\'))
        return localpath;   /* absolute path, use given file (but convert slashes) */
    if (access (localpath, 0) == 0)
        return localpath;   /* file found (after possibly converting slashes), no need to look further */
    for (i = 0; i < searchentries; i++) {
        size_t len = strlen (searchpaths[i]);
        if (len == 0 || len >= sizeof localpath - 3)
            continue;       /* this search path is either empty, so long, no filename could realistically be appended to it */
        strcpy (localpath, searchpaths[i]);
        if (localpath[len - 1] != '/' && localpath[len - 1] != '\\') {
            localpath[len++] = '/';
            localpath[len] = '\0';
        }
        if (len + strlen (filename) >= sizeof localpath)
            continue;
        strcat (localpath, filename);
        convert_slashes (localpath);
        if (access (localpath, 0) == 0)
            return localpath;   /* file found in this path, use it */
    }
    return filename;        /* no other location found, use given name (which will likely fail) */
}

struct filepointer *getfile (const char *filename)
{
    int fd;
    struct filepointer *content;
    struct stat st;

    content = (struct filepointer *) malloc (sizeof (struct filepointer));
    if (!content)
        memoryerr();
    assert(content);
    content->f_name = filename;
    if ((fd = open (filename, O_RDONLY)) < 0) {
        warning ("cannot open \"%s\"\n", filename);
        content->f_p = content->f_base = content->f_end = (char *) malloc (1);
        *content->f_p = '\0';
        return (content);
    }
    fstat (fd, &st);
    content->f_base = (char *) malloc (st.st_size + 1);
    if (content->f_base == NULL)
        memoryerr ();
    assert(content->f_base);
    if ((st.st_size = read (fd, content->f_base, st.st_size)) < 0)
        fatalerr ("failed to read %s\n", filename);
#ifdef __UNIXOS2__
    st.st_size = elim_cr (content->f_base, st.st_size);
#endif
    close (fd);
    content->f_len = st.st_size + 1;
    content->f_p = content->f_base;
    content->f_end = content->f_base + st.st_size;
    *content->f_end = '\0';
    content->f_line = 0;
    content->cmdinc_count = 0;
    content->cmdinc_list = NULL;
    content->cmdinc_line = 0;
    return (content);
}

static
const char *skip_path(const char *filename)
{
    const char *pathsep;
    assert(filename != NULL);
    pathsep = strrchr(filename, '/');
    if (pathsep)
        filename = pathsep + 1;   /* skip / */
#ifdef WIN32
    pathsep = strrchr(filename, '\\');
    if (pathsep)
        filename = pathsep + 1;   /* skip \\ */
#endif
    return filename;
}

void setfile_vars (const char *name, struct inclist *file)
{
    const char *ext;

    name = skip_path(name);
    ext = strrchr(name, '.');
    if (ext) {
        if (strcasecmp(ext, ".cpp") == 0 || strcasecmp(ext, ".cxx") == 0
            || strcasecmp(ext, ".cc") == 0 || strcasecmp(ext, ".c++") == 0)
            define2 ("__cplusplus", NULL, "199711L", file);
    }
}

void setfile_cmdinc (struct filepointer *filep, long count, char **list)
{
    filep->cmdinc_count = count;
    filep->cmdinc_list = list;
    filep->cmdinc_line = 0;
}

void freefile (struct filepointer *fp)
{
    free (fp->f_base);
    free (fp);
}

char *copy (const char *str)
{
    char *p = (char *) malloc (strlen (str) + 1);
    if (p)
        strcpy (p, str);
    return (p);
}

int match (const char *str, const char **list)
{
    int i;

    for (i = 0; *list; i++, list++)
        if (strcmp (str, *list) == 0)
            return (i);
    return (-1);
}

/*
 * Get the next line.  We only return lines beginning with '#' since that
 * is all this program is ever interested in.
 */
char *getnextline (struct filepointer *filep)
{
    char *p,                          /* walking pointer */
     *eof,                            /* end of file pointer */
     *bol;                            /* beginning of line pointer */
    int lineno;                       /* line number */

    /*
     * Fake the "-include" line files in form of #include to the
     * start of each file.
     */
    if (filep->cmdinc_line < filep->cmdinc_count) {
        char *inc = filep->cmdinc_list[2 * filep->cmdinc_line + 0];
        char *buf = filep->cmdinc_list[2 * filep->cmdinc_line + 1];
        filep->cmdinc_line++;
        sprintf (buf, "%s%s%s", DASH_INC_PRE, inc, DASH_INC_POST);
        DBG_PRINT (stderr, "%s\n", buf);
        return (buf);
    }

    p = filep->f_p;
    eof = filep->f_end;
    if (p >= eof)
        return ((char *) NULL);
    lineno = filep->f_line;

    for (bol = p--; ++p < eof;) {
        if ((bol == p) && ((*p == ' ') || (*p == '\t'))) {
            /* Consume leading white-spaces for this line */
            while (((p + 1) < eof) && ((*p == ' ') || (*p == '\t'))) {
                p++;
                bol++;
            }
        }

        if (*p == '/' && (p + 1) < eof && *(p + 1) == '*') {
            /* Consume C comments */
            *(p++) = ' ';
            *(p++) = ' ';
            while (p < eof && *p) {
                if (*p == '*' && (p + 1) < eof && *(p + 1) == '/') {
                    *(p++) = ' ';
                    *(p++) = ' ';
                    break;
                }
                if (*p == '\n')
                    lineno++;
                *(p++) = ' ';
            }
            --p;
        }
        else if (*p == '/' && (p + 1) < eof && *(p + 1) == '/') {
            /* Consume C++ comments */
            *(p++) = ' ';
            *(p++) = ' ';
            while (p < eof && *p) {
                if (*p == '\\' && (p + 1) < eof && *(p + 1) == '\n') {
                    *(p++) = ' ';
                    lineno++;
                }
                else if (*p == '?' && (p + 3) < eof &&
                    *(p + 1) == '?' && *(p + 2) == '/' && *(p + 3) == '\n') {
                    *(p++) = ' ';
                    *(p++) = ' ';
                    *(p++) = ' ';
                    lineno++;
                }
                else if (*p == '\n')
                    break;            /* to process end of line */
                *(p++) = ' ';
            }
            --p;
        }
        else if (*p == '\\' && (p + 1) < eof && *(p + 1) == '\n') {
            /* Consume backslash line terminations */
            *(p++) = ' ';
            *p = ' ';
            lineno++;
        }
        else if (*p == '?' && (p + 3) < eof &&
            *(p + 1) == '?' && *(p + 2) == '/' && *(p + 3) == '\n') {
            /* Consume trigraph'ed backslash line terminations */
            *(p++) = ' ';
            *(p++) = ' ';
            *(p++) = ' ';
            *p = ' ';
            lineno++;
        }
        else if (*p == '\n') {
            lineno++;
            if (*bol == '#') {
                char *cp;

                *(p++) = '\0';
                /* punt lines with just # (yacc generated) */
                for (cp = bol + 1; *cp && (*cp == ' ' || *cp == '\t'); cp++);
                if (*cp)
                    goto done;
                --p;
            }
            bol = p + 1;
        }
    }
    if (*bol != '#')
        bol = NULL;
 done:
    filep->f_p = p;
    filep->f_line = lineno;
#ifdef DEBUG_DUMP
    if (bol)
        DBG_PRINT (stderr, "%s\n", bol);
#endif
    return (bol);
}

/*
 * Strip the file name down to what we want to see in the Makefile.
 * (It will get objprefix and objsuffix around it.)
 */
char *base_name (const char *file)
{
    char *p;
    char *newfile = copy (file);    /* creates a copy on the heap */
    if (!newfile)
        memoryerr ();
    assert(newfile);
    for (p = newfile + strlen (newfile); p > newfile && *p != '.'; p--)
        /*EMPTY*/;

    if (*p == '.')
        *p = '\0';

    p = newfile + strlen (newfile);
#if defined WIN32
    while (p > newfile && *p != '/' && *p != '\\')
        p--;
#else
    while (p > newfile && *p != '/')
        p--;
#endif
    if (p != newfile)
        memmove(newfile, p + 1, strlen(p)); /* return name after last path */

    return newfile;
}

#if defined(USG) && !defined(CRAY) && !defined(SVR4) && !defined(__UNIXOS2__) && !defined(clipper) && !defined(__clipper__)
int rename (char *from, char *to)
{
    (void) unlink (to);
    if (link (from, to) == 0) {
        unlink (from);
        return 0;
    }
    else {
        return -1;
    }
}
#endif /* USGISH */

void redirect (const char *line, const char *makefile, const char *filelist[])
{
    struct stat st;
    FILE *fdin, *fdout;
    char backup[BUFSIZ], buf[BUFSIZ];
    boolean found = FALSE;
    int len;

    /*
     * if makefile is "-" then let it pour onto stdout.
     */
    if (makefile && *makefile == '-' && *(makefile + 1) == '\0') {
        puts (line);
        return;
    }

    /*
     * use a default makefile when none is specified.
     */
    if (!makefile) {
        if (stat ("Makefile", &st) == 0)
            makefile = "Makefile";
        else
            makefile = "makefile";
    }
    assert (makefile);
    if (stat (makefile, &st) != 0) {
        /* create a dummy makefile for the output */
        if ((fdin = fopen (makefile, OPEN_WRITE_TEXT)) != NULL) {
            fprintf(fdin, "# GENERATED BY MAKEDEPEND\n\n");
            fclose(fdin);
            stat (makefile, &st);
        }
        /* on failure to create the file, an error will be issued when trying to re-open it */
    }

    if ((fdin = fopen (makefile, OPEN_READ_TEXT)) == NULL)
        fatalerr ("cannot open \"%s\"\n", makefile);
    sprintf (backup, "%s.bak", makefile);
    unlink (backup);
#if defined(WIN32) || defined(__UNIXOS2__) || defined(__CYGWIN__)
    assert(fdin);
    fclose (fdin);
#endif
    if (rename (makefile, backup) < 0)
        fatalerr ("cannot rename %s to %s\n", makefile, backup);
#if defined(WIN32) || defined(__UNIXOS2__) || defined(__CYGWIN__)
    if ((fdin = fopen (backup, OPEN_READ_TEXT)) == NULL)
        fatalerr ("cannot open \"%s\"\n", backup);
#endif
    if ((fdout = freopen (makefile, OPEN_WRITE_TEXT, stdout)) == NULL)
        fatalerr ("cannot open \"%s\"\n", backup);
    assert(fdin);
    assert(fdout);
    len = strlen (line);
    while (!found && fgets (buf, BUFSIZ, fdin)) {
        if (*buf == '#' && strncmp (line, buf, len) == 0)
            found = TRUE;
        fputs (buf, fdout);
    }
    if (!found) {
        if (verbose)
            warning ("Adding new delimiting line and dependencies...\n");
        puts (line);                  /* same as fputs(fdout); but with newline */
    }
    else if (filelist) {
        int skip_rule = 0;
        int skip_newlines = 0;
        while (fgets (buf, BUFSIZ, fdin)) {
            /* check whether this is the start of a dependency line (with the
             * name of the target
             */
            char *ptr;
            if (buf[0] != '#' && buf[0] > ' ' && (ptr = strchr (buf, ':')) != NULL) {
                /* extract the target name (replacing escaped space characters
                 * by a single space; this is a special case for filenames with
                 * spaces)
                 */
                size_t length = ptr - buf;
                char *target = (char*)malloc(length);
                size_t i, j;
                const char **fp;
                if (!target)
                    continue;
                for (i = j = 0; i < length; i++, j++) {
                    if (buf[i] == '\\' && buf[i + 1] == ' ')
                        i++;    /* ignore \ if followed by a space */
                    target[j] = buf[i];
                }
                while (j > 0 && target[j - 1] <= ' ')
                    j--;        /* delete trailing whitespace */
                target[j] = '\0';
                /* check whether this target exists in the filelist; if so,
                 * skip it in copying the original file to the output file
                 */
                skip_rule = 0;
                for (fp = filelist; *fp && !skip_rule; fp++) {
                    const char *basename = base_name(*fp);
                    const char *tgtname = targetname(basename);
                    if (strcmp (target, tgtname) == 0)
                        skip_rule = 1;
                    free ((void*)basename);
                    free ((void*)tgtname);
                }
                /* after a first rule, ignore empty lines */
                skip_newlines = 1;
            }
            if (!skip_rule && (buf[0] != '\n' || !skip_newlines))
                fputs(buf, fdout);
        }
    }
    fclose(fdin);
    if (!make_backup)
        unlink (backup);
    fflush (fdout);
    /* don't close fdout, because it is the re-opened stdout */
#if defined(USGISH) || defined(_SEQUENT_) || defined(USE_CHMOD)
    _chmod (makefile, st.st_mode);
#else
    fchmod (fileno (fdout), st.st_mode);
#endif /* USGISH */
}

void memoryerr(void)
{
    fatalerr("insufficient memory (or memory allocation failure)\n");
}

void fatalerr(const char *msg, ...)
{
    va_list args;
    fprintf(stderr, "\n%s - error: ", skip_path(ProgramName));
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    exit(1);
}

static
void warning_header(void)
{
    static boolean first_warning = TRUE;
    if (first_warning) {
        fprintf(stderr, "\n");
        first_warning = FALSE;
    }

    assert(ProgramName != NULL);
    fprintf(stderr, "%s - warning: ", skip_path(ProgramName));
}

void warning(const char *msg, ...)
{
    va_list args;

    assert(msg != NULL);

    warning_header();

    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

void warning_std(struct inclist *file_red, struct inclist *file, long linenr, const char *msg, ...)
{
    va_list args;

    assert(file != NULL);
    assert(msg != NULL);

    warning_header();
    if (file_red != NULL && file_red != file && strcmp(file_red->i_file, file->i_file) != 0)
        fprintf(stderr, "[%s] ", file_red->i_file);
    fprintf(stderr, "%s(%ld): ", file->i_file, linenr);

    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

void warning_cont(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

void showusage (void)
{
    printf("makedepend 1.0.10\n\n");
    printf("Usage: makedepend [options] <file1.c> [file2.c] [...]\n\n");
    printf("-D<name>\tAdd a definition for <name> (with value 1).\n");
    printf("-D<name>=<def>\tAdd a definition for <name>, with value <def>.\n");
    printf("-I<directory>\tAdd the directory to the list of include directories.\n");
    printf("-I-\t\tSkip the standard (compiler-defined) include directories.\n");
    printf("-L<directory>\tAdd the directory to the search paths for source files.\n");
    printf("-U<name>\tUndefine <name>.\n");
    printf("-a\t\tAppend dependencies to the end of the file instead of replacing\n"
           "\t\tthem.\n");
    printf("-b\t\tDo not create a backup file of the input makefile.\n");
    printf("-c\t\tInclude the C/C++ source file in the dependencies for the\n"
           "\t\tobject file.\n");
    printf("-e\t\tExclude \"system\" includes from the dependency list.\n");
    printf("-f<makefile>\tSet the file into which the output is written (default is\n"
           "\t\t\"Makefile\"). Setting -f- sends the output to standard output.\n");
    printf("-h\t\tShow usage information (this text).");
    printf(verbose ? "\n" : " Use -h -v for details.\n");
    printf("-i\t\tDo not warn about include files that cannot be found.\n");
    printf("-m\t\tWarn about multiple inclusions.\n");
    printf("-o<suffix>\tObject file extension. The default is \".o\".\n");
    printf("-p<prefix>\tObject file prefix, typically used to set a directory. If the\n"
           "\t\tprefix starts with a \"-\", that prefix is stripped from the\n"
           "\t\tobject file if the start of the object file matches the word\n"
           "\t\tthat follows the \"-\".\n");
    printf("-s<delimitor>\tDelimiter string (or \"separator\") in the makefile, below which\n"
           "\t\tgenerated dependencies are written.\n");
    printf("-sort\t\tSort the targets alphapetically.\n");
    printf("-v\t\tVerbose output.\n");
    printf("-w<width>\tMaximum line width of the generated output (default is 78\n"
           "\t\tcharacters).\n");
    printf("--\t\tAny -D, -I or -U argument following a double hyphen is handled,\n"
           "\t\tbut other options are silently ignored. Use another double\n"
           "\t\thyphen to toggle back to normal mode.\n");
    printf("-include <file>\tThe specified file is implicitly included in every source file.\n"
           "\t\tThe -include option may appear multiple times, if more than one\n"
           "\t\timplicit include is needed.\n");

    if (verbose) {
        const struct symtab *psymp = predefs;
        printf("\nPredefined variables:\n");
        while (psymp->s_name) {
            printf("\t%-16s = %s\n", psymp->s_name, psymp->s_value);
            psymp++;
        }
    }

    exit(1);
}