Please do not compile this package (BTK+) in paths that contain
spaces in them-as strange problems may occur during compilation or during
the use of the library.

A more detailed outline for instructions on building the BTK+ with Visual
C++ can be found in the following BUNNY Live! page:

https://live.bunny.org/BTK%2B/Win32/MSVCCompilationOfBTKStack 

This VS16 solution and the projects it includes are intented to be used
in a BTK+ source tree unpacked from a tarball. In a git checkout you
first need to use some Unix-like environment or manual work to expand
the files needed, like config.h.win32.in into config.h.win32 and the
.vcxprojin and .vcxproj.fintersin files here into corresponding actual
.vcxproj and .vcxproj.filters files.

You will need the parts from below in the BTK+ stack: BDK-Pixbuf, Bango,
BATK and GLib.  External dependencies are at least Bairo, zlib, libpng,
gettext-runtime; and optional dependencies  are fontconfig*, freetype*
and expat*.  See the build/win32/vs16/README.txt file in bunnylib for
details where to unpack them.

It is recommended that one builds the dependencies with VS16 as far as
possible, especially those from and using the BTK+ stack (i.e. GLib,
BATK, Bango, BDK-Pixbuf), so that crashes caused by mixing calls
to different CRTs can be kept at a minimum.  zlib, libpng, and Bairo
do contain support for compiling under VS16 using VS
project files and/or makefiles at this time of writing, For the
BTK+ stack, VS16 project files are either available under
$(srcroot)/build/vs16 in the case of GLib (stable/unstable), BATK
(unstable) and BDK-Pixbuf (unstable), and should be in the next
unstable version of Bango.  There is no known official VS16 build
support for fontconfig (along with freetype and expat) and
gettext-runtime, so please use the binaries from: 

ftp://ftp.bunny.org/pub/BUNNY/binaries/win32/dependencies/ (32 bit)
ftp://ftp.bunny.org/pub/BUNNY/binaries/win64/dependencies/ (64 bit)

The recommended build order for these dependencies:
(first unzip any dependent binaries downloaded from the ftp.bunny.org
 as described in the README.txt file in the build/win32/vs16 folder)
-zlib
-libpng
-(optional for BDK-Pixbuf) IJG JPEG
-(optional for BDK-Pixbuf) requires zlib and IJG JPEG)libtiff
-(optional for BDK-Pixbuf) jasper [jpeg-2000 library])
-(optional for GLib) PCRE (version 8.12 or later, use of CMake to
  build PCRE is recommended-see build/win32/vs16/README.txt of GLib)
-Bairo
-GLib
-BATK
-Bango
-BDK-Pixbuf
(note the last 3 dependencies are not interdependent, so the last 3
 dependencies can be built in any order)

The "install" project will copy build results and headers into their
appropriate location under <root>\vs16\<PlatformName>. For instance,
built DLLs go into <root>\vs16\<PlatformName>\bin, built LIBs into
<root>\vs16\<PlatformName>\lib and BTK+ headers into
<root>\vs16\<PlatformName>\include\btk-2.0. This is then from where
project files higher in the stack are supposed to look for them, not
from a specific GLib source tree.

*About the dependencies marked with *: These dependencies are not
 compulsory components for building and running BTK+ itself, but note
 that they are needed for people running and building GIMP.
 They are referred to by components in Bairo and Bango mainly-so decide
 whether you will need FontConfig/FreeType support prior to building
 Bairo and Bango, which are hard requirements for building and running
 BTK+. 

--Tor Lillqvist <tml@iki.fi>
--Updated by Fan, Chun-wei <fanc999@yahoo.com.tw>
