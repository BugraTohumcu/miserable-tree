#ifndef INDEX_REPO
#define INDEX_REPO

#include "./CrudRepository.h"
#include "../index/IndexEntry.h"
#include <fstream>

namespace mislib
{
   class IndexRepo: public CrudRepository<IndexEntry>{
    
        private:
            std::fstream indexFile;

        public:
            IndexRepo(const char* path);
            bool create(const IndexEntry& data) override;
   };
} // namespace mislib

#endif