#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H

#include <vector>
#include <optional>
#include <iostream>
#include <string>

#define TREE_ORDER 128

namespace mislib
{
    struct BPlusNode {
        bool isLeaf;
        BPlusNode* parent;
        BPlusNode* next;

        std::vector<size_t> keys;
        std::vector<size_t> offsets;
        std::vector<BPlusNode*> children;
        
        BPlusNode(bool leaf) : isLeaf(leaf), parent(nullptr), next(nullptr) {}

        static BPlusNode* createLeaf() {
            return new BPlusNode(true);
        }

        static BPlusNode* createInternal() {
            return new BPlusNode(false);
        }
    };

    class BPlusTree {
    public:
        static const int order = TREE_ORDER; 
    private:
        BPlusNode* root;

        /** * @param id: Search id for the item.         
         * @brief Traverses down to the leaf level and returns the node where the given ID          
         * should reside (or be inserted). Useful for split/insert operations.         
         * * @return Proper position for new node to be add         
         */
        BPlusNode* searchPosition(size_t id);

        /** * @brief Promotes a split-key up into the parent node and handles recursive tree splitting.         
         * * This method inserts the `promotedKey` into the `parentNode` and links the `newSibling`          
         * right next to the original `childNode`. If the parent node also overflows after insertion,          
         * this method will trigger a recursive split up towards the root.         
         * * @param parentNode Pointer to the parent node. Can be `nullptr` if the child is the current root,          
         * which triggers the creation of a new root node.         
         * @param childNode  Pointer to the original left child node that just caused the split.         
         * @param promotedKey The key that is being pushed up to the parent index level.         
         * @param newSibling Pointer to the newly created right sibling node that splits from the child.         
         */
        void promote(BPlusNode* parentNode, BPlusNode* childNode, size_t promotedKey, BPlusNode* newSibling);
        
        /**
         * @brief Handles the creation of a new root when the current root splits.
         */
        void createNewRoot(BPlusNode* oldLeft, size_t key, BPlusNode* newRight);

    public:
        BPlusTree();
        ~BPlusTree();

        friend class TreeSerializer;
    

        /** * @param id: Item id         
         * @param offset: Offset of real record in data file         
         * @brief Inserts new node in tree         
         * @returns Returns bool to show success         
         */
        bool insert(size_t id, size_t offset);
        
        /**
         * @param id: Item id to be removed
         * @brief Logically removes the record from the index
         * @returns Returns bool indicating if the deletion was successful
         */
        bool remove(size_t id);

        /** * @param id: Search id for the item.         
         * @brief Searches for a record offset by its ID in the B+ Tree.         
         * @return Returns std::nullopt if the tree is empty or the ID doesn't exist.         
         */
        std::optional<size_t> search(size_t id);

        /**
         * @param filename: Path to the data file
         * @brief Traverses the leaf-level linked list to print all records in ascending order.
         * This leverages the B+ Tree structure for O(N) sequential scanning.
         */
        void listAllRecords(const std::string& filename);
       
    };
} // namespace mislib

#endif // B_PLUS_TREE_H
