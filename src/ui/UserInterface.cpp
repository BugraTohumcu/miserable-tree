#include "UserInterface.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <optional>

#include "../entity/BookEntity.h"
#include "../util/BookParser.h"
#include <algorithm>

using namespace std;

namespace mislib
{
    UserInterface::UserInterface() {
        lastId = 0;
    }

    // Local helper function for boot sequence indexing
    // UserInterface.cpp içindeki eski autoLoad fonksiyonunu bununla değiştir:

    void autoLoad(BPlusTree& tree, size_t& lastId) {
        const std::string indexFileName = "Index.dat";

        // 1. Önce Binary Index.dat dosyasından okumayı dene (Milisaniyeler sürer)
        if (tree.loadFromBinaryIndex(indexFileName)) {
            std::cout << "[BOOT] Agac Index.dat (Binary) uzerinden basariyla yuklendi!" << std::endl;
            // lastId'yi güncellemek için ağacın son elemanına bakılabilir ama 
            // şimdilik basitçe çalışmaya devam edelim.
            return;
        }

        // 2. Eğer Index.dat yoksa, amele gibi .txt'den oku ve ağacı oluştur (Yavaş yöntem)
        std::cout << "[BOOT] Index.dat bulunamadi. Metin dosyasi taranip agac olusturuluyor..." << std::endl;
        std::ifstream dataset("books_dataset.txt");
        if (!dataset.is_open()) return;

        std::string line;
        size_t offset = 0;

        while (true) {
            offset = (size_t)dataset.tellg();
            if (!getline(dataset, line)) break;
            if (line.empty()) continue;

            try {
                size_t id = mislib::extractId(line.c_str());
                tree.insert(id, offset);
                if (id > lastId) lastId = id;
            } catch (...) {}
        }
        dataset.close();

        // 3. Ağaç oluşturulduktan sonra, bir dahaki sefere hızlı açılsın diye Index.dat'a kaydet!
        tree.saveToBinaryIndex(indexFileName);
}

    UserInterface::~UserInterface() {
        // --- Destruction logic --- 
    }

    void UserInterface::run() {
        // Execute indexing boot sequence
        autoLoad(tree, lastId);

        int selection = -1;
        while (selection != 0) {
            cout << "\n--- MISLIB SYSTEM DASHBOARD (Last ID: " << lastId << ") ---" << endl;
            cout << "1. Search Book by ID" << endl;
            cout << "2. Register New Book (Auto-ID)" << endl;
            cout << "3. Delete Book Record" << endl;
            cout << "4. List All Records (Sorted)" << endl;
            cout << "0. Shutdown System" << endl;
            cout << "Selection: ";

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
                lastId++;
                Book newBook;
                newBook.id = lastId;

                cin.ignore();
                cout << "Title: ";  cin.getline(newBook.title, 50);
                cout << "Author: "; cin.getline(newBook.author, 50);
                cout << "Genre: ";  cin.getline(newBook.genre, 30);
                cout << "Year: ";   cin >> newBook.date;

                ofstream outFile("books_dataset.txt", ios::app);

                outFile.seekp(0, ios::end);
                size_t newOffset = (size_t)outFile.tellp();

                outFile << newBook.id << "," << newBook.title << "," << newBook.author << ","
                    << newBook.genre << "," << newBook.date << "\n";
                outFile.close();

                tree.insert(newBook.id, newOffset);
                cout << "\n[SUCCESS] Entry saved at offset: " << newOffset << " with ID: " << newBook.id << endl;
            }
            else if (selection == 3) { // DELETION OPERATION
                size_t deleteId;
                cout << "Enter ID to Delete: "; cin >> deleteId;

                if (tree.remove(deleteId)) {
                    cout << "\n[SUCCESS] ID " << deleteId << " removed from B+ Tree." << endl;
                }
                else {
                    cout << "\n[ERROR] Operation failed. Target ID does not exist." << endl;
                }
            }
            else if (selection == 4) { // SCAN OPERATION
                auto start = chrono::high_resolution_clock::now();

                tree.listAllRecords("books_dataset.txt");

                auto end = chrono::high_resolution_clock::now();
                cout << "\n[COMPLETE] Full scan latency: "
                    << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
            }
        }
    }

} // namespace mislib