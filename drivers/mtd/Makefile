#
# Makefile for the memory technology device drivers.
#

# Core functionality.
mtd-y				:= mtdcore.o mtdsuper.o
mtd-$(CONFIG_MTD_PARTITIONS)	+= mtdpart.o
obj-$(CONFIG_MTD)		+= $(mtd-y)

obj-$(CONFIG_MTD_CONCAT)	+= mtdconcat.o
obj-$(CONFIG_MTD_REDBOOT_PARTS) += redboot.o
obj-$(CONFIG_MTD_CMDLINE_PARTS) += cmdlinepart.o
obj-$(CONFIG_MTD_AFS_PARTS)	+= afs.o

# 'Users' - code which presents functionality to userspace.
obj-$(CONFIG_MTD_CHAR)		+= mtdchar.o
obj-$(CONFIG_MTD_BLKDEVS)	+= mtd_blkdevs.o
obj-$(CONFIG_MTD_BLOCK)		+= mtdblock.o
# WTF is that supposed to include!?
# This is GPLed code, they ought to release that as SRC
#obj-$(CONFIG_MTD_BLOCK)		+= mtdblock-jz.o
obj-$(CONFIG_MTD_BLOCK_RO)	+= mtdblock_ro.o
obj-$(CONFIG_FTL)		+= ftl.o
obj-$(CONFIG_NFTL)		+= nftl.o
obj-$(CONFIG_INFTL)		+= inftl.o
obj-$(CONFIG_RFD_FTL)		+= rfd_ftl.o
obj-$(CONFIG_SSFDC)		+= ssfdc.o

nftl-objs		:= nftlcore.o nftlmount.o
inftl-objs		:= inftlcore.o inftlmount.o

obj-y		+= chips/ maps/ devices/ nand/ onenand/

obj-$(CONFIG_MTD_UBI)		+= ubi/

$(obj)/mtdblock-jz.o: $(obj)/mtdblock-jz.uu
	uudecode $(obj)/mtdblock-jz.uu -o $(obj)/mtdblock-jz.o