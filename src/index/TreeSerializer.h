#ifndef TREE_SERIALIZER_H
#define TREE_SERIALIZER_H

#include <string>
#include <fstream>
#include "../index/BPlusTree.h"

namespace mislib {

    class TreeSerializer {
    private:
        // Recursive helper to save tree nodes via DFS (Depth-First Search)
        static void serializeNode(BPlusNode* node, std::ofstream& file);
        
        // Recursive helper to load tree nodes and reconstruct pointers
        static BPlusNode* deserializeNode(std::ifstream& file, BPlusNode* parent, BPlusNode*& lastLeaf);

    public:
        // Static methods so we don't need to create an instance of the class
        static void saveTree(const BPlusTree& tree, const std::string& filename);
        static bool loadTree(BPlusTree& tree, const std::string& filename);
    };

} // namespace mislib

#endif // TREE_SERIALIZER_H