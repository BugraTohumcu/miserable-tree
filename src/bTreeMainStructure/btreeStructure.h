#ifndef BTREE_STRUCTURE_H
#define BTREE_STRUCTURE_H

#include <iostream>
#include <vector>
#include <string>

namespace mislib {

    // B-Tree Düğüm Sınıfı
    class BTreeNode {
    public:
        int t;                           // Minimum derece (Minimum degree)
        std::vector<std::string> keys;             // Anahtarlar (Örn: "Wuthering Heights")
        std::vector<std::vector<int>> id_lists;    // Her anahtar için kitap ID'leri listesi (Aynı isimde 2 kitap olabilir)
        std::vector<BTreeNode*> children;     // Alt düğümler
        bool leaf;                       // Yaprak düğüm mü?

        BTreeNode(int _t, bool _leaf) {
            t = _t;
            leaf = _leaf;
        }

        // Arama fonksiyonu
        std::vector<int> search(const std::string& k);

        // Tam dolu olmayan düğüme ekleme
        void insertNonFull(const std::string& k, int id);

        // Dolu olan çocuğu (y) ikiye bölme fonksiyonu (Mülakatta kesin sorulur!)
        void splitChild(int i, BTreeNode* y);
    };

    // B-Tree Ana Sınıfı
    class BTree {
    private:
        BTreeNode* root;
        int t;

    public:
        BTree(int _t) {
            root = nullptr;
            t = _t;
        }

        // Ağaca yeni bir anahtar (key) ve kitap ID'si (id) ekleme
        void insert(const std::string& k, int id);

        // Ağaçta anahtar arama
        std::vector<int> search(const std::string& k) {
            if (root == nullptr) return {};
            return root->search(k);
        }
    };

} // namespace mislib

#endif // BTREE_STRUCTURE_H