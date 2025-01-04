CC = gcc
CPP = g++
AS = nasm
CFLAGS = -Wall -Wextra -std=c99
CPPFLAGS = -Wall -Wextra -std=c++17 -I$(SRC_DIR)
LDFLAGS = -pthread -lreadline -lhistory
MENUCONFIG_LDFLAGS = -lncurses -lmenu

SRC_DIR = C
OBJ_DIR = obj
BIN_DIR = bin
CONFIG_DIR = config
SYSTEM_DIR = $(SRC_DIR)/system
IMAGE_DIR = images

# Include configurations if they exist
-include $(CONFIG_DIR)/migux.conf
-include $(CONFIG_DIR)/oem.conf

# Default OEM configurations if not set
DISTRO_NAME ?= MIG/UX
DISTRO_VERSION ?= 1.0
DISTRO_CODENAME ?= initial
MEDIA_TYPE ?= iso
ISO_LABEL ?= MIGUX_INSTALL
COMPRESSION ?= xz
BOOTLOADER ?= grub2
PARTITION_LAYOUT ?= standard
DEFAULT_FS ?= ext4
INCLUDE_DOCS ?= 1
INCLUDE_DEV ?= 0
CUSTOM_LOGO ?=
DEFAULT_ROOT_PASS ?= migux
ENABLE_SECURE_BOOT ?= 0
SIGN_PACKAGES ?= 1

# Export OEM configuration for use in C++ code
CPPFLAGS += -DDISTRO_NAME=\"$(DISTRO_NAME)\" \
            -DDISTRO_VERSION=\"$(DISTRO_VERSION)\" \
            -DDISTRO_CODENAME=\"$(DISTRO_CODENAME)\" \
            -DMEDIA_TYPE=\"$(MEDIA_TYPE)\" \
            -DISO_LABEL=\"$(ISO_LABEL)\" \
            -DCOMPRESSION=\"$(COMPRESSION)\" \
            -DBOOTLOADER=\"$(BOOTLOADER)\" \
            -DPARTITION_LAYOUT=\"$(PARTITION_LAYOUT)\" \
            -DDEFAULT_FS=\"$(DEFAULT_FS)\" \
            -DINCLUDE_DOCS=$(INCLUDE_DOCS) \
            -DINCLUDE_DEV=$(INCLUDE_DEV) \
            -DCUSTOM_LOGO=\"$(CUSTOM_LOGO)\" \
            -DDEFAULT_ROOT_PASS=\"$(DEFAULT_ROOT_PASS)\" \
            -DENABLE_SECURE_BOOT=$(ENABLE_SECURE_BOOT) \
            -DSIGN_PACKAGES=$(SIGN_PACKAGES)

# Default system configurations if not set
SYSTEM_SIZE ?= medium
MAX_USERS ?= 10
ENABLE_NETWORK ?= 1
ENABLE_DEVICES ?= full
INIT_SYSTEM ?= simple
ENABLE_MODULES ?= 1
ROOT_FS_TYPE ?= ext4
ENABLE_TMPFS ?= 1
MOUNT_PROC ?= 1
MOUNT_SYS ?= 1
MOUNT_DEV ?= 1
ENABLE_IPV6 ?= 1
ENABLE_DHCP ?= 1
ENABLE_DNS ?= 1
ENABLE_PAM ?= 1
ENABLE_SELINUX ?= 0
ENABLE_CAPABILITIES ?= 1
ENABLE_PACKAGE_MANAGER ?= 0
ENABLE_UDEV ?= 1
ENABLE_INPUT ?= 1
ENABLE_SOUND ?= 1
ENABLE_DEVELOPMENT ?= 0
ENABLE_GDB ?= 0
ENABLE_STRACE ?= 0

# Size-specific package configurations
ifeq ($(SYSTEM_SIZE),minimal)
    SYSTEM_PACKAGES ?= ash,busybox
else ifeq ($(SYSTEM_SIZE),small)
    SYSTEM_PACKAGES ?= ash,busybox,coreutils-minimal
else ifeq ($(SYSTEM_SIZE),medium)
    SYSTEM_PACKAGES ?= bash,coreutils,net-tools
else ifeq ($(SYSTEM_SIZE),large)
    SYSTEM_PACKAGES ?= bash,coreutils,net-tools,development
endif

# Export system configuration for use in C++ code
CPPFLAGS += -DSYSTEM_PACKAGES=\"$(SYSTEM_PACKAGES)\" \
            -DMAX_USERS=$(MAX_USERS) \
            -DENABLE_NETWORK=$(ENABLE_NETWORK) \
            -DENABLE_DEVICES=\"$(ENABLE_DEVICES)\" \
            -DINIT_SYSTEM=\"$(INIT_SYSTEM)\" \
            -DENABLE_MODULES=$(ENABLE_MODULES) \
            -DROOT_FS_TYPE=\"$(ROOT_FS_TYPE)\" \
            -DENABLE_TMPFS=$(ENABLE_TMPFS) \
            -DMOUNT_PROC=$(MOUNT_PROC) \
            -DMOUNT_SYS=$(MOUNT_SYS) \
            -DMOUNT_DEV=$(MOUNT_DEV) \
            -DENABLE_IPV6=$(ENABLE_IPV6) \
            -DENABLE_DHCP=$(ENABLE_DHCP) \
            -DENABLE_DNS=$(ENABLE_DNS) \
            -DENABLE_PAM=$(ENABLE_PAM) \
            -DENABLE_SELINUX=$(ENABLE_SELINUX) \
            -DENABLE_CAPABILITIES=$(ENABLE_CAPABILITIES) \
            -DENABLE_PACKAGE_MANAGER=$(ENABLE_PACKAGE_MANAGER) \
            -DENABLE_UDEV=$(ENABLE_UDEV) \
            -DENABLE_INPUT=$(ENABLE_INPUT) \
            -DENABLE_SOUND=$(ENABLE_SOUND) \
            -DENABLE_DEVELOPMENT=$(ENABLE_DEVELOPMENT) \
            -DENABLE_GDB=$(ENABLE_GDB) \
            -DENABLE_STRACE=$(ENABLE_STRACE)

# Core system modules - only include modules that have source files
CORE_MODULES = init mount network

# Unix programs
UNIX_PROGRAMS = ash greenbox root autoboot

# Optional modules will be added when their source files are created
# For now, we only build modules that have source files in C/system/core/

# Source files
CORE_SRCS = $(addprefix $(SYSTEM_DIR)/core/, $(addsuffix .cpp, $(CORE_MODULES)))
UNIX_SRCS = $(addprefix $(SYSTEM_DIR)/, $(addsuffix .cpp, $(UNIX_PROGRAMS)))
ALL_SRCS = $(CORE_SRCS) $(UNIX_SRCS)

# Main targets
.PHONY: all clean menuconfig oemconfig bootmaker packages core-modules mkimage installer unix-programs

all: bootmaker core-modules unix-programs packages

core-modules: $(addprefix $(BIN_DIR)/, $(CORE_MODULES))
	@echo "Core modules built: $(CORE_MODULES)"

unix-programs: $(addprefix $(BIN_DIR)/, $(UNIX_PROGRAMS))
	@echo "Unix programs built: $(UNIX_PROGRAMS)"

$(BIN_DIR)/%: $(SYSTEM_DIR)/core/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CPP) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

$(BIN_DIR)/%: $(SYSTEM_DIR)/%.cpp
	@mkdir -p $(BIN_DIR)
	$(CPP) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

$(BIN_DIR)/bootmaker: $(SRC_DIR)/src/bootmaker.cpp $(SRC_DIR)/system/root.cpp
	@mkdir -p $(BIN_DIR)
	$(CPP) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

$(BIN_DIR)/menuconfig: $(SRC_DIR)/tools/menuconfig.cpp
	@mkdir -p $(BIN_DIR)
	$(CPP) $(CPPFLAGS) -o $@ $< $(MENUCONFIG_LDFLAGS)

$(BIN_DIR)/oemconfig: $(SRC_DIR)/tools/oemconfig.cpp
	@mkdir -p $(BIN_DIR)
	$(CPP) $(CPPFLAGS) -o $@ $< $(MENUCONFIG_LDFLAGS)

$(BIN_DIR)/mkimage: $(SRC_DIR)/tools/mkimage.cpp
	@mkdir -p $(BIN_DIR)
	$(CPP) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

$(BIN_DIR)/installer: $(SRC_DIR)/tools/installer.cpp
	@mkdir -p $(BIN_DIR)
	$(CPP) $(CPPFLAGS) -o $@ $< $(MENUCONFIG_LDFLAGS)

bootmaker: $(BIN_DIR)/bootmaker
	@echo "Bootmaker compiled successfully"
	@echo "Run 'sudo ./bin/bootmaker <rootfs-path>' to create and enter chroot environment"

menuconfig: $(BIN_DIR)/menuconfig
	@mkdir -p $(CONFIG_DIR)
	@$(BIN_DIR)/menuconfig

oemconfig: $(BIN_DIR)/oemconfig
	@mkdir -p $(CONFIG_DIR)
	@$(BIN_DIR)/oemconfig

mkimage: $(BIN_DIR)/mkimage packages
	@mkdir -p $(IMAGE_DIR)
	@$(BIN_DIR)/mkimage $(CONFIG_DIR)/oem.conf

installer: $(BIN_DIR)/installer

packages: $(BIN_DIR)
	@echo "Building packages..."
	@for pkg in $(subst $(comma), ,$(SYSTEM_PACKAGES)); do \
		if [ -f "$(SYSTEM_DIR)/$$pkg.cpp" ]; then \
			echo "Building $$pkg..."; \
			$(CPP) $(CPPFLAGS) -o $(BIN_DIR)/$$pkg $(SYSTEM_DIR)/$$pkg.cpp $(LDFLAGS); \
		fi \
	done

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(IMAGE_DIR)

# Dependencies
$(BIN_DIR)/bootmaker: $(SRC_DIR)/system/root.cpp