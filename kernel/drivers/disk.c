// floppy disk controller
#include "disk.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// Open floppy device
int floppy_open(Floppy *floppy) {
    floppy->fd = open(FLOPPY_DEVICE, O_RDWR);
    if (floppy->fd < 0) {
        perror("Failed to open floppy device");
        return -1;
    }
    return 0;
}

// Close floppy
void floppy_close(Floppy *floppy) {
    if (floppy->fd >= 0) {
        close(floppy->fd);
    }
}

// Seek to block
static int floppy_seek(Floppy *floppy, uint32_t block) {
    if (block >= FLOPPY_TOTAL_BLOCKS) {
        fprintf(stderr, "Invalid block number\n");
        return -1;
    }

    off_t offset = block * FLOPPY_BLOCK_SIZE;

    if (lseek(floppy->fd, offset, SEEK_SET) < 0) {
        perror("Seek failed");
        return -1;
    }

    return 0;
}

// Read block
int floppy_read_block(Floppy *floppy, uint32_t block, uint8_t *buffer) {
    if (floppy_seek(floppy, block) < 0)
        return -1;

    ssize_t bytes = read(floppy->fd, buffer, FLOPPY_BLOCK_SIZE);
    if (bytes != FLOPPY_BLOCK_SIZE) {
        perror("Read failed");
        return -1;
    }

    return 0;
}

// Write block
int floppy_write_block(Floppy *floppy, uint32_t block, const uint8_t *buffer) {
    if (floppy_seek(floppy, block) < 0)
        return -1;

    ssize_t bytes = write(floppy->fd, buffer, FLOPPY_BLOCK_SIZE);
    if (bytes != FLOPPY_BLOCK_SIZE) {
        perror("Write failed");
        return -1;
    }

    return 0;
}

// Check if block is used (any byte != 0)
int floppy_block_is_used(Floppy *floppy, uint32_t block) {
    uint8_t buffer[FLOPPY_BLOCK_SIZE];

    if (floppy_read_block(floppy, block, buffer) < 0)
        return -1;

    for (size_t i = 0; i < FLOPPY_BLOCK_SIZE; i++) {
        if (buffer[i] != 0)
            return 1; // used
    }

    return 0; // free
}
