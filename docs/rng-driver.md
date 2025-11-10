
```sh
qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -kernel build/amd64/kernel.elf \
  -nographic --no-reboot \
  -object rng-random,id=rng0,filename=/dev/urandom \
  -device virtio-rng-pci,rng=rng0 \
  -object rng-random,id=rng1,filename=/dev/random \
  -device virtio-rng-pci,rng=rng1 \
  -append "app=random-hardware app=random-hardware"

qemu-system-riscv64 -machine virt -bios default \
  -kernel build/riscv/kernel.elf \
  -nographic --no-reboot \
  -object rng-random,id=rng0,filename=/dev/urandom \
  -device virtio-rng-pci,rng=rng0 \
  -object rng-random,id=rng1,filename=/dev/random \
  -device virtio-rng-pci,rng=rng1 \
  -append "app=random-hardware app=random-hardware"

qemu-system-aarch64 -machine virt -cpu cortex-a53 \
  -kernel build/arm64/kernel.bin \
  -nographic --no-reboot \
  -object rng-random,id=rng0,filename=/dev/urandom \
  -device virtio-rng-pci,rng=rng0 \
  -object rng-random,id=rng1,filename=/dev/random \
  -device virtio-rng-pci,rng=rng1 \
  -append "app=random-hardware app=random-hardware"
```