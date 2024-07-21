.PHONY: all
all: $(patsubst %.hex,%.bin,$(wildcard *.hex))

%.bin: %.hex
	objcopy --gap-fill 0xFF --pad-to 0x040000 -I ihex $< -O binary $@
