# Do not change this order if you don't know what you are doing.
AUTOMAKE_OPTIONS = 1.7 gnu

SUBDIRS = stage2 stage1


EXTRA_DIST = BUGS MAINTENANCE

noinst_DATA = 63s-image

63s-image: 
	dd if=stage1/stage1 of=$@.$$$$ bs=512 count=1 &&		\
	dd if=$(srcdir)/stage2/63s_grub_cfg of=$@.$$$$ bs=512 seek=1 count=1 &&	\
	dd if=stage2/pre_stage2 of=$@.$$$$ bs=512 seek=2 &&		\
	dd if=/dev/zero of=$@.$$$$ bs=512 seek=160650 count=0 &&		\
	cp $@.$$$$ $@;								\
	rm $@.$$$$

PHONY: test
test: all 
	$(QEMU) -hda 63s-image -boot c
