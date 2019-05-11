/* $Xorg: pr.c,v 1.4 2001/02/09 02:03:16 xorgcvs Exp $ */
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
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/config/makedepend/pr.c,v 1.4 2001/04/29 23:25:02 tsi Exp $ */

#include "def.h"

extern char *objprefix;
extern char *objsuffix;
extern int  width;
extern boolean  printed;
extern boolean  verbose;

static void
strxcat(char *target, const char *source, size_t tgtlength, int expand)
{
    size_t len;

    assert(target != NULL);
    assert(source != NULL);

    len = strlen(target);
    if (tgtlength <= len)
        return; /* no space at all (not even for the zero terminator) */

    tgtlength -= len;
    target += len;
    while (*source != '\0') {
        if (expand && *source == ' ') {
            if (tgtlength > 2) {
                *target++ = '\\';
                *target++ = ' ';
                tgtlength -= 2;
            }
        } else {
            if (tgtlength > 1) {
                *target++ = *source;
                tgtlength--;
            }
        }
        source++;
    }
    *target = '\0';
}

char *
targetname(const char *base)
{
    int prefixlen;
    char *target = malloc(BUFSIZ);
    if (!target)
        memoryerr();

    assert(target);
    *target = '\0';
    assert(base != NULL);
    prefixlen = strlen(objprefix);
    if (*objprefix == '-' && strncmp(base, objprefix + 1, prefixlen - 1) == 0) {
        /* delete prefix (on match) */
        strxcat(target, base + prefixlen - 1, BUFSIZ, 0);
    } else {
        /* add prefix */
        strxcat(target, objprefix, BUFSIZ, 0);
        strxcat(target, base, BUFSIZ, 0);
    }
    strxcat(target, objsuffix, BUFSIZ, 0);

    target = realloc(target, strlen(target) + 1);
    assert(target != NULL); /* since the size shrinks, realloc always succeeds */
    return target;
}

static void
pr(struct inclist *ip, const char *file, const char *base)
{
    static const char *lastfile;
    static int  current_len;
    int    len, i;
    char   buf[ BUFSIZ ];

    printed = TRUE;
    assert(ip != NULL);
    len = strlen(ip->i_file)+1;
    assert(file != NULL);
    if (file != lastfile) {
        const char *target = targetname(base);
        strcpy(buf, "\n");
        strxcat(buf, target, sizeof(buf), 1);
        strxcat(buf, " : ", sizeof(buf), 0);
        strxcat(buf, ip->i_file, sizeof(buf), 1);
        lastfile = file;
        free((void*)target);
        len = current_len = strlen(buf);
    } else if (current_len + len + 5 > width) {
        strcpy(buf, " \\\n\t");
        strxcat(buf, ip->i_file, sizeof(buf), 1);
        len = current_len = strlen(buf);
    } else {
        strcpy(buf, " ");
        strxcat(buf, ip->i_file, sizeof(buf), 1);
        current_len += len;
    }
    fwrite(buf, len, 1, stdout);

    /*
     * If verbose is set, then print out what this file includes.
     */
    if (! verbose || ip->i_list == NULL || ip->i_flags & NOTIFIED)
        return;
    ip->i_flags |= NOTIFIED;
    lastfile = NULL;
    printf("\n# %s includes:", ip->i_file);
    for (i=0; i<ip->i_listlen; i++)
        printf("\n#\t%s", ip->i_list[ i ]->i_incstring);
}

void
recursive_pr_include(struct inclist *head, const char *file, const char *base)
{
    int i;

    if (head->i_flags & MARKED)
        return;
    head->i_flags |= MARKED;
    if (head->i_file != file || (head->i_flags & FORCED_DEP) != 0)
        pr(head, file, base);
    for (i=0; i<head->i_listlen; i++)
        recursive_pr_include(head->i_list[ i ], file, base);
}
