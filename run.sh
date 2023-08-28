#!/bin/bash

set -euo pipefail

[ -d gnu-efi ] || git clone https://git.code.sf.net/p/gnu-efi/code gnu-efi

CFLAGS='-target x86_64-unknown-windows 
	-ffreestanding 
	-fshort-wchar 
	-mno-red-zone
	-nostdlib
	-I./gnu-efi/inc
	-Wl,-entry:efi_main 
	-Wl,-subsystem:efi_application
	-fuse-ld=lld-link'
clang $CFLAGS -o bootx64.efi main.c

dd if=/dev/zero of=fat.img bs=1k count=1440
mformat -i fat.img -f 1440 ::
mmd -i fat.img ::/efi
mmd -i fat.img ::/efi/boot
mcopy -i fat.img bootx64.efi ::/efi/boot

qemu-system-x86_64 -cpu host -smp 1 -enable-kvm -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.fd -drive format=raw,file=fat.img
