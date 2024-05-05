obj-m += testFS.o
testFS-objs := fs.o testFS_alloc.o inode.o file.o

KDIR ?= /lib/modules/$(shell uname -r)/build

MKFS = mkfs.testFS

all: $(MKFS)
	make -C $(KDIR) M=$(shell pwd) modules

IMAGE ?= test.img
IMAGESIZE ?= 200
# To test max files(40920) in directory, the image size should be at least 159.85 MiB
# 40920 * 4096(block size) ~= 159.85 MiB

$(MKFS): mkfs.c
	$(CC) -std=gnu99 -Wall -o $@ $<

$(IMAGE): $(MKFS)
	dd if=/dev/zero of=${IMAGE} bs=1M count=${IMAGESIZE}
	./$< $(IMAGE)

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm -f *~ $(PWD)/*.ur-safe
	rm -f $(MKFS) $(IMAGE)

.PHONY: all clean