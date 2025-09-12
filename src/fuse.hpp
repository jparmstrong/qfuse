/*
    qfuse - virtual file system for KDB/Q
    Copyright (C) 2025 JP Armstrong <jp@armstrong.sh>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

*/

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <unistd.h>

// Common FUSE operation stubs
static int fuse_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    DirNode* node = model.find(path);
    if(!node) return -ENOENT;

    if (node->isDir) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        struct stat st;
        if(::stat(node->originalPath.c_str(), &st) != 0) {
            ERROR("Unable to get file size for {} -> {}: {}", path, node->originalPath, strerror(errno));
            return -ENOENT;
        }
        *stbuf = st;
        stbuf->st_mode = S_IFREG | 0444;
    }
    (void)fi;
    return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;
    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);

    DirNode *node = model.find(path);
    if(node && node->isDir) {
        for(auto child : node->children) {
            filler(buf, child.name.c_str(), NULL, 0, (fuse_fill_dir_flags) 0);
        }
    }
    return 0;
}

static int fuse_open(const char* path, struct fuse_file_info* fi) {
    DirNode *node = model.find(path);
    if (!node || node->isDir) {
        return -ENOENT;
    }
    const char* opath = node->originalPath.c_str();
    int fd = open(opath, O_RDONLY);
    if (fd == -1) {
        ERROR("open error {} -> {}: {}", path, opath, strerror(errno));
        return -errno;
    }
    DEBUG("open {} (orig: {}) -> fd {}", path, opath, fd);
    fi->fh = fd;
    return 0;
}

static int fuse_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    (void) path;
    int fd = fi->fh;
    if (fd <= 0) {
        return -EBADF;
    }
    ssize_t res = pread(fd, buf, size, offset);
    if (res == -1) {
        DEBUG("read error {}: {}", path, strerror(errno));
        return -errno;
    }
    DEBUG("read {} {} bytes at offset {}", path, size, offset);
    return res;
}

static int fuse_release(const char* path, struct fuse_file_info *fi) {
    (void) path;
    if (fi->fh) {
        DEBUG("close {}", path);
        close(fi->fh);
    }
    return 0;
}
