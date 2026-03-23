#ifndef FS_H
#define FS_H

#include <stdint.h>

#define BLOCK_SIZE 512
#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_SECTIONS 5
#define UNUSED_BLOCK 0xFFFF

// --- MFT Entry Structure ---
typedef struct {
    char filename[MAX_FILENAME];
    uint8_t num_sections;
    uint16_t section_start[MAX_SECTIONS];
    uint16_t section_len[MAX_SECTIONS];
    uint32_t file_size;
} mft_entry_t;

// --- Filesystem Functions ---
int init_fs(void);
int create_file(const char *name, const char *data, int size);
int delete_file(const char *name);
int append_file(const char *name, const char *data, int size);
int edit_file(const char *name, const char *data, int size);

// --- MFT Helpers ---
int find_empty_mft_slot(void);
int find_file(const char *name);

// --- Block Management ---
uint16_t alloc_block(void);
void free_block(uint16_t block);
void build_free_list(void);

#endif // FS_H
