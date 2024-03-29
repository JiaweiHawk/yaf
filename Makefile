PWD                                     := $(shell pwd)
NPROC                                   := $(shell nproc)
PORT                                    := 1234
MEM                                     := 4G
SMP                                     := 2

QEMU                                    := qemu-system-x86_64
QEMU_OPTIONS                            := -smp ${SMP}
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -m ${MEM}
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -kernel ${PWD}/kernel/arch/x86_64/boot/bzImage
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -append "rdinit=/sbin/init panic=-1 console=ttyS0 nokaslr"
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -initrd ${PWD}/rootfs.cpio
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -fsdev local,id=shares,path=${PWD}/shares,security_model=none
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -device virtio-9p-pci,fsdev=shares,mount_tag=shares
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -drive file=${PWD}/test.img,index=0,if=virtio,media=disk,format=raw
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -enable-kvm
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -nographic
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -no-reboot

.PHONY: debug driver env img kernel rootfs run srcs test tool

srcs: driver tool
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf sources'

tool:
	bear --append --output ${PWD}/compile_commands.json -- \
		gcc -g -Wall -Werror -I${PWD}/kernel/build/include -o ${PWD}/tool/mkfs ${PWD}/tool/mkfs.c ${PWD}/tool/arguments.c
	cp ${PWD}/tool/mkfs ${PWD}/shares
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf tool'

driver:
	bear --append --output ${PWD}/compile_commands.json -- \
		make -C ${PWD}/driver KDIR=${PWD}/kernel -j ${NPROC}
	cp ${PWD}/driver/yaf.ko ${PWD}/shares
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf kernel module'

kernel:
	if [ ! -d ${PWD}/kernel ]; then \
		sudo apt update; \
		sudo apt install -y bc bear bison debootstrap dwarves flex libelf-dev libssl-dev; \
		wget -O ${PWD}/linux.tar.xz https://mirrors.ustc.edu.cn/kernel.org/linux/kernel/v6.x/linux-6.7.tar.xz; \
		tar -Jxvf ${PWD}/linux.tar.xz; \
		mv ${PWD}/linux-6.7 ${PWD}/kernel; \
		rm -rf ${PWD}/linux.tar.xz; \
		(cd ${PWD}/kernel && make defconfig); \
		(cd ${PWD}/kernel && ./scripts/config -e CONFIG_DEBUG_INFO_DWARF5 && yes "" | make oldconfig); \
		(cd ${PWD}/kernel && ./scripts/config -e CONFIG_GDB_SCRIPTS && yes "" | make oldconfig); \
		(cd ${PWD}/kernel && ./scripts/config -e CONFIG_X86_X2APIC && yes "" | make oldconfig); \
		make -C ${PWD}/kernel headers_install INSTALL_HDR_PATH=${PWD}/kernel/build -j ${NPROC}; \
	fi
	bear --append --output ${PWD}/compile_commands.json -- \
		make -C ${PWD}/kernel -j ${NPROC}
	@echo -e '\033[0;32m[*]\033[0mbuild the linux kernel'

rootfs:
	if [ ! -d ${PWD}/rootfs ]; then \
		sudo apt update; \
		sudo apt-get install -y debootstrap; \
		sudo debootstrap \
			--components=main,contrib,non-free,non-free-firmware \
			stable ${PWD}/rootfs https://mirrors.ustc.edu.cn/debian/; \
		sudo chroot ${PWD}/rootfs /bin/bash -c "apt update && apt install -y gdb strace"; \
		mkdir shares; \
		\
		#configure shared directory \
		echo "shares /mnt/shares 9p trans=virtio 0 0" | sudo tee -a ${PWD}/rootfs/etc/fstab; \
		\
		#configure debug hint \
		echo "\033[0m\033[1;37mWhen debugging the driver, execute \033[31mlx-symbols in gdb\033[37m before running insmod to set breakpoints using the driver's function name\033[0m" | sudo tee -a ${PWD}/rootfs/etc/issue; \
		echo "" | sudo tee -a ${PWD}/rootfs/etc/issue; \
		\
		#configure welcome message \
		echo "" | sudo tee -a ${PWD}/rootfs/etc/motd; \
		echo "\033[0m\033[1;37mThe shared folder for host side is at \033[31m${PWD}/shares\033[0m" | sudo tee -a ${PWD}/rootfs/etc/motd; \
		echo "\033[0m\033[1;37mThe shared folder for guest side is at \033[31m/mnt/shares\033[0m" | sudo tee -a ${PWD}/rootfs/etc/motd; \
		echo "" | sudo tee -a ${PWD}/rootfs/etc/motd; \
		\
		#configure password \
		sudo chroot ${PWD}/rootfs /bin/bash -c "passwd -d root"; \
	fi
	cd ${PWD}/rootfs; sudo find . | sudo cpio -o --format=newc -F ${PWD}/rootfs.cpio >/dev/null
	@echo -e '\033[0;32m[*]\033[0mbuild the rootfs'

img:
	if [ ! -f ${PWD}/test.img ]; then \
		dd if=/dev/zero of=${PWD}/test.img bs=1M count=1024 status=progress; \
	fi
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf test image'

env: kernel rootfs srcs img
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf environment'

run:
	${QEMU} \
		${QEMU_OPTIONS} \
		-no-shutdown

debug:
	gnome-terminal -- gdb ${PWD}/kernel/vmlinux --init-eval-command="set confirm on" --init-eval-command="add-auto-load-safe-path ${PWD}/kernel/scripts/gdb/vmlinux-gdb.py" --eval-command="target remote localhost:${PORT}"
	${QEMU} \
		${QEMU_OPTIONS} \
		-no-shutdown \
		-S -gdb tcp::${PORT}

test:
	${PWD}/test.py --command='''${QEMU} ${QEMU_OPTIONS}''' --history=${PWD}/shares/setup.sh
