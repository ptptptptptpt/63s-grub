# Do not change this order if you don't know what you are doing.
AUTOMAKE_OPTIONS = 1.7 gnu

SUBDIRS = stage2 stage1


EXTRA_DIST = BUGS MAINTENANCE

63s-image: all
	dd if=stage1/stage1 of=$@.$$$$ bs=512 count=1 &&		\
	dd if=$(srcdir)/stage2/63s_grub_cfg of=$@.$$$$ bs=512 seek=1 count=1 &&	\
	dd if=stage2/pre_stage2 of=$@.$$$$ bs=512 seek=2 &&		\
	dd if=/dev/zero of=$@.$$$$ bs=512 seek=160650 count=0 &&		\
	cp $@.$$$$ $@;								\
	rm $@.$$$$

PHONY: test clean-local
test: 63s-image
	@if [ x$(QEMU) = xno ] ; then			\
		echo "You have disabled QEMU support or qemu was not detected; try --with-qemu=(full path of qemu) when calling configure.";	\
		false;					\
	fi
	$(QEMU) -hda 63s-image -boot c

clean-local:
	rm 63s-image
