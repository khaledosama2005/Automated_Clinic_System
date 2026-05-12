#ifndef TRIE_H
#define TRIE_H

#include "Patient.h"
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

// ============================================================
// Trie — O(m) prefix search where m = prefix length
//
// Why not a linear scan?
// The receptionist searches by name prefix in real time —
// one keypress at a time. A linear scan over all registered
// patients is O(n) per keystroke, so a busy system with 200
// patients in state.json costs 200 comparisons every time the
// receptionist types a letter.
// A Trie descends exactly m nodes to reach the prefix node,
// then collects every Patient* stored beneath it.  The cost
// is O(m + k) where k is the number of matches — completely
// independent of how many patients are registered.
//
// Design:
//   - Each TrieNode holds 26 children (a–z only; names are
//     lower-cased on insert/search so case is irrelevant).
//   - A node can be a "terminal" for multiple patients
//     (two patients can share a name, so we store a
//     vector<Patient*> rather than a single pointer).
//   - The Trie does NOT own Patient* memory —
//     HospitalEngine's unordered_map<int, unique_ptr<Patient>>
//     retains ownership, exactly like PriorityQueue does.
//   - insert() and remove() are O(m).
//   - searchPrefix() returns all matches in O(m + k).
// ============================================================

// ---- TrieNode -----------------------------------------------
struct TrieNode {
    TrieNode*           children[26];
    std::vector<Patient*> patients; // non-owning; patients whose full name ends here

    TrieNode() {
        for (int i = 0; i < 26; ++i)
            children[i] = nullptr;
    }

    ~TrieNode() {
        for (int i = 0; i < 26; ++i)
            delete children[i];
    }

    bool isLeaf() const {
        for (int i = 0; i < 26; ++i)
            if (children[i]) return false;
        return patients.empty();
    }
};


// ---- Trie ---------------------------------------------------
class Trie {
public:
    Trie() : root(new TrieNode()), totalInserted(0) {}
    ~Trie() { delete root; }

    // ---- Insert ---------------------------------------------
    // Inserts p under its lower-cased name.
    // Non-alpha characters (spaces, digits) are kept so that
    // "Sara Ahmed" is a valid key — they map to the node's
    // raw char value rather than the a-z slot, which is fine
    // since we handle them in the index function below.
    // O(m) where m = name length.
    void insert(Patient* p) {
        if (!p) return;
        std::string key = normalize(p->getName());
        TrieNode* cur   = root;
        for (char c : key) {
            int idx = charIndex(c);
            if (idx < 0) continue;          // skip characters outside a-z
            if (!cur->children[idx])
                cur->children[idx] = new TrieNode();
            cur = cur->children[idx];
        }
        cur->patients.push_back(p);
        ++totalInserted;
    }

    // ---- Remove ---------------------------------------------
    // Removes exactly one entry for p (matched by pointer).
    // Leaves the trie structure intact so other patients sharing
    // a name prefix are unaffected.
    // O(m).
    bool remove(Patient* p) {
        if (!p) return false;
        std::string key = normalize(p->getName());
        return removeHelper(root, key, 0, p);
    }

    // ---- Prefix search --------------------------------------
    // Returns all Patient* whose names start with the given prefix.
    // O(m + k) — m to reach the prefix node, k to collect matches.
    std::vector<Patient*> searchPrefix(const std::string& prefix) const {
        std::string key = normalize(prefix);
        TrieNode* node  = findNode(key);
        std::vector<Patient*> results;
        if (!node) return results;          // prefix not found
        collect(node, results);
        return results;
    }

    // ---- Exact search ---------------------------------------
    // Returns all patients registered under exactly this name.
    // O(m).
    std::vector<Patient*> searchExact(const std::string& name) const {
        std::string key = normalize(name);
        TrieNode* node  = findNode(key);
        if (!node) return {};
        return node->patients;              // may be empty if name is only a prefix
    }

    // ---- Accessors ------------------------------------------
    int  getTotalInserted() const { return totalInserted; }
    bool isEmpty()          const { return totalInserted == 0; }

    // ---- Display --------------------------------------------
    // Prints all names currently in the trie (DFS traversal).
    void display() const {
        if (isEmpty()) {
            std::cout << "  (trie is empty)\n";
            return;
        }
        std::string buf;
        displayHelper(root, buf);
    }

private:
    TrieNode* root;
    int       totalInserted;

    // ---- Helpers --------------------------------------------

    // Lower-case the string; non-alpha chars are kept as-is so
    // that spaces in "Sara Ahmed" don't break prefix matching.
    static std::string normalize(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        return out;
    }

    // Maps a-z → 0-25; returns -1 for everything else.
    // Non-alpha characters (spaces, hyphens) are skipped during
    // traversal so "sara ahmed" and "saraahmed" are different keys
    // only in that the space node exists between them.
    // We keep spaces: charIndex(' ') returns -1 so spaces are
    // skipped during descent, which means "sara " and "sara" land
    // on the same terminal node.  This is intentional — receptionists
    // don't type trailing spaces.
    static int charIndex(char c) {
        if (c >= 'a' && c <= 'z') return c - 'a';
        // treat space as a separator we skip, not a node
        return -1;
    }

    // Walk the trie along key; return the terminal node or nullptr.
    TrieNode* findNode(const std::string& key) const {
        TrieNode* cur = root;
        for (char c : key) {
            int idx = charIndex(c);
            if (idx < 0) continue;
            if (!cur->children[idx]) return nullptr;
            cur = cur->children[idx];
        }
        return cur;
    }

    // DFS: collect every Patient* stored in this subtree.
    void collect(TrieNode* node, std::vector<Patient*>& out) const {
        if (!node) return;
        for (Patient* p : node->patients)
            if (p) out.push_back(p);
        for (int i = 0; i < 26; ++i)
            collect(node->children[i], out);
    }

    // Recursive remove: walks to terminal, erases the pointer, prunes
    // empty nodes on the way back up.
    bool removeHelper(TrieNode* node, const std::string& key, int depth, Patient* target) {
        if (!node) return false;

        // Skip non-alpha characters the same way insert does
        while (depth < (int)key.size() && charIndex(key[depth]) < 0)
            ++depth;

        if (depth == (int)key.size()) {
            // At the terminal node — remove the matching pointer
            auto& vec = node->patients;
            auto  it  = std::find(vec.begin(), vec.end(), target);
            if (it == vec.end()) return false;
            vec.erase(it);
            --totalInserted;
            return true;
        }

        int idx = charIndex(key[depth]);
        if (idx < 0 || !node->children[idx]) return false;

        bool removed = removeHelper(node->children[idx], key, depth + 1, target);

        // Prune child if it became completely empty after the remove
        if (removed && node->children[idx]->isLeaf()) {
            delete node->children[idx];
            node->children[idx] = nullptr;
        }
        return removed;
    }

    // DFS display helper — reconstructs the name character by character
    void displayHelper(TrieNode* node, std::string& buf) const {
        if (!node) return;
        for (Patient* p : node->patients) {
            if (p)
                std::cout << "  [" << p->getId() << "] " << p->getName()
                          << " | urgency=" << p->getCondition().category.name
                          << " | clinic=" << p->getClinicId() << "\n";
        }
        for (int i = 0; i < 26; ++i) {
            if (!node->children[i]) continue;
            buf.push_back(static_cast<char>('a' + i));
            displayHelper(node->children[i], buf);
            buf.pop_back();
        }
    }
};

#endif
