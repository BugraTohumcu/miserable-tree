#include "IndexManager.h"
#include "../index/TreeSerializer.h"
#include <fstream>
#include <iostream>

namespace mislib
{
    /**
     * @brief Initializes the IndexManager with the path of the serialized index file.
     * @param indexPath Absolute or relative path to the index file.
     */
    IndexManager::IndexManager(const std::string& indexPath)
        : indexPath(indexPath) {}

    /**
     * @brief Determines whether the index tree should be deserialized from disk
     *        or rebuilt by scanning the raw data file line by line.
     *
     * @details If a valid serialized index file exists at the configured path,
     *          it will be loaded directly to avoid redundant computation.
     *          Otherwise, the data file is scanned from scratch and each line
     *          is dispatched to the caller-provided callback for processing.
     *
     * @param dataFilePath  Absolute or relative path to the raw data file.
     * @param onLine        Callback invoked for each non-empty line during build.
     *                      Receives the raw line content and its byte offset.
     *
     * @note The callback is NOT invoked when loading from a pre-built index.
     * @note Call save() after this method if persistence is required.
     */
    void IndexManager::loadOrBuild(const std::string& dataFilePath, LineCallback onLine) {
        if (TreeSerializer::loadTree(tree, indexPath)) {
            std::cout << "[INDEX] Loaded from " << indexPath << "\n";
            return;
        }
        std::cout << "[INDEX] Building from " << dataFilePath << "\n";
        buildFrom(dataFilePath, onLine);
        save();
    }

    /**
     * @brief Builds the B+ Tree index by sequentially scanning the raw data file.
     *
     * @param dataFilePath  Absolute or relative path to the raw data file.
     * @param onLine        Callback invoked for each non-empty line during build.
     *                      Receives the raw line content and its byte offset.
     *
     * @note Called internally by loadOrBuild when no pre-built index is found.
     */
    void IndexManager::buildFrom(const std::string& dataFilePath, LineCallback onLine) {
        std::ifstream file(dataFilePath);
        if (!file.is_open()) {
            std::cerr << "[INDEX] Failed to open: " << dataFilePath << "\n";
            return;
        }

        std::string line;
        while (file.good()) {
            // tellg BEFORE getline — offset must point to line start
            std::streampos pos = file.tellg();
            if (pos < 0) break; // tellg failed, stop safely

            if (!std::getline(file, line)) break;
            if (!line.empty()) onLine(line, (size_t)pos);
        }
    }

    /**
     * @brief Persists the current index state to disk.
     *
     * @note Must be called explicitly after structural changes to the tree
     *       if persistence across sessions is required.
     */
    void IndexManager::save() {
        TreeSerializer::saveTree(tree, indexPath);
    }

} // namespace mislib