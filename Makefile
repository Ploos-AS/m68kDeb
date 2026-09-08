.PHONY: check-current build-current qualify-current

check-current:
	sh scripts/check-current-ports.sh

build-current: check-current
	sh rootfs/build-current.sh

qualify-current: build-current
	@mkdir -p out/qualification
	sha256sum out/rootfs-current-m68k.tar.xz | tee out/qualification/SHA256SUMS
	stat -c '%n %s bytes' out/rootfs-current-m68k.tar.xz | tee out/qualification/SIZE.txt
	git rev-parse HEAD > out/qualification/GIT_HEAD.txt
