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
#include <algorithm>
#include <iostream>

#include "log.hpp"

struct DirNode {
    std::string name;
    std::unordered_map<std::string, DirNode> children;
    DirNode* parent;
    std::string originalPath;

    DirNode() = default;

    DirNode(const std::string& name, const std::unordered_map<std::string, DirNode>& children, DirNode* parent, const std::string& originalPath)
        : name(name), children(children), parent(parent), originalPath(originalPath) {}
};

class Model {
public:
    Model();
    ~Model() = default;

    DirNode* find(const std::string& path);
    void add(const std::string& ns, const std::string& rootDir, const std::string& path);
    int size(const DirNode& node) const;
    int size() const;

private:
    DirNode root;
};

extern Model model;