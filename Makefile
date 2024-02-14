PWD                                     := $(shell pwd)
NPROC                                   := $(shell nproc)

MEM                                     := 2G
SMP                                     := 2
QEMU                                    := qemu-system-x86_64
QEMU_OPTIONS                            := -smp ${SMP}
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -m ${MEM}
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -kernel ${PWD}/kernel/arch/x86_64/boot/bzImage
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -append "rdinit=/sbin/init console=ttyS0 nokaslr"
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -initrd ${PWD}/rootfs.cpio
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -fsdev local,id=shares,path=${PWD}/shares,security_model=passthrough
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -device virtio-9p-pci,fsdev=shares,mount_tag=shares
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -enable-kvm
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -nographic
QEMU_OPTIONS                            := ${QEMU_OPTIONS} -no-shutdown -no-reboot

.PHONY: driver env kernel rootfs run tool

tool:
	bear --append --output ${PWD}/compile_commands.json -- \
		gcc -g -o ${PWD}/tool/mkfs ${PWD}/tool/mkfs.c
	cp ${PWD}/tool/mkfs ${PWD}/shares
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf tool'

driver:
	bear --append --output ${PWD}/compile_commands.json -- \
		make -C ${PWD}/driver KDIR=${PWD}/kernel -j ${NPROC}
	cp ${PWD}/driver/yaf.ko ${PWD}/shares
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf kernel module'

kernel:
	if [ ! -d ${PWD}/kernel ]; then \
		sudo apt-get install -y bc bear bison debootstrap dwarves flex libelf-dev libssl-dev; \
		wget -O ${PWD}/linux.tar.xz https://mirrors.ustc.edu.cn/kernel.org/linux/kernel/v6.x/linux-6.7.tar.xz; \
		tar -Jxvf ${PWD}/linux.tar.xz; \
		mv ${PWD}/linux-6.7 ${PWD}/kernel; \
		rm -rf ${PWD}/linux.tar.xz; \
		(cd ${PWD}/kernel && make defconfig); \
		(cd ${PWD}/kernel && ./scripts/config -e CONFIG_DEBUG_INFO_DWARF5 && yes "" | make oldconfig); \
		(cd ${PWD}/kernel && ./scripts/config -e CONFIG_GDB_SCRIPTS && yes "" | make oldconfig); \
		(cd ${PWD}/kernel && ./scripts/config -e CONFIG_X86_X2APIC && yes "" | make oldconfig); \
	fi
	bear --append --output ${PWD}/compile_commands.json -- \
		make -C ${PWD}/kernel -j ${NPROC}
	@echo -e '\033[0;32m[*]\033[0mbuild the linux kernel'

rootfs:
	if [ ! -d ${PWD}/rootfs ]; then \
		sudo apt-get install -y debootstrap > /dev/null; \
		sudo debootstrap \
			--components=main,contrib,non-free,non-free-firmware \
			stable ${PWD}/rootfs https://mirrors.ustc.edu.cn/debian/; \
		sudo chroot ${PWD}/rootfs /bin/bash -c "apt install -y gdb strace"; \
		mkdir shares; \
		sudo chroot ${PWD}/rootfs /bin/bash -c "echo \"shares /mnt/shares 9p trans=virtio 0 0\" >> /etc/fstab"; \
		sudo chroot ${PWD}/rootfs /bin/bash -c "echo \"echo -e \\\"The shared folder for host side is at \033[0;31m${PWD}/shares\033[0m\\\"\" >> ~/.bashrc"; \
		sudo chroot ${PWD}/rootfs /bin/bash -c "echo \"echo -e \\\"The shared folder for guest side is at \033[0;31m/mnt/shares\033[0m\\\"\" >> ~/.bashrc"; \
		sudo chroot ${PWD}/rootfs /bin/bash -c "passwd -d root"; \
	fi
	cd ${PWD}/rootfs; sudo find . | sudo cpio -o --format=newc -F ${PWD}/rootfs.cpio >/dev/null
	@echo -e '\033[0;32m[*]\033[0mbuild the rootfs'

env: kernel rootfs
	@echo -e '\033[0;32m[*]\033[0mbuild the yaf environment'

run:
	${QEMU} \
		${QEMU_OPTIONS}
