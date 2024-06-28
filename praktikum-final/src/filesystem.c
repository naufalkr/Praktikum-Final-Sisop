#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int i = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
  for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int node_found = 0;
    struct node_item* node = NULL;
    int i = 0;
    int j = 0;

    // Read node and data sectors into memory buffers
    readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Find the node in the node_fs_buf using while loop
    while (!node_found && i < FS_MAX_NODE) {
        if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name, MAX_FILENAME) == 0 &&
            node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
            node = &(node_fs_buf.nodes[i]);
            node_found = 1;
        }
        i++;
    }

    // If node not found, return error
    if (!node_found) {
        *status = FS_R_NODE_NOT_FOUND;
        return;
    }

    // Check if the found node is a directory
    if (node->data_index == FS_NODE_D_DIR) {
        *status = FS_R_TYPE_IS_DIRECTORY;
        return;
    }

    // Initialize filesize to 0
    metadata->filesize = 0;
    j = 0;

    // Read file data sectors into metadata->buffer using while loop
    while (j < FS_MAX_SECTOR && data_fs_buf.datas[node->data_index].sectors[j] != 0x00) {
        readSector(metadata->buffer + j * SECTOR_SIZE, data_fs_buf.datas[node->data_index].sectors[j]);
        metadata->filesize += SECTOR_SIZE;
        j++;
    }

    // Set status to success
    *status = FS_SUCCESS;
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int empty_node_index = -1;
    int empty_data_index = -1;
    int empty_blocks = 0;
    int i = 0;
    int j = 0;

    // Read filesystem from disk into memory buffers
    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Check for existing node with the same node_name and parent_index
    while (i < FS_MAX_NODE) {
        if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name, MAX_FILENAME) == 0 &&
            node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
            *status = FS_W_NODE_ALREADY_EXISTS;
            return;
        }
        i++;
    }

    // Find an empty node slot
    i = 0;
    while (i < FS_MAX_NODE) {
        if (strcmp(node_fs_buf.nodes[i].node_name, "", MAX_FILENAME) == 0) {
            empty_node_index = i;
            break;
        }
        i++;
    }

    // If no empty node found, return error
    if (empty_node_index == -1) {
        *status = FS_W_NO_FREE_NODE;
        return;
    }

    // Find an empty data sector
    i = 0;
    while (i < FS_MAX_DATA) {
        if (data_fs_buf.datas[i].sectors[0] == 0x00) {
            empty_data_index = i;
            break;
        }
        i++;
    }

    // If no empty data sector found, return error
    if (empty_data_index == -1) {
        *status = FS_W_NO_FREE_DATA;
        return;
    }

    // Count empty blocks in map
    i = 0;
    while (i < SECTOR_SIZE) {
        if (!map_fs_buf.is_used[i]) {
            empty_blocks++;
        }
        i++;
    }

    // Check if enough space is available
    if (empty_blocks < metadata->filesize / SECTOR_SIZE) {
        *status = FS_W_NOT_ENOUGH_SPACE;
        return;
    }

    // Populate node information in node_fs_buf
    strcpy(node_fs_buf.nodes[empty_node_index].node_name, metadata->node_name, MAX_FILENAME);
    node_fs_buf.nodes[empty_node_index].parent_index = metadata->parent_index;
    node_fs_buf.nodes[empty_node_index].data_index = empty_data_index;

    // Write data to data sectors
    j = 0;
    i = 0;
    while (j < SECTOR_SIZE && i < metadata->filesize) {
        if (!map_fs_buf.is_used[i]) {
            data_fs_buf.datas[empty_data_index].sectors[j] = i;
            writeSector(metadata->buffer + j * SECTOR_SIZE, i);
            j++;
        }
        i++;
    }

    // Write modified filesystem back to disk
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Set success status
    *status = FS_SUCCESS;
}

// char* _strchr(const char* str, int c) {
//   while (*str != '\0') {
//     if (*str == (char)c) {
//       return (char*)str;
//     }
//     str++;
//   }
//   if (c == '\0') {
//     return (char*)str;
//   }
//   return NULL;
// }

// // Implement the _strrchr function
// char* _strrchr(const char* str, int c) {
//   const char* last = NULL;
//   while (*str != '\0') {
//     if (*str == (char)c) {
//       last = str;
//     }
//     str++;
//   }
//   if (c == '\0') {
//     return (char*)str;
//   }
//   return (char*)last;
// }
