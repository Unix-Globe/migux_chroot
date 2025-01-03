# MIG/UX Chroot Environment

MIG/UX is a flexible and customizable chroot-based Unix-like distribution environment builder. It allows you to create minimal to full-featured chroot environments with a focus on security, modularity, and ease of configuration.

## Features

### Core System
- Modular init system with multiple init types (simple, systemd-like, custom)
- Flexible filesystem mounting (proc, sys, dev, tmpfs)
- Network stack with IPv6 and DHCP support
- Security features including PAM, SELinux, and POSIX capabilities
- Device management with udev support
- Package management system

### Configuration Systems
- **menuconfig**: ncurses-based system configuration
  - System size presets (minimal, small, medium, large)
  - Network and device configuration
  - Security settings
  - Package selection

- **oemconfig**: Distribution customization
  - Media type (ISO, tarball)
  - Bootloader configuration
  - Partition layouts
  - Custom branding
  - Security settings

### Installation Media
- ISO image creation with bootloader
- Compressed tarballs (gz, bz2, xz, zstd)
- Optional secure boot support
- Package signing
- Documentation and development tools

## Building

### Prerequisites
```bash
# Required packages
gcc/g++ (C++17 support)
ncurses
libmenu
make
```

### Basic Build
```bash
# Configure the system
make menuconfig

# Build the basic system
make
```

### Creating Installation Media
```bash
# Configure OEM settings
make oemconfig

# Build installation media
make mkimage
```

### Available Make Targets
- `make menuconfig`: Configure system settings
- `make oemconfig`: Configure OEM/distribution settings
- `make`: Build the base system
- `make bootmaker`: Build only the bootmaker utility
- `make core-modules`: Build core system modules
- `make packages`: Build selected packages
- `make mkimage`: Create installation media
- `make clean`: Clean build artifacts

## System Configuration

### Size Presets
- **minimal**: Basic system with ash and busybox
- **small**: Minimal system with basic coreutils
- **medium**: Standard system with common utilities
- **large**: Full system with development tools

### Core Components
- **Init System**: Process management and system initialization
- **Mount System**: Filesystem mounting and management
- **Network Stack**: Interface and protocol management
- **Security Framework**: Authentication and access control
- **Device Manager**: Hardware detection and configuration

### Package Selection
Packages are selected based on system size and can be customized through menuconfig:
- **minimal**: ash, busybox
- **small**: ash, busybox, coreutils-minimal
- **medium**: bash, coreutils, net-tools
- **large**: bash, coreutils, net-tools, development tools

## OEM Configuration

### Media Options
- ISO image creation
- Compressed tarballs
- Bootloader selection (GRUB2, systemd-boot)
- Partition layout templates

### Customization
- Distribution branding
- Default configurations
- Documentation inclusion
- Development tools
- Custom boot logo

### Security
- Secure boot support
- Package signing
- Default security policies
- Root password configuration

## Installation

### Using the Installer
1. Build the installer:
   ```bash
   make installer
   ```

2. Run the installer as root:
   ```bash
   sudo ./bin/installer
   ```

3. Follow the installation wizard to:
   - Specify target directory
   - Configure virtual filesystems
   - Enable/disable networking
   - Choose development tools

4. Enter the chroot environment:
   ```bash
   sudo chroot /path/to/chroot /bin/sh
   ```

The installer provides:
- Directory structure creation
- Virtual filesystem mounting (proc, sys, dev, pts)
- Network configuration
- Base system installation
- Optional development tools

### Manual Installation
1. Create the chroot directory:
   ```bash
   sudo mkdir -p /path/to/chroot
   ```

2. Extract the base system:
   ```bash
   sudo tar xf /usr/share/mig_ux/base.tar.xz -C /path/to/chroot
   ```

3. Mount virtual filesystems:
   ```bash
   sudo mount -t proc proc /path/to/chroot/proc
   sudo mount -t sysfs sys /path/to/chroot/sys
   sudo mount -o bind /dev /path/to/chroot/dev
   sudo mount -o bind /dev/pts /path/to/chroot/dev/pts
   ```

4. Enter the chroot:
   ```bash
   sudo chroot /path/to/chroot /bin/sh
   ```

## Usage

### Creating a Chroot Environment
```bash
# Configure the system
make menuconfig

# Build the base system
make

# Create and enter chroot
sudo ./bin/bootmaker /path/to/rootfs
```

### Creating Distribution Tarball
```bash
# Configure OEM settings
make oemconfig

# Build tarball
make mkimage

# Output will be in images/ directory
```

## Development

### Directory Structure
```
mig_ux_chroot/
├── C/
│   ├── src/           # Core source files
│   ├── system/        # System components
│   │   ├── core/      # Core modules
│   │   └── packages/  # System packages
│   └── tools/         # Configuration tools
├── config/            # Configuration files
├── bin/              # Built binaries
└── images/           # Generated images
```

### Adding New Packages
1. Create package source in `C/system/`
2. Add package to appropriate size preset in Makefile
3. Update menuconfig options if needed

### Contributing
1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Authors
- Miguel Romero

## Acknowledgments
- Linux From Scratch project
- Buildroot project
- BusyBox project
