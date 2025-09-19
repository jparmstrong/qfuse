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

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <variant>
#include <algorithm>
#include <iostream>

#include "log.hpp"

struct DirNode {
    std::string name;
    std::unordered_map<std::string, DirNode> children;
    std::string originalPath;

    DirNode() = default;

    DirNode(const std::string& name, const std::unordered_map<std::string, DirNode>& children, const std::string& originalPath)
        : name(name), children(children), originalPath(originalPath) {}
};

class Model {
public:
    Model() {
        auto c = std::unordered_map<std::string, DirNode>();
        c.reserve(512);
        this->root = {"root", c, ""};
    }
 
    ~Model() = default;

    DirNode* find(const std::string& path) {
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

    void add(const std::string& ns, const std::string& rootDir, const std::string& path) {
        std::stringstream ss(path);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, '/')) {
            if (!token.empty()) tokens.push_back(token);
        }
        DirNode* current = &root;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (auto search = current->children.find(tokens[i]) ; search != current->children.end() ) {
                current = &(search->second);
            } else {
                std::string opath = rootDir + "/" + path;
                current->children.insert({tokens[i], DirNode(tokens[i], {}, opath)});
                current = &current->children.at(tokens[i]);
            } 
        }
    }

    int size(const DirNode& node) const {
        int count = 1;
        for (const auto& [key, child] : node.children) {
            count += size(child);
        }
        return count;
    }

    int size() const {
        return size(root);
    } 

private:
    DirNode root;

    // bool has_dot_d(const DirNode* node) {
    //     auto dd_it = std::find_if(node->children.begin(), node->children.end(),
    //         [&](const DirNode& n) { return n.name == ".d"; }
    //     );
    //     return dd_it != node->children.end();
    // }
};


