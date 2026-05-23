#include "BookRepository.h"
#include "../util/BookParser.h"
#include <stdexcept>
#include <iostream>
#include <fstream>

namespace mislib
{
    BookRepo::BookRepo(const std::string& dataPath)
        : dataPath(dataPath),
          indexManager("Index.dat")
    {
        readFile.open(dataPath, std::ios::in);
        writeFile.open(dataPath, std::ios::out | std::ios::app);

        if (!readFile.is_open() || !writeFile.is_open())
            throw std::runtime_error("[REPO] Cannot open data file: " + dataPath);

        // Inject book-specific parsing logic into the index build process.
        // IndexManager has no knowledge of Book — it only sees raw lines and offsets.
        // Soft-deleted records (prefixed with '!') are excluded from the index.
        indexManager.loadOrBuild(dataPath, [this](const std::string& line, size_t offset) {
            if (line.front() == DELETED_MARKER) return;

            size_t id = mislib::extractId(line.c_str());
            indexManager.getTree().insert(id, offset);
            if (id > lastId) lastId = id;
        });
    }

    BookRepo::~BookRepo() {
        indexManager.save();
        readFile.close();
        writeFile.close();
    }

    bool BookRepo::create(const Book& data) {
        Book book = data;
        book.id = ++lastId;

        // Always resolve the true end-of-file position before writing
        // to avoid stale offset caused by ios::app or buffering side effects.
        writeFile.flush();
        size_t offset = (size_t)writeFile.tellp();

        writeFile << book.id    << ","
                  << book.title  << ","
                  << book.author << ","
                  << book.genre  << ","
                  << book.date   << "\n";
        writeFile.flush();

        return indexManager.getTree().insert(book.id, offset);
    }

    bool BookRepo::get(size_t id, Book& out) {
        auto offsetOpt = indexManager.getTree().search(id);
        if (!offsetOpt.has_value()) return false;

        readFile.clear();
        readFile.seekg((std::streamoff)*offsetOpt);

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
        readFile.seekg((std::streamoff)*offsetOpt);

        std::string existingLine;
        if (!std::getline(readFile, existingLine)) return false;
        if (existingLine.front() == DELETED_MARKER) return false;

        // Soft-delete the old record and append the updated version at the end.
        // In-place overwrite is unsafe since new content may differ in byte length.
        std::fstream patchFile(dataPath, std::ios::in | std::ios::out | std::ios::binary);
        if (!patchFile.is_open()) return false;
        patchFile.seekp((std::streamoff)*offsetOpt);
        patchFile.put(DELETED_MARKER);
        patchFile.flush();
        patchFile.close();

        writeFile.flush();
        size_t newOffset = (size_t)writeFile.tellp();

        writeFile << data.id    << ","
                  << data.title  << ","
                  << data.author << ","
                  << data.genre  << ","
                  << data.date   << "\n";
        writeFile.flush();

        // Re-register the updated offset in the index.
        indexManager.getTree().remove(data.id);
        return indexManager.getTree().insert(data.id, newOffset);
    }

    bool BookRepo::remove(size_t id) {
        auto offsetOpt = indexManager.getTree().search(id);
        if (!offsetOpt.has_value()) return false;

        // Read the original line first
        readFile.clear();
        readFile.seekg((std::streamoff)*offsetOpt);
        std::string line;
        if (!std::getline(readFile, line)) return false;
        if (line.front() == DELETED_MARKER) return false;

        // Append the soft-deleted version at the end of the file.
        // In-place overwrite is unsafe — prepending '!' shifts all bytes by one.
        writeFile.flush();
        size_t newOffset = (size_t)writeFile.tellp();

        writeFile << DELETED_MARKER << line << "\n";
        writeFile.flush();

        // Patch the original location to prevent it from being re-indexed on next boot.
        std::fstream patchFile(dataPath, std::ios::in | std::ios::out | std::ios::binary);
        if (!patchFile.is_open()) return false;
        patchFile.seekp((std::streamoff)*offsetOpt);
        patchFile.put(DELETED_MARKER);
        patchFile.flush();
        patchFile.close();

        return indexManager.getTree().remove(id);
    }


    void BookRepo::listAll() {
        indexManager.getTree().listAllRecords(dataPath);
    }

} // namespace mislib