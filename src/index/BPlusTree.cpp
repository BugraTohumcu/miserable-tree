#include "BPlusTree.h"
#include <algorithm>
#include "../entity/BookEntity.h"   
#include "../util/BookParser.h"     
#include "../util/BPlusTreeUtils.h"
#include <fstream>
#include <string>

namespace mislib
{
    BPlusTree::BPlusTree() {
        root = BPlusNode::createLeaf();
    }

    BPlusTree::~BPlusTree() {
        // --- Destruction logic will be implemented --- 
    }

    BPlusNode* BPlusTree::searchPosition(size_t id) {
        // Edge Case: Handle empty tree scenarios to prevent potential segmentation faults
        if (root == nullptr) return nullptr;

        BPlusNode* current = root;

        // Drill down the internal nodes until we hit the leaf level
        while (!current->isLeaf) {
            // Binary search to find the correct subtree router
            auto [index, it] = mislib::IdSearch(current->keys, id);
            current = current->children[index];
        }

        return current;
    }

    std::optional<size_t> BPlusTree::search(size_t id) {
        if (root == nullptr) return std::nullopt;
        BPlusNode* current = root;

        // Traverse down the internal nodes until we hit the leaf level
        while (!current->isLeaf) {
            // Binary search within the node keys to find the branch guide
            auto it = std::lower_bound(current->keys.begin(), current->keys.end(), id);
            size_t index = std::distance(current->keys.begin(), it);

            // Edge Case: If the exact ID exists in an internal node, B+ Tree rules
            // dictate that the actual data points to the right child (index + 1).
            if (index < current->keys.size() && current->keys[index] == id) {
                current = current->children[index + 1];
            }
            else {
                current = current->children[index];
            }
        }

        // Now at the leaf level, execute the final search for the actual record
        auto [index, it] = mislib::IdSearch(current->keys, id);

        // Validate index boundaries and verify if the key matches our target
        if (index < current->keys.size() && current->keys[index] == id) {
            return current->offsets[index];
        }

        return std::nullopt;
    }

    bool BPlusTree::insert(size_t id, size_t offset) {
        // Find the proper location for new node
        BPlusNode* leaf = searchPosition(id);

        // Find index with binary search
        auto [index, it] = mislib::IdSearch(leaf->keys, id);

        // If id is already there do not add
        if (index < leaf->keys.size() && leaf->keys[index] == id) {
            return false;
        }

        // Insert the key and offset inside the leaf
        leaf->keys.insert(it, id);
        leaf->offsets.insert(leaf->offsets.begin() + index, offset);

        // Overflow control: Split if the node size reaches the order
        if (leaf->keys.size() >= order) {
            // Split half and get the promoted key
            size_t splitIndex = leaf->keys.size() / 2;
            size_t promotedKey = leaf->keys[splitIndex];

            // Create new right sibling node
            BPlusNode* newRight = BPlusNode::createLeaf();

            // Move everything starting from split index to new node (B+ Tree Leaf Split Rule)
            newRight->keys.assign(leaf->keys.begin() + splitIndex, leaf->keys.end());
            newRight->offsets.assign(leaf->offsets.begin() + splitIndex, leaf->offsets.end());

            // Remove the moved keys from current leaf
            leaf->keys.erase(leaf->keys.begin() + splitIndex, leaf->keys.end());
            leaf->offsets.erase(leaf->offsets.begin() + splitIndex, leaf->offsets.end());

            // Update the linked list for range queries
            newRight->next = leaf->next;
            leaf->next = newRight;

            // Promote the new node to parent
            promote(leaf->parent, leaf, promotedKey, newRight);
        }
        return true;
    }

    bool BPlusTree::remove(size_t id) {
        // Locate the leaf containing the target ID
        BPlusNode* leaf = searchPosition(id);
        if (!leaf) return false;

        auto [index, it] = mislib::IdSearch(leaf->keys, id);

        // Verify if the key exists in the leaf
        if (index >= leaf->keys.size() || leaf->keys[index] != id) {
            return false;
        }

        // Erase from leaf vectors
        leaf->keys.erase(it);
        leaf->offsets.erase(leaf->offsets.begin() + index);

        // Update hierarchy: if the deleted key is used as a router in internal nodes
        BPlusNode* parentNode = leaf->parent;
        while (parentNode != nullptr) {
            for (size_t i = 0; i < parentNode->keys.size(); ++i) {
                if (parentNode->keys[i] == id) {
                    parentNode->keys[i] = leaf->keys.empty() ? 0 : leaf->keys[0];
                }
            }
            parentNode = parentNode->parent;
        }
        return true;
    }
    void BPlusTree::listAllRecords(const std::string& filename) {
        if (root == nullptr) return;

        // Step 1: Go straight down to the leftmost leaf node (with the smallest ID).
        BPlusNode* current = root;
        while (!current->isLeaf) {
            current = current->children[0];
        }

        // Step 2: Read the physical data file
        std::ifstream file(filename);
        if (!file.is_open()) return;

        std::cout << "\n--- ALL RECORDS ARE BEING LISTED  ---" << std::endl;

        // Step 3: Linearly trace the linked list between the leaves
        while (current != nullptr) {
            for (size_t offset : current->offsets) {
                file.seekg(offset);
                std::string line;
                if (std::getline(file, line)) {
                    Book b = mislib::parseToBook(line.c_str());
                    std::cout << b;
                }
            }
            current = current->next; 
        }
        file.close();
    }

    void BPlusTree::createNewRoot(BPlusNode* oldLeft, size_t key, BPlusNode* newRight) {
        // Initialize a new internal node to act as the root
        root = BPlusNode::createInternal();
        root->keys.push_back(key);
        root->children.push_back(oldLeft);
        root->children.push_back(newRight);

        // Update parent pointers
        oldLeft->parent = root;
        newRight->parent = root;
    }

    void BPlusTree::promote(BPlusNode* parentNode, BPlusNode* childNode, size_t promotedKey, BPlusNode* newSibling) {
        // If we reached the top, create a new root
        if (parentNode == nullptr) {
            createNewRoot(childNode, promotedKey, newSibling);
            return;
        }

        // Insert the promoted key into the parent
        auto [index, it] = mislib::IdSearch(parentNode->keys, promotedKey);
        parentNode->keys.insert(it, promotedKey);
        parentNode->children.insert(parentNode->children.begin() + index + 1, newSibling);
        newSibling->parent = parentNode;

        // Recursive split if the parent overflows
        if (parentNode->keys.size() >= order) {
            size_t midIndex = parentNode->keys.size() / 2;
            size_t upKey = parentNode->keys[midIndex];

            BPlusNode* newInternal = BPlusNode::createInternal();

            // Move upper half to new internal node
            newInternal->keys.assign(parentNode->keys.begin() + midIndex + 1, parentNode->keys.end());
            newInternal->children.assign(parentNode->children.begin() + midIndex + 1, parentNode->children.end());

            // Re-map children to their new parent
            for (auto* child : newInternal->children) {
                if (child) child->parent = newInternal;
            }

            parentNode->keys.erase(parentNode->keys.begin() + midIndex, parentNode->keys.end());
            parentNode->children.erase(parentNode->children.begin() + midIndex + 1, parentNode->children.end());

            promote(parentNode->parent, parentNode, upKey, newInternal);
        }
    }

} // namespace mislib
