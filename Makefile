.PHONY: check-current build-current

check-current:
	sh scripts/check-current-ports.sh

build-current: check-current
	sh rootfs/build-current.sh
