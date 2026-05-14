#ifndef BOOK_PARSER
#define BOOK_PARSER

// Imports
#include <fstream>
#include <iostream>
#include "../entity/BookEntity.h"
#include <cstring>
#include <sstream>
#include <vector>


namespace mislib
{
    class BookParser{
    public:

        static Book parseToBook(const std::string& line) {
            Book book;
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
