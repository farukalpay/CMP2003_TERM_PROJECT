#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std;

// Supplied code: tracked memory, owned keys, and the index interface.
namespace findfast {
using ListId = uint32_t;

namespace memory {
    inline size_t live = 0;
    inline size_t peak = 0;

    // Store the allocation size just before the bytes returned to the caller.
    struct alignas(max_align_t) Header { size_t bytes; };

    void* allocate(size_t bytes) {
        if (bytes > numeric_limits<size_t>::max() - sizeof(Header))
            throw bad_alloc();
        auto* header = ::new (::operator new(sizeof(Header) + bytes)) Header{bytes};
        live += bytes;
        if (live > peak) peak = live;
        return header + 1;
    }

    void deallocate(void* pointer) noexcept {
        if (!pointer) return;
        auto* header = static_cast<Header*>(pointer) - 1;
        live -= header->bytes;
        ::operator delete(header);
    }

    template<class T, class... Args>
    T* make(Args&&... args) {
        static_assert(alignof(T) <= alignof(max_align_t));
        void* storage = allocate(sizeof(T));
        try {
            return ::new (storage) T(forward<Args>(args)...);
        } catch (...) {
            deallocate(storage);
            throw;
        }
    }

    template<class T>
    void destroy(T* pointer) noexcept {
        if (!pointer) return;
        pointer->~T();
        deallocate(pointer);
    }
}

// A Key owns its characters. Copying is disabled, but ownership can be moved.
class Key {
    char* data_ = nullptr;
    size_t length_ = 0;

public:
    Key() noexcept = default;
    explicit Key(string_view word) : length_(word.size()) {
        if (length_) {
            data_ = static_cast<char*>(memory::allocate(length_));
            char_traits<char>::copy(data_, word.data(), length_);
        }
    }
    Key(const Key&) = delete;
    Key& operator=(const Key&) = delete;
    Key(Key&& other) noexcept
        : data_(exchange(other.data_, nullptr)),
          length_(exchange(other.length_, 0)) {}
    Key& operator=(Key&& other) noexcept {
        if (this != &other) {
            memory::deallocate(data_);
            data_ = exchange(other.data_, nullptr);
            length_ = exchange(other.length_, 0);
        }
        return *this;
    }
    ~Key() { memory::deallocate(data_); }
    string_view view() const noexcept {
        if (!data_) return {};
        return {data_, length_};
    }
};

struct Record {
    Key word;
    ListId list = 0;
    Record() = default;
    Record(string_view key, ListId id) : word(key), list(id) {}
};

class Index {
public:
    virtual ~Index() = default;
    virtual void put(string_view word, ListId list) = 0;
    virtual bool find(string_view word, ListId& result) = 0;
    virtual bool erase(string_view word) = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
};

struct IndexDeleter {
    void (*destroy)(Index*);
    void operator()(Index* index) const { destroy(index); }
};
using IndexPtr = unique_ptr<Index, IndexDeleter>;
}

// Edit only the marked implementation and keep make_index unchanged.
// Implement the structure yourself. No standard containers or adapters,
// library search/sort algorithms, secondary indexes, query caches, hard-coded
// answers, I/O, threads, or untracked allocation in your implementation.
// This applies to unqualified names too, such as vector, map, stack, queue, and sort.
// Use Key, Record, and the memory helpers above.
// BEGIN STUDENT IMPLEMENTATION
namespace findfast {
class StudentIndex final : public Index {
    // Her tablo gozu bir tek bagli listenin basini tutar.
    struct Node {
        Record record;
        Node* next = nullptr;
        Node(string_view word, ListId list) : record(word, list) {}
    };
    size_t HTSize_;
    Node** table_;
    size_t count_ = 0;

    size_t hashFunction(string_view word) const {
        size_t value = 0;
        for (unsigned char character : word) {
            value = value * 31 + character;
        }
        return value % HTSize_;
    }

public:
    explicit StudentIndex(size_t tableSize = 1024) : HTSize_(tableSize) {
        if (HTSize_ == 0 || HTSize_ > numeric_limits<size_t>::max() / sizeof(Node*)) {
            throw invalid_argument("Invalid table size");
        }
        table_ = static_cast<Node**>(memory::allocate(HTSize_ * sizeof(Node*)));
        for (size_t i = 0; i < HTSize_; ++i) table_[i] = nullptr;
    }
    StudentIndex(const StudentIndex&) = delete;
    StudentIndex& operator=(const StudentIndex&) = delete;

    void put(string_view word, ListId list) override {
        const size_t index = hashFunction(word);
        for (Node* current = table_[index]; current != nullptr; current = current->next) {
            if (current->record.word.view() == word) {
                current->record.list = list;
                return;
            }
        }
        Node* newNode = memory::make<Node>(word, list);
        newNode->next = table_[index];
        table_[index] = newNode;
        ++count_;
    }

    bool find(string_view word, ListId& result) override {
        const size_t index = hashFunction(word);
        for (Node* current = table_[index]; current != nullptr; current = current->next) {
            if (current->record.word.view() == word) {
                result = current->record.list;
                return true;
            }
        }
        return false;
    }

    bool erase(string_view word) override {
        const size_t index = hashFunction(word);
        Node* previous = nullptr;
        Node* current = table_[index];
        while (current != nullptr) {
            if (current->record.word.view() == word) {
                if (previous == nullptr) {
                    table_[index] = current->next;
                } else {
                    previous->next = current->next;
                }
                memory::destroy(current);
                --count_;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    size_t size() const override { return count_; }

    void clear() override {
        for (size_t i = 0; i < HTSize_; ++i) {
            while (table_[i] != nullptr) {
                Node* current = table_[i];
                table_[i] = current->next;
                memory::destroy(current);
            }
        }
        count_ = 0;
    }

    ~StudentIndex() override {
        clear();
        memory::deallocate(table_);
    }
};

IndexPtr make_index(string_view name) {
    if (name != "student") throw invalid_argument("Unknown index");
    return IndexPtr(memory::make<StudentIndex>(), {[](Index* index) {
        memory::destroy(static_cast<StudentIndex*>(index));
    }});
}
}
// END STUDENT IMPLEMENTATION

namespace findfast {
// Supplied search engine. It uses StudentIndex to find document list IDs.
using DocId = uint32_t;
using Posting = set<DocId>;

set<string> words(string_view text) {
    set<string> result;
    string word;
    for (char ch : text) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            word += ch;
        } else if (!word.empty()) {
            result.insert(word);
            word.clear();
        }
    }
    if (!word.empty()) result.insert(word);
    return result;
}

class Engine {
    struct Document { string name; set<string> terms; };
    IndexPtr index_ = make_index("student");
    map<DocId, Document> documents_;
    map<ListId, Posting> postings_;
    vector<ListId> free_lists_;
    uint64_t next_list_ = 0;
    uint64_t next_doc_ = 1;

    static uint32_t issue(uint64_t& next) {
        if (next > numeric_limits<uint32_t>::max())
            throw overflow_error("ID space exhausted");
        return static_cast<uint32_t>(next++);
    }

    void unlink(DocId id, const set<string>& terms) {
        for (const auto& term : terms) {
            ListId list = 0;
            if (!index_->find(term, list)) throw logic_error("Missing indexed word");
            auto& docs = postings_.at(list);
            docs.erase(id);
            if (docs.empty()) {
                if (!index_->erase(term)) throw logic_error("Could not erase indexed word");
                postings_.erase(list);
                free_lists_.push_back(list);
            }
        }
    }

    void link(DocId id, const set<string>& terms) {
        for (const auto& term : terms) {
            ListId list = 0;
            if (!index_->find(term, list)) {
                if (free_lists_.empty()) {
                    list = issue(next_list_);
                } else {
                    list = free_lists_.back();
                    free_lists_.pop_back();
                }
                index_->put(term, list);
            }
            postings_[list].insert(id);
        }
    }

    const Posting& lookup(string_view query) {
        static const Posting empty;
        auto terms = words(query);
        ListId list = 0;
        if (terms.size() != 1 || !index_->find(*terms.begin(), list)) return empty;
        return postings_.at(list);
    }

public:
    DocId add(string name, string_view text) {
        auto terms = words(text);
        DocId id = issue(next_doc_);
        link(id, terms);
        documents_.emplace(id, Document{move(name), move(terms)});
        return id;
    }

    bool replace(DocId id, string name, string_view text) {
        auto doc = documents_.find(id);
        if (doc == documents_.end()) return false;
        auto terms = words(text);
        unlink(id, doc->second.terms);
        link(id, terms);
        doc->second = {move(name), move(terms)};
        return true;
    }

    bool remove(DocId id) {
        auto doc = documents_.find(id);
        if (doc == documents_.end()) return false;
        unlink(id, doc->second.terms);
        documents_.erase(doc);
        return true;
    }

    void clear() {
        index_->clear();
        documents_.clear();
        postings_.clear();
        free_lists_.clear();
        next_list_ = 0;
        next_doc_ = 1;
    }

    size_t word_count() const { return index_->size(); }

    vector<DocId> search(string_view left, string_view op = {},
                              string_view right = {}) {
        const auto& first = lookup(left);
        if (op.empty()) return {first.begin(), first.end()};
        const auto& second = lookup(right);
        vector<DocId> result;
        if (op == "AND") {
            set_intersection(first.begin(), first.end(), second.begin(), second.end(),
                                  back_inserter(result));
        } else if (op == "OR") {
            set_union(first.begin(), first.end(), second.begin(), second.end(),
                           back_inserter(result));
        } else {
            throw invalid_argument("Expected AND or OR");
        }
        return result;
    }

    void print(DocId id) const { cout << id << ' ' << documents_.at(id).name << '\n'; }
    void list() const { for (const auto& entry : documents_) print(entry.first); }
};

// Public correctness tests and their assertion helpers.
void check(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

void hit(Index& index, string_view word, ListId expected) {
    ListId found = ~expected;
    check(index.find(word, found) && found == expected, "Wrong lookup result");
}

void miss(Index& index, string_view word) {
    ListId found = 987654321;
    check(!index.find(word, found) && found == 987654321, "Miss changed result or reported a hit");
}

void check_size(const Index& index, size_t expected) {
    check(index.size() == expected, "Wrong distinct-word count");
}

void populate_demo(Engine& engine) {
    engine.add("book.txt", "Data structures and algorithms.");
    engine.add("notes.txt", "Network protocols, NETWORK!");
    engine.add("report.txt", "Network security and data.");
}

void test(int id, size_t scale, uint32_t seed) {
    if (id == 19 || id == 20) {
        Engine engine;
        populate_demo(engine);
        if (id == 19) {
            check(engine.search("NETWORK") == vector<DocId>{2, 3}, "Single word");
            check(engine.search("network", "AND", "security") == vector<DocId>{3}, "AND");
            check(engine.search("network", "OR", "data") == vector<DocId>{1, 2, 3}, "OR");
            check(engine.search("missing").empty(), "Missing word");
            check(engine.search("missing", "AND", "data").empty(), "AND with missing word");
            check(engine.search("missing", "OR", "data") == vector<DocId>{1, 3}, "OR with missing word");
            check(engine.search("network", "OR", "network") == vector<DocId>{2, 3}, "Repeated word");
            check(engine.search("network", "AND", "network") == vector<DocId>{2, 3}, "Repeated AND");
        } else {
            check(!engine.replace(99, "missing.txt", "unused"), "Missing replacement");
            check(engine.replace(3, "report.txt", "archive security"), "Replacement");
            check(engine.search("network") == vector<DocId>{2}, "Stale posting");
            check(engine.search("security") == vector<DocId>{3}, "Shared old and new term");
            check(engine.search("archive") == vector<DocId>{3}, "New posting");
            check(engine.remove(2) && !engine.remove(2), "Repeated removal");
            check(engine.search("network").empty(), "Deleted last posting");
            engine.clear();
            check(engine.word_count() == 0 && engine.search("archive").empty(), "Application clear");
            check(engine.add("new.txt", "archive") == 1, "Application reuse");
            check(engine.search("archive") == vector<DocId>{1}, "Search after clear");
        }
        return;
    }

    const auto before = memory::live;
    auto holder = make_index("student");
    auto& index = *holder;
    const auto empty = memory::live;
    const array<string_view, 3> keys{"delta", "alpha", "omega"};
    const auto fill = [&] {
        for (size_t i = 0; i < keys.size(); ++i) index.put(keys[i], static_cast<ListId>(i));
    };
    switch (id) {
    case 1:
        check_size(index, 0);
        break;
    case 2:
        index.put("network", 17);
        check_size(index, 1);
        hit(index, "network", 17);
        break;
    case 3:
        fill();
        check_size(index, keys.size());
        for (size_t i = 0; i < keys.size(); ++i) hit(index, keys[i], static_cast<ListId>(i));
        break;
    case 4:
        for (ListId i = 0; i < 12; ++i) index.put("same", i);
        check_size(index, 1);
        hit(index, "same", 11);
        break;
    case 5:
        miss(index, "absent");
        fill();
        miss(index, "absent");
        miss(index, "delt");
        break;
    case 6:
        fill();
        for (size_t i = keys.size(); i > 0; --i) hit(index, keys[i - 1], static_cast<ListId>(i - 1));
        break;
    case 7:
        fill();
        for (int i = 0; i < 20; ++i) {
            for (size_t j = 0; j < keys.size(); ++j) hit(index, keys[j], static_cast<ListId>(j));
            miss(index, "unknown");
            check_size(index, keys.size());
        }
        break;
    case 8: {
        string buffer = "temporary buffer with enough characters to allocate";
        const auto original = buffer;
        index.put(buffer, 42);
        buffer.front() = 'X';
        hit(index, original, 42);
        miss(index, buffer);
        index.put(string("another temporary key"), 51);
        hit(index, "another temporary key", 51);
        break;
    }
    case 9:
        fill();
        check(index.erase("alpha"), "Erase existing key");
        check_size(index, 2);
        miss(index, "alpha");
        hit(index, "delta", 0);
        hit(index, "omega", 2);
        break;
    case 10:
        check(!index.erase("unknown"), "Erase from empty");
        fill();
        check(!index.erase("unknown"), "Erase missing key");
        check_size(index, 3);
        check(index.erase("delta") && !index.erase("delta"), "Repeat erase");
        check_size(index, 2);
        break;
    case 11:
        fill();
        for (auto key : keys) check(index.erase(key), "Erase all");
        check_size(index, 0);
        index.put("new", 99);
        hit(index, "new", 99);
        check_size(index, 1);
        break;
    case 12:
        check_size(index, 0);
        fill();
        check_size(index, 3);
        index.put("alpha", 25);
        check_size(index, 3);
        check(!index.erase("unknown"), "Missing erase");
        check_size(index, 3);
        check(index.erase("alpha"), "Existing erase");
        check_size(index, 2);
        index.clear();
        check_size(index, 0);
        break;
    case 13:
        fill();
        index.clear();
        check_size(index, 0);
        check(memory::live == empty, "Clear did not release storage");
        for (auto key : keys) miss(index, key);
        index.put("alpha", 17);
        hit(index, "alpha", 17);
        break;
    case 14:
        index.clear();
        index.clear();
        check_size(index, 0);
        fill();
        index.clear();
        index.clear();
        check_size(index, 0);
        index.put("fresh", 31);
        hit(index, "fresh", 31);
        break;
    case 15:
        fill();
        holder.reset();
        check(memory::live == before, "Destructor did not release storage");
        break;
    case 16: {
        const string prefix = "sharedprefix";
        const string long_key(4096, 'q');
        const auto similar = long_key.substr(0, 4095) + 'x';
        index.put(prefix, 76);
        index.put(long_key, 77);
        index.put(similar, 78);
        for (size_t i = 0; i < scale; ++i) index.put(prefix + to_string(i), static_cast<ListId>(i));
        for (size_t i = 0; i < scale; i += 2) check(index.erase(prefix + to_string(i)), "Scaled erase");
        for (size_t i = 0; i < scale; ++i) {
            const auto key = prefix + to_string(i);
            if (i % 2) hit(index, key, static_cast<ListId>(i));
            else miss(index, key);
        }
        hit(index, prefix, 76);
        hit(index, long_key, 77);
        hit(index, similar, 78);
        check_size(index, scale / 2 + 3);
        break;
    }
    case 17: {
        mt19937 rng(seed);
        map<string, ListId> expected;
        for (size_t i = 0; i < scale * 12; ++i) {
            auto key = "random" + to_string(rng() % scale);
            switch (rng() % 8) {
            case 0: case 1: case 2: case 3: {
                auto value = static_cast<ListId>(rng());
                index.put(key, value);
                expected[key] = value;
                break;
            }
            case 4:
                check(index.erase(key) == (expected.erase(key) == 1), "Random erase");
                break;
            case 5: case 6: {
                const auto found = expected.find(key);
                if (found == expected.end()) miss(index, key);
                else hit(index, key, found->second);
                break;
            }
            case 7:
                if (i % 53 == 0) {
                    index.clear();
                    expected.clear();
                }
                break;
            }
            check_size(index, expected.size());
        }
        for (const auto& entry : expected) hit(index, entry.first, entry.second);
        break;
    }
    case 18: {
        constexpr auto max_id = numeric_limits<ListId>::max();
        index.put("zero", 0);
        index.put("max", max_id);
        hit(index, "zero", 0);
        hit(index, "max", max_id);
        index.put("zero", max_id);
        index.put("max", 0);
        hit(index, "zero", max_id);
        hit(index, "max", 0);
        break;
    }
    default:
        throw logic_error("Unknown test");
    }
}

bool run_tests(string_view selection, size_t scale, uint32_t seed) {
    int attempted = 0;
    int passed = 0;
    for (int id = 1; id <= 20; ++id) {
        const string name = (id < 10 ? "T0" : "T") + to_string(id);
        if (selection != "all" && selection != name) continue;
        ++attempted;
        const auto before = memory::live;
        string failure;
        try {
            test(id, scale, seed);
        } catch (const exception& error) {
            failure = error.what();
        }
        if (memory::live != before) {
            if (!failure.empty()) failure += " | ";
            failure += "Tracked memory was not released";
        }
        if (failure.empty()) {
            ++passed;
            cout << "PASS " << name << '\n';
        } else {
            cout << "FAIL " << name << ": " << failure << '\n';
        }
    }
    if (!attempted) throw invalid_argument("Use --test all or --test T01 through T20");
    cout << "SUMMARY " << passed << '/' << attempted << '\n';
    return passed == attempted;
}

// Benchmark workload, file reader, and timing.
struct Operation {
    char kind;
    string word;
    ListId value = 0;
};

struct Workload {
    vector<Operation> build, queries, updates;
};

Workload synthetic_workload(size_t n, size_t queries, size_t updates,
                            uint32_t seed) {
    Workload work;
    mt19937 rng(seed);
    const auto key = [](size_t i) {
        return "term" + to_string(i);
    };
    for (size_t i = 0; i < n; ++i) work.build.push_back({'P', key(i), static_cast<ListId>(rng())});
    shuffle(work.build.begin(), work.build.end(), rng);
    for (size_t i = 0; i < queries; ++i) {
        const auto number = rng() % n;
        work.queries.push_back({'F', key(i % 4 == 0 ? n + number : number)});
    }
    for (size_t i = 0; i < updates; ++i) {
        const auto number = rng() % n;
        if (i % 4 == 0) work.updates.push_back({'P', key(2 * n + i), static_cast<ListId>(rng())});
        else if (i % 4 == 1) work.updates.push_back({'E', key(number)});
        else if (i % 4 == 2) work.updates.push_back({'P', key(number), static_cast<ListId>(rng())});
        else work.updates.push_back({'E', key(n + number)});
    }
    return work;
}

uint64_t number(string_view input, uint64_t max) {
    if (input.empty()) throw invalid_argument("Expected an unsigned integer");
    uint64_t value = 0;
    for (char digit : input) {
        if (digit < '0' || digit > '9') throw invalid_argument("Expected an unsigned integer");
        const auto next = static_cast<unsigned>(digit - '0');
        if (next > max || value > (max - next) / 10) throw out_of_range("Integer out of range");
        value = 10 * value + next;
    }
    return value;
}

Workload read_workload(const filesystem::path& path) {
    ifstream file(path);
    if (!file) throw runtime_error("Cannot open workload: " + path.string());
    Workload work;
    string line;
    const auto read_line = [&] {
        if (!getline(file, line)) return false;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return true;
    };
    if (!read_line() || line != "phase,op,word,list_id")
        throw runtime_error("Invalid workload header");
    size_t row = 1;
    while (read_line()) {
        ++row;
        istringstream input(line);
        string phase, op, word, id;
        if (!getline(input, phase, ',') || !getline(input, op, ',')
            || !getline(input, word, ',') || word.empty() || !getline(input, id)
            || (op != "P" && id != "-"))
            throw runtime_error("Invalid workload row " + to_string(row));
        if (phase == "build" && op == "P") {
            work.build.push_back({'P', move(word),
                static_cast<ListId>(number(id, numeric_limits<ListId>::max()))});
        } else if (phase == "lookup" && op == "F") {
            work.queries.push_back({'F', move(word)});
        } else if (phase == "update" && (op == "P" || op == "E")) {
            const auto value = op == "P" ? static_cast<ListId>(number(id, numeric_limits<ListId>::max())) : 0;
            work.updates.push_back({op[0], move(word), value});
        } else {
            throw runtime_error("Invalid phase or operation on row " + to_string(row));
        }
    }
    if (file.bad()) throw runtime_error("Error reading workload");
    if (work.build.empty()) throw runtime_error("Workload must contain a build phase");
    return work;
}

void validate_index(Index& index, const map<string, ListId>& expected) {
    check_size(index, expected.size());
    for (const auto& entry : expected) hit(index, entry.first, entry.second);
}

void benchmark(const Workload& work) {
    using Clock = chrono::steady_clock;
    const auto seconds = [](Clock::duration duration) { return chrono::duration<double>(duration).count(); };
    constexpr ListId untouched = 0xdeadbeefu;
    vector<pair<bool, ListId>> answers(work.queries.size(), {false, untouched});
    vector<unsigned char> removed(work.updates.size());
    map<string, ListId> expected;
    for (const auto& op : work.build) expected[op.word] = op.value;

    const auto baseline = memory::live;
    memory::peak = baseline;
    const auto start_build = Clock::now();
    auto index = make_index("student");
    for (const auto& op : work.build) index->put(op.word, op.value);
    const auto end_build = Clock::now();
    const auto build_peak = memory::peak;
    validate_index(*index, expected);
    memory::peak = max(build_peak, memory::live);

    const auto start_lookup = Clock::now();
    for (size_t i = 0; i < work.queries.size(); ++i) {
        auto& answer = answers[i];
        answer.first = index->find(work.queries[i].word, answer.second);
    }
    const auto end_lookup = Clock::now();
    for (size_t i = 0; i < work.queries.size(); ++i) {
        const auto found = expected.find(work.queries[i].word);
        if (found == expected.end()) check(answers[i] == make_pair(false, untouched), "Incorrect benchmark miss");
        else check(answers[i] == make_pair(true, found->second), "Incorrect benchmark hit");
    }

    const auto start_update = Clock::now();
    for (size_t i = 0; i < work.updates.size(); ++i) {
        const auto& op = work.updates[i];
        if (op.kind == 'P') index->put(op.word, op.value);
        else removed[i] = index->erase(op.word);
    }
    const auto end_update = Clock::now();
    const auto peak_bytes = memory::peak - baseline;
    const auto build_time = end_build - start_build;
    const auto lookup_time = end_lookup - start_lookup;
    const auto update_time = end_update - start_update;
    for (size_t i = 0; i < work.updates.size(); ++i) {
        const auto& op = work.updates[i];
        if (op.kind == 'P') expected[op.word] = op.value;
        else check(removed[i] == expected.erase(op.word), "Incorrect benchmark erase");
    }
    validate_index(*index, expected);
    index.reset();
    check(memory::live == baseline, "Benchmark leaked tracked memory");
    cout << fixed << setprecision(9)
              << "build_seconds " << seconds(build_time) << '\n'
              << "lookup_seconds " << seconds(lookup_time) << '\n'
              << "update_seconds " << seconds(update_time) << '\n'
              << "total_seconds " << seconds(build_time + lookup_time + update_time) << '\n'
              << "peak_bytes " << peak_bytes << '\n';
}

// Demo and interactive command handling.
string read_text(const filesystem::path& path) {
    ifstream file(path, ios::binary);
    if (!file) throw runtime_error("Cannot open document: " + path.string());
    ostringstream text;
    text << file.rdbuf();
    if (file.bad() || text.bad()) throw runtime_error("Cannot read document: " + path.string());
    return text.str();
}

void print_results(Engine& engine, const vector<DocId>& matches) {
    cout << "results " << matches.size() << '\n';
    for (auto id : matches) engine.print(id);
}

void demo() {
    Engine engine;
    populate_demo(engine);
    cout << "> network\n";
    print_results(engine, engine.search("network"));
    cout << "> network AND security\n";
    print_results(engine, engine.search("network", "AND", "security"));
    cout << "> network OR data\n";
    print_results(engine, engine.search("network", "OR", "data"));
}

void app_help() {
    cout << "search WORD [AND|OR WORD]\nlist\nadd FILE\nreplace ID FILE\n"
                 "remove ID\nclear\nhelp\nquit\n"
                 "Quote paths with spaces. --data DIR loads .txt files in filename order.\n";
}

void app(const string& directory) {
    Engine engine;
    if (!directory.empty()) {
        vector<filesystem::path> paths;
        for (const auto& entry : filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") paths.push_back(entry.path());
        }
        sort(paths.begin(), paths.end());
        for (const auto& path : paths) engine.add(path.filename().string(), read_text(path));
    }
    string line;
    while (cout << "> " && getline(cin, line)) {
        istringstream input(line);
        string command;
        if (!(input >> command)) continue;
        try {
            if (command == "search") {
                string left, op, right, extra;
                if (!(input >> left) || ((input >> op) && (!(input >> right) || (input >> extra))))
                    throw invalid_argument("Use search WORD [AND|OR WORD]");
                print_results(engine, engine.search(left, op, right));
                continue;
            }
            if (command == "add" || command == "replace" || command == "remove") {
                DocId id = 0;
                string argument, filename, extra;
                if (command != "add") {
                    if (!(input >> argument)) throw invalid_argument("Expected a document ID");
                    id = static_cast<DocId>(number(argument, numeric_limits<DocId>::max()));
                }
                if (command != "remove" && !(input >> quoted(filename, '"', '\0')))
                    throw invalid_argument("Expected a filename");
                if (input >> extra) throw invalid_argument("Unexpected argument");
                if (command == "add") {
                    const auto added = engine.add(filesystem::path(filename).filename().string(), read_text(filename));
                    cout << "added " << added << '\n';
                } else if (command == "replace") {
                    const bool replaced = engine.replace(id, filesystem::path(filename).filename().string(), read_text(filename));
                    cout << (replaced ? "replaced" : "missing") << '\n';
                } else {
                    cout << (engine.remove(id) ? "removed" : "missing") << '\n';
                }
                continue;
            }
            string extra;
            if (input >> extra) throw invalid_argument("Unexpected argument");
            if (command == "list") engine.list();
            else if (command == "clear") engine.clear();
            else if (command == "help") app_help();
            else if (command == "quit") break;
            else throw invalid_argument("Unknown command. Type help.");
        } catch (const exception& error) {
            cout << "error: " << error.what() << '\n';
        }
    }
}

void usage() {
    cout << "findfast --test all|T01..T20 [--scale N] [--seed N]\n"
                 "findfast --demo\nfindfast --app [--data DIR|--help]\n"
                 "findfast --bench [--n N] [--queries N] [--updates N] [--seed N]\n"
                 "findfast --bench --workload FILE (try data/small.csv)\n";
}
}

int main(int argc, char** argv) {
    using namespace findfast;
    try {
        if (argc == 1 || (argc == 2 && string_view(argv[1]) == "--help")) {
            usage();
            return 0;
        }
        const string_view mode = argv[1];
        if (mode == "--demo" && argc == 2) { demo(); return 0; }
        if (mode == "--app") {
            if (argc == 3 && string_view(argv[2]) == "--help") app_help();
            else if (argc == 2) app({});
            else if (argc == 4 && string_view(argv[2]) == "--data") app(argv[3]);
            else throw invalid_argument("Use --app [--data DIR|--help]");
            return 0;
        }
        if (mode == "--bench" && argc == 4 && string_view(argv[2]) == "--workload") {
            benchmark(read_workload(argv[3]));
            return 0;
        }
        if (mode != "--test" && mode != "--bench") throw invalid_argument("See --help");
        if (mode == "--test" && argc < 3) throw invalid_argument("Specify a test or all");
        constexpr auto max_seed = numeric_limits<uint32_t>::max();
        constexpr auto max_count = numeric_limits<size_t>::max() / 16;
        size_t scale = 100, n = 100, queries = 0, updates = 0;
        uint32_t seed = 42;
        set<string_view> options;
        for (int i = mode == "--test" ? 3 : 2; i < argc; i += 2) {
            const string_view option = argv[i];
            if (i + 1 == argc || !options.insert(option).second)
                throw invalid_argument("Missing value or repeated option");
            if (option == "--seed") seed = static_cast<uint32_t>(number(argv[i + 1], max_seed));
            else if (mode == "--test" && option == "--scale") scale = static_cast<size_t>(number(argv[i + 1], max_count));
            else if (mode == "--bench" && option == "--n") n = static_cast<size_t>(number(argv[i + 1], max_count));
            else if (mode == "--bench" && option == "--queries") queries = static_cast<size_t>(number(argv[i + 1], max_count));
            else if (mode == "--bench" && option == "--updates") updates = static_cast<size_t>(number(argv[i + 1], max_count));
            else throw invalid_argument("Unknown option");
        }
        if (mode == "--test") {
            if (!scale) throw invalid_argument("--scale must be positive");
            return run_tests(argv[2], scale, seed) ? 0 : 1;
        }
        if (!n) throw invalid_argument("--n must be positive");
        if (!options.count("--queries")) queries = 2 * n;
        if (!options.count("--updates")) updates = n;
        benchmark(synthetic_workload(n, queries, updates, seed));
        return 0;
    } catch (const exception& error) {
        cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
