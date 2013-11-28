# Installation

## Windows

### Eclipse

1. Download & install Java http://java.com/en/download/manual.jsp
  * Select Windows Offline
2. Download Eclipse from http://www.eclipse.org
3. Extract and copy it to Program Files/Eclipse
4. Create a shortcut on your desktop to Ecliplse
5. Install CDT http://www.eclipse.org/cdt/

### Software Tools

1. Install CodeSourcery Lite ARM toolchain
2. Python Items
  * Python: https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/rh48m51erv19vt4/python-2.7.2.msi
  * PIL: https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/5lk8jj1zrb35bi1/PIL-1.1.7.win32-py2.7.exe
  * PyYAML: https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/4kuzb5nktab3gsf/PyYAML-3.10.win32-py2.7.exe
3. Cygwin: http://www.cygwin.com/ Include the following packages:
  * Make
4. OpenOCD https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/b9phnhj8sx2wrs8/openocd-0.7.0.7z
5. BMFont https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/6pq696denystrl7/install_bmfont_1.13.exe
6. Zadig http://zadig.akeo.ie/

## JTAG

Using (Olimex ARM-USB-TINY-H programmer)[https://www.olimex.com/Products/ARM/JTAG/ARM-USB-TINY-H/]

Connect the device, when Windows asks for drivers to be installed,
cancel out of the dialog and do not install any recommended drivers.

Run `Zadig`, select Olimex from the drop down, there should be 2, and
select `WinUSB` for the driver and select install for each Olimex in the
dropdown.

# Building

* Check out ChibiOS from the svn repo: (http://svn.code.sf.net/p/chibios/svn/tags/ver_2.4.3)[http://svn.code.sf.net/p/chibios/svn/tags/ver_2.4.3]
  * Place it at the same level as this codebase in ChibiOS
  * Alternatively, you can create a `chibios.mk` the following format:
```
CHIBIOS ?= <path to your ChibiOS dir>

export CHIBIOS
```
  * Run `make` to build the software
  * To load the binaries on the board, connect JTAG to the board and run
    `make download`

