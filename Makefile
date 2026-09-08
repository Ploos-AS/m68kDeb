.PHONY: check-current build-current qualify-current build-amiga-kernel build-amiga-initramfs fetch-amiboot build-amiga-boot

# Current Debian Ports/m68k host-side qualification targets.
check-current:
	sh scripts/check-current-ports.sh

build-current: check-current
	sh rootfs/build-current.sh

qualify-current: build-current
	@mkdir -p out/qualification
	sha256sum out/rootfs-current-m68k.tar.xz | tee out/qualification/SHA256SUMS
	stat -c '%n %s bytes' out/rootfs-current-m68k.tar.xz | tee out/qualification/SIZE.txt
	git rev-parse HEAD > out/qualification/GIT_HEAD.txt

# M1.3 Amiga boot artifact targets.
build-amiga-kernel:
	sh kernel/build-amiga.sh

build-amiga-initramfs:
	sh initramfs/build-amiga.sh

fetch-amiboot:
	sh platforms/amiga/fetch-amiboot.sh

build-amiga-boot: build-amiga-kernel build-amiga-initramfs fetch-amiboot
	@mkdir -p out/m68kdeb-amiga
	cp out/amiboot/amiboot out/m68kdeb-amiga/amiboot
	cp out/kernel-amiga/vmlinux-m68k-amiga out/m68kdeb-amiga/
	cp out/kernel-amiga/vmlinuz-m68k-amiga out/m68kdeb-amiga/
	cp out/initramfs-amiga/initramfs-m68kdeb.gz out/m68kdeb-amiga/
	cp platforms/amiga/bootstrap/Start-m68kDeb out/m68kdeb-amiga/
	cp platforms/amiga/bootstrap/Start-m68kDeb-Serial out/m68kdeb-amiga/
	chmod +x out/m68kdeb-amiga/Start-m68kDeb out/m68kdeb-amiga/Start-m68kDeb-Serial
	@{ \
		echo 'Local M1.3 build provenance'; \
		echo -n 'kernel_version='; cat out/kernel-amiga/VERSION; \
		echo -n 'kernel_source='; cat out/kernel-amiga/SOURCE_URL; \
		echo -n 'busybox_source='; cat out/initramfs-amiga/BUSYBOX_SOURCE_URL; \
		echo -n 'amiboot_source='; cat out/amiboot/SOURCE_URL; \
		echo -n 'amiboot_sha256='; awk '{print $$1}' out/amiboot/SHA256SUMS; \
	} > out/m68kdeb-amiga/PROVENANCE.txt
	(cd out/m68kdeb-amiga && sha256sum amiboot vmlinux-m68k-amiga vmlinuz-m68k-amiga initramfs-m68kdeb.gz Start-m68kDeb Start-m68kDeb-Serial PROVENANCE.txt > SHA256SUMS)
	sh scripts/check-amiga-bootstrap.sh out/m68kdeb-amiga
