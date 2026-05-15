#ifndef BOOK
#define BOOK

#include <string>
#include <ostream>

#pragma pack(push, 1)
namespace mislib
{
    struct Book{
    int id;
    char title[100];
    char author[80];
    char genre[40];
    int date;
    
    friend std::ostream& operator<<(std::ostream& os, const Book& book){
        os << "id: " << book.id
           << "\ntitle: " << book.title
           << "\nauthor: " << book.author
           << "\ngenre: " << book.genre
           << "\ndate: " << book.date
           << "\n\n";
        return os;
    }
};

} // namespace mislib

#pragma pack(pop)
#endif