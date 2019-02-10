# makedepend - create dependencies in makefiles


## Introduction
-----
makedepend scans a list of C and C++ source files for `#include` statements,
and then rewrites the makefile to add these header files as "dependencies" of
the target for the C/C++ files. This will cause the `make` program to recompile
the C/C++ sources if one of the header files is modified.

Originally, makedepend was distributed as part of the X Window system
(copyright by the Open Group). The version presented here is a derivative of
the original utility. See section [Change List](#Change-List) for details.


## Usage
-----
In a typical case, a makefile contains a pseudo-target to update the dependencies.
The pseudo-target is typically called "depend". Thus, when you `make depend`,
the dependencies for the C/C++ sources get rebuilt.

```
SRCS   = file1.c file2.c
CFLAGS = -Wall -DDEBUG -I../include

depend :
        makedepend -- $(CFLAGS) $(SRCS)
```

In the above example, makedepend parses the options in the `CFLAGS` macro.
This macro is set after a double hyphen (`--`), so that makedepend will not
warn for any options in `CFLAGS` that it does not recognize. Specifically, in
this example, makedepend handles the `-D` and `-I` options in `CFLAGS` but
silently ignores the `-Wall` option.

By default, makedepend writes its output to a file called `makefile` (on
operating systems with case-sensitive filenames, it tries both `makefile` and
`Makefile`). If it finds it, it appends the generated dependencies to that
makefile. Following the above example, the output of running `make depend` might
become:

```
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
remove or change that comment, because if makedepend does not find that line,
it appends its dependency list to the makefile again, rather than replacing the
existing (and possibly outdated) dependency list.

In an environment where you work in a team, and especially when the makefile is
committed to version control, it may be preferable to keep machine-generated
dependencies separate from the general build rules and options. Accordingly,
makedepend is set to write its output to a different file, which is then
included in the makefile.

For example:
```
SRCS   = file1.c file2.c
CFLAGS = -Wall -DDEBUG -I../include

depend :
        makedepend -fmakefile.dep -- $(CFLAGS) $(SRCS)

-include makefile.dep
```

The `-f` option sets the name of the output file. This output file ("makefile.dep")
is then included in the makefile. The `include` directive is prefixed by a `-`
so that `make` won't complain when makefile.dep is initially missing (this is
the GNU `make` syntax; other `make`'s may need a different syntax).


## Options
-----
`-D`*name=def* or `-D`*name*
:   Defines a symbol for the makedepend preprocessor (which works like the
    C/C++ preprocessor). When no value is explicitly defined, the symbol is
    as "1".

`-I`*path*
:   Adds a path to its list of directories that makedepend searches for
    files when it encounters a `#include` directive.

    By default, makedepend appends the standard include directories at the end
    of the directory list. In Linux, Unix and OS/2, makedepend evaluates the
    `C_INCLUDE_PATH` environment variable for the standard includes; in
    Microsoft Windows, makedepend evaluates the `INCLUDE` environment
    variable. When the option `-I-` is set, makedepend does *not* append
    the standard include directories (and thus prevents makedepend from
    searching the standard include directories).

`-U`*name*
:   Undefines a symbol. This is mostly useful to remove a predefined symbol.

`-a`
:   Append the dependencies to the end of the file instead of replacing them.

`-f`*makefile*
:   Sets the output filename, instead of the default name "makefile". When set
    tp "-" (i.e. option `-f-`), the output is sent to standard output (the
    console).

`-h`
:   Shows brief usage information for makedepend. When verbose output is
    enabled (see `-v`), the list of predefined variables is listed too.

`-m`
:   Enables warnings for multiple inclusion (of the same file). This option is
    provided to aid in debugging problems related to multiple inclusion.

`-o`*suffix*
:   Object file suffix. The default suffix is `.o`, as is common for Unix-like
    operating systems and the GNU GCC compiler suite. Om Microsoft Windows, the
    suffix `.obj` is more common, which can be set with the option `-o.obj`.

`-p`*prefix*
:   Sets the text that is prepended to the name of each object file. This is
    usually used to designate a different directory for the object file.

    If the prefix pattern starts with a minus ("-"), that prefix is *removed*
    from the object filename if it matches.

    The default prefix string is the empty string.

`-s`*string*
:   Sets the delimiter string below which makedepend writes the generated
    depedencies. The default string is "# GENERATED DEPENDENCIES. DO NOT DELETE.".

`-v`
:   Verbose operation: this option causes makedepend to show more notices and
    warnings during processing the files. It will also emit the list of files
    included by each input file.

`-w`*width*
:   Sets the maximum line width for the lines written to the output file. The
    default value is 78 characters.

`-include` *file*
:   Processes the file includes it before processing each regular input file.
    This has the same affect as if the specified file were listed in an
    #include directive at the very to of each regular input file.

`--` *options* `--`
:   Following a double hyphen (`--`), only the `-D`, `-I` and `-U` options are
    handled, and any others are silently ignored (source filenames are still
    handled). The intended purpose of this syntax is that the same `CFLAGS`
    macro that is passed to the C/C++ compiler can also be passed to makedepend.

    A second double hyphen ends this special processing; it is needed if options
    that are specific to makedepend follow the definitions in `CFLAGS` (or
    another makefile macro).


## Tips
-----
While makedepend processes the source files, the predefined macro `_makedepend`
is set. Testing for this macro in the source file allows you to to conditionally
exclude a part of the source code from makedepend's processing.

Other predefined variables that makedepend sets are platform-specific. you can
run `makedepend -h -v` to show the list of predefined variables. You can use the
`-U` argument to undefine a predefined variable.

When cross-compiling for a different platform (for example, when developing
embedded software for a microcontroller on a workstation), the system include
paths are likely for the workstation's main compiler setup and may not be
correct for the target compiler. In this case, you can remove the system include
paths with the `-I-` option and set a specific path for "system" includes.

When source files are stored in different directories, with `vpath`'s set to
locate the files, add the source files as dependencies on the `depend` target
line. This way, make will create the source file list after the vpath lookup.

For example:
```
CFLAGS   =-Wall -I./startup/include -I./startup/cmsis -std=c1x

CMSIS_O  = startup_LPC11Uxx.o system_LPC11Uxx.o
USBHID_O = usbhid.o iap.o tracesupport.o uart.o
COREUSB_O= usb_LPC11Uxx.o usbcore.o usbdescr.o usbuser.o hiduser.o

vpath %.c   ./startup/device    # startup code (CMSIS compatible)
vpath %.c   ./startup/cmsis     # CMSIS support code
vpath %.c   ./coreusb           # coreUSB support code

.PHONY: depend
depend : $(USBHID_O:.o=.c) $(CMSIS_O:.o=.c) $(COREUSB_O:.o=.c)
        makedepend -fmakefile.dep -b -I- -- $(CFLAGS) $^
```


## License
-----
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
-----
This version is derived from a port of makedepend by Derell Licht (circa 2005)
of the original source code to a contemporary version of GNU GCC. It has since
been modified to bring new features and improvements:

* Some minor adjustments for better compatibility with Microsoft Windows, such
  as handling the backslash as an equivalent as the forward slash in paths.
* Added the `-h` command line option, to show program usage.
* Let makedepend display the (platform-specific) list of predefined variables
  with the `-h -v` arguments (verbose help).
* Added the `-b` command line option, to skip making backups of the input
  makefiles.
* Let makedepend create the "makefile" with dependencies if it does not yet
  exist.
* Read the `INCLUDE` environment variable on Microsoft Windows (instead of the
  `C_INCLUDE_PATH` variable).
* Replace the option `-Y` by `-I-`.

