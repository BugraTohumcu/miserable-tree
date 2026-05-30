#include "UserInterface.h"
#include <iostream>
#include <chrono>
#include <string>
#include <limits>

using namespace std;

namespace mislib
{
    UserInterface::UserInterface(const std::string& dataPath)
        : repo(dataPath) {}

    void UserInterface::printMenu() const {
        cout << "\n--- MISLIB SYSTEM DASHBOARD (Last ID: " << repo.getLastId() << ") ---\n"
             << "1. Search Book by ID\n"
             << "2. Register New Book (Auto-ID)\n"
             << "3. Delete Book Record\n"
             << "4. List All Records (Sorted)\n"
             << "5. Search Books by Author\n"
             << "6. Search Books by Genre\n"
             << "0. Shutdown System\n"
             << "Selection: ";
    }

    void UserInterface::handleSearch() {
        size_t id;
        cout << "Enter Search ID: "; cin >> id;

        auto start = chrono::high_resolution_clock::now();

        Book book;
        bool found = repo.get(id, book);

        auto elapsed = chrono::duration_cast<chrono::microseconds>
                       (chrono::high_resolution_clock::now() - start).count();

        if (found) {
            cout << "\n[RESULT] Record Located Successfully:\n"
                 << book
                 << "Access Latency: " << elapsed << " us\n";
        } else {
            cout << "\n[ERROR] ID " << id << " not found in the index.\n";
        }
    }

    void UserInterface::handleInsert() {
        Book book;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "Title: ";  cin.getline(book.title,  50);
        cout << "Author: "; cin.getline(book.author, 50);
        cout << "Genre: ";  cin.getline(book.genre,  30);
        cout << "Year: ";   cin >> book.date;

        if (repo.create(book))
            cout << "\n[SUCCESS] Book registered with ID: " << repo.getLastId() << "\n";
        else
            cout << "\n[ERROR] Registration failed.\n";
    }

    void UserInterface::handleDelete() {
        size_t id;
        cout << "Enter ID to Delete: "; cin >> id;

        if (repo.remove(id))
            cout << "\n[SUCCESS] ID " << id << " removed from index.\n";
        else
            cout << "\n[ERROR] Target ID does not exist.\n";
    }

    void UserInterface::handleListAll() {
        auto start = chrono::high_resolution_clock::now();

        repo.listAll();

        auto elapsed = chrono::duration_cast<chrono::milliseconds>
                       (chrono::high_resolution_clock::now() - start).count();

        cout << "\n[COMPLETE] Full scan latency: " << elapsed << " ms\n";
    }

    void UserInterface::handleSearchByAuthor() {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        char author[80];
        cout << "Enter Author Name (Exact match): ";
        cin.getline(author, 80);

        auto start = chrono::high_resolution_clock::now();
        std::vector<Book> results = repo.getByAuthor(author);
        auto elapsed = chrono::duration_cast<chrono::microseconds>
                       (chrono::high_resolution_clock::now() - start).count();

        if (!results.empty()) {
            cout << "\n[RESULT] Found " << results.size() << " books by '" << author << "':\n";
            for (const auto& b : results) {
                cout << "- ID: " << b.id << " | Title: " << b.title << " | Genre: " << b.genre << " | Year: " << b.date << "\n";
            }
            cout << "\nAccess Latency: " << elapsed << " us\n";
        } else {
            cout << "\n[ERROR] No books found for author: " << author << "\n";
        }
    }

    void UserInterface::handleSearchByGenre() {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        char genre[40];
        cout << "Enter Genre Name (Exact match): ";
        cin.getline(genre, 40);

        auto start = chrono::high_resolution_clock::now();
        std::vector<Book> results = repo.getByGenre(genre);
        auto elapsed = chrono::duration_cast<chrono::microseconds>
                       (chrono::high_resolution_clock::now() - start).count();

        if (!results.empty()) {
            cout << "\n[RESULT] Found " << results.size() << " books in genre '" << genre << "':\n";
            for (const auto& b : results) {
                cout << "- ID: " << b.id << " | Title: " << b.title << " | Author: " << b.author << " | Year: " << b.date << "\n";
            }
            cout << "\nAccess Latency: " << elapsed << " us\n";
        } else {
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
                continue;
            }

            switch (selection) {
                case 1: handleSearch();  break;
                case 2: handleInsert();  break;
                case 3: handleDelete();  break;
                case 4: handleListAll(); break;
                case 5: handleSearchByAuthor(); break;
                case 6: handleSearchByGenre(); break;
                case 0: cout << "\n[BOOT] System shutting down.\n"; break;
                default: cout << "\n[WARN] Invalid selection.\n"; break;
            }
        }
    }

} // namespace mislib
