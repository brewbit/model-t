
all: bootloader controller

make_prog = $(MAKE) -f src/$(1)/$(1).mk

controller:
	$(call make_prog,controller)
	
wifi:
	$(call make_prog,wifi)

bootloader:
	$(call make_prog,bootloader)

prog_download = @openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "flash write_image erase build/$(1)/$(1).elf" \
	-c "reset init" \
	-c "reset run" \
	-c shutdown download.log 2>&1 && \
	echo Download complete

download_controller: controller
	$(call prog_download,controller)

download_bootloader: bootloader
	$(call prog_download,bootloader)

download: download_controller download_bootloader

clean:
	rm -rf .dep build
