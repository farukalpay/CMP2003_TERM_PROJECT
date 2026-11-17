#include <cstdlib>
// Test driver only: import the instructor's file without its main function.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunqualified-std-cast-call"
#endif
#define main findfast_test_course_main
#include "../findfast.cpp"
#undef main
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

static std::ptrdiff_t allocations_before_failure = -1;
void* operator new(std::size_t bytes) {
    if (allocations_before_failure == 0) throw std::bad_alloc();
    if (allocations_before_failure > 0) --allocations_before_failure;
    if (void* result = std::malloc(bytes ? bytes : 1)) return result;
    throw std::bad_alloc();
}
void operator delete(void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete[](void* value) noexcept { ::operator delete(value); }
void operator delete[](void* value, std::size_t) noexcept { ::operator delete(value); }

using namespace findfast;

void configurable_table_sizes() {
    const size_t sizes[] = {1, 2, 7, 13, 31, 128, 512, 2048};
    for (size_t size : sizes) {
        StudentIndex index(size);
        for (ListId i = 0; i < 200; ++i) index.put("entry" + to_string(i), i);
        for (ListId i = 0; i < 200; ++i) hit(index, "entry" + to_string(i), i);
        for (ListId i = 0; i < 200; i += 2) check(index.erase("entry" + to_string(i)), "Sized erase");
        check_size(index, 100);
        index.clear();
        check_size(index, 0);
        index.put("reuse", 1);
        hit(index, "reuse", 1);
    }
    const size_t invalid_sizes[] = {0, numeric_limits<size_t>::max()};
    for (size_t size : invalid_sizes) {
        bool rejected = false;
        try { StudentIndex index(size); }
        catch (const invalid_argument&) { rejected = true; }
        check(rejected, "Invalid table size accepted");
    }
    cout << "PASS constructor sizes, forced single-bucket collisions and invalid sizes\n";
}

void collisions_and_deletion() {
    auto index = make_index("student");
    const char* keys[] = {"AaAa", "AaBB", "BBAa", "BBBB"};
    // With multiply-by-31, Aa and BB have the same numeric hash.
    for (ListId i = 0; i < 4; ++i) index->put(keys[i], i);
    check(index->erase("BBAa"), "Erase middle of a collision chain");
    check(index->erase("BBBB"), "Erase head of a collision chain");
    check(index->erase("AaAa"), "Erase tail of a collision chain");
    hit(*index, "AaBB", 1);
    miss(*index, "BBBB");
    check_size(*index, 1);
    index->put("AaAa", 99);
    hit(*index, "AaAa", 99);
    index->clear();
    index->clear();
    index->put("reuse", 33);
    hit(*index, "reuse", 33);
    std::cout << "PASS collision chains, head/middle/tail deletion and reuse\n";
}

void owned_binary_and_long_keys() {
    auto index = make_index("student");
    index->put({}, 0);
    hit(*index, {}, 0);
    std::string input = "owned temporary word";
    index->put(input, 17);
    input[0] = 'X';
    hit(*index, "owned temporary word", 17);
    miss(*index, input);
    std::string keys[256];
    for (unsigned i = 0; i < 256; ++i) {
        keys[i] = std::string(1, static_cast<char>(i)) + "suffix";
        index->put(keys[i], i);
    }
    for (unsigned i = 0; i < 256; ++i) hit(*index, keys[i], i);
    std::string first(1024 * 1024, 'q'), second = first;
    second[second.size() / 2] = 'x';
    index->put(first, 81);
    index->put(second, 82);
    hit(*index, first, 81);
    hit(*index, second, 82);
    check(index->erase(first), "Erase long key");
    hit(*index, second, 82);
    std::cout << "PASS owned, empty, binary and long keys\n";
}

void failed_allocations() {
    for (int budget = 0; budget < 2; ++budget) {
        const auto before = memory::live;
        bool failed = false;
        allocations_before_failure = budget;
        try { auto index = make_index("student"); }
        catch (const std::bad_alloc&) { failed = true; }
        catch (...) { allocations_before_failure = -1; throw; }
        allocations_before_failure = -1;
        check(failed && memory::live == before, "Failed construction leaked storage");
    }

    const char* existing_keys[] = {"alpha", "beta", "gamma"};
    for (int existing : {0, 3}) {
        bool succeeded = false;
        for (int budget = 0; budget < 5 && !succeeded; ++budget) {
            auto index = make_index("student");
            for (int i = 0; i < existing; ++i) index->put(existing_keys[i], static_cast<ListId>(i));
            const auto before = memory::live;
            allocations_before_failure = budget;
            try { index->put("incoming", 900); succeeded = true; }
            catch (const std::bad_alloc&) {
                allocations_before_failure = -1;
                check(memory::live == before, "Failed insertion leaked or changed storage");
                check_size(*index, static_cast<size_t>(existing));
                miss(*index, "incoming");
            } catch (...) { allocations_before_failure = -1; throw; }
            allocations_before_failure = -1;
            for (int i = 0; i < existing; ++i) hit(*index, existing_keys[i], static_cast<ListId>(i));
            if (succeeded) {
                allocations_before_failure = 0;
                index->put("incoming", 901);
                hit(*index, "incoming", 901);
                check(index->erase("incoming"), "Erase without allocation");
                index->clear();
                allocations_before_failure = -1;
            }
        }
        check(succeeded, "Insertion did not recover");
    }
    std::cout << "PASS object, table, node and key allocation failures\n";
}

void random_operations(uint32_t seed) {
    constexpr size_t key_count = 512;
    struct Expected { bool present = false; ListId value = 0; };
    Expected expected[key_count];
    std::string keys[key_count];
    for (size_t i = 0; i < key_count; ++i) keys[i] = "word" + std::to_string(i);
    auto index = make_index("student");
    size_t expected_count = 0;
    std::mt19937 rng(seed);
    for (size_t step = 0; step < 100000; ++step) {
        const size_t selected = rng() % key_count;
        const auto value = static_cast<ListId>(rng());
        switch (rng() % 4) {
        case 0: case 1:
            index->put(keys[selected], value);
            if (!expected[selected].present) ++expected_count;
            expected[selected] = {true, value};
            break;
        case 2:
            check(index->erase(keys[selected]) == expected[selected].present, "Random erase result");
            if (expected[selected].present) --expected_count;
            expected[selected].present = false;
            break;
        default:
            if (expected[selected].present) hit(*index, keys[selected], expected[selected].value);
            else miss(*index, keys[selected]);
        }
        check_size(*index, expected_count);
        if (step % 10007 == 10006) {
            index->clear();
            expected_count = 0;
            for (size_t i = 0; i < key_count; ++i) expected[i].present = false;
        }
    }
    for (size_t i = 0; i < key_count; ++i) {
        if (expected[i].present) hit(*index, keys[i], expected[i].value);
        else miss(*index, keys[i]);
    }
    std::cout << "PASS 100000 operations against a plain-array reference\n";
}

int main() {
    try {
        const auto baseline = memory::live;
        configurable_table_sizes();
        collisions_and_deletion();
        owned_binary_and_long_keys();
        failed_allocations();
        for (uint32_t seed : {0u, 42u, 987654321u}) random_operations(seed);
        check(memory::live == baseline, "Extended tests leaked tracked memory");
        return 0;
    } catch (const std::exception& error) {
        allocations_before_failure = -1;
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
