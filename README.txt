LINE 
http://line.sourceforge.net/
$Id: README.txt,v 1.8 2001/05/29 16:16:06 mvines Exp $


LINE Is Not an Emulator.  LINE executes unmodified Linux applications on 
Windows 98/2000 by intercepting Linux system calls.  The Linux 
applications themselves are not emulated.  They run directly on the 
CPU like all other Windows applications.  


LINE runs best on Windows 2000 because that is my primary development platform.
However I do some of the LINE development on a Windows 98 system (mostly on
weekends).  Generally I find that fork()ing is much slower on Windows 98 than 
Windows 2000.   Also Windows 98 is much less tolerant about crashing than 
Windows 2000.  If LINE crashes on 98 chances are very good that your system 
will die.  I have never had Windows 2000 killed by LINE.

LINE has not been tested at all on Windows 95, Windows NT or Windows ME.  
However I have received reports that LINE runs fine on Windows NT. 


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

There are a number of small test programs in the test/ subdirectory that
will get you playing with LINE quickly. 

Example:
  Line.exe test/hello 

Example:
  Line.exe test/argtest arg1 arg2 arg3


NOTE: Before running LINE the first time, you should probably run the command:
      logconf -r logcon.dll
      
      See docs/Logging.txt for info on what this command actually does


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
