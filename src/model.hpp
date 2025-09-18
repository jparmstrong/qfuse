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
    uint8_t isDir = true;  // larger, to pack
    std::unordered_map<std::string, DirNode> children;
    std::string originalPath;

    DirNode() = default;

    DirNode(const std::string& name, bool isDir, const std::unordered_map<std::string, DirNode>& children, const std::string& originalPath)
        : name(name), isDir(isDir), children(children), originalPath(originalPath) {}
};

class Model {
public:
    Model() {
        auto c = std::unordered_map<std::string, DirNode>();
        c.reserve(512);
        this->root = {"root", true, c, ""};
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

    void add(const std::string& ns, const std::string& rootDir, const std::string& path, int isDir, int dirCount) {
        std::stringstream ss(path);
        std::string token;
        DirNode* current = &root;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, '/')) {
            if (!token.empty()) tokens.push_back(token);
        }
        for (size_t i = 0; i < tokens.size(); ++i) {
            bool nodeIsDir = (i == tokens.size() - 1) ? isDir : true;
 
            // current->children.find(node.name == ns + "." + tokens[i]) != current->children.end()
            // found node
            if (auto search = current->children.find(token) ; search != current->children.end() ) {
                current = &(search->second);
            }
            else {
                std::string opath = (!nodeIsDir) ? rootDir + "/" + path : "";
                std::unordered_map<std::string, DirNode> initChild = std::unordered_map<std::string, DirNode>();
                initChild.reserve(dirCount*2);
                DirNode child = DirNode(tokens[i], nodeIsDir, initChild, opath);
                current->children.insert({tokens[i], child});
                current = &current->children.at(tokens[i]);
            }
        }
    }

    // bool remove(const std::string& path) {
    //     std::stringstream ss(path);
    //     std::string token;
    //     DirNode* current = &root;
    //     std::vector<DirNode*> parents = {&root};
    //     std::vector<std::string> tokens;
    //     while (std::getline(ss, token, '/')) {
    //         if (!token.empty()) tokens.push_back(token);
    //     }
    //     for (size_t i = 0; i < tokens.size(); ++i) {
    //         auto it = std::find_if(
    //             current->children.begin(), current->children.end(),
    //             [&](const DirNode& node) { return node.name == tokens[i]; }
    //         );
    //         if (it == current->children.end()) return false;
    //         parents.push_back(current);
    //         current = &(*it);
    //     }
    //     // Remove from parent's children
    //     DirNode* parent = parents[parents.size() - 2];
    //     auto& siblings = parent->children;
    //     siblings.erase(
    //         std::remove_if(siblings.begin(), siblings.end(),
    //             [&](const DirNode& node) { return node.name == tokens.back(); }),
    //         siblings.end()
    //     );
    //     return true;
    // }

    int size(const DirNode& node) const {
        int count = 0;
        for (const auto& [key, child] : node.children) {
            count += (child.isDir) ? size(child) : 1;
        }
        return count;
    }

    int size() const {
        return size(root);
    } 

    void print(const DirNode& node, int indent = 0) const {
        std::string indent_str(indent * 2, ' ');
        DEBUG("{} {} -> {}", indent_str, (node.isDir ? "📁" : "  "), node.name, node.originalPath);
        if (!node.isDir) {
            return;
        }
        // Print files, one per line
        for (const auto& [key, child] : node.children) {
            if (!child.isDir) {
                std::string child_indent_str((indent + 1) * 2, ' ');
                DEBUG("{}[F] {} -> {}", child_indent_str, child.name, child.originalPath);
            }
        }
        for (const auto& [key, child] : node.children) {
            if (child.isDir) {
                print(child, indent + 1);
            }
        }
    }

    void print() const {
        print(root);
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


