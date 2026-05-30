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
        BPlusNode* getRoot() const {
            return this->root;
        }
    private:
        BPlusNode* root;

        /** * @param id: Search id for the item.
         * @brief Traverses down to the leaf level and returns the node where the given ID
         * should reside (or be inserted). HER ZAMAN SOLA SAPAR (Multi-Map Uyumu).
         * @return Proper position for new node to be add
         */
        BPlusNode* searchPosition(size_t id);

        /** * @brief Promotes a split-key up into the parent node and handles recursive tree splitting.
         * @param parentNode Pointer to the parent node.
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

        /** * @param id: Item id (veya Hash değeri)
         * @param offset: Offset of real record in data file (veya İkincil Ağaç için Kitap ID'si)
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

        /**
         * @brief Handles underflow conditions when a node falls below the minimum key threshold.
         */
        void handleUnderflow(BPlusNode* node);

        /**
         * @brief Borrows the largest key from the left sibling and updates the parent router.
         */
        void borrowFromLeft(BPlusNode* node, BPlusNode* leftSibling, BPlusNode* parent, size_t parentIdx);

        /**
         * @brief Borrows the smallest key from the right sibling and updates the parent router.
         */
        void borrowFromRight(BPlusNode* node, BPlusNode* rightSibling, BPlusNode* parent, size_t parentIdx);

        /**
         * @brief Merges the right node into the left node and removes the separator key from the parent.
         */
        void mergeNodes(BPlusNode* leftNode, BPlusNode* rightNode, BPlusNode* parent, size_t parentIdx);

        /** * @param id: Search id for the item.
         * @brief Searches for a SINGLE record offset by its ID in the B+ Tree (Primary Index).
         * @return Returns std::nullopt if the tree is empty or the ID doesn't exist.
         */
        std::optional<size_t> search(size_t id);

        /**
         * @brief Searches for ALL records matching a given ID (Secondary Index Multi-Map).
         * Crucial for Secondary Indexing where one key (e.g., Author Hash) has multiple values (Book IDs).
         */
        std::vector<size_t> searchAll(size_t id);

        /**
         * @param filename: Path to the data file
         * @brief Traverses the leaf-level linked list to print all records in ascending order.
         */
        void listAllRecords(const std::string& filename);

        /**
         * @param node current node 
         * @brief Deletes nodes by recursively traversing from current node to the leaf node
         */
        void clear(BPlusNode* node);
        
    };
} // namespace mislib

#endif // B_PLUS_TREE_H