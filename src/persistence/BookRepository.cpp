#include "BookRepository.h"
#include "../util/BookParser.h"
#include <stdexcept>
#include <iostream>
#include <fstream>

namespace mislib
{
    BookRepo::BookRepo(const std::string& dataPath)
        : dataPath(dataPath),
        indexManager("Index.dat"),
        lastId(0)
    {
        readFile.open(dataPath, std::ios::in);
        writeFile.open(dataPath, std::ios::out | std::ios::binary | std::ios::ate | std::ios::in);

        if (!writeFile.is_open()) {
            std::ofstream createMe(dataPath);
            createMe.close();
            readFile.open(dataPath, std::ios::in);
            writeFile.open(dataPath, std::ios::out | std::ios::binary | std::ios::ate | std::ios::in);
        }

        if (!readFile.is_open() || !writeFile.is_open())
            throw std::runtime_error("[REPO] Cannot open data file: " + dataPath);

        // Inject book-specific parsing logic into the index build process.
        // Now it parses the full book to populate secondary in-memory indices.
        indexManager.loadOrBuild(dataPath, [this](const std::string& line, size_t offset) {
            if (line.empty() || line.front() == DELETED_MARKER) return;

            Book book = mislib::parseToBook(line.c_str());
            
            indexManager.getTree().insert(book.id, offset);
            
            // Populate secondary in-memory indices
            authorIndex[book.author].push_back(book.id);
            genreIndex[book.genre].push_back(book.id);

            if (book.id > lastId) lastId = book.id;
        });

        // If the index was loaded from a serialized file, lastId remains 0.
        // We traverse to the rightmost leaf of the B+ Tree to recover the maximum active ID.
        if (lastId == 0) {
            mislib::BPlusTree& tree = indexManager.getTree();

            if (tree.getRoot() != nullptr) { // tree.root yerine tree.getRoot() yapıldı
                BPlusNode* current = tree.getRoot();

                // Drill down to the rightmost leaf node to recover index state
                while (current != nullptr && !current->isLeaf && !current->children.empty()) {
                    current = current->children.back();
                }

                if (current != nullptr && current->isLeaf && !current->keys.empty()) {
                    lastId = current->keys.back();
                }
            }
        }
    }

    BookRepo::~BookRepo() {
        indexManager.save();
        if (readFile.is_open()) readFile.close();
        if (writeFile.is_open()) writeFile.close();
    }

    bool BookRepo::create(const Book& data) {
        Book book = data;
        book.id = ++lastId;

        // Always resolve the true end-of-file position before writing
        // to avoid stale offset caused by ios::app or buffering side effects.
        writeFile.seekp(0, std::ios::end);
        size_t offset = static_cast<size_t>(writeFile.tellp());

        writeFile << book.id << ","
            << book.title << ","
            << book.author << ","
            << book.genre << ","
            << book.date << "\n";
        writeFile.flush();

        bool result = indexManager.getTree().insert(book.id, offset);

        // CRITICAL FIX: Commit index structural state to Index.dat immediately
        indexManager.save();

        // Update secondary indices in memory
        authorIndex[book.author].push_back(book.id);
        genreIndex[book.genre].push_back(book.id);

        return result;
    }

    bool BookRepo::get(size_t id, Book& out) {
        auto offsetOpt = indexManager.getTree().search(id);
        if (!offsetOpt.has_value()) return false;

        readFile.clear();
        readFile.seekg(static_cast<std::streamoff>(*offsetOpt));

        std::string line;
        if (!std::getline(readFile, line)) return false;

        // Guard against reading a soft-deleted record directly via offset.
        if (line.front() == DELETED_MARKER) return false;

        out = mislib::parseToBook(line.c_str());
        return true;
    }

    bool BookRepo::update(const Book& data) {
        auto offsetOpt = indexManager.getTree().search(data.id);
        if (!offsetOpt.has_value()) return false;

        readFile.clear();
        readFile.seekg(static_cast<std::streamoff>(*offsetOpt));

        std::string existingLine;
        if (!std::getline(readFile, existingLine)) return false;
        if (existingLine.front() == DELETED_MARKER) return false;

        // Soft-delete the old record and append the updated version at the end.
        // In-place overwrite is unsafe since new content may differ in byte length.
        std::fstream patchFile(dataPath, std::ios::in | std::ios::out | std::ios::binary);
        if (!patchFile.is_open()) return false;
        patchFile.seekp(static_cast<std::streamoff>(*offsetOpt));
        patchFile.put(DELETED_MARKER);
        patchFile.flush();
        patchFile.close();

        writeFile.seekp(0, std::ios::end);
        size_t newOffset = static_cast<size_t>(writeFile.tellp());

        writeFile << data.id << ","
            << data.title << ","
            << data.author << ","
            << data.genre << ","
            << data.date << "\n";
        writeFile.flush();

        // Re-register the updated offset in the index.
        indexManager.getTree().remove(data.id);
        bool result = indexManager.getTree().insert(data.id, newOffset);

        // CRITICAL FIX: Save structural index changes immediately
        indexManager.save();

        return result;
    }

    bool BookRepo::remove(size_t id) {
        auto offsetOpt = indexManager.getTree().search(id);
        if (!offsetOpt.has_value()) return false;

        // Patch the original location to prevent it from being re-indexed on next boot.
        // In-place marker insertion is optimized to prevent file space inflation.
        std::fstream patchFile(dataPath, std::ios::in | std::ios::out | std::ios::binary);
        if (!patchFile.is_open()) return false;
        patchFile.seekp(static_cast<std::streamoff>(*offsetOpt));
        patchFile.put(DELETED_MARKER);
        patchFile.flush();
        patchFile.close();

        bool result = indexManager.getTree().remove(id);

        // CRITICAL FIX: Synchronize changes to transactional disk index
        indexManager.save();

        return result;
    }

    void BookRepo::listAll() {
        indexManager.getTree().listAllRecords(dataPath);
    }

    // --- SECONDARY INDEX OPERATIONS ---

    std::vector<Book> BookRepo::getByAuthor(const std::string& author) {
        std::vector<Book> results;
        auto it = authorIndex.find(author);
        
        if (it != authorIndex.end()) {
            for (size_t id : it->second) {
                Book b;
                // get() ignores soft-deleted records implicitly
                if (get(id, b)) {
                    results.push_back(b);
                }
            }
        }
        return results;
    }

    std::vector<Book> BookRepo::getByGenre(const std::string& genre) {
        std::vector<Book> results;
        auto it = genreIndex.find(genre);
        
        if (it != genreIndex.end()) {
            for (size_t id : it->second) {
                Book b;
                if (get(id, b)) {
                    results.push_back(b);
                }
            }
        }
        return results;
    }

} // namespace mislib