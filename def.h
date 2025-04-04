/* $Xorg: def.h,v 1.4 2001/02/09 02:03:16 xorgcvs Exp $ */
/*

Copyright (c) 1993, 1994, 1998 The Open Group.

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
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/config/makedepend/def.h,v 3.15tsi Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#if !defined _MSC_VER
  #include <unistd.h>
#endif
#if 0
  #ifndef X_NOT_POSIX
    #ifndef _POSIX_SOURCE
    #define _POSIX_SOURCE
    #endif
  #endif
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#if defined _MSC_VER
  #define snprintf    _snprintf
#endif

#define MAXDEFINES  512
#define MAXFILES    2048
#define MAXINCFILES 128 /* "-include" files */
#define MAXDIRS     64
#define SYMTABINC   10  /* must be > 1 for define() to work right */
#define TRUE        1
#define FALSE       0

/* the following must match the directives table in main.c */
#define IF              0
#define IFDEF           1
#define IFNDEF          2
#define ELSE            3
#define ENDIF           4
#define DEFINE          5
#define UNDEF           6
#define INCLUDE         7   /* system include, using <...> */
#define LINE            8
#define PRAGMA          9
#define ERROR           10
#define IDENT           11
#define SCCS            12
#define ELIF            13
#define EJECT           14
#define WARNING         15
#define INCLUDENEXT     16  /* system include_next, using <...> */
#define IFFALSE         17  /* pseudo value --- never matched */
#define ELIFFALSE       18  /* pseudo value --- never matched */
#define INCLUDEUSR      19  /* pseudo value --- never matched (non-system #include, using "...") */
#define IFGUESSFALSE    20  /* pseudo value --- never matched */
#define ELIFGUESSFALSE  21  /* pseudo value --- never matched */
#define INCLUDENEXTUSR  22  /* pseudo value --- never matched (non-system #include_next, using "..." */

#ifdef DEBUG
  extern int  _debugmask;
  /*
   * debug levels are:
   *
   *     0    show ifn*(def)*,endif
   *     1    trace defined/!defined
   *     2    show #include
   *     3    show #include SYMBOL
   *     4-6  unused
   */
  #define debug(level,arg) { if (_debugmask & (1 << level)) warning arg; }
#else
  #define debug(level,arg) /**/
#endif /* DEBUG */

typedef unsigned char boolean;

struct symtab {
  const char *s_name;
  const char *s_value;
  const char *s_args;
};

/* possible i_flag */
#define DEFCHECKED  (1<<0)  /* whether defines have been checked */
#define NOTIFIED    (1<<1)  /* whether we have revealed includes */
#define MARKED      (1<<2)  /* whether it's in the makefile */
#define SEARCHED    (1<<3)  /* whether we have read this */
#define FINISHED    (1<<4)  /* whether we are done reading this */
#define INCLUDED_SYM    (1<<5)  /* whether #include SYMBOL was found
                                 * Can't use i_list if TRUE */
#define FORCED_DEP  (1<<6)  /* file is forced as a dependency */

struct  inclist {
  char   *i_incstring;      /* string from #include line */
  char   *i_file;           /* path name of the include file */
  struct inclist **i_list;  /* list of files it itself includes */
  int    i_listlen;         /* length of i_list */
  struct symtab **i_defs;   /* symbol table for this file and its children
                             * when merged */
  int     i_ndefs;          /* current # defines */
  boolean *i_merged;        /* whether we have merged child defines */
  unsigned char i_flags;
};

struct filepointer {
  const char *f_name;
  char    *f_p;
  char    *f_base;
  char    *f_end;
  long    f_len;
  long    f_line;
  long    cmdinc_count;
  char    **cmdinc_list;
  long    cmdinc_line;
};

#include <stdlib.h>
#if defined(macII) && !defined(__STDC__)  /* stdlib.h fails to define these */
  char *malloc(), *realloc();
#endif /* macII */

char*  copy(const char *str);
int    match(const char *str, const char **list);
char*  getnextline(struct filepointer *fp);
char*  base_name (const char *file);
struct symtab** slookup(const char *symbol, struct inclist *file);
struct symtab** isdefined(const char *symbol, struct inclist *file,
                          struct inclist **srcfile);
struct symtab** fdefined(const char *symbol, struct inclist *file,
                         struct inclist **srcfile);
char *locatefile(char *filename, const char **searchpath, int searchentries);
struct filepointer* getfile(const char *filename);
void   included_by(struct inclist *ip, struct inclist *newfile);
struct inclist* newinclude(char *newfile, const char *incstring);
void   inc_clean(void);
struct inclist* inc_path(const char *file, char *include, int type);

void   freefile(struct filepointer *fp);

void   define2(const char *name, const char *args, const char *val,
               struct inclist *file);
void   define(char *def, struct inclist *file);
void   undefine(const char *symbol, struct inclist *file);
int    find_includes(struct filepointer *filep,
                     struct inclist *file,
                     struct inclist *file_red,
                     int recursion, boolean failOK);

char*  targetname(const char *base);
void   recursive_pr_include(struct inclist *head, const char *file);

int    cppsetup(const char *filename,
                const char *line,
                struct filepointer *filep,
                struct inclist *inc);

void   memoryerr(void);
void   fatalerr(const char*, ...);
void   warning(const char*, ...);
void   warning_std(struct inclist *file_red, struct inclist *file, long linenr, const char *msg, ...);
void   warning_cont(const char*, ...);
void   showusage(void);

/* defined in main.c */
extern const char * const directives[];
