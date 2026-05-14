#ifndef INDEX_REPO
#define INDEX_REPO


#include "./CrudRepository.h"
#include "../index/IndexEntry.h"
#include <fstream>
#include <iostream>

namespace mislib
{
   class IndexRepo: public CrudRepository<IndexEntry>{
    
        private:
            std::fstream indexFile;

        public:

            IndexRepo(const char* path) {
                indexFile.open(path, std::ios::in | std::ios::out | std::ios::binary);

                // Create index file if not exist
                if (!indexFile.is_open()) {
                    indexFile.clear(); 
                    indexFile.open(path, std::ios::out | std::ios::binary); 
                    indexFile.close();
                    indexFile.open(path, std::ios::in | std::ios::out | std::ios::binary); 
                }

                if (!indexFile.is_open()) {
                    throw std::runtime_error("Index file could not be opened or created.");
                }
            }

            bool create(const IndexEntry& data) override {
                try {
                    indexFile.seekp(0, std::ios::end);
                    indexFile.write(reinterpret_cast<const char*>(&data), sizeof(IndexEntry));

                    if (indexFile.fail()) {
                        return false;
                    }

                    indexFile.flush();

                    return true;
                } catch (const std::exception& e) {
                    std::cerr << "Index write error: " << e.what() << std::endl;
                    return false;
                }
            }
        

    };
} // namespace mislib



#endif