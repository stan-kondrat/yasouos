# Linux Development Setup

## Requirements

To build YasouOS you only need:
- GCC cross-compilers (riscv64, aarch64, x86_64)
- QEMU for testing
- tcpdump, netcat (for e2e network tests)

## Ubuntu / Debian

```bash
apt install gcc-riscv64-unknown-elf
apt install gcc-aarch64-linux-gnu
apt install gcc-multilib  # for x86_64
apt install qemu-system
apt install tcpdump netcat-openbsd
```

## Build from Source

For distributions without pre-packaged cross-compilers, build GCC and QEMU from source.

### Build Dependencies

```bash
# Debian/Ubuntu
apt install build-essential ninja-build pkg-config flex bison \
  libgmp-dev libmpfr-dev libmpc-dev libglib2.0-dev libpixman-1-dev \
  libslirp-dev python3-venv python3-pip meson git wget curl texinfo tcpdump netcat-openbsd

# Void Linux
xbps-install -S base-devel ninja pkg-config flex bison \
  gmp-devel mpfr-devel libmpc-devel glib-devel pixman-devel \
  libslirp-devel python3 meson git wget curl texinfo tcpdump openbsd-netcat
```

### Build QEMU 10

```bash
wget -q https://download.qemu.org/qemu-10.2.0.tar.xz
tar xf qemu-10.2.0.tar.xz
cd qemu-10.2.0

./configure --prefix=$HOME/.local \
  --target-list=riscv64-softmmu,aarch64-softmmu,x86_64-softmmu \
  --enable-virtfs \
  --disable-docs \
  --disable-gtk \
  --disable-sdl \
  --disable-vnc

make -j$(nproc)
make install
```

## Build GCC 15 Cross-Compilers

### Build Binutils

```bash
wget -q https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.xz
tar xf binutils-2.45.tar.xz

for TARGET in riscv64-linux-gnu aarch64-linux-gnu x86_64-linux-gnu; do
  echo "Building binutils for $TARGET"
  mkdir -p build-binutils-$TARGET
  cd build-binutils-$TARGET

  ../binutils-2.45/configure --prefix=$HOME/.local \
    --target=$TARGET \
    --disable-nls \
    --disable-werror \
    --with-sysroot

  make -j$(nproc)
  make install
  cd ..
  rm -rf build-binutils-$TARGET
done
```

### Build GCC

```bash
wget -q https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.xz
tar xf gcc-15.2.0.tar.xz

for TARGET in riscv64-linux-gnu aarch64-linux-gnu x86_64-linux-gnu; do
  echo "Building GCC 15.2 for $TARGET"
  mkdir -p build-$TARGET
  cd build-$TARGET

  ../gcc-15.2.0/configure --prefix=$HOME/.local \
    --target=$TARGET \
    --enable-languages=c \
    --disable-multilib \
    --disable-bootstrap \
    --disable-threads \
    --disable-libssp \
    --disable-libquadmath \
    --disable-libgomp \
    --disable-libvtv \
    --disable-libstdcxx \
    --disable-nls \
    --disable-shared \
    --disable-libcc1 \
    --disable-libatomic \
    --disable-libsanitizer \
    --disable-libquadmath-support \
    --disable-gold \
    --disable-lto \
    --disable-plugin \
    --with-system-zlib

  make -j$(nproc) all-gcc
  make install-gcc
  cd ..
  rm -rf build-$TARGET
done
```

## Verify Installation

Ensure `~/.local/bin` is in your PATH, then verify:

```bash
# QEMU
qemu-system-riscv64 --version
qemu-system-aarch64 --version
qemu-system-x86_64 --version

# Binutils
riscv64-linux-gnu-ld --version | head -1
aarch64-linux-gnu-ld --version | head -1
x86_64-linux-gnu-ld --version | head -1

# GCC
riscv64-linux-gnu-gcc --version | head -1
aarch64-linux-gnu-gcc --version | head -1
x86_64-linux-gnu-gcc --version | head -1
```

## Build YasouOS

```bash
make check-deps    # Verify all dependencies
make build         # Build all architectures
make test          # Run tests
```
