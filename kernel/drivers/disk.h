#include <stdint.h>

#define FLOPPY_TOTAL_BLOCKS 2880   // 1.44 MB, 512 bytes per block
#define FLOPPY_BLOCK_SIZE 512

typedef struct {
    // any registers, port info, or state for your floppy controller
    uint8_t last_status;
} Floppy;

int floppy_open(Floppy *floppy);
int floppy_read_block(Floppy *floppy, uint32_t block, uint8_t *buffer);
int floppy_write_block(Floppy *floppy, uint32_t block, const uint8_t *buffer);
int floppy_block_is_used(Floppy *floppy, uint32_t block);
