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
  * [Python](https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/rh48m51erv19vt4/python-2.7.2.msi)
  * [PIL](https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/5lk8jj1zrb35bi1/PIL-1.1.7.win32-py2.7.exe)
  * [yYAML](https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/4kuzb5nktab3gsf/PyYAML-3.10.win32-py2.7.exe)
3. [Cygwin](http://www.cygwin.com/) Include the following packages:
  * Make
4. [OpenOCD](https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/b9phnhj8sx2wrs8/openocd-0.7.0.7z)
5. [BMFont](https://s3.amazonaws.com/uploads.hipchat.com/49452/333815/6pq696denystrl7/install_bmfont_1.13.exe)
6. [Zadig](http://zadig.akeo.ie/)

## Dependencies

* [ChibiOS/RT](https://github.com/brewbit/ChibiOS-RT/tree/stable_2.4.x)
* [snacka](https://github.com/brewbit/snacka)
* [nanopb](https://code.google.com/p/nanopb) 0.2.4

## JTAG

Using [Olimex ARM-USB-TINY-H programmer](https://www.olimex.com/Products/ARM/JTAG/ARM-USB-TINY-H/)

Connect the device, when Windows asks for drivers to be installed,
cancel out of the dialog and do not install any recommended drivers.

Run `Zadig`, select Olimex from the drop down, there should be 2, and
select `WinUSB` for the driver and select install for each Olimex in the
dropdown.

# Building

* Download and extract all dependencies along side the Model-T sources. Your source tree should look like:

```
<MODEL_T_BUILD_DIR>
|-- model-t
|-- ChibiOS-RT
|-- snacka
|-- nanopb
```

* Alternatively, you can edit `deps.mk` point to your dependencies.

## Build snacka

```
cd snacka
make CC=arm-none-eabi-gcc AR=arm-none-eabi-ar WITH_BACKEND=NO lib
```

## Build Model-T Firmware

* Run `make` to build the software
* To load the binaries on the board, connect JTAG to the board and run `make download`

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

## Some useful commands

* `monitor reset halt` resets the device and stops it so you can setup a new session
* `info threads` shows a list of the current chibios threads, their states and current program counters
* `thread <#>` where `<#>` is the thread id from the 'info threads' list - switches to that thread context
* `thread apply <#> bt` shows a backtrace for a given thread
* printing `dbg_panic_msg` should help narrow down which error happened
* if a hard fault exception is generated, you can inspect the registers that are dumped to stack variables in the handler
  * `PC` (program counter) and `LR` (link register) will be of most interest
  * Information on those registers is in the .map file

