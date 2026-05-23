#ifndef INDEX_MANAGER
#define INDEX_MANAGER

#include "BPlusTree.h"
#include <functional>

namespace mislib{


    /**
     * @brief This class responsible for loading, building and saving the index files
     * 
     */
    class IndexManager{

        private: 
            const std::string indexPath;
            BPlusTree tree;
        public:
        IndexManager(const std::string& indexPath);

        // Telling index manager what to do for each line
        using LineCallback = std::function<void(const std::string line, size_t offset)>;


        /**
         * @brief Determines whether the index tree should be deserialized from disk
         * or rebuilt by scanning the raw data file line by line.
         * @param dataFilePath Absolute or relative path of the raw data file to be converted to tree
         * @param onLine Instructions of what IndexManager should do in each line of record
         */
        void loadOrBuild(const std::string& dataFilePath, LineCallback onLine);

        /**
         * @brief Builds the B+ Tree index by sequentially scanning the raw data file.
         * @param dataFilePath Absolute or relative path of the raw data file to be converted to tree
         * @param onLine Instructions of what IndexManager should do in each line of record
         */
        void buildFrom(const std::string& dataFilePath,LineCallback onLine);


        /**
         * @brief Persists the current index state to disk.
         *
         * @note Must be called explicitly after structural changes to the tree
         *       if persistence across sessions is required.
         */
        void save();


        /**
         * @brief Returns current tree
         */
        mislib::BPlusTree& getTree(){
            return this->tree;
        }
    };
}


#endif