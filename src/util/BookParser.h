#ifndef BOOK_PARSER
#define BOOK_PARSER

// Imports
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <cstdlib> 
#include "../entity/BookEntity.h"


namespace mislib
{
    class BookParser{
    public:


    static Book parseToBook(const std::string& line) {
        Book book = {}; 
        
        const char* ptr = line.c_str(); 

        book.id = std::atoi(ptr); 

        while (*ptr != '\0' && *ptr != ',') ptr++;
        if (*ptr == ',') ptr++; 
        if (*ptr == ' ') ptr++; 

        int i = 0;
        while (*ptr != '\0' && *ptr != ',') {
            if (i < sizeof(book.title) - 1) {
                book.title[i++] = *ptr;
            }
            ptr++;
        }
        book.title[i] = '\0'; 
        
        if (*ptr == ',') ptr++;
        if (*ptr == ' ') ptr++;

        i = 0;
        while (*ptr != '\0' && *ptr != ',') {
            if (i < sizeof(book.author) - 1) {
                book.author[i++] = *ptr;
            }
            ptr++;
        }
        book.author[i] = '\0';

        if (*ptr == ',') ptr++;
        if (*ptr == ' ') ptr++;

        i = 0;
        while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') { // Satır sonuna kadar
            if (i < sizeof(book.genre) - 1) {
                book.genre[i++] = *ptr;
            }
            ptr++;
        }
        book.genre[i] = '\0';

        return book;
    }
        static size_t parseIdFromLine(const std::string& line){
            const char* p1 = line.c_str();
            std::string token;
            
            //Reach the first comma
            while (*p1 != ','){
                p1++;
            }
            


            std::stringstream ss(line);
            if(std::getline(ss,token)){
                if (!token.empty() && token[0] == ' ')
                    token.erase(0, 1);
            }

            return std::stoi(token);

        }
};
} // namespace mislib



#endif
