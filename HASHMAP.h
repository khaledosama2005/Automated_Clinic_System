#ifndef HASHMAP_H
#define HASHMAP_H

#include <string>
#include <iostream>

struct Node {
    std::string key;
    int         value;
    Node*       next;

    Node(const std::string& k, int v) : key(k), value(v), next(nullptr) {}
};


class HashMap {
public:
    
    explicit HashMap(int initialCapacity = 16)
        : capacity(initialCapacity), size(0)
    {
        table = new Node*[capacity];
        for (int i = 0; i < capacity; ++i)
        table[i] = nullptr;
    }

    ~HashMap() { clear(); delete[] table; }

    
    void insert(const std::string& key, int value) {
        if (loadFactor() >= MAX_LOAD) 
        rehash();

        int idx   = hash(key);
        Node* cur = table[idx];

        while (cur) {
            if (cur->key == key) {
                cur->value = value;
                return;
            }
            cur = cur->next;
        }

        Node* node  = new Node(key, value);
        node->next  = table[idx];
        table[idx]  = node;
        ++size;
    }

    
    int* search(const std::string& key) {
        int idx   = hash(key);
        Node* cur = table[idx];
        while (cur) {
            if (cur->key == key) 
            return &cur->value;
            cur = cur->next;
        }
        return nullptr;
    }

   
    bool remove(const std::string& key) {
        int idx    = hash(key);
        Node* cur  = table[idx];
        Node* prev = nullptr;

        while (cur) {
            if (cur->key == key) {
                if (prev) prev->next = cur->next;
                else      table[idx] = cur->next;
                delete cur;
                --size;
                return true;
            }
            prev = cur;
            cur  = cur->next;
        }
        return false;
    }

    
    int  getSize()     const { return size; }
    int  getCapacity() const { return capacity; }
    bool isEmpty()     const { return size == 0; }

    
    void display() const {
        for (int i = 0; i < capacity; ++i) {
            if (!table[i]) continue;
            std::cout << "Bucket " << i << ": ";
            Node* cur = table[i];
            while (cur) {
                std::cout << "[" << cur->key << " -> " << cur->value << "]";
                if (cur->next) std::cout << " -> ";
                cur = cur->next;
            }
            std::cout << "\n";
        }
    }

private:
    Node** table;
    int    capacity;
    int    size;

    static const float MAX_LOAD = 0.75;

    
   int hash(const std::string& key) const {
    unsigned long h = 5381;

    for (size_t i = 0; i < key.length(); i++) {
        h = ((h << 5) + h) + (unsigned char)key[i];
    }

    return (int)(h % (unsigned long)capacity);
}

    float loadFactor() const {
        return (float)size / (float)capacity;
    }

    
    void rehash() {
        int    oldCap   = capacity;
        Node** oldTable = table;

        capacity = oldCap * 2;
        table    = new Node*[capacity];
        for (int i = 0; i < capacity; ++i) table[i] = nullptr;
        size = 0;

        for (int i = 0; i < oldCap; ++i) {
            Node* cur = oldTable[i];
            while (cur) {
                insert(cur->key, cur->value);
                Node* tmp = cur->next;
                delete cur;
                cur = tmp;
            }
        }
        delete[] oldTable;
    }


    void clear() {
        for (int i = 0; i < capacity; ++i) {
            Node* cur = table[i];
            while (cur) {
                Node* tmp = cur->next;
                delete cur;
                cur = tmp;
            }
            table[i] = nullptr;
        }
        size = 0;
    }
};

#endif 