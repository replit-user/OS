#include "disk.h"
#include <stdint.h>
#include "fs.h"

// --- Simple string/memory functions ---
static void *k_memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static void k_memset(void *dest, int val, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    for (size_t i = 0; i < n; i++) d[i] = (uint8_t)val;
}

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void k_strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = 0;
}

// --- Simple in-memory MFT cache ---
mft_entry_t mft[MAX_FILES];

// Keep track of free blocks in memory
uint16_t free_list[FLOPPY_TOTAL_BLOCKS];
int free_count = 0;

// --- Helper Functions ---
void build_free_list() {
    free_count = 0;
    for(int i = 0; i < FLOPPY_TOTAL_BLOCKS; i++) {
        if(!floppy_block_is_used(0, i)) {
            free_list[free_count++] = i;
        }
    }
}

uint16_t alloc_block() {
    if(free_count == 0) return UNUSED_BLOCK;
    return free_list[--free_count];
}

void free_block(uint16_t block) {
    free_list[free_count++] = block;
}

// --- MFT Persistence ---
#define MFT_START_BLOCK 1
#define MFT_BLOCKS ((sizeof(mft) + BLOCK_SIZE - 1) / BLOCK_SIZE)

static void save_mft_to_disk() {
    for(int i = 0; i < MFT_BLOCKS; i++) {
        char blk[BLOCK_SIZE];
        k_memset(blk, 0, BLOCK_SIZE);
        int copy_size = BLOCK_SIZE;
        if ((i+1)*BLOCK_SIZE > sizeof(mft))
            copy_size = sizeof(mft) - i*BLOCK_SIZE;
        k_memcpy(blk, ((uint8_t*)mft) + i*BLOCK_SIZE, copy_size);
        floppy_write_block(0, MFT_START_BLOCK + i, blk);
    }
}

static void load_mft_from_disk() {
    for(int i = 0; i < MFT_BLOCKS; i++) {
        char blk[BLOCK_SIZE];
        floppy_read_block(0, MFT_START_BLOCK + i, blk);
        int copy_size = BLOCK_SIZE;
        if ((i+1)*BLOCK_SIZE > sizeof(mft))
            copy_size = sizeof(mft) - i*BLOCK_SIZE;
        k_memcpy(((uint8_t*)mft) + i*BLOCK_SIZE, blk, copy_size);
    }
}

// --- FS Initialization ---
int init_fs() {
    floppy_open(0);
    build_free_list();

    if(free_count == 0) return -1;

    // Write superblock
    char sblk[BLOCK_SIZE];
    k_memset(sblk, 0, BLOCK_SIZE);
    k_memcpy(sblk, "FLP1", 4);                   // Magic
    uint16_t total_blocks = FLOPPY_TOTAL_BLOCKS;
    k_memcpy(sblk + 4, &total_blocks, 2);       // Total blocks
    uint16_t block_size = BLOCK_SIZE;
    k_memcpy(sblk + 6, &block_size, 2);         // Block size
    uint16_t first_free = free_list[free_count-1];
    k_memcpy(sblk + 8, &first_free, 2);         // Pointer to first free block

    floppy_write_block(0, free_list[--free_count], sblk); // Reserve first free for superblock

    // Load MFT from disk (if it exists)
    load_mft_from_disk();
    return 0;
}

// --- MFT Helpers ---
int find_empty_mft_slot() {
    for(int i = 0; i < MAX_FILES; i++) {
        if(mft[i].filename[0] == 0) return i;
    }
    return -1;
}

int find_file(const char *name) {
    for(int i = 0; i < MAX_FILES; i++) {
        if(k_strcmp(mft[i].filename, name) == 0) return i;
    }
    return -1;
}

// --- File Operations ---
int create_file(const char *name, const char *data, int size) {
    int slot = find_empty_mft_slot();
    if(slot == -1) return -1;

    mft_entry_t *entry = &mft[slot];
    k_strncpy(entry->filename, name, MAX_FILENAME);
    entry->num_sections = 1;
    entry->file_size = size;

    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if(blocks_needed > free_count) return -1;

    entry->section_start[0] = alloc_block();
    entry->section_len[0] = blocks_needed;

    for(int i = 0; i < blocks_needed; i++) {
        char blk[BLOCK_SIZE];
        k_memset(blk, 0, BLOCK_SIZE);
        int copy_size = (i == blocks_needed - 1) ? (size % BLOCK_SIZE) : BLOCK_SIZE;
        if(copy_size == 0) copy_size = BLOCK_SIZE;
        k_memcpy(blk, data + i * BLOCK_SIZE, copy_size);
        floppy_write_block(0, entry->section_start[0] + i, blk);
    }

    save_mft_to_disk(); // persist MFT
    return 0;
}

int delete_file(const char *name) {
    int idx = find_file(name);
    if(idx == -1) return -1;

    mft_entry_t *entry = &mft[idx];
    for(int i = 0; i < entry->num_sections; i++) {
        for(int b = 0; b < entry->section_len[i]; b++) {
            free_block(entry->section_start[i] + b);
        }
    }
    k_memset(entry, 0, sizeof(mft_entry_t));

    save_mft_to_disk(); // persist MFT
    return 0;
}

int append_file(const char *name, const char *data, int size) {
    int idx = find_file(name);
    if(idx == -1) return -1;
    mft_entry_t *entry = &mft[idx];

    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if(blocks_needed > free_count) return -1;
    if(entry->num_sections >= MAX_SECTIONS) return -1;

    entry->section_start[entry->num_sections] = alloc_block();
    entry->section_len[entry->num_sections] = blocks_needed;

    for(int i = 0; i < blocks_needed; i++) {
        char blk[BLOCK_SIZE];
        k_memset(blk, 0, BLOCK_SIZE);
        int copy_size = (i == blocks_needed - 1) ? (size % BLOCK_SIZE) : BLOCK_SIZE;
        if(copy_size == 0) copy_size = BLOCK_SIZE;
        k_memcpy(blk, data + i * BLOCK_SIZE, copy_size);
        floppy_write_block(0, entry->section_start[entry->num_sections] + i, blk);
    }
    entry->num_sections++;
    entry->file_size += size;

    save_mft_to_disk(); // persist MFT
    return 0;
}

int edit_file(const char *name, const char *data, int size) {
    if(delete_file(name) != 0) return -1;
    return create_file(name, data, size);
}
