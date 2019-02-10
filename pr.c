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

extern char	*objprefix;
extern char	*objsuffix;
extern int	width;
extern boolean	printed;
extern boolean	verbose;

static void
pr(struct inclist *ip, const char *file, const char *base)
{
	static const char *lastfile;
	static int	current_len;
	register int	len, i, prefixlen;
	char	buf[ BUFSIZ ];

	printed = TRUE;
	len = strlen(ip->i_file)+1;
	if (file != lastfile) {
		lastfile = file;
		prefixlen = strlen(objprefix);
		if (*objprefix == '-' && strncmp(base, objprefix + 1, prefixlen - 1) == 0) {
		  /* delete prefix (on match) */
		  snprintf(buf, sizeof(buf), "\n%s%s : %s", base + prefixlen - 1, objsuffix, ip->i_file);
		} else {
		  /* normal prefix */
		  snprintf(buf, sizeof(buf), "\n%s%s%s : %s", objprefix, base, objsuffix, ip->i_file);
		}
		len = current_len = strlen(buf);
    } else if (current_len + len + 5 > width) {
		snprintf(buf, sizeof(buf), " \\\n\t%s", ip->i_file);
		len = current_len = strlen(buf);
	} else {
		buf[0] = ' ';
		strcpy(buf+1, ip->i_file);
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
	int	i;

	if (head->i_flags & MARKED)
		return;
	head->i_flags |= MARKED;
	if (head->i_file != file)
		pr(head, file, base);
	for (i=0; i<head->i_listlen; i++)
		recursive_pr_include(head->i_list[ i ], file, base);
}
