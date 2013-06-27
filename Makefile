
all: bootloader app

make_prog = $(MAKE) -f src/$(1)/$(1).mk

app:
	$(call make_prog,app)
	
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
	
download: all
	$(call prog_download,app)
	$(call prog_download,bootloader)