#ifndef BOOK_PARSER
#define BOOK_PARSER

// Imports
#include <cstring>
#include <sstream>
#include <cstdlib> 
#include <array>
#include <charconv>
#include <memory>
#include "../entity/BookEntity.h"


#define DELIMETER ','


namespace mislib
{
    class BookParser{
        public:
        Book parseToBook(const char* line) {
        
            //Comma table for direct access
            std::array<int,4> commaTable = this->findDelimeters(line);
            Book book{};
            if(commaTable[0] <= -1) return book;
        
            // convert id to int  
            std::from_chars(line, line + commaTable[0], book.id);
        
            copyField(book.title,  line, commaTable[0] + 1, commaTable[1], sizeof(book.title));
            copyField(book.author, line, commaTable[1] + 1, commaTable[2], sizeof(book.author));
            copyField(book.genre,  line, commaTable[2] + 1, commaTable[3], sizeof(book.genre));
        
            // Start after the 4th comma and skip any leading spaces      
            const char* dateStart = line + commaTable[3] + 1;
            while (*dateStart == ' ') dateStart++;
        
            // Convert date to int 
            std::from_chars(dateStart, line + std::strlen(line), book.date);
        
            return book;
        }
        
    
    private:
    void copyField(char* dest, const char* src, int start, int end, size_t destSize){

        // Skip the spaces at the begining
        while (start < end && src[start] == ' ') {
            start++;
        }

        // Prevent buffer overflow by truncating
        int len = end - start;
        if(len >= destSize){
            len = destSize - 1;
        }

        std::memcpy(dest, src + start, len);
        dest[len] = '\0';
    }

    std::array<int, 4> findDelimeters(const char* line){
        const char* start = line;
        const char* current = start;
        int len = std::strlen(line);
        std::array<int, 4> delimeterTable = {0}; 
        int nc = 0;

        if(*current == '\0') return {-1,-1,-1,-1};
        // Skip spaces 
        while (*current && *current == ' ') current++;
        
        // Find the first delimeter for id 
        while (*current && *current != ',') current++;
            
        // Safe exit if there is no comma
        if (*current == '\0') {
            return delimeterTable; 
        }

        // ID delimeter location
        delimeterTable[nc++] = (int) (current - start); 
            
        // Read from the end to isolate internal commas inside the book title
        nc = 3;
        current = start + len - 1;
        while(nc != 0 && current >= start){
            if(*current == ','){
                delimeterTable[nc--] = (int) (current - start);
            }
            current--;
        }
            
        return delimeterTable;
    }
};
} // namespace mislib



#endif
