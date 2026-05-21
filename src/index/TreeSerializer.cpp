#include "TreeSerializer.h"
#include <iostream>

namespace mislib {

    void TreeSerializer::serializeNode(BPlusNode* node, std::ofstream& file) {
        if (!node) return;

        // 1. Save Node Type (Leaf or Internal)
        file.write(reinterpret_cast<const char*>(&node->isLeaf), sizeof(bool));

        // 2. Save Keys
        size_t keyCount = node->keys.size();
        file.write(reinterpret_cast<const char*>(&keyCount), sizeof(size_t));
        if (keyCount > 0) {
            file.write(reinterpret_cast<const char*>(node->keys.data()), keyCount * sizeof(size_t));
        }

        if (node->isLeaf) {
            // 3. Save Offsets for Leaf Nodes
            size_t offsetCount = node->offsets.size();
            file.write(reinterpret_cast<const char*>(&offsetCount), sizeof(size_t));
            if (offsetCount > 0) {
                file.write(reinterpret_cast<const char*>(node->offsets.data()), offsetCount * sizeof(size_t));
            }
        } else {
            // 3. Save Children Recursively for Internal Nodes
            size_t childCount = node->children.size();
            file.write(reinterpret_cast<const char*>(&childCount), sizeof(size_t));
            for (auto* child : node->children) {
                serializeNode(child, file);
            }
        }
    }

    BPlusNode* TreeSerializer::deserializeNode(std::ifstream& file, BPlusNode* parent, BPlusNode*& lastLeaf) {
        bool isLeaf;
        if (!file.read(reinterpret_cast<char*>(&isLeaf), sizeof(bool))) {
            return nullptr; // End of file
        }

        BPlusNode* node = new BPlusNode(isLeaf);
        node->parent = parent;

        size_t keyCount;
        file.read(reinterpret_cast<char*>(&keyCount), sizeof(size_t));
        node->keys.resize(keyCount);
        if (keyCount > 0) {
            file.read(reinterpret_cast<char*>(node->keys.data()), keyCount * sizeof(size_t));
        }

        if (isLeaf) {
            size_t offsetCount;
            file.read(reinterpret_cast<char*>(&offsetCount), sizeof(size_t));
            node->offsets.resize(offsetCount);
            if (offsetCount > 0) {
                file.read(reinterpret_cast<char*>(node->offsets.data()), offsetCount * sizeof(size_t));
            }

            // Reconstruct the Linked List sequence for Leaf Nodes
            if (lastLeaf != nullptr) {
                lastLeaf->next = node;
            }
            lastLeaf = node;
        } else {
            size_t childCount;
            file.read(reinterpret_cast<char*>(&childCount), sizeof(size_t));
            node->children.resize(childCount);
            for (size_t i = 0; i < childCount; ++i) {
                node->children[i] = deserializeNode(file, node, lastLeaf);
            }
        }

        return node;
    }

    void TreeSerializer::saveTree(const BPlusTree& tree, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Could not create binary index file: " << filename << std::endl;
            return;
        }

        std::cout << "[SYSTEM] Serializing B+ Tree nodes to binary file..." << std::endl;
        serializeNode(tree.root, file);
        file.close();
        std::cout << "[SYSTEM] Tree serialization completed successfully!" << std::endl;
    }

    bool TreeSerializer::loadTree(BPlusTree& tree, const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        std::cout << "[BOOT] Loading B+ Tree structure from binary file..." << std::endl;
        
        BPlusNode* lastLeaf = nullptr;
        tree.root = deserializeNode(file, nullptr, lastLeaf);

        file.close();
        return tree.root != nullptr;
    }

} // namespace mislib