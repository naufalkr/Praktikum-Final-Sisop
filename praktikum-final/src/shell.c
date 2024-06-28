#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"


void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  struct node_fs node_fs_buf;
  struct node_item* node;

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);


    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else if (strcmp(cmd, "pwd")) {
      printCWD(cwd);
      printString("\n");
    }
    else printString("Invalid command\n");
  }
}



// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
    struct node_fs node_fs_buf;
    char path_buffer[256];  // Buffer to store the full path
    char temp[MAX_FILENAME];  // Temporary buffer for each directory name
    int length = 0;  // Current length of the path
    int i, j;
    int temp_length;

    // Read the node sectors into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Traverse from the current node to the root
    while (cwd != FS_NODE_P_ROOT) {
        // Copy the current node name to the temporary buffer
        for (i = 0; i < MAX_FILENAME && node_fs_buf.nodes[cwd].node_name[i] != '\0'; i++) {
            temp[i] = node_fs_buf.nodes[cwd].node_name[i];
        }
        temp[i] = '\0';  // Null-terminate the string

        // Calculate the total length if adding this directory name
        temp_length = i + 1; // Directory name length + '/'
        length += temp_length;

        // Prevent buffer overflow
        if (length >= 256) {
            printString("Path length exceeds buffer size.\n");
            return;
        }

        // Move existing path to make space for new directory
        for (j = length - 1; j >= temp_length; j--) {
            path_buffer[j] = path_buffer[j - temp_length];
        }

        // Add the new directory name and '/'
        path_buffer[0] = '/';
        for (i = 0; i < temp_length - 1; i++) {
            path_buffer[i + 1] = temp[i];
        }

        // Move to the parent directory
        cwd = node_fs_buf.nodes[cwd].parent_index;
    }

    // Add root '/' if path is empty
    if (length == 0) {
        path_buffer[length++] = '/';
    }
    path_buffer[length] = '\0';  // Null-terminate the string

    // Print the constructed path
    printString(path_buffer);
    printString("\n");
}


// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0, j = 0, k = 0;

    // Initialize cmd and arg to empty strings
    clear(cmd, 64);
    clear(arg[0], 64);
    clear(arg[1], 64);

    // Skip leading whitespace
    while (buf[i] == ' ') {
        i++;
    }

    // Parse command
    while (buf[i] != ' ' && buf[i] != '\0') {
        cmd[j++] = buf[i++];
    }
    cmd[j] = '\0';

    // Skip whitespace between command and first argument
    while (buf[i] == ' ') {
        i++;
    }

    // Parse first argument
    while (buf[i] != ' ' && buf[i] != '\0') {
        arg[0][k++] = buf[i++];
    }
    arg[0][k] = '\0';

    // Skip whitespace between first and second argument
    while (buf[i] == ' ') {
        i++;
    }

    // Reset k for second argument parsing
    k = 0;

    // Parse second argument
    while (buf[i] != ' ' && buf[i] != '\0') {
        arg[1][k++] = buf[i++];
    }
    arg[1][k] = '\0';
}


// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
    struct node_fs node_fs_buf;
    bool found = false;
    int i;

    // Read the node sectors into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Handle special case for root directory
    if (strcmp(dirname, "/")) {
        *cwd = FS_NODE_P_ROOT;
        return;
    }

    // Handle special case for parent directory
    if (strcmp(dirname, "..")) {
        // If already at root, stay at root
        if (*cwd != FS_NODE_P_ROOT) {
            *cwd = node_fs_buf.nodes[*cwd].parent_index;
        }
        return;
    }

    // Search for the directory in the current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == *cwd && 
            node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR && 
            strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
            *cwd = i; // Update the current working directory
            found = true;
            break;
        }
    }

    if (!found) {
        printString("Directory not found\n");
    }
}


// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    struct node_item* node;
    byte dir_index = cwd;
    int i;
    bool found = false;

    // Read the node sectors into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Check if dirname is empty, ".", or NULL
    if (dirname == '\0' || strcmp(dirname, "") || strcmp(dirname, ".")) {
        // List the contents of the current working directory
        for (i = 0; i < FS_MAX_NODE; i++) {
            node = &(node_fs_buf.nodes[i]);
            if (node->parent_index == cwd) {
                if (node->data_index == FS_NODE_D_DIR) {
                    printString("[dir] ");
                    printString(node->node_name);
                    printString("\n");
                } else {
                    printString("[file] ");
                    printString(node->node_name);
                    printString("\n");
                }
            }
        }
    } else {
        // Search for the directory named dirname in the current working directory
        for (i = 0; i < FS_MAX_NODE; i++) {
            node = &(node_fs_buf.nodes[i]);
            if (node->parent_index == cwd && strcmp(node->node_name, dirname)) {
                if (node->data_index == FS_NODE_D_DIR) {
                    dir_index = i;
                    found = true;
                    break;
                } else {
                    printString("Error: ");
                    printString(dirname);
                    printString(" is not a directory\n");
                    return;
                }
            }
        }

        if (found) {
            // List the contents of the found directory
            for (i = 0; i < FS_MAX_NODE; i++) {
                node = &(node_fs_buf.nodes[i]);
                if (node->parent_index == dir_index) {
                    if (node->data_index == FS_NODE_D_DIR) {
                        printString("[DIR] ");
                    } else {
                        printString("[FILE] ");
                    }
                    printString(node->node_name);
                    printString("\n");
                }
            }
        } else {
            printString("Directory not found\n");
        }
    }
}

char* strchr(const char* str, int c) {
  while (*str != '\0') {
    if (*str == (char)c) {
      return (char*)str;
    }
    str++;
  }
  if (c == '\0') {
    return (char*)str;
  }
  return NULL;
}

// Implement the _strrchr function
char* strrchr(const char* str, int c) {
  const char* last = NULL;
  while (*str != '\0') {
    if (*str == (char)c) {
      last = str;
    }
    str++;
  }
  if (c == '\0') {
    return (char*)str;
  }
  return (char*)last;
}

// void printInt(int num) {
//     char buffer[20]; // Buffer untuk menyimpan angka yang akan dicetak
//     sprintf(buffer, "%d", num); // Mengkonversi integer menjadi string
//     printf("%s", buffer); // Mencetak string ke output (misalnya konsol)
// }


char* strncpy(char* destination, const char* source, size_t num) {
    char* dest = destination;

    // Copy at most 'num' characters from 'source' to 'destination'
    while (num > 0 && *source != '\0') {
        *dest++ = *source++;
        num--;
    }

    // If there's remaining space in 'destination', fill with '\0'
    while (num > 0) {
        *dest++ = '\0';
        num--;
    }

    return destination;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    size_t i = 0;
    while (i < n && s1[i] != '\0' && s2[i] != '\0' && s1[i] == s2[i]) {
        i++;
    }
    if (i == n) {
        return 0; // Reached end of both strings without difference
    } else {
        return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
}


// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    int i, source_index = -1;
    byte dst_dir_index = cwd;
    int dst_len = strlen(dst);
    int last_slash_index = -1;
    char dst_dir[MAX_FILENAME];
    char new_filename[MAX_FILENAME];
    bool found = false;
        

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Find the source file index under current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd &&
            node_fs_buf.nodes[i].data_index != FS_NODE_D_DIR &&
            strcmp(node_fs_buf.nodes[i].node_name, src)) {
            source_index = i;
            break;
        }
    }

    // Check if the source file was found
    if (source_index == -1) {
        printString("Source file not found\n");
        return;
    }

    // Separate target directory and new filename
    for (i = dst_len - 1; i >= 0; i--) {
        if (dst[i] == '/') {
            last_slash_index = i;
            break;
        }
    }

    if (last_slash_index != -1) {
        // Copy target directory and new filename
        strncpy(dst_dir, dst, last_slash_index);
        dst_dir[last_slash_index] = '\0'; // Terminate dst_dir at the last '/'
        strcpy(new_filename, &dst[last_slash_index + 1]);
    } else {
        // No '/' found, treat dst as new filename and dst_dir as current directory
        strcpy(dst_dir, ".");
        strcpy(new_filename, dst);
    }

    // Determine destination directory index
    if (dst_dir[0] == '/' || dst_dir[0] == '\0') {
        dst_dir_index = FS_NODE_P_ROOT;
    } else if (strcmp(dst_dir, "..")) {
        // Move to parent directory
        dst_dir_index = node_fs_buf.nodes[cwd].parent_index;
    } else {
        // Find directory under current working directory
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd &&
                node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR &&
                strcmp(node_fs_buf.nodes[i].node_name, dst_dir)) {
                dst_dir_index = i;
                found = true;
                break;
            }
        }
        if (!found) {
            printString("Destination directory not found\n");
            return;
        }
    }

    // Update filesystem node for the source file
    node_fs_buf.nodes[source_index].parent_index = dst_dir_index;
    strcpy(node_fs_buf.nodes[source_index].node_name, new_filename);

    // Write back updated filesystem nodes to disk
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Debug message: Print new filename and directory
    printString("mv: ");
    printString(src);
    printString(" -> ");
    printString(dst);
    printString("\n");

    // Success message
    printString("File moved successfully\n");
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    struct node_fs node_fs_buf_temp;
    int i, source_index = -1;
    byte dst_dir_index = cwd;
    int dst_len = strlen(dst);
    int last_slash_index = -1;
    char dst_dir[MAX_FILENAME];
    char new_filename[MAX_FILENAME];
    bool found = false;
        

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Find the source file index under current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd &&
            node_fs_buf.nodes[i].data_index != FS_NODE_D_DIR &&
            strcmp(node_fs_buf.nodes[i].node_name, src)) {
            source_index = i;
            break;
        }
    }

    // Check if the source file was found
    if (source_index == -1) {
        printString("Source file not found\n");
        return;
    }

    // Separate target directory and new filename
    for (i = dst_len - 1; i >= 0; i--) {
        if (dst[i] == '/') {
            last_slash_index = i;
            break;
        }
    }

    if (last_slash_index != -1) {
        // Copy target directory and new filename
        strncpy(dst_dir, dst, last_slash_index);
        dst_dir[last_slash_index] = '\0'; // Terminate dst_dir at the last '/'
        strcpy(new_filename, &dst[last_slash_index + 1]);
    } else {
        // No '/' found, treat dst as new filename and dst_dir as current directory
        strcpy(dst_dir, ".");
        strcpy(new_filename, dst);
    }

    // Determine destination directory index
    if (dst_dir[0] == '/' || dst_dir[0] == '\0') {
        dst_dir_index = FS_NODE_P_ROOT;
    } else if (strcmp(dst_dir, "..")) {
        // Move to parent directory
        dst_dir_index = node_fs_buf.nodes[cwd].parent_index;
    } else {
        // Find directory under current working directory
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd &&
                node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR &&
                strcmp(node_fs_buf.nodes[i].node_name, dst_dir)) {
                dst_dir_index = i;
                found = true;
                break;
            }
        }
        if (!found) {
            printString("Destination directory not found\n");
            return;
        }
    }

    // duplikat file
    node_fs_buf_temp.nodes[source_index] = node_fs_buf.nodes[source_index];

    // Update filesystem node for the source file
    node_fs_buf.nodes[source_index].parent_index = dst_dir_index;
    strcpy(node_fs_buf.nodes[source_index].node_name, new_filename);

    // Write back updated filesystem nodes to disk
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
    writeSector(&(node_fs_buf_temp.nodes[0]), FS_NODE_SECTOR_TEMP_NUMBER);
    writeSector(&(node_fs_buf_temp.nodes[32]), FS_NODE_SECTOR_TEMP_NUMBER + 1);


    // Debug message: Print new filename and directory
    printString("cp: ");
    printString(src);
    printString(" -> ");
    printString(dst);
    printString("\n");

    // Success message
    printString("File moved successfully\n");
}



// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct file_metadata file;
    enum fs_return status;

    file.parent_index = cwd;
    strcpy(file.node_name, filename);

    fsRead(&file, &status);

    if (status == FS_R_NODE_NOT_FOUND) {
        printString("File not found\n");
        return;
    }

    if (status == FS_R_TYPE_IS_DIRECTORY) {
        printString("Cannot read a directory\n");
        return;
    }

    // Print the file content
    file.buffer[file.filesize] = '\0'; // Null-terminate the buffer
    printString(file.buffer);
    printString("\n");
}
// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i, empty_index = -1;

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Validate directory name
    if (strlen(dirname) == 0 || strlen(dirname) > MAX_FILENAME) {
        printString("Invalid directory name length\n");
        return;
    }

    // Find an empty spot in filesystem nodes for the new directory
    empty_index = FS_MAX_NODE;
    for (i = 1; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == 0 &&
            node_fs_buf.nodes[i].data_index == 0) {
            empty_index = i;
            break;
        }
    }

    // Handle case where no empty spot is found
    if (empty_index == -1) {
        printString("No free node available\n");
        return;
    }

    // Write new directory information to the found empty node
    node_fs_buf.nodes[empty_index].parent_index = cwd;
    node_fs_buf.nodes[empty_index].data_index = FS_NODE_D_DIR;
    strcpy(node_fs_buf.nodes[empty_index].node_name, dirname);

    // Update filesystem nodes to reflect the new directory
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Success message
    printString("Directory created successfully\n");
}
