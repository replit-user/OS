# By Ember2819, google, and random people on the internet.... What are we doing????
# C compiler
CC = clang
# Secondary C compiler
CC2 = gcc
# Assembler (for boot.s)
AS = nasm
# Linker
LD = ld
# Truncate (to make the kernel.bin divisible by 512)
TRUNCATE = truncate
TRUNC_AMNT = 360448
# Objcopy (to translate elf to bin)
OBJCOPY = objcopy
OBJCOPY_ARGS = -O binary
CC_FLAGS = -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -c
AS_FLAGS = -f bin
LD_FLAGS = -m elf_i386 -T linker.ld
KERNEL_OBJECTS = kernel/kernel.o kernel/ports.o kernel/mem.o
DRIVER_OBJECTS = kernel/drivers/vga.o kernel/drivers/keyboard.o
MISC_OBJECTS = kernel/colors.o kernel/terminal/terminal.o kernel/commands.o kernel/layouts/kb_layouts.o kernel/fs.o #last edited by replit-user
# Builds the final disk image
all: os.img

# If no clang detected, use gcc
%.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# Assemble the bootloader
bootloader/boot.bin: bootloader/boot.s
	$(AS) $(AS_FLAGS) $< -o $@
# Added by MorganPG1
kernel/colors.o: kernel/colors.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/layouts/kb_layouts.o: kernel/layouts/kb_layouts.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# Compile drivers
kernel/drivers/vga.o: kernel/drivers/vga.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/drivers/keyboard.o:  kernel/drivers/keyboard.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/ports.o: kernel/ports.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/mem.o: kernel/mem.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel/terminal/terminal.o: kernel/terminal/terminal.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# added by replit-user
kernel/drivers/disk.o: kernel/drivers/disk.c
	$(CC) $(CC_FLAGS) -c $< -o $@ || $(CC2) $(CC_FLAGS) -c $< -o $@
# Added by Ember2819
kernel/commands.o: kernel/commands.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
# End added by Ember2819
# Link all kernel objects
#added by replit-user
kernel.elf: $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) kernel/drivers/disk.o
	$(LD) $(LD_FLAGS) $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) kernel/drivers/disk.o -o kernel.elf
kernel/fs.o: kernel/drivers/fs.c
	$(CC) $(CC_FLAGS) $< -o $@ || $(CC2) $(CC_FLAGS) $< -o $@
kernel.bin: kernel.elf
	$(OBJCOPY) $(OBJCOPY_ARGS) kernel.elf kernel.bin
	$(TRUNCATE) -s $(TRUNC_AMNT) kernel.bin
os.img: bootloader/boot.bin kernel.bin
	cat bootloader/boot.bin kernel.bin > os.img
# Launch the image in QEMU
run: os.img
	qemu-system-i386 -drive format=raw,file=os.img
clean:
	rm -f $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(MISC_OBJECTS) kernel.elf kernel.bin bootloader/boot.bin
.PHONY: all run clean
