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

#include <iostream>
#include <filesystem>
#include <fstream>
#include <csignal>
#include <dirent.h>

#include "model.hpp"
static Model model;
#include "fuse.hpp"

namespace fs = std::filesystem;

struct Config {
    std::string ns;
    std::string dir;
};

std::vector<Config> readConfig(const std::string& config_file) {
    std::ifstream file(config_file);
    if(!file.is_open()) {
        std::cerr << "Failed to open config file: " << config_file << std::endl;
        exit(0);
    }

    std::vector<Config> config = {};
    std::string line;
    std::getline(file, line); // skip first line
    while(std::getline(file, line)) {
        if(line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string ns, dir;
        std::getline(ss, ns, ',');
        std::getline(ss, dir, ',');
        config.push_back({ns, dir});
    }

    return config;
}

int dirSize(const std::string& path) {
    int count = 0;
    DIR* dir = opendir(path.c_str());
    if (!dir) return 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        ++count;
    }
    closedir(dir);
    return count;
}

void _scanDirIter(const std::string& ns, const std::string& rootDir, const std::string& path) {
    std::string fullPath = rootDir + "/" + path;
    DIR *dir = opendir(fullPath.c_str());
    if (!dir) {
        closedir(dir);
        return;
    }
    struct dirent *entry;
    bool isDir;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, "par.txt") == 0) continue;
        std::string relPath = path + "/" + entry->d_name;
        TRACE("> PATH: {}", relPath);
        if (entry->d_type == DT_DIR) {
            isDir = true;
        } else if (entry->d_type == DT_REG) {
            isDir = false;
        } else {
            struct stat st;
            if (stat((rootDir + "/" + relPath).c_str(), &st) == -1) continue;
            isDir = S_ISDIR(st.st_mode);
        }
        if(isDir) 
            _scanDirIter(ns, rootDir, relPath);
        else 
            model.add(ns, rootDir, relPath);
    }
    closedir(dir);
}

void scanDir(const std::string& ns, const std::string& rootDir) {
    INFO("Scanning {}", rootDir);
    u_int64_t start = utime();
    _scanDirIter(ns, rootDir, "");
    DEBUG("Scan took: {:.3f} secs", static_cast<double> (utime()-start) / 1'000'000);
}

void scanDirsInConfigs(const Config& config) {
    fs::path par_file = fs::path(config.dir) / "par.txt";
    if (fs::exists(par_file)) {
        std::ifstream infile(par_file);
        std::string line;
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            fs::path scan_dir = fs::path(config.dir) / line;
            if (!fs::exists(scan_dir) || !fs::is_directory(scan_dir)) continue;
            scanDir(config.ns, scan_dir);
        }
    } else {
        scanDir(config.ns, config.dir);
    }
}

static std::string mountpoint;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <config.csv> [-f] <mount point>" << std::endl;
        return 1;
    }

    std::vector<Config> configs = readConfig(argv[1]);
    for (Config config : configs) {
        scanDirsInConfigs(config);
    }

#ifdef TRACE_LOGS
    model.print();
#endif

    INFO("Done. Size: {}", model.size());

    // FUSE operations setup
    static struct fuse_operations operations = {};
    operations.getattr = fuse_getattr;
    operations.open    = fuse_open;
    operations.read    = fuse_read;
    operations.release = fuse_release;
    operations.readdir = fuse_readdir;

    int fuse_argc = argc-1;
    std::vector<char*> fuse_argv(fuse_argc);
    fuse_argv[0] = argv[0];
    for(int i = 1; i < fuse_argc; i++) {
        fuse_argv[i] = argv[i+1];
    }
    mountpoint = argv[fuse_argc-1];
    return fuse_main(fuse_argc, fuse_argv.data(), &operations, nullptr);
}