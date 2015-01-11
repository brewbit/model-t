.PHONY: update_deps

include deps.mk

ifeq ($(JTAG),jlink)
	INTERFACE_SCRIPT=jlink
	TARGET_SCRIPT=stm32f2x
endif

ifeq ($(JTAG),stlink)
	INTERFACE_SCRIPT=stlink-v2
	TARGET_SCRIPT=stm32f2x_stlink
endif

ifeq ($(JTAG),olimex)
	INTERFACE_SCRIPT=ftdi/olimex-arm-usb-tiny-h
	TARGET_SCRIPT=stm32f2x
endif

all: bootloader app_mt

make_prog = $(MAKE) -f src/$(1)/$(1).mk
openocd_script = nc localhost 4444 < scripts/openocd/$(1).cfg > /dev/null

app_mt:
	$(call make_prog,app_mt) autogen
	$(call make_prog,app_mt)

bootloader:
	$(call make_prog,bootloader)

clear_app_hdr:
	@$(call openocd_script,clear_app_hdr)
	@echo App config section has been erased

upgrade_image: app_mt
	arm-none-eabi-objcopy -O binary --only-section header build/app_mt/app_mt.elf build/app_mt/app_mt_hdr.bin
	arm-none-eabi-objcopy -O binary --remove-section cfg --remove-section header build/app_mt/app_mt.elf build/app_mt/app_mt_app.bin
	python scripts/build_app_image.py build/app_mt/app_mt_hdr.bin build/app_mt/app_mt_app.bin build/app_mt/app_mt_update.bin

download_app_mt: upgrade_image attach
	@$(call openocd_script,download_app_mt)
	@echo Download complete

download_bootloader: bootloader attach
	@$(call openocd_script,download_bootloader)
	@echo Download complete

download: download_app_mt download_bootloader

debug_app_mt: app_mt attach
	@arm-none-eabi-gdb build/app_mt/app_mt.elf -ex "source scripts/gdb/startup.gdb"

debug_bootloader: bootloader attach
	@arm-none-eabi-gdb build/bootloader/bootloader.elf -ex "source scripts/gdb/startup.gdb"

attach:
	$(if $(JTAG),,$(error JTAG variable is not set. Supported options: jlink, stlink, olimex))
	@if pgrep openocd > /dev/null ;\
	then \
	  echo "Already attached" ;\
	else \
	  screen -S brewbit openocd -f interface/$(INTERFACE_SCRIPT).cfg -f target/$(TARGET_SCRIPT).cfg -f scripts/openocd/startup.cfg ;\
	  if pgrep openocd > /dev/null ;\
	  then \
	    echo "Attached" ;\
	  else \
	    echo "Attach failed!" ;\
	  fi \
	fi
	@pgrep openocd > /dev/null

detach:
	@if pgrep openocd > /dev/null ;\
	then \
	  $(call openocd_script,shutdown) ;\
	  echo "Detached" ;\
	else \
	  echo "Already detached" ;\
	fi

build/app_mt/app_mt.dfu: upgrade_image
	python scripts/dfu.py \
		-b 0x08008000:build/app_mt/app_mt_hdr.bin \
		-b 0x08008200:build/app_mt/app_mt_app.bin \
		build/app_mt/app_mt.dfu

build/bootloader/bootloader.dfu: bootloader
	python scripts/dfu.py \
		-b 0x08000000:build/bootloader/bootloader.bin \
		build/bootloader/bootloader.dfu

download_dfu_app_mt: build/app_mt/app_mt.dfu
	dfu-util -a 0 -t 2048 -D build/app_mt/app_mt.dfu

download_dfu_bootloader: build/bootloader/bootloader.dfu
	dfu-util -a 0 -t 2048 -D build/bootloader/bootloader.dfu

build/all.dfu: upgrade_image bootloader
	python scripts/dfu.py \
		-b 0x08000000:build/bootloader/bootloader.bin \
		-b 0x08008000:build/app_mt/app_mt_hdr.bin \
		-b 0x08008200:build/app_mt/app_mt_app.bin \
		build/all.dfu

download_dfu: build/all.dfu
	dfu-util -a 0 -t 2048 -D build/all.dfu
	
autoload_dfu: build/all.dfu
	python scripts/autoload.py

clean:
	rm -rf .dep build

