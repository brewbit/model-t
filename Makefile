
all: bootloader app_mt

make_prog = $(MAKE) -f src/$(1)/$(1).mk

app_mt:
	$(call make_prog,app_mt)
	
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

download_app_mt: app_mt
	arm-none-eabi-objcopy -O binary --only-section header build/app_mt/app_mt.elf build/app_mt/app_mt-hdr.bin
	arm-none-eabi-objcopy -O binary --remove-section cfg --remove-section header build/app_mt/app_mt.elf build/app_mt/app_mt-app.bin
	python scripts/complete_app_hdr.py build/app_mt/app_mt-hdr.bin build/app_mt/app_mt-app.bin
	@openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "flash erase_sector 0 2 last" \
	-c "flash write_bank 0 build/app_mt/app_mt-app.bin 0x8200" \
	-c "flash write_bank 0 build/app_mt/app_mt-hdr.bin 0x8000" \
	-c "reset init" \
	-c "reset run" \
	-c shutdown download.log 2>&1 && \
	echo Download complete

download_bootloader: bootloader
	$(call prog_download,bootloader)

download: download_app_mt download_bootloader

clean:
	rm -rf .dep build
