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

#include "model.hpp"

// Global model instance definition
Model model;

Model::Model() {
    auto c = std::unordered_map<std::string, DirNode>();
    c.reserve(512);
    this->root = {"root", c, nullptr, ""};
}

DirNode* Model::find(const std::string& path) {
    std::stringstream ss(path);
    std::string token;
    DirNode* current = &root;
    while (std::getline(ss, token, '/')) {
        if (token.empty()) continue;
        if (const auto search = current->children.find(token) ; search != current->children.end() ) {
            current = &(search->second);
        } else {
            return nullptr;
        }
    }
    return current;
}

void Model::add(const std::string& ns, const std::string& rootDir, const std::string& path) {
    std::stringstream ss(path);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, '/')) {
        if (!token.empty()) tokens.push_back(token);
    }
    DirNode* current = &root;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (auto search = current->children.find(ns + "." + tokens[i]) ; search != current->children.end() ) {
            current = &(search->second);
        } else if (auto search = current->children.find(tokens[i]) ; search != current->children.end() ) {
            current = &(search->second);
        } else {
            // if dir contains .d, add namespace to parent dir
            if (tokens.size() > 0 && tokens[i] == ".d") {
                auto it = current->parent->children.find(tokens[i-1]);
                if(it!=current->parent->children.end()) {
                    std::string nskey = ns + "." + tokens[i-1];
                    current->parent->children[nskey] = std::move(it->second);
                    current->parent->children.erase(it);
                    current = &(current->parent->children[nskey]);
                    current->name = nskey;
                }
            }
            std::string opath = rootDir + "/" + path;
            current->children.insert({tokens[i], DirNode(tokens[i], std::unordered_map<std::string, DirNode>(), current, opath)});
            current = &current->children.at(tokens[i]);
        } 
    }
}

int Model::size(const DirNode& node) const {
    int count = 1;
    for (const auto& [key, child] : node.children) {
        count += size(child);
    }
    return count;
}

int Model::size() const {
    return size(root);
}
