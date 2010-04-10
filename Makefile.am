# Do not change this order if you don't know what you are doing.
AUTOMAKE_OPTIONS = 1.7 gnu

SUBDIRS = stage2 stage1 test

EXTRA_DIST = BUGS MAINTENANCE tools/menuc.c tools/test.menu

PHONY: test clean-local imageupdate

stage1/stage1 stage2/stage2: all

IMAGE = 63s-image

$(IMAGE): $(top_srcdir)/test/$(IMAGE).bz2
	@echo "Unpacking 63s-image..."
	@bzcat $(top_srcdir)/test/$@.bz2 > $@

imageupdate: stage1/stage1 stage2/stage2 $(IMAGE)
	@echo "Updating 63s-image..."
	@dd conv=notrunc if=stage1/stage1 of=$(IMAGE) bs=1 count=440 &&		\
	dd conv=notrunc if=stage2/stage2 of=$(IMAGE) bs=512 seek=1 count=62

test: imageupdate
	@echo "Running QEMU..."
	@if [ x$(QEMU) = xno ] ; then			\
		echo "You have disabled QEMU support or qemu was not detected; try --with-qemu=(full path of qemu) when calling configure.";	\
		false;					\
	fi
	@$(QEMU) -hda 63s-image -boot c

clean-local:
	@rm 63s-image
