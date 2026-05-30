        #include "UserInterface.h"
#include <iostream>
#include <chrono>
#include <string>
#include <limits>
#include <cstring> 

using namespace std;

namespace mislib
{
    UserInterface::UserInterface(const std::string& dataPath)
        : repo(dataPath) {
    }

    void UserInterface::printMenu() const {
        cout << "\n==================================================\n"
            << "         MISLIB DISK-BASED DBMS DASHBOARD         \n"
            << "==================================================\n"
            << " [Active Last ID: " << repo.getLastId() << "]\n\n"
            << "  1. Search Book by ID\n"
            << "  2. Register New Book (Auto-ID)\n"
            << "  3. Update Existing Book\n"
            << "  4. Delete Book Record (Soft-Delete)\n"
            << "  5. List All Active Records (Sorted)\n"
            << "  6. Search Books by Author\n"
            << "  7. Search Books by Genre\n"
            << "  0. Shutdown System\n"
            << "--------------------------------------------------\n"
            << "Selection: ";
    }

    void UserInterface::handleSearch() {
        size_t id;
        cout << "Enter Search ID: "; cin >> id;

        auto start = chrono::high_resolution_clock::now();
        Book book;
        bool found = repo.get(id, book);
        auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();

        if (found) {
            cout << "\n[RESULT] Record Located Successfully (Latency: " << elapsed << " us):\n"
                << "--------------------------------------------------\n" << book;
        }
        else {
            cout << "\n[ERROR] Book with ID " << id << " not found or deleted. (Latency: " << elapsed << " us)\n";
        }
    }

    void UserInterface::handleInsert() {
        Book book;
        book.id = 0; 

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

       
        cout << "Enter Title: ";  cin.getline(book.title, sizeof(book.title));
        cout << "Enter Author: "; cin.getline(book.author, sizeof(book.author));
        cout << "Enter Genre: ";  cin.getline(book.genre, sizeof(book.genre));
        cout << "Enter Year: ";   cin >> book.date;

        auto start = chrono::high_resolution_clock::now();
        bool success = repo.create(book);
        auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();

        if (success) {
            cout << "\n[SUCCESS] Book registered successfully! (Latency: " << elapsed << " us)\n";
        }
        else {
            cout << "\n[ERROR] Failed to write record to disk.\n";
        }
    }

    void UserInterface::handleUpdate() {
        size_t id;
        cout << "Enter Book ID to Update: "; cin >> id;

        Book oldBook;
        if (!repo.get(id, oldBook)) {
            cout << "\n[ERROR] Cannot update. Book ID " << id << " does not exist.\n";
            return;
        }

        cout << "\n[INFO] Current Data: " << oldBook.title << " by " << oldBook.author << "\n";

        Book newBook;
        newBook.id = id; 

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter New Title (Leave empty to keep current): ";
        cin.getline(newBook.title, sizeof(newBook.title));
        if (strlen(newBook.title) == 0) strcpy(newBook.title, oldBook.title);

        cout << "Enter New Author (Leave empty to keep current): ";
        cin.getline(newBook.author, sizeof(newBook.author));
        if (strlen(newBook.author) == 0) strcpy(newBook.author, oldBook.author);

        cout << "Enter New Genre (Leave empty to keep current): ";
        cin.getline(newBook.genre, sizeof(newBook.genre));
        if (strlen(newBook.genre) == 0) strcpy(newBook.genre, oldBook.genre);

        cout << "Enter New Year (0 to keep current): ";
        cin >> newBook.date;
        if (newBook.date == 0) newBook.date = oldBook.date;

        auto start = chrono::high_resolution_clock::now();
        bool success = repo.update(newBook);
        auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();

        if (success) {
            cout << "\n[SUCCESS] Book ID " << id << " updated on disk! (Latency: " << elapsed << " us)\n";
        }
        else {
            cout << "\n[ERROR] Failed to update record.\n";
        }
    }

    void UserInterface::handleDelete() {
        size_t id;
        cout << "Enter Book ID to Delete: "; cin >> id;

        auto start = chrono::high_resolution_clock::now();
        bool success = repo.remove(id);
        auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();

        if (success) {
            cout << "\n[SUCCESS] Record soft-deleted (marked with '" << '!' << "'). (Latency: " << elapsed << " us)\n";
        }
        else {
            cout << "\n[ERROR] Record not found or already deleted.\n";
        }
    }

    void UserInterface::handleListAll() {
        cout << "\n--- ALL ACTIVE RECORDS (B+ TREE IN-ORDER TRAVERSAL) ---\n";
        auto start = chrono::high_resolution_clock::now();
        repo.listAll();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();
        cout << "--------------------------------------------------\n"
            << "Total Traversal Latency: " << elapsed << " us\n";
    }

    void UserInterface::handleSearchByAuthor() {
        string author;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Author Name: "; getline(cin, author);

        auto start = chrono::high_resolution_clock::now();
        std::vector<Book> results = repo.getByAuthor(author);
        auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();

        if (!results.empty()) {
            cout << "\n[RESULT] Found " << results.size() << " books (Secondary Index Latency: " << elapsed << " us):\n";
            for (const auto& b : results) {
                cout << "  - ID: " << b.id << " | Title: " << b.title << " | Year: " << b.date << "\n";
            }
        }
        else {
            cout << "\n[ERROR] No books found for author: " << author << "\n";
        }
    }

    void UserInterface::handleSearchByGenre() {
        string genre;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Genre: "; getline(cin, genre);

        auto start = chrono::high_resolution_clock::now();
        std::vector<Book> results = repo.getByGenre(genre);
        auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();

        if (!results.empty()) {
            cout << "\n[RESULT] Found " << results.size() << " books (Secondary Index Latency: " << elapsed << " us):\n";
            for (const auto& b : results) {
                cout << "  - ID: " << b.id << " | Title: " << b.title << " | Author: " << b.author << " | Year: " << b.date << "\n";
            }
        }
        else {
            cout << "\n[ERROR] No books found in genre: " << genre << "\n";
        }
    }

    void UserInterface::run() {
        int selection = -1;
        while (selection != 0) {
            printMenu();

            if (!(cin >> selection)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "\n[ERROR] Invalid selection. Please enter a number.\n";
                continue;
            }

            switch (selection) {
            case 1: handleSearch();         break;
            case 2: handleInsert();         break;
            case 3: handleUpdate();         break;
            case 4: handleDelete();         break;
            case 5: handleListAll();        break;
            case 6: handleSearchByAuthor(); break;
            case 7: handleSearchByGenre();  break;
            case 0: cout << "\n[SYSTEM] Safely flushing indices to disk. Shutting down DBMS...\n"; break;
            default: cout << "\n[ERROR] Unknown option. Try again.\n"; break;
            }
        }
    }
} // namespace mislib
