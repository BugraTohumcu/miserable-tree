#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <optional>

// Internal project headers
#include "entity/BookEntity.h"
#include "index/BPlusTree.h"
#include "persistence/BookRepository.h"
#include "util/BookParser.h"

using namespace std;
using namespace mislib;

// Global tracker for the latest record ID
size_t lastId = 0;

/** * System Boot Logic: Automatically populates the B+ Tree from the dataset
 */
void autoLoad(BPlusTree& tree) {
    ifstream dataset("books_dataset.txt");
    if (!dataset.is_open()) return;

    string line;
    size_t offset = 0;

    // Iterate through the entire file to map keys to their physical disk positions
    while (true) {
        // Capture the start position of the current line
        offset = (size_t)dataset.tellg();

        if (!getline(dataset, line)) break;
        if (line.empty()) continue;

        try {
            // Extract unique ID and sync with the B+ Tree index
            size_t id = mislib::extractId(line.c_str());
            tree.insert(id, offset);

            // Maintain the highest ID for upcoming manual insertions
            if (id > lastId) lastId = id;
        }
        catch (...) {
            // Skip corrupted or malformed lines
        }
    }
    dataset.close();
}

int main() {
    // Initialize repository and indexing engine
    BookRepo repo("books_dataset.txt");
    BPlusTree tree;

    // Trigger automatic index population on startup
    autoLoad(tree);

    int selection = -1;
    while (selection != 0) {
        cout << "\n--- MISLIB SYSTEM DASHBOARD (Last ID: " << lastId << ") ---" << endl;
        cout << "1. Search Book by ID" << endl;
        cout << "2. Register New Book (Auto-ID)" << endl;
        cout << "3. Delete Book Record" << endl;
        cout << "0. Shutdown System" << endl;
        cout << "Selection: ";

        // Validate user input type
        if (!(cin >> selection)) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }

        if (selection == 1) { // SEARCH OPERATION
            size_t id;
            cout << "Enter Search ID: "; cin >> id;

            auto start = chrono::high_resolution_clock::now();
            auto offset = tree.search(id);

            if (offset.has_value()) {
                // Point-to-point disk access using the captured offset
                ifstream file("books_dataset.txt");
                file.seekg(*offset);

                string line;
                if (getline(file, line)) {
                    Book b = mislib::parseToBook(line.c_str());
                    auto end = chrono::high_resolution_clock::now();

                    cout << "\n[RESULT] Record Located Successfully:" << endl;
                    cout << b;
                    cout << "Access Latency: " << chrono::duration_cast<chrono::microseconds>(end - start).count() << " us" << endl;
                }
                file.close();
            }
            else cout << "\n[ERROR] ID " << id << " not found in the index." << endl;
        }
        else if (selection == 2) { // INSERTION OPERATION
            // Increment ID automatically based on the last known peak
            lastId++;
            Book newBook;
            newBook.id = lastId;

            cin.ignore(); // Flush newline from previous cin
            cout << "Title: ";  cin.getline(newBook.title, 50);
            cout << "Author: "; cin.getline(newBook.author, 50);
            cout << "Genre: ";  cin.getline(newBook.genre, 30);
            cout << "Year: ";   cin >> newBook.date;

            // Persistent storage: Append the new record to the CSV file
            ofstream outFile("books_dataset.txt", ios::app);

            // Sync current write pointer for indexing
            outFile.seekp(0, ios::end);
            size_t newOffset = (size_t)outFile.tellp();

            // Append with standardized CSV format
            outFile << newBook.id << "," << newBook.title << "," << newBook.author << ","
                << newBook.genre << "," << newBook.date << "\n";
            outFile.close();

            // Synchronize the B+ Tree with the newly written record
            tree.insert(newBook.id, newOffset);
            cout << "\n[SUCCESS] Entry saved at offset: " << newOffset << " with ID: " << newBook.id << endl;
        }
        else if (selection == 3) { // DELETION OPERATION
            size_t deleteId;
            cout << "Enter ID to Delete: "; cin >> deleteId;

            // Remove from the logical index structure
            if (tree.remove(deleteId)) {
                cout << "\n[SUCCESS] ID " << deleteId << " removed from B+ Tree." << endl;
            }
            else {
                cout << "\n[ERROR] Operation failed. Target ID does not exist." << endl;
            }
        }
    }

    return 0;
}
