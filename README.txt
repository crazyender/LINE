LINE 
https://github.com/crazyender/LINE
cloned from another project: http://line.sourceforge.net/
$Id: README.txt,v 1.8 2001/05/29 16:16:06 mvines Exp $
$Id: README.txt,v 1.9 2012/02/16 11:24:06 crazyender $

LINE Is Not an Emulator.  LINE executes unmodified Linux applications on 
Windows 7 by intercepting Linux system calls.  The Linux 
applications themselves are not emulated.  They run directly on the 
CPU like all other Windows applications.  


LINE runs best on Windows 7 because that is my primary development platform.
Currently LINE can not run glibc that with NPTL feature.

LINE will use modified putty as it's console, but this can not be done perfectly
before I merge file io system calls into kernel space

License
-------
LINE is released under the GNU General Public License.  See the file 
COPYING.txt for details.

LINE contains code from the Linux 2.2.x kernel.


Usage
-----
The main executable is Line.exe.  The first command line parameter is the 
Linux application to run, any other command line parameters will be passed 
along to the app.  
You can build a workspace by copy binarys from linux system, for example, 
copy /bin /lib /usr/bin from linux into c:\line, and put line.exe, linexec.exe,
cygwin1.dll into the same folder. Open a cmd, enter into c:\line, type 
following command:
c:\line\line.exe bash
Then you can see bash from linux is running, and simple command(ls, rm, etc...)
works too.


Compiling LINE
--------------
You will need the full Cygwin environment <http://www.cygwin.com> to
build LINE yourself.  

From the src/ subdirectory, type 'make all'.  Assuming no errors occur, 
your shiny new Line.exe executable will be located in the root LINE directory.
      

Documentation
-------------
There is preliminary documentation regarding various aspects of LINE in the 
docs/ subdirectory.
