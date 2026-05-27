#include "BookRepository.h"
#include "../util/BookParser.h"
#include "../util/BPlusTreeUtils.h" 
#include "../index/TreeSerializer.h" 
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

        // Try loading secondary tree indexes directly from disk binaries
        bool authorsLoaded = mislib::TreeSerializer::loadTree(authorTree, "AuthorIndex.dat");
        bool genresLoaded = mislib::TreeSerializer::loadTree(genreTree, "GenreIndex.dat");

        // --- CRITICAL FIX: HAYALET DOSYA KONTROLÜ EKLENDİ ---
        // Eğer dosya okunmuş ama ağaç tamamen boşsa, sahte yüklemeyi iptal et ve baştan inşa et!
        if (authorsLoaded && authorTree.getRoot() != nullptr && authorTree.getRoot()->keys.empty() && authorTree.getRoot()->isLeaf) {
            authorsLoaded = false;
        }
        if (genresLoaded && genreTree.getRoot() != nullptr && genreTree.getRoot()->keys.empty() && genreTree.getRoot()->isLeaf) {
            genresLoaded = false;
        }
        // ----------------------------------------------------

        // Inject book-specific parsing logic into the index build process.
        // Now it parses the full book to populate secondary tree indices.
        indexManager.loadOrBuild(dataPath, [this, authorsLoaded, genresLoaded](const std::string& line, size_t offset) {
            if (line.empty() || line.front() == DELETED_MARKER) return;

            size_t id = mislib::extractId(line.c_str());
            indexManager.getTree().insert(id, offset);

            if (id > lastId) lastId = id;
        });

        // Fail-safe recovery: If binary files are missing or empty, scan the dataset to regenerate them
        if (!authorsLoaded || !genresLoaded) {
            std::ifstream buildFile(dataPath);
            std::string line;
            while (buildFile.good()) {
                if (!std::getline(buildFile, line)) break;
                if (line.empty() || line.front() == DELETED_MARKER) continue;
                
                Book book = mislib::parseToBook(line.c_str());
                if (!authorsLoaded) authorTree.insert(mislib::HashString(book.author), book.id);
                if (!genresLoaded) genreTree.insert(mislib::HashString(book.genre), book.id);
            }
            buildFile.close();
        }

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
        // Serialize pure secondary tree structures to disk
        mislib::TreeSerializer::saveTree(authorTree, "AuthorIndex.dat");
        mislib::TreeSerializer::saveTree(genreTree, "GenreIndex.dat");
        
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

        // Insert into secondary indices by hashing strings and map to book primary key ID
        authorTree.insert(mislib::HashString(book.author), book.id);
        genreTree.insert(mislib::HashString(book.genre), book.id);

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

        // Add the new text mapping in case author/genre data strings were updated
        authorTree.insert(mislib::HashString(data.author), data.id);
        genreTree.insert(mislib::HashString(data.genre), data.id);

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
        std::vector<size_t> ids = authorTree.searchAll(mislib::HashString(author));
        for (size_t id : ids) {
            Book b;
            // get() ignores soft-deleted records implicitly
            if (get(id, b) && std::string(b.author) == author) {
                results.push_back(b);
            }
        }
        return results;
    }

    std::vector<Book> BookRepo::getByGenre(const std::string& genre) {
        std::vector<Book> results;
        std::vector<size_t> ids = genreTree.searchAll(mislib::HashString(genre));
        for (size_t id : ids) {
            Book b;
            if (get(id, b) && std::string(b.genre) == genre) {
                results.push_back(b);
            }
        }
        return results;
    }
} // namespace mislib