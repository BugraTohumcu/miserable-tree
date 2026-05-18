#include "BPlusTree.h"
#include <algorithm>


#include "../util/BPlusTreeUtils.h"
namespace mislib
{
    BPlusTree::BPlusTree(){
        root = BPlusNode::createLeaf();
    }

    BPlusTree::~BPlusTree() {
        // --- Destruction logic will be implemented  --- 
    }

    BPlusNode* BPlusTree::searchPosition(size_t id) {
        // Edge Case: Handle empty tree scenarios to prevent potential segmentation faults
        if (root == nullptr) return nullptr;

        BPlusNode *current = root;

        // Drill down the internal nodes until we hit the leaf level
        while (!current->isLeaf) {
            // Binary search to find the correct subtree router
            auto [index, it] = mislib::IdSearch(current->keys, id);
            current = current->children[index];
        }

        return current;   
    }


    std::optional<size_t> BPlusTree::search(size_t id){
        if (root == nullptr) return std::nullopt;
        BPlusNode *current = root;

        // Traverse down the internal nodes until we hit the leaf level
        while (!current->isLeaf){

            // Binary search within the node keys to find the branch guide
            auto it = std::lower_bound(current->keys.begin(), current->keys.end(), id);
            size_t index = std::distance(current->keys.begin(), it);
            
            // Edge Case: If the exact ID exists in an internal node, B+ Tree rules
            // dictate that the actual data points to the right child (index + 1).
            if(index < current->keys.size() && current->keys[index] == id){
                current = current->children[index + 1];
            }else {
                current = current->children[index];
            } 
        }

        // Now at the leaf level, execute the final search for the actual record
        auto [index, it] = mislib::IdSearch(current->keys, id);


        // Validate index boundaries and verify if the key matches our target
        if(index < current->keys.size() && current->keys[index] == id){
            return current->offsets[index];
        }

        return std::nullopt;
    }


    bool BPlusTree::insert(size_t id, size_t offset){

        // Find the proper location for new node
        BPlusNode* leaf = searchPosition(id);

        // Find index with binary search
        auto [index, it] = mislib::IdSearch(leaf->keys, id);

        // If id is already there do not add
        if (index < leaf->keys.size() && leaf->keys[index] == id) {
            return false;
        }

        
        // Inser the node inside the leaf
        leaf->keys.insert(it, index);
        leaf->offsets.insert(leaf->offsets.begin() + index, offset);

        //Overflow control
        if(leaf->keys.size() >= order){
            
            //Split half and get the promoted key
            size_t splitIndex = leaf->keys.size() / 2;
            size_t promotedKey = leaf->keys[splitIndex];

            // Create new right sibling node
            BPlusNode* newRight = BPlusNode::createLeaf();

            // Move everything after split index to new node
            newRight->keys.assign(leaf->keys.begin() + splitIndex + 1, leaf->keys.end());
            newRight->offsets.assign(leaf->offsets.begin() + splitIndex + 1, leaf->offsets.end());

            // Remove the moved keys from current leaf
            leaf->keys.erase(leaf->keys.begin() + splitIndex + 1, leaf->keys.end());
            leaf->offsets.erase(leaf->keys.begin() + splitIndex + 1, leaf->keys.end());


            // Update the linked list
            newRight->next = leaf->next;
            leaf->next = newRight;


            // Promote the new node
            promote(leaf->parent, leaf, promotedKey, newRight);

        }

        
    }



} // namespace mislib
