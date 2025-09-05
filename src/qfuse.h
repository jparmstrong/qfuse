#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "dirtree.h"

#define MAX_LINE_LENGTH 5120

#define _strncpy(dest, src) { \
    strncpy((dest), (src), sizeof((dest)) - 1); \
    (dest)[sizeof(src)-1] = '\0'; \
}

typedef struct ConfigEntry {
    char database[16];
    char path[4096];
    struct ConfigEntry *next;
} ConfigEntry;

ConfigEntry* load_config(const char *file) {

    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        perror("Error opening file");
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    int line_number = 0;

    ConfigEntry *head = NULL;
    ConfigEntry *current = NULL;

    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        if (0 == line_number++) {
            continue; // Skip header line
        }
        else if (line[0] == '#' || line[0] == '\n') {
            continue; // Skip comments and empty lines
        }

        line[strcspn(line, "\n")] = 0;
    
        if (current == NULL) {
            head = (ConfigEntry *)malloc(sizeof(ConfigEntry));
            current = head;
        } else {
            current->next = (ConfigEntry *)malloc(sizeof(ConfigEntry));
            current = current->next;
        }

        current->database[0] = '\0';
        current->path[0] = '\0';

        char *token;

        token = strtok(line, ",");
        strncpy(current->database, token, sizeof(current->database) - 1);
        current->database[sizeof(current->database) - 1] = '\0';
        
        token = strtok(NULL, ",");
        strncpy(current->path, token, sizeof(current->path) - 1);
        current->path[sizeof(current->path) - 1] = '\0';
  
        char* par_path;
        asprintf(&par_path, "%s/par.txt", current->path);
        INFO("Expanding par.txt in %s\n", par_path);
        if (access(par_path, F_OK) == 0) {
            FILE* par_file = fopen(par_path, "r");
            if (par_file) {
                char par_line[4096];
                while (fgets(par_line, sizeof(par_line), par_file)) {
                    par_line[strcspn(par_line, "\n")] = 0; // Remove newline
                    if (strlen(par_line) > 0) {
                        ConfigEntry* new_entry = (ConfigEntry*)malloc(sizeof(ConfigEntry));
                        _strncpy(new_entry->database, current->database);
                        _strncpy(new_entry->path, par_line);
                        new_entry->next = current->next;
                        current->next = new_entry;
                        current = new_entry;
                    }
                }
                fclose(par_file);
            }
        }
        free(par_path);

        DEBUG("Loaded config: database='%s', path='%s'\n", current->database, current->path);
    }

    fclose(fp);
    return head;
}


// Helper to find child directory by name
Node* find_child_dir(Node* parent, const char* name) {
    if (!parent || !parent->is_directory) return NULL;
    for (int i = 0; i < parent->num_children; i++) {
        Node* child = parent->children[i];
        if (child->is_directory && strcmp(child->name, name) == 0) {
            return child;
        }
    }
    return NULL;
}

// Helper: check if directory contains a .d file
static int has_dot_d_file(const char* path) {
    char dot_d_path[4096];
    snprintf(dot_d_path, sizeof(dot_d_path), "%s/.d", path);
    return access(dot_d_path, F_OK) == 0;
}

// Modified scan_path to support .database.table structure
void scan_path(const char *path, Node *parent, const char* database) {
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, "par.txt") == 0) continue;
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        DEBUG("Scanning %s\n", full_path);
        struct stat st;
        if (stat(full_path, &st) == -1) continue;
        if (S_ISDIR(st.st_mode)) {
            char new_dir_name[512];
            // If directory contains a .d file, prefix with .database.
            if (has_dot_d_file(full_path)) {
                snprintf(new_dir_name, sizeof(new_dir_name), "%s.%s", database, entry->d_name);
            } else {
                _strncpy(new_dir_name, entry->d_name);
            }
            Node *dir_node = find_child_dir(parent, new_dir_name);
            if (!dir_node) {
                dir_node = tree_add_dir(parent, new_dir_name);
            }
            scan_path(full_path, dir_node, database);
        } else if (S_ISREG(st.st_mode)) {
            tree_add_file(parent, entry->d_name, full_path);
        }
    }
    closedir(dir);
}

void scan_paths(ConfigEntry* configs, Node* root) {
    ConfigEntry* current = configs;
    while (current != NULL) {
        INFO("Scanning path for database '%s': %s\n", current->database, current->path);
        scan_path(current->path, root, current->database);
        current = current->next;
    }

    tree_qsort(root);
    INFO("Tree of size %d loaded.\n", tree_count(root));
}
