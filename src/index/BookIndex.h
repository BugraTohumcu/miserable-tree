#ifndef BOOK_INDEX
#define BOOK_INDEX

#include <cstdlib>
#include <string>
#include <vector>

// dosyayı  sistemine bağlıyoruz
#include "../bTreeMainStructure/btreeStructure.h" 

namespace mislib
{   
    class BookIndex{
        private:
         
            BTree titleIndex; 

        public:
            
            BookIndex() : titleIndex(50) {} 

            
            size_t getIndex(size_t id){
                return id;
            }

            // --- YENİ EKLENENLER ---
            //  Parser'ı okuduğu kitapları senin ağacına buradan ekleyecek
            void insertToTitleIndex(const std::string& title, int id) {
                titleIndex.insert(title, id);
            }

            // Arayüz kitap arattığında senin ağacın ID'leri buradan döndürecek
            std::vector<int> getIdsByTitle(const std::string& title) {
                return titleIndex.search(title);
            }
    };
}// namespace mislib

#endif
