#include "btreeStructure.h"

using namespace std;

namespace mislib {

    // --- 1. ARAMA (SEARCH) ---
    vector<int> BTreeNode::search(const string& k) {
        int i = 0;
        // Anahtarı k'dan büyük veya eşit olan ilk indeksi bul
        while (i < keys.size() && k > keys[i]) {
            i++;
        }

        // Eğer anahtarı bulduysak, o anahtara ait ID listesini döndür
        if (i < keys.size() && keys[i] == k) {
            return id_lists[i];
        }

        // Eğer yaprak düğümdeysek ve bulamadıysak boş liste döndür
        if (leaf) {
            return {};
        }

        // Çocuğa in ve aramaya devam et
        return children[i]->search(k);
    }

    // --- 2. EKLEME (INSERT - KÖK YÖNETİMİ) ---
    void BTree::insert(const string& k, int id) {
        if (root == nullptr) {
            // Ağaç boşsa yeni kök oluştur
            root = new BTreeNode(t, true);
            root->keys.push_back(k);
            root->id_lists.push_back({id});
        } else {
            // Kök tam doluysa (2*t - 1 anahtar varsa), kökü bölmemiz (split) gerekir
            if (root->keys.size() == 2 * t - 1) {
                BTreeNode* newRoot = new BTreeNode(t, false);
                newRoot->children.push_back(root);
                newRoot->splitChild(0, root);
                
                // Bölündükten sonra hangi çocuğa inileceğini seç
                int i = 0;
                if (newRoot->keys[0] < k) {
                    i++;
                }
                newRoot->children[i]->insertNonFull(k, id);
                root = newRoot; // Yeni kök ataması
            } else {
                // Kök dolu değilse doğrudan ekle
                root->insertNonFull(k, id);
            }
        }
    }

    // --- 3. DOLU OLMAYAN DÜĞÜME EKLEME ---
    void BTreeNode::insertNonFull(const string& k, int id) {
        int i = keys.size() - 1;

        // Eğer aynı anahtar zaten varsa, ID listesine ekleyip çıkalım
        for (int j = 0; j <= i; j++) {
            if (keys[j] == k) {
                id_lists[j].push_back(id);
                return;
            }
        }

        if (leaf) {
            // Yaprak düğümdeysek, doğru yeri bulana kadar elemanları kaydır ve ekle
            keys.push_back(""); // Yer aç
            id_lists.push_back({});
            while (i >= 0 && keys[i] > k) {
                keys[i + 1] = keys[i];
                id_lists[i + 1] = id_lists[i];
                i--;
            }
            keys[i + 1] = k;
            id_lists[i + 1] = {id};
        } else {
            // Yaprak değilsek, inilecek doğru çocuğu bul
            while (i >= 0 && keys[i] > k) {
                i--;
            }
            i++;

            // İneceğimiz çocuk tam doluysa önce onu böl (split)
            if (children[i]->keys.size() == 2 * t - 1) {
                splitChild(i, children[i]);
                // Bölündükten sonra anahtarın sağa mı sola mı gideceğine karar ver
                if (keys[i] < k) {
                    i++;
                }
            }
            children[i]->insertNonFull(k, id);
        }
    }

    // --- 4. DÜĞÜM BÖLME (SPLIT CHILD) - Mülakatın Yıldızı! ---
    void BTreeNode::splitChild(int i, BTreeNode* y) {
        // y düğümü (dolu olan düğüm) ikiye bölünecek ve sağ yarısı z düğümüne gidecek
        BTreeNode* z = new BTreeNode(y->t, y->leaf);
        
        // t-1 tane anahtarı z'ye kopyala
        for (int j = 0; j < t - 1; j++) {
            z->keys.push_back(y->keys[j + t]);
            z->id_lists.push_back(y->id_lists[j + t]);
        }

        // Eğer y yaprak değilse, t tane çocuğu da z'ye kopyala
        if (!y->leaf) {
            for (int j = 0; j < t; j++) {
                z->children.push_back(y->children[j + t]);
            }
        }

        // y'nin boyutunu güncelle (Fazlalıkları vektörden sil)
        y->keys.resize(t - 1);
        y->id_lists.resize(t - 1);
        if (!y->leaf) {
            y->children.resize(t);
        }

        // Yeni çocuğu (z) mevcut düğümün (this) çocuklarına ekle
        children.insert(children.begin() + i + 1, z);

        // Ortanca anahtarı y'den yukarıya (mevcut düğüme) taşı
        keys.insert(keys.begin() + i, y->keys[t - 1]);
        id_lists.insert(id_lists.begin() + i, y->id_lists[t - 1]);
    }

} // namespace mislib