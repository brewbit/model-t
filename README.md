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

Download and install the following while also adding them to your [PATH](http://publib.boulder.ibm.com/iseries/v5r2/ic2924/books/5445775168.htm):

#### Windows

1. [GNU Tools for ARM Embedded Processors](https://launchpad.net/gcc-arm-embedded/+download) latest version
2. Python Items
  * [Python 2.7](http://www.python.org/download/releases/2.7/)
  * [pygame 1.9.2](https://bitbucket.org/pygame/pygame/downloads/pygame-1.9.2a0.win32-py2.7.msi)
  * [pystache](https://pypi.python.org/pypi/pystache)
  * [Python-protobufs](https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/EnIwDJuDwzL8I7u/protobuf-2.5.0.win32.exe)
3. [Cygwin](http://www.cygwin.com/) Include the following packages:
  * Make (gcc)
  * Google protobufs (libprotobuf-devel)
4. [OpenOCD](https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/b9phnhj8sx2wrs8/openocd-0.7.0.7z)
  * After installing OpenOCD rename the executable in the /bin directory to "openocd"
5. [Zadig](http://zadig.akeo.ie/)

#### OS X

1. [GNU Tools for ARM Embedded Processors](https://launchpad.net/gcc-arm-embedded/4.7/4.7-2013-q3-update/+download/gcc-arm-none-eabi-4_7-2013q3-20130916-mac.tar.bz2)
2. Python Items
  * `brew install python && brew link python`
  * `pip install pygame pystache`
  * `pip install pyprotobuf`
3. Protobuf stuff
  * `brew install protobuf`

## Dependencies

* [ChibiOS/RT](https://github.com/brewbit/ChibiOS-RT/branches/stable_2.4.x)
* [snacka](https://github.com/brewbit/snacka)
* [nanopb](http://koti.kapsi.fi/~jpa/nanopb/download/nanopb-0.2.4.tar.gz)
* [protobufs](https://github.com/brewbit/brewbit-protobuf-messages)

* Download and extract all dependencies along side the Model-T sources. Your source tree should look like:

```
<MODEL_T_BUILD_DIR>
|-- model-t
|-- ChibiOS-RT
|-- snacka
|-- nanopb
|-- brewbit-protobuf-messages
```

* Alternatively, you can edit `deps.mk` and point to your dependencies.

## JTAG

Using [Olimex ARM-USB-TINY-H programmer](https://www.olimex.com/Products/ARM/JTAG/ARM-USB-TINY-H/)

Connect the device, when Windows asks for drivers to be installed,
cancel out of the dialog and do not install any recommended drivers.

Run `Zadig`, select Olimex from the drop down, there should be 2, and
select `WinUSB` for the driver and select install for each Olimex in the
dropdown.

# Build/Load Model-T Firmware via Eclipse

1. Open Make Target dialog
2. Select model-t drop down
3. Targets:
  * app_mt              - builds just the application
  * bootloader          - builds just the bootloader
  * clean               - cleans all built files
  * download            - builds both the application and bootloader and downloads to the model-t via JTAG
  * download_app_mt     - builds only the application and downloads to the model-t via JTAG
  * download_bootloader - builds only the bootloader and downloads to the model-t via JTAG (only need to do once)

# Build/Load Model-T Firmware via command line

* Run `make` to build the software and pass it one of the targets defined above:

# Debugging

* At the top of the directory
* Run the following in terminal

```
openocd -f interface/olimex-arm-usb-tiny-h.cfg      \
        -f target/stm32f2x.cfg                      \
        -f stm32f2x-setup.cfg                       \
        -c init                                     \
        -c "reset init"                             \
        -c halt                                     \
        -c "stm32f2x.cpu configure -rtos auto"
```

* In another terminal run:

```
arm-none-eabi-gdb.exe build/app_mt/app_mt.elf               \
                      --eval-command "target remote:3333"   \
                      --eval-command "monitor poll"         \
                      --eval-command "monitor reset halt"
```

* You can now debug in the gdb window

 Some useful commands

* `monitor reset halt` resets the device and stops it so you can setup a new session
* `info threads` shows a list of the current chibios threads, their states and current program counters
* `thread <#>` where `<#>` is the thread id from the 'info threads' list - switches to that thread context
* `thread apply <#> bt` shows a backtrace for a given thread
* printing `dbg_panic_msg` should help narrow down which error happened
* if a hard fault exception is generated, you can inspect the registers that are dumped to stack variables in the handler
  * `PC` (program counter) and `LR` (link register) will be of most interest
  * Information on those registers is in the .map file

