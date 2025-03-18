# makedepend - create dependencies in makefiles


## Introduction

**makedepend** scans a list of C and C++ source files for `#include` statements,
and then rewrites the makefile to add these header files as "dependencies" of
the target for the C/C++ files. This will cause the Make program to recompile
the C/C++ sources if one of the header files is modified.

Originally, **makedepend** was distributed as part of the X Window system
(copyright by the Open Group). The version presented here is a derivative of
the original utility. See section [Change List](#Change-List) for details.


## Usage

In a typical case, a makefile contains a pseudo-target to update the dependencies.
The pseudo-target is typically called "depend". Thus, when you `make depend`,
the dependencies for the C/C++ sources get rebuilt.

```make
SRCS   = file1.c file2.c
CFLAGS = -Wall -DDEBUG -I../include

depend :
        makedepend -- $(CFLAGS) $(SRCS)
```

In the above example, **makedepend** parses the options in the `CFLAGS` macro.
This macro is set after a double hyphen (`--`), so that **makedepend** will not
warn for any options in `CFLAGS` that it does not recognize. Specifically, in
this example, **makedepend** handles the `-D` and `-I` options in `CFLAGS` but
silently ignores the `-Wall` option.

By default, **makedepend** writes its output to a file called `makefile` (on
operating systems with case-sensitive filenames, it tries both `makefile` and
`Makefile`). If it finds it, it appends the generated dependencies to that
makefile. Following the above example, the output of running `make depend` might
become:

```make
SRCS   = file1.c file2.c
CFLAGS = -Wall -DDEBUG -I../include

depend :
        makedepend -- $(CFLAGS) $(SRCS)

# GENERATED DEPENDENCIES. DO NOT DELETE.
file1.o : file1.h file2.h common.h
file2.o : file2.h common.h
```

Running `make depend` again will replace the lines below the comment `# GENERATED DEPENDENCIES...`
with the freshly generated dependency list. This is why it is important to not
remove or change that comment, because if **makedepend** does not find that line,
it appends its dependency list to the makefile again, rather than replacing the
existing (and possibly outdated) dependency list.

### Keep dependencies separate from build rules

In an environment where you work in a team, and especially when the makefile is
committed to version control, it may be preferable to keep machine-generated
dependencies separate from the general build rules and options. Accordingly,
**makedepend** is set to write its output to a different file, which is then
included in the makefile.

For example:
```make
SRCS   = file1.c file2.c
CFLAGS = -Wall -DDEBUG -I../include

depend :
        makedepend -fmakefile.dep -- $(CFLAGS) $(SRCS)

-include makefile.dep
```

The `-f` option sets the name of the output file. This output file ("makefile.dep")
is then included in the makefile. The `include` directive (on the last line) is
prefixed by a `-` so that Make won't complain when makefile.dep is initially
missing (this is the GNU Make syntax; other Make's may need a different syntax).

### Automatic updating dependencies

The above examples update the dependencies when making the target `depend`. This
step can also be automated, such that the dependencies are updated when some
targets when the relevant source files change. To this end, the rule for
**makedepend** must list the source files as the dependencies, and the targets for
the object file must list the generated output file as a dependency.

```make
SRCS   = file1.c file2.c
CFLAGS = -Wall -DDEBUG -I../include

makefile.dep : $(SRCS)
        makedepend -a -f$@ -- $(CFLAGS) $?

$(SRCS:.c=.o) : makefile.dep

-include makefile.dep
```

Note that the shell line for **makedepend** uses the `$?` macro to list only the
files from `$(SRCS)` on the command line that are newer than the target
`makefile.dep`. This reduces the time needed to update the dependencies, because
only the modified files are scanned.

The other important change on the shell line for **makedepend** is the `-a` option.
Without this option, the resulting `makefile.dep` would contain only the
dependencies of the files of the most recent run. By default, **makedepend** removes
all dependency lines from the output file. The `-a` option lets **makedepend**
remove <em>only</em> the dependency lines of the files on the command line
(which will be regenerated).

The line to add `makefile.dep` as a dependency to all targets that are built
from the list of sources, is GNU Make syntax, using suffix substitution of the
`.c` file extension to `.o`.

### Predefined variables

While **makedepend** processes the source files, the predefined macro `_makedepend`
is set. Testing for this macro in the source file allows you to to conditionally
exclude a part of the source code from **makedepend**'s processing.

Other predefined variables that **makedepend** sets are platform-specific. you can
run `makedepend -h -v` to show the list of predefined variables. You can use the
`-U` argument to undefine a predefined variable.

When a source file on the **makedepend** command line has the extension `.cpp`, `.cxx`,
`.cc` or `.c++`, the variable `__cplusplus` is predefined for that file.

### Setting "system" include paths

When cross-compiling for a different platform (for example, when developing
embedded software for a microcontroller on a workstation), the system include
paths are likely for the workstation's main compiler setup and may not be
correct for the target compiler. In this case, you can remove the system include
paths with the `-I-` option and set a specific path for *system* includes.

*System* includes refer to include directives with angle brackets (`< ... >`):
```#include <stdio.h>
```

### Locate source files with VPATH

When source files are stored in different directories, with `vpath`'s set to
locate the files, add the source files as dependencies on the `depend` target
line. This way, Make will create the source file list after the `vpath` lookup.

```make
CFLAGS   =-Wall -I./startup/cmsis -std=c1x

CMSIS_O  = bootLPC11Uxx.o systemLPC11Uxx.o
USBHID_O = usbhid.o iap.o tracesupport.o uart.o
COREUSB_O= usb_LPC11Uxx.o usbcore.o usbdescr.o usbuser.o hiduser.o

vpath %.c   ./startup/cmsis     # startup code (CMSIS compatible)
vpath %.c   ./coreusb           # USB support code

.PHONY: depend
depend : $(USBHID_O:.o=.c) $(CMSIS_O:.o=.c) $(COREUSB_O:.o=.c)
        makedepend -fmakefile.dep -b -I- -- $(CFLAGS) $^
```

In the example above, from the top there are the definitions of `CFLAGS` and of
lists of object files for various modules. It is common to list the object files,
because these are the dependencies of the linker command. Following that, are
`vpath` definitions, indicating where Make will search for the C files (this
example assumes GNU Make, the syntax may be different for other Make utilities).

The `depend` target is declared as "phony" for good measure (as it is not a target
that will exist as a file). The dependencies of the `depend` target are the three
lists with the object files, except that the extension `.o` is replaced by `.c`.
This enables you to use the automatic macro `$^` (for "all sources") on the
`makedepend` command line. When Make builds the file list for `$^`, it looks
through the `vpath` directories for the files, and it will add the directories
to the files where applicable. For example, if `bootLPC11Uxx.c` is found in
`./startup/cmsis/`, the full path `./startup/cmsis/bootLPC11Uxx.c` is included
in `$^`.

### Dependencies for generated includes

When **makedepend** cannot access an include file, it issues a warning and does
*not* add it to the dependency list for the target. In other words, an include
file must exist (in the search paths), for **makedepend** to add it to the
dependency list of the target.

When an include file is *generated* by a utility, that file would be missed by
**makedepend**, because it does not exist *yet*. To remediate this, add the `-i`
option to the **makedepend** command line. With this option, the warning message
for a missing include file is suppressed and the name is added to the dependency
list. However, this applies only to *user* includes, not *system* includes.

*User* includes refer to `#include` directives using double quotes, whereas
*system* includes use angle brackets (`<...>`):
```#include "config.h"   /* user include */
#include <stdio.h>    /* system include */
```

## Options

For environments with limited command line lengths, **makedepend** can read the
options for a *response file*. The response file is passed to **makedepend**
with the following syntax (note the "@" character):
```makedepend @options.txt
```

The response file may have all options split over multiple lines.

<dl>
<dt> <code>-D</code>name=def </dt>
<dt> <code>-D</code>name </dt>
<dd>
  Defines a symbol for the <b>makedepend</b> preprocessor (which works like the
  C/C++ preprocessor). When no value is explicitly defined, the symbol is
  defined as "1".
</dd>

<dt> <code>-I</code>path </dt>
<dd>
  Adds a path to its list of directories that <b>makedepend</b> searches for
  files when it encounters a <code>#include</code> directive.
  <br><br>
  By default, <b>makedepend</b> appends the standard include directories at the end
  of the directory list. In Linux, Unix and OS/2, <b>makedepend</b> evaluates the
  <code>C_INCLUDE_PATH</code> environment variable for the standard includes; in
  Microsoft Windows, <b>makedepend</b> evaluates the <code>INCLUDE</code> environment
  variable. When the option <code>-I-</code> is set, <b>makedepend</b> does <em>not</em>
  append the standard include directories (and thus prevents <b>makedepend</b> from
  searching the standard include directories).
</dd>

<dt> <code>-L</code>path </dt>
<dd>
  Adds a path for <b>makedepend</b> to locate <em>source</em> files. That is, if
  <b>makedepend</b> does not find a source file that is specified on the command
  line in the active directory, it prepends the paths set this the <code>-L</code>
  option and tries again.
  <br><br>
  The <code>-L</code> option may appear multiple times. The paths are searched
  in the order that they appear on the command line. The active directory is
  always searched first.
</dd>

<dt> <code>-U</code>name </dt>
<dd>
  Undefines a symbol. This is mostly useful to remove a predefined symbol.
</dd>

<dt> <code>-a</code> </dt>
<dd>
  Accumulates the dependencies in the output file instead of removing the
  dependencies for files that are <em>not</em> listed on the command line of
  <b>makedepend</b>. With this option, you can call <b>makedepend</b> multiple times with
  different filename lists, and obtaining the accumulated dependencies of all
  those calls.
  <br><br>
  Note that dependencies for files that are listed on the command line are
  replaced in the output file. The <code>-a</code> option is not a simple
  "append".
</dd>

<dt> <code>-b</code> </dt>
<dd>
  No backups. By default, <b>makedepend</b> copies the makefile to one with a .bak
  extension before modifying it. When this option is set, <b>makedepend</b> deletes
  the backup file.
</dd>

<dt> <code>-c</code> </dt>
<dd>
  Includes the C/C++ source file in the list of dependencies. By default,
  <b>makedepend</b> only lists all files <em>included</em> by the source file on the
  dependency line.
</dd>

<dt> <code>-e</code> </dt>
<dd>
  Excludes <em>system includes</em> from the list of dependencies. System
  includes are files that are between angle brackets (after the #include).
  These are less relevant to list as dependecies; the system include files
  can be considered stable.
</dd>

<dt> <code>-f</code>makefile </dt>
<dd>
  Sets the output filename, instead of the default name "makefile". When set
  to "-" (i.e. option <code>-f-</code>), the output is sent to standard output
  (the console).
</dd>

<dt> <code>-h</code> </dt>
<dd>
  Help. Shows brief usage information for <b>makedepend</b>. When verbose output is
  enabled (see <code>-v</code>), the list of predefined variables is listed too.
</dd>

<dt> <code>-i</code> </dt>
<dd>
  Ignore include files that cannot be located. That is, do <i>not</i> warn about
  any include files that cannot be found. In addition, any missing include files
  are <i>still</i> added as a dependency to the target. (Without this option,
  include files that are not found, are <i>removed</i> from the dependency list of
  the target; and a warning is issued.)
</dd>

<dt> <code>-m</code> </dt>
<dd>
  Enables warnings for multiple inclusion (of the same file). This option is
  provided to aid in debugging problems related to multiple inclusion.
</dd>

<dt> <code>-o</code>suffix </dt>
<dd>
  Object file suffix. The default suffix is ".o", as is common for Unix-like
  operating systems and the GNU GCC compiler suite. Om Microsoft Windows, the
  suffix ".obj" is more common, which can be set with the option <code>-o.obj</code>.
</dd>

<dt> <code>-p</code>prefix </dt>
<dd>
  Sets the text that is prepended to the name of each object file. This is
  usually used to designate a different directory for the object file.
  <br><br>
  If the prefix pattern starts with a minus ("-"), that prefix is <em>removed</em>
  from the object filename (if the object filename starts with that prefix).
  <br><br>
  The default prefix string is the empty string.
</dd>

<dt> <code>-s</code>string </dt>
<dd>
  Sets the delimiter string (or "separator") below which <b>makedepend</b> writes the
  generated depedencies. The default string is
  "#&nbsp;GENERATED&nbsp;DEPENDENCIES.&nbsp;DO&nbsp;NOT&nbsp;DELETE.".
</dd>

<dt> <code>-v</code> </dt>
<dd>
  Verbose operation: this option causes <b>makedepend</b> to show more notices and
  warnings during processing the files. It will also emit the list of files
  included by each input file.
</dd>

<dt> <code>-w</code>width </dt>
<dd>
  Sets the maximum line width for the lines written to the output file. The
  default value is 78 characters.
</dd>

<dt> <code>-include</code> file </dt>
<dd>
  Processes the file includes it before processing each regular input file.
  This has the same affect as if the specified file were listed in an
  <code>#include</code> directive at the very to of each regular input file.
  <br><br>
  The <code>-include</code> option may be present multiple times on the command
  line.
</dd>

<dt> <code>--</code> options <code>--</code> </dt>
<dd>
  Following a double hyphen (<code>--</code>), only the <code>-D</code>, <code>-I</code> and <code>-U</code> options are
  handled, and any others are silently ignored (source filenames are still
  handled). The intended purpose of this syntax is that the same <code>CFLAGS</code>
  macro that is passed to the C/C++ compiler can also be passed to <b>makedepend</b>.
  <br><br>
  A second double hyphen ends this special processing; it is needed if options
  that are specific to <b>makedepend</b> follow the definitions in <code>CFLAGS</code> (or
  another makefile macro).
</dd>
</dl>


## License

Copyright (c) 1993, 1994, 1998 The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that the
above copyright notice appear in all copies and that both that copyright notice
and this permission notice appear in supporting documentation.

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


## Change List

This version is derived from a port of **makedepend** by [Derell Licht](https://github.com/DerellLicht/makedepend) (circa 2005)
of the original source code to a contemporary version of GNU GCC. It has since
been modified to bring new features and improvements:

* Some minor adjustments for better compatibility with Microsoft Windows, such
  as handling the backslash as an equivalent as the forward slash in paths.
* Filenames with space characters are written with the space characters
  "escaped" with a backslash (in a way compatible with GNU Make).
* The `-a` command line option now does an intelligent "append", where
  dependencies for new files are added, but if a file that **makedepend** scans
  already appears in the section below the delimiter, the dependencies for that
  file are replaced.
* Added the `-b` command line option, to skip making backups of the input
  makefiles.
* Added the `-c` command line option, to add the source file as a dependency
  for each target (in addition to the files included by that source file).
* Added the `-e` command line option, to list only project-specific include
  files as dependencies (and skip any system includes). The rationale is that
  this option makes a Makefile more portable, because all listed dependencies
  are part of the project (all system-specific dependencies are excluded).
* Added the `-h` command line option, to show program usage. The `-?` option is
  equivalent to `-h`.
* Let **makedepend** display the (platform-specific) list of predefined variables
  with the `-h -v` arguments (verbose help).
* Replace the option `-Y` by `-I-`.
* Add option `-L` as an alternative to using `vpath` and/or specifying full
  paths on the command line.
* Let **makedepend** create the "makefile" with dependencies if it does not yet
  exist.
* Read the `INCLUDE` environment variable on Microsoft Windows (instead of the
  `C_INCLUDE_PATH` variable).
* Added the prededined `_makedepend` and `__cplusplus` variables.
* Add a warning for the case that an include file cannot be located (and don't
  add that file as a dependency). The `-i` option reverts this behaviour
  (silences the warning, and still adding it as a dependency).

