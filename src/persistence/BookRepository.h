#ifndef BOOK_REPO
#define BOOK_REPO

#include <fstream>
#include "CrudRepository.h"
#include "../entity/BookEntity.h"
#include "../index/BookIndex.h"

namespace mislib
{
    class BookRepo : public CrudRepository<Book> { 
    
    private: 
        std::fstream dataFile;
        mislib::BookIndex index;

    public:
        // Constructor
        BookRepo(const char* path);

        // CRUD Fonksiyonları
        bool create(const Book& data) override;
        bool get(size_t id, Book& out);
    };
        
} // namespace mislib

#endif