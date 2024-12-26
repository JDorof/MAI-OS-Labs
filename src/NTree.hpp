#pragma once

#include <unordered_map>
#include <iostream>
#include <vector>
#include <algorithm>

struct Node {
    int i;
    int id;
    Node* parent;
    std::vector<Node*> kids;

    Node(int i, int id) : i(i), id(id), parent(nullptr), kids() {}
};

class NTree {
private:
    Node* head;
    std::unordered_map<int, Node*> ids;

public:
    NTree() : head(nullptr) {}

    void add(int parent, int id) {
        Node* new_node = new Node(parent == -1 ? 0 : ids[parent]->kids.size(), id);
        ids[id] = new_node;

        if (parent == -1) {
            head = new_node;
        } else {
            Node* parent_node = ids[parent];
            parent_node->kids.push_back(new_node);
            new_node->parent = parent_node;
        }
    }

    std::vector<int> get_ids() {
        std::vector<int> only_ids;
        for (auto [id, _] : ids) {
            only_ids.push_back(id);
        }
        return only_ids;
    }

    bool find(int id) {
        return ids.count(id);
    }

    std::string get_path_to(int id) {
        Node* cur_node = ids[id];

        std::string reversed_path;
        while (cur_node != nullptr) {
            reversed_path.push_back(static_cast<char>(cur_node->i)); // Преобразуем int в char
            cur_node = cur_node->parent;
        }

        // Ревёрсаем путь
        std::reverse(reversed_path.begin(), reversed_path.end());
        return reversed_path.substr(1, reversed_path.size() - 1);
    }

    ~NTree() {
        // Очистка памяти
        std::vector<Node*> nodes_to_delete;
        for (auto& pair : ids) {
            nodes_to_delete.push_back(pair.second);
        }
        for (Node* node : nodes_to_delete) {
            delete node;
        }
    }
};