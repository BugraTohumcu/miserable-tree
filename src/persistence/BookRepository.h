#ifndef BOOK_REPO
#define BOOK_REPO


#include <fstream>
#include <memory>
#include "CrudRepository.h"
#include "../entity/BookEntity.h"
#include "../index/BookIndex.h"


namespace mislib
{
    template <typename T>
    class BookRepo : public CrudRepository<Book>{ 

        private: 
            std::fstream dataFile;
            mislib::BookIndex index;

        public:
            BookRepo(const char* path){
                this->dataFile = std::fstream(path, std::ios::in | std::ios::out);
                if (!dataFile.is_open()){
                    throw std::runtime_error("Data file open failed");
                }
            }

            bool create(const T& data) override{
                return false;
            }

            bool get(size_t id, T& out){
                size_t offset = index.getIndex(id);
                
                dataFile.clear();
                dataFile.seekg(0);

                std::string line;

                while (std::getline(dataFile, line))
                {
                    Book book = BookParser::parseToBook(line);

                    if (book.id == id){
                        out = book;
                        return true;
                    }
                }

                return false;
            }
                };

            
            } // namespace mislib


#endif