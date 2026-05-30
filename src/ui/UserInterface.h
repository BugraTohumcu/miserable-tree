#ifndef USER_INTERFACE
#define USER_INTERFACE

#include "../persistence/BookRepository.h"
#include <string>

namespace mislib
{
    class UserInterface
    {
    public:
        /**
         * @brief Initializes the interface and boots the repository layer.
         * @param dataPath Absolute or relative path to the raw data file.
         */
        explicit UserInterface(const std::string& dataPath);

        /**
         * @brief Runs the main menu loop until the user requests shutdown.
         */
        void run();

    private:
        mislib::BookRepo repo;
        

        void handleSearch();
        void handleInsert();
        void handleDelete();
        void handleListAll();
        void printMenu() const;
        void handleSearchByAuthor();
        void handleSearchByGenre();
    };

} // namespace mislib

#endif