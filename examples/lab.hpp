#pragma once
#include <iostream>
#include <iomanip>
#include <stdexcept>

namespace classroom {
inline int seqSearch(const int list[], int length, int item, int& comparisons) {
    comparisons = 0;
    for (int i = 0; i < length; ++i) {
        ++comparisons;
        if (list[i] == item) return i;
    }
    return -1;
}

inline int binarySearch(const int list[], int length, int item, int& comparisons) {
    comparisons = 0;
    int first = 0, last = length - 1;
    while (first <= last) {
        const int mid = first + (last - first) / 2;
        ++comparisons;
        const int value = list[mid];
        if (value == item) return mid;
        if (value > item) last = mid - 1;
        else first = mid + 1;
    }
    return -1;
}

inline int interpolationSearch(const int list[], int length, int item, int& comparisons) {
    comparisons = 0;
    int low = 0, high = length - 1;
    while (low <= high && list[low] <= item && item <= list[high]) {
        int pos = low;
        if (list[low] != list[high]) {
            const long long offset = (static_cast<long long>(item) - list[low]) * (high - low);
            const long long range = static_cast<long long>(list[high]) - list[low];
            pos = low + static_cast<int>(offset / range);
        }
        // The lab examples count the probed element, not the endpoint reads
        // used to calculate pos: an exact interpolation hit counts as one.
        ++comparisons;
        const int value = list[pos];
        if (value == item) return pos;
        if (value < item) low = pos + 1;
        else high = pos - 1;
    }
    return -1;
}

class hashTable {
    static constexpr int EMPTY = -1;
    int* table_ = nullptr;
    int HTSize_ = 0;
public:
    explicit hashTable(int size) : HTSize_(size) {
        if (size <= 0) throw std::invalid_argument("Table size must be positive");
        table_ = new int[size];
        for (int i = 0; i < size; ++i) table_[i] = EMPTY;
    }
    hashTable(const hashTable&) = delete;
    hashTable& operator=(const hashTable&) = delete;
    bool insert(int key) {
        if (key < 0) {
            std::cerr << "Keys must be nonnegative\n";
            return false;
        }
        int index = key % HTSize_;
        for (int visited = 0; visited < HTSize_; ++visited) {
            if (table_[index] == key) {
                std::cerr << "Key already present\n";
                return false;
            }
            if (table_[index] == EMPTY) {
                table_[index] = key;
                return true;
            }
            index = (index + 1) % HTSize_;
        }
        std::cerr << "Hash table is full\n";
        return false;
    }
    int search(int key, int& comparisons) const {
        comparisons = 0;
        if (key < 0) return -1;
        int index = key % HTSize_;
        for (int visited = 0; visited < HTSize_; ++visited) {
            ++comparisons;
            const int value = table_[index];
            if (value == key) return index;
            if (value == EMPTY) return -1;
            index = (index + 1) % HTSize_;
        }
        return -1;
    }
    int search(int key) const { int comparisons; return search(key, comparisons); }
    int slot(int index) const { return table_[index]; }
    void print(std::ostream& out) const {
        for (int i = 0; i < HTSize_; ++i) {
            out << '[' << i << "] ";
            if (table_[i] == EMPTY) out << '-';
            else out << table_[i];
            out << "  ";
        }
        out << '\n';
    }
    ~hashTable() { delete[] table_; }
};

class chainHashTable {
    struct Node { int key; Node* next; };
    Node** table_ = nullptr;
    int HTSize_ = 0;
public:
    explicit chainHashTable(int size) : HTSize_(size) {
        if (size <= 0) throw std::invalid_argument("Table size must be positive");
        table_ = new Node*[size];
        for (int i = 0; i < size; ++i) table_[i] = nullptr;
    }
    chainHashTable(const chainHashTable&) = delete;
    chainHashTable& operator=(const chainHashTable&) = delete;
    bool insert(int key) {
        if (key < 0) {
            std::cerr << "Keys must be nonnegative\n";
            return false;
        }
        const int index = key % HTSize_;
        for (Node* current = table_[index]; current != nullptr; current = current->next) {
            if (current->key == key) {
                std::cerr << "Key already present\n";
                return false;
            }
        }
        Node* newNode = new Node{key, nullptr};
        newNode->next = table_[index];
        table_[index] = newNode;
        return true;
    }
    bool search(int key, int& comparisons) const {
        comparisons = 0;
        if (key < 0) return false;
        for (Node* current = table_[key % HTSize_]; current != nullptr; current = current->next) {
            ++comparisons;
            if (current->key == key) return true;
        }
        return false;
    }
    bool search(int key) const { int comparisons; return search(key, comparisons); }
    void print(std::ostream& out) const {
        for (int i = 0; i < HTSize_; ++i) {
            out << '[' << i << "]: ";
            for (Node* current = table_[i]; current != nullptr; current = current->next) out << current->key << " -> ";
            out << "nullptr\n";
        }
    }
    ~chainHashTable() {
        for (int i = 0; i < HTSize_; ++i) {
            Node* current = table_[i];
            while (current != nullptr) {
                Node* next = current->next;
                delete current;
                current = next;
            }
        }
        delete[] table_;
    }
};

inline void run_demo(std::ostream& out) {
    int values[1000];
    for (int i = 0; i < 1000; ++i) values[i] = i * 3;
    const int queries[] = {2997, 1500, 1501};
    out << "=== Q1-Q3: searching a sorted list of 1000 numbers ===\n";
    for (int item : queries) {
        int comparisons = 0;
        out << "search " << item << ":\n";
        int index = seqSearch(values, 1000, item, comparisons);
        out << "  sequential    -> index " << index << ", " << comparisons << " comparisons\n";
        index = binarySearch(values, 1000, item, comparisons);
        out << "  binary        -> index " << index << ", " << comparisons << " comparisons\n";
        index = interpolationSearch(values, 1000, item, comparisons);
        out << "  interpolation -> index " << index << ", " << comparisons << " comparisons\n";
    }
    out << "\n=== Q4-Q5: linear probing (HTSize = 13) ===\n";
    hashTable linear(13);
    const int keys[] = {18, 41, 22, 44, 59, 32, 31, 73};
    for (int key : keys) {
        out << "insert " << key << " (hash " << key % 13 << ")\n";
        linear.insert(key);
    }
    linear.print(out);
    out << "search 31 -> index " << linear.search(31) << "\nsearch 99 -> index " << linear.search(99) << '\n';
    out << "\n=== Q6-Q7: chaining (HTSize = 7) ===\n";
    chainHashTable chains(7);
    const int chained[] = {50, 85, 92, 73, 101, 76, 700};
    for (int key : chained) chains.insert(key);
    chains.print(out);
    out << "search 85 -> " << (chains.search(85) ? "true" : "false")
        << "\nsearch 99 -> " << (chains.search(99) ? "true" : "false") << '\n';
}
}

#include <chrono>

namespace classroom {
struct Measurement {
    double build_ms = 0;
    double search_ms = 0;
    long long comparisons = 0;
};

inline Measurement measure_chaining(int table_size, const int queries[], int query_count,
                                    int key_count, bool answers[]) {
    using Clock = std::chrono::steady_clock;
    Measurement result;
    const auto build_start = Clock::now();
    chainHashTable table(table_size);
    for (int key = 0; key < key_count; ++key) table.insert(key);
    const auto build_end = Clock::now();

    const auto search_start = Clock::now();
    for (int i = 0; i < query_count; ++i) {
        int comparisons = 0;
        answers[i] = table.search(queries[i], comparisons);
        result.comparisons += comparisons;
    }
    const auto search_end = Clock::now();

    for (int i = 0; i < query_count; ++i) {
        if (answers[i] != (queries[i] < key_count)) {
            throw std::runtime_error("Incorrect comparison result");
        }
    }
    result.build_ms = std::chrono::duration<double, std::milli>(build_end - build_start).count();
    result.search_ms = std::chrono::duration<double, std::milli>(search_end - search_start).count();
    return result;
}

inline void run_comparison(std::ostream& out) {
    constexpr int key_count = 5000, query_count = 100000, trials = 7;
    const int sizes[3] = {256, 1024, 4096};
    int queries[query_count];
    bool answers[query_count];
    Measurement totals[3];

    for (int i = 0; i < query_count; ++i) {
        if (i % 4 == 0) queries[i] = key_count + i % key_count;
        else queries[i] = i % key_count;
    }
    for (int trial = 0; trial < trials; ++trial) {
        for (int offset = 0; offset < 3; ++offset) {
            const int variant = (trial + offset) % 3;
            Measurement sample = measure_chaining(sizes[variant], queries, query_count,
                                                  key_count, answers);
            totals[variant].build_ms += sample.build_ms;
            totals[variant].search_ms += sample.search_ms;
            totals[variant].comparisons = sample.comparisons;
        }
    }

    out << "\nChaining: tablo boyutu karsilastirmasi\n"
        << key_count << " anahtar, " << query_count << " sorgu, %75 bulunan\n"
        << "7 tekrar; sureler ortalama, dugum sayisi bir calistirmanin toplami.\n"
        << std::left << std::setw(12) << "HTSize" << std::right << std::setw(14) << "Kurulum ms"
        << std::setw(14) << "Arama ms" << std::setw(20) << "Dugum karsilastirma" << '\n';
    const double reference = totals[0].search_ms / trials;
    for (int i = 0; i < 3; ++i) {
        const double build = totals[i].build_ms / trials;
        const double search = totals[i].search_ms / trials;
        out << std::fixed << std::setprecision(3) << std::left << std::setw(12) << sizes[i]
            << std::right << std::setw(14) << build << std::setw(14) << search
            << std::setw(20) << totals[i].comparisons;
        if (search > 0) out << "  " << reference / search << "x";
        out << '\n';
    }
    out << "Her satir ayni chaining kodu; yalniz HTSize degisiyor.\n"
        << "Buyuk tablo daha cok bellek tutar, zincirleri kisaltabilir.\n";
}
}
