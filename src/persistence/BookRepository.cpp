#include "BookRepository.h"
#include "../util/BookParser.h" 
#include <string>
#include <stdexcept>

namespace mislib
{
    BookRepo::BookRepo(const char* path) {
        this->dataFile = std::fstream(path, std::ios::in | std::ios::out);
        if (!dataFile.is_open()){
            throw std::runtime_error("Data file open failed");
        }
    }

    bool BookRepo::create(const Book& data) {
        return false;
    }

    bool BookRepo::get(size_t id, Book& out) {
        size_t offset = index.getIndex(id); 
        
        dataFile.clear();
        dataFile.seekg(0);

        std::string line;

        while (std::getline(dataFile, line)) {
            size_t current_id = mislib::extractId(line.c_str());

            if (current_id == id) {
                out = mislib::parseToBook(line.c_str());
                return true;
            }
        }
        return false;
    }

} // namespace mislib