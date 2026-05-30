#ifndef BOOK_REPO
#define BOOK_REPO

#include <fstream>
#include <optional>
#include <string>
#include <vector>        
#include "CrudRepository.h"
#include "../entity/BookEntity.h"
#include "../index/IndexManager.h"
#include "../index/BPlusTree.h" 

namespace mislib
{
    class BookRepo : public CrudRepository<Book>
    {
    public:
        /**
         * @brief Opens the data file and loads or builds the index.
         * @param dataPath Absolute or relative path to the raw data file.
         * @throws std::runtime_error If the data file cannot be opened.
         */
        explicit BookRepo(const std::string& dataPath);

        /**
         * @brief Persists the current index state before destruction.
         */
        ~BookRepo();

        // --- CrudRepository interface ---

        /**
         * @brief Appends a new book record to the data file and registers it in the index.
         * @param data Book to be persisted. Its id field will be auto-assigned.
         * @return true if the record was successfully inserted.
         */
        bool create(const Book& data) override;

        /**
         * @brief Locates a book by its unique ID using the B+ Tree index.
         * @param id  Unique identifier of the target book.
         * @param out Output parameter populated on successful lookup.
         * @return true if the record was found and parsed successfully.
         */
        bool get(size_t id, Book& out) override;

        /**
         * @brief Updates an existing book record in the data file.
         * @param data Book record containing the updated fields. ID must match an existing entry.
         * @return true if the record was found and updated successfully.
         * @note Soft-deleted records cannot be updated.
         */
        bool update(const Book& data) override;

        /**
         * @brief Soft-deletes a book entry by marking it in the data file and removing it from the index.
         * @param id Unique identifier of the book to be removed.
         * @return true if the entry existed and was marked as deleted.
         * @note This does not physically delete the record from the data file.
         */
        bool remove(size_t id) override;

        /**
         * @brief Traverses all index entries in sorted order and prints each record.
         * @details Follows the B+ Tree leaf linked list to guarantee ID-sorted output
         * without loading the entire dataset into memory.
         */
        void listAll();

        // --- Secondary Index Operations ---

        /**
         * @brief Retrieves a collection of books written by a specific author.
         * @details Utilizes the secondary B+ Tree index for O(log N) retrieval, 
         * followed by primary B+ Tree lookups for the actual record extraction.
         * @param author The exact name of the author to search for.
         * @return A vector containing all books matching the given author name.
         */
        std::vector<Book> getByAuthor(const std::string& author);

        /**
         * @brief Retrieves a collection of books belonging to a specific genre.
         * @details Utilizes the secondary B+ Tree index for O(log N) retrieval, 
         * followed by primary B+ Tree lookups for the actual record extraction.
         * @param genre The exact genre category to search for.
         * @return A vector containing all books matching the given genre.
         */
        std::vector<Book> getByGenre(const std::string& genre);

        size_t getLastId() const { return lastId; }

    private:
        std::ifstream readFile;
        std::ofstream writeFile;
        std::string   dataPath;
        IndexManager  indexManager; // Primary Index
        size_t        lastId = 0;

        // --- SECONDARY INDICES ---
        mislib::BPlusTree authorTree;
        mislib::BPlusTree genreTree;

        // Marks a line as soft-deleted in the data file by prefixing it with '!'.
        static constexpr char DELETED_MARKER = '!';
    };

} // namespace mislib

#endif
