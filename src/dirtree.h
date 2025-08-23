#pragma once
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Represents a node in the directory tree (either a directory or a file)
typedef struct Node {
    const char* name;      // Name of the directory or file
    int is_directory;      // 1 if a directory, 0 if a file
    int num_children;      // Number of children
    int capacity_children; // Current capacity of the children array
    const char* orig_path;
    struct Node **children;// Array of pointers to child nodes
} Node;

// Function to create a new node
static Node* _create_node(const char* name, int is_dir, const char* orig_path) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (node == NULL) {
        perror("Failed to allocate memory for node");
        exit(EXIT_FAILURE);
    }

    DEBUG("Creating node: %s (is_dir=%d) to %s\n", name, is_dir, orig_path);
    node->name = strdup(name);
    node->orig_path = (!is_dir) ? strdup(orig_path) : NULL;
    node->is_directory = is_dir;
    node->children = NULL;
    node->num_children = 0;
    node->capacity_children = 0;

    return node;
}

static int _compare(const void* a, const void* b) {
    Node* nodeA = *(Node**)a;
    Node* nodeB = *(Node**)b;
    return strcmp(nodeA->name, nodeB->name);
}

static int _compare_name(const void* a, const void* b) {
    const char* name = (const char*)a;
    Node* node = *(Node**)b;
    return strcmp(name, node->name);
}

static void _free_child(Node* child) {
    if (child == NULL) return;
    if (child->is_directory) {
        for (int i = 0; i < child->num_children; i++) {
            _free_child(child->children[i]);
        }
        free(child->children);
    }
    free((void*)child->name);
    free((void*)child->orig_path);
    free(child);
}


// Function to add a child to a directory node
static void _add_child(Node* parent, Node* child) {
    if (!parent->is_directory) {
        fprintf(stderr, "Error: Cannot add child to a file node.\n");
        return;
    }

    for (int i = 0; i < parent->num_children; i++) {
        if (strcmp(parent->children[i]->name, child->name) == 0) {
            _free_child(child);
            return;
        }
    }

    if (parent->num_children == parent->capacity_children) {
        // Double the capacity if needed
        int new_capacity = (parent->capacity_children == 0) ? 2 : parent->capacity_children * 2;
        Node** new_children = (Node**)realloc(parent->children, new_capacity * sizeof(Node*));
        if (new_children == NULL) {
            perror("Failed to reallocate memory for children");
            exit(EXIT_FAILURE);
        }
        parent->children = new_children;
        parent->capacity_children = new_capacity;
    }

    parent->children[parent->num_children++] = child;
    DEBUG("Adding child %s (%s) to parent %s [%d/%d]\n", child->name, child->orig_path, parent->name, parent->num_children, parent->capacity_children);
}

static Node* tree_add_dir(Node* parent, const char* name) {
    Node* node = _create_node(name, 1, NULL);
    if (parent == NULL || !parent->is_directory) {
        return node;
    }
    _add_child(parent, node);
    return node;
}

static Node* tree_add_file(Node* parent, const char* name, const char* orig_path) {
    Node* node = _create_node(name, 0, orig_path);
    _add_child(parent, node);
    return node;
}

static Node* tree_find(Node* current, const char* path) {
    if (!current || !path) return NULL;
    if (strcmp(path, "/") == 0) return current;
    // Make a local copy of path and skip leading '/'
    const char *p = path;
    DEBUG("Finding path: %s\n", path);
    if (p[0] == '/') p++;
    char temp[512];
    strncpy(temp, p, sizeof(temp)-1);
    temp[sizeof(temp)-1] = '\0';
    char *tptr;
    char *token = strtok_r(temp, "/", &tptr);
    Node *node = current;
    while (token && node) {
        Node** result = (Node**)bsearch(token, node->children, node->num_children, sizeof(Node*), _compare_name);
        DEBUG("Searching for '%s' in '%s' found: %s\n", token, node->name, result ? (*result)->name : "not found");
        if (result) {
            node = *result;
        }
        token = strtok_r(NULL, "/",  &tptr);
    }
    return node;
}

static void tree_qsort(Node* node) {
    if (node == NULL || !node->is_directory || node->num_children <= 1) {
        return;
    }
    // Sort children by name
    qsort(node->children, node->num_children, sizeof(Node*), _compare);
    // Recursively sort each child's children
    for (int i = 0; i < node->num_children; i++) {
        tree_qsort(node->children[i]);
    }
}

static inline int tree_count(Node* node) {
    if (node == NULL) return 0;
    int total = 1; // Count this node
    if (node->is_directory) {
        for (int i = 0; i < node->num_children; i++) {
            total += tree_count(node->children[i]);
        }
    }
    return total;
}

// Function to free the memory allocated for the tree
static void tree_free(Node* node) {
    if (node == NULL) {
        return;
    }
    if (node->is_directory) {
        for (int i = 0; i < node->num_children; i++) {
            tree_free(node->children[i]);
        }
        free(node->children);
    }
    free(node);
}

static inline int _file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

int tree_gc(Node* node) {
    if (!node) return 0;
    int removed = 0;
    if (node->is_directory) {
        for (int i = 0; i < node->num_children; ) {
            Node* child = node->children[i];
            removed += tree_gc(child);
            // If child is a file and its orig_path no longer exists, remove it
            if (!child->is_directory && !_file_exists(child->orig_path)) {
                DEBUG("Removing stale file node: %s (orig: %s)\n", child->name, child->orig_path);
                // Remove child from parent's children array
                for (int j = i; j < node->num_children - 1; j++) {
                    node->children[j] = node->children[j + 1];
                }
                node->num_children--;
                _free_child(child);
                removed++;
            } else {
                i++;
            }
        }
    }
    return removed;
}