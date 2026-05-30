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
        clear(root);
        root = nullptr;
        clear(root);
        root = nullptr;
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
            auto [index, it] = mislib::IdSearch(current->keys, id);
            
            current = current->children[index];
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

        // ENGEL KALDIRILDI: Artık aynı ID'den (örneğin aynı yazar hash'inden) birden fazla eklenebilir!
        // if (index < leaf->keys.size() && leaf->keys[index] == id) { return false; }

        // Insert the key and offset inside the leaf
        leaf->keys.insert(it, id);
        leaf->offsets.insert(leaf->offsets.begin() + index, offset);

        // Overflow control: Split if the node size reaches the order
        if (leaf->keys.size() >= BPlusTree::order) {
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

        // Check if there is underflow
        size_t minKeys = BPlusTree::order / 2;
        if (leaf != root && leaf->keys.size() < minKeys) {
            handleUnderflow(leaf);
        }

        // Shrink tree height if root becomes empty after cascade merges
        if (root->keys.empty() && !root->isLeaf) {
            BPlusNode* oldRoot = root;
            root = root->children[0];
            root->parent = nullptr;
            delete oldRoot;
        }

        return true;
    }

    void BPlusTree::handleUnderflow(BPlusNode* node) {
        BPlusNode* parent = node->parent;
        if (!parent) return;

        // Find the index of the current node inside parent's children vector
        auto it = std::find(parent->children.begin(), parent->children.end(), node);
        size_t idx = std::distance(parent->children.begin(), it);

        // Define siblings without exceeding boundaries
        BPlusNode* leftSibling = (idx > 0) ? parent->children[idx - 1] : nullptr;
        BPlusNode* rightSibling = (idx < parent->children.size() - 1) ? parent->children[idx + 1] : nullptr;

        size_t minKeys = BPlusTree::order / 2;

        // Strategy 1: Check left sibling first, borrow if it has enough keys
        if (leftSibling && leftSibling->keys.size() > minKeys) {
            borrowFromLeft(node, leftSibling, parent, idx - 1);
            return;
        }

        // Strategy 2: Check right sibling, borrow if it has enough keys
        if (rightSibling && rightSibling->keys.size() > minKeys) {
            borrowFromRight(node, rightSibling, parent, idx);
            return;
        }

        // Strategy 3: Merge if borrowing is not possible from both siblings
        if (leftSibling) {
            mergeNodes(leftSibling, node, parent, idx - 1);
        } else if (rightSibling) {
            mergeNodes(node, rightSibling, parent, idx);
        }

        // Cascade underflow check up to the parent node
        if (parent != root && parent->keys.size() < minKeys) {
            handleUnderflow(parent);
        }
    }

    void BPlusTree::borrowFromLeft(BPlusNode* node, BPlusNode* leftSibling, BPlusNode* parent, size_t parentIdx) {
        if (node->isLeaf) {
            // Move the largest key from left sibling to the beginning of current node
            node->keys.insert(node->keys.begin(), leftSibling->keys.back());
            node->offsets.insert(node->offsets.begin(), leftSibling->offsets.back());

            // Remove the transferred item from left sibling
            leftSibling->keys.pop_back();
            leftSibling->offsets.pop_back();

            // Update the router key in parent to reflect the new minimum key of current node
            parent->keys[parentIdx] = node->keys[0];
        } 
        else {
            // INTERNAL NODE BORROW (Rotation Mechanism)
            
            // 1. Demote the parent's separator key down to the beginning of current node's keys
            node->keys.insert(node->keys.begin(), parent->keys[parentIdx]);

            // 2. Promote the left sibling's largest key up to the parent's separator position
            parent->keys[parentIdx] = leftSibling->keys.back();
            leftSibling->keys.pop_back();

            // 3. Transfer the left sibling's last child to the current node's first child position
            node->children.insert(node->children.begin(), leftSibling->children.back());
            leftSibling->children.pop_back();

            // 4. Update the parent pointer of the transferred child node
            node->children.front()->parent = node;
        }
    }
    
    void BPlusTree::borrowFromRight(BPlusNode* node, BPlusNode* rightSibling, BPlusNode* parent, size_t parentIdx) {
        if (node->isLeaf) {
            // Move the smallest key from right sibling to the end of current node
            node->keys.push_back(rightSibling->keys.front());
            node->offsets.push_back(rightSibling->offsets.front());

            // Remove the transferred item from right sibling
            rightSibling->keys.erase(rightSibling->keys.begin());
            rightSibling->offsets.erase(rightSibling->offsets.begin());

            // Update the router key in parent to reflect the new minimum key of right sibling
            parent->keys[parentIdx] = rightSibling->keys[0];
        } 
        else {
            // INTERNAL NODE BORROW (Counter-Clockwise Rotation Mechanism)
            
            // 1. Demote the parent's separator key down to the end of current node's keys
            node->keys.push_back(parent->keys[parentIdx]);

            // 2. Promote the right sibling's smallest key up to the parent's separator position
            parent->keys[parentIdx] = rightSibling->keys.front();
            rightSibling->keys.erase(rightSibling->keys.begin());

            // 3. Transfer the right sibling's first child to the current node's last child position
            node->children.push_back(rightSibling->children.front());
            rightSibling->children.erase(rightSibling->children.begin());

            // 4. Update the parent pointer of the transferred child node
            node->children.back()->parent = node;
        }
    }

    void BPlusTree::mergeNodes(BPlusNode* leftNode, BPlusNode* rightNode, BPlusNode* parent, size_t parentIdx) {
        if (leftNode->isLeaf) {
            // your existing leaf merge code stays exactly the same
            leftNode->keys.insert(leftNode->keys.end(), rightNode->keys.begin(), rightNode->keys.end());
            leftNode->offsets.insert(leftNode->offsets.end(), rightNode->offsets.begin(), rightNode->offsets.end());
            leftNode->next = rightNode->next;

            parent->keys.erase(parent->keys.begin() + parentIdx);
            parent->children.erase(parent->children.begin() + parentIdx + 1);
            delete rightNode;

        } else {
            // Pull the separator key down from parent into left node
            leftNode->keys.push_back(parent->keys[parentIdx]);

            // Move all keys and children from right node into left node
            leftNode->keys.insert(leftNode->keys.end(), rightNode->keys.begin(), rightNode->keys.end());
            leftNode->children.insert(leftNode->children.end(), rightNode->children.begin(), rightNode->children.end());

            // Re-map children to their new parent
            for (auto* child : rightNode->children) {
                if (child) child->parent = leftNode;
            }

            // Remove separator key and right child pointer from parent
            parent->keys.erase(parent->keys.begin() + parentIdx);
            parent->children.erase(parent->children.begin() + parentIdx + 1);
            delete rightNode;
        }
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
        if (parentNode->keys.size() >= BPlusTree::order) {
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

    void BPlusTree::clear(BPlusNode* node){
        if(node == nullptr) return;

        // If it is not a leaf node traverse recurively and delete leaf node first
        if(!node->isLeaf){
            for(const auto& child: node->children){
                clear(child);
            }
        }

        //Delete node if it is a leaf node        
        delete node;
    }

    // --- MULTI-MAP SECONDARY SEARCH ---
    std::vector<size_t> BPlusTree::searchAll(size_t id) {
        std::vector<size_t> results;
        if (root == nullptr) return results;

        BPlusNode* current = searchPosition(id);
        if (!current) return results;

        while (current != nullptr) {
            for (size_t i = 0; i < current->keys.size(); ++i) {
                if (current->keys[i] == id) {
                    results.push_back(current->offsets[i]);
                } else if (current->keys[i] > id) {

                    return results; 
                }
            }
            current = current->next; 
        }
        return results;
    }

} // namespace mislib