#define FUSE_USE_VERSION 31
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#include "qfuse.h"

_Atomic(Node*) root = NULL;
_Atomic(Node*) root2 = NULL; // for double buffering

ConfigEntry* configs = NULL;

static int qfuse_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));
    Node *node = tree_find(root, path);
    if (!node) {
        (void)fi;
        return -ENOENT;
    }
    if (node->is_directory) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        struct stat st;
        if(stat(node->orig_path, &st) != 0) {
            ERROR("Unable to get file size for %s -> %s: %s\n", path, node->orig_path, strerror(errno));
            return -1;
        }
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = st.st_size; // Example size, update as needed

        stbuf->st_mtime = st.st_mtime;
        stbuf->st_atime = st.st_atime;
        stbuf->st_ctime = st.st_ctime;
    }
    (void)fi;
    return 0;
}

static int qfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    Node *dir = tree_find(root, path);
    if (dir && dir->is_directory) {
        for (int i = 0; i < dir->num_children; i++) {
            Node *child = dir->children[i];
            if(child->is_directory && child->num_children == 0) {
                continue;
            } 
            filler(buf, child->name, NULL, 0, 0);
        }
    }
    return 0;
}

static int qfuse_open(const char *path, struct fuse_file_info *fi) {
    Node *node = tree_find(root, path);
    if (!node || node->is_directory) {
        return -ENOENT;
    }
    const char* orig_path = node->orig_path;
    int fd = open(orig_path, O_RDONLY);
    if (fd == -1) {
        ERROR("open error %s -> %s: %s\n", path, orig_path, strerror(errno));
        return -errno;
    }
    DEBUG("open %s (orig: %s) -> fd %d\n", path, orig_path, fd);
    fi->fh = fd;
    return 0;
}

static int qfuse_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    (void) path;
    int fd = fi->fh;
    if (fd <= 0) {
        return -EBADF;
    }
    ssize_t res = pread(fd, buf, size, offset);
    if (res == -1) {
        DEBUG("read error %s: %s\n", path, strerror(errno));
        return -errno;
    }
    DEBUG("read %s %zu bytes at offset %ld\n", path, size, offset);
    return res;
}

static int qfuse_release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    if (fi->fh) {
        DEBUG("close %s\n", path);
        close(fi->fh);
    }
    return 0;
}

static const struct fuse_operations qfuse_oper = {
    .getattr    = qfuse_getattr,
    .readdir    = qfuse_readdir,
    .open       = qfuse_open,
    .read       = qfuse_read,
    .release    = qfuse_release,
};

void* loop(void* _) {
    while (1) {
        sleep(60);
        struct timeval start, end;
        gettimeofday(&start, NULL);
        
        if (root2 == NULL) {
            root2 = tree_add_dir(NULL, "root");
        }
        scan_paths(configs, root2);
        tree_gc(root2);
        
        // Swap trees
        root = atomic_exchange(&root, root2);
        
        gettimeofday(&end, NULL);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        INFO("Loop completed in %.3f seconds\n", elapsed);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        ERROR("Usage: %s <config.csv> <mountpoint>\n", argv[0]);
        return 1;
    }

    root = tree_add_dir(NULL, "root");
    configs = load_config(argv[1]);
    scan_paths(configs, root);
    
    // Shift arguments for fuse_main (remove config file arg)
    argc--;
    for (int i = 1; i < argc; i++) {
        argv[i] = argv[i + 1];
    }

    pthread_t loop_thread;
    pthread_create(&loop_thread, NULL, loop, NULL);
    
    fuse_main(argc, argv, &qfuse_oper, NULL);

    tree_free(root); // Free allocated memory
}
