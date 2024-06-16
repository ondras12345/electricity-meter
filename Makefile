## 		TARGETS
.PHONY: all
all:            ## Compile, default target for Vim
all: compile_commands.json compile

.PHONY: compile
compile:        ## Compile firmware.
compile: include/version.h
	pio run

.PHONY: clean
clean:          ## Clean project.
	pio run --target clean

.PHONY: upload
upload:         ## Upload firmware via local programmer.
	pio run --target upload

.PHONY: check
check:          ## Run static code analysis
	pio check

.PHONY: test
test:           ## Run native tests.
	pio test -e native

compile_commands.json: platformio.ini
	pio run --target compiledb

# This needs to be regenerated on every run to be able to
# detect -dirty state.
.PHONY: include/version.h
include/version.h:
	echo "#define ELECTRICITY_METER_VERSION \"$(shell git describe --dirty --always --tags)\"" > include/version.h
	echo "#define ELECTRICITY_METER_BUILD_TIME \"$(shell date --utc +"%Y%m%dT%H%M%SZ")\"" >> include/version.h

.PHONY: help
help:           ## Show this help.
	@grep -F -h "##" $(MAKEFILE_LIST) | sed -e '/unique_BhwaDzu7C/d;s/\\$$//;s/##//'
