#include "../examples/lab.hpp"
#include <sstream>
#include <limits>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        int comparisons = 99;
        const int unsorted[] = {7, 3, 9, 4};
        require(classroom::seqSearch(unsorted, 4, 9, comparisons) == 2 && comparisons == 3, "Q1 hit");
        require(classroom::seqSearch(unsorted, 4, 5, comparisons) == -1 && comparisons == 4, "Q1 miss");
        const int sorted[] = {2, 5, 8, 12, 16, 23, 38};
        require(classroom::binarySearch(sorted, 7, 23, comparisons) == 5 && comparisons == 2, "Q2 example");
        const int spaced[] = {10, 20, 30, 40, 50, 60, 70};
        require(classroom::interpolationSearch(spaced, 7, 60, comparisons) == 5 && comparisons == 1, "Q3 example");
        const int repeated[] = {4, 4, 4, 4};
        require(classroom::interpolationSearch(repeated, 4, 4, comparisons) == 0 && comparisons == 1, "Equal endpoints");
        require(classroom::interpolationSearch(repeated, 4, 5, comparisons) == -1 && comparisons == 0, "Outside range");
        require(classroom::seqSearch(nullptr, 0, 1, comparisons) == -1 && comparisons == 0, "Empty sequential");
        require(classroom::binarySearch(nullptr, 0, 1, comparisons) == -1 && comparisons == 0, "Empty binary");
        require(classroom::interpolationSearch(nullptr, 0, 1, comparisons) == -1 && comparisons == 0, "Empty interpolation");
        const int extremes[] = {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
        require(classroom::interpolationSearch(extremes, 2, extremes[1], comparisons) == 1, "Interpolation arithmetic range");
        int numbers[1000];
        for (int i = 0; i < 1000; ++i) numbers[i] = i * 3;
        require(classroom::seqSearch(numbers, 1000, 2997, comparisons) == 999 && comparisons == 1000, "Sequential last");
        require(classroom::binarySearch(numbers, 1000, 2997, comparisons) == 999 && comparisons == 10, "Binary last");
        require(classroom::binarySearch(numbers, 1000, 1500, comparisons) == 500 && comparisons == 9, "Binary middle");
        require(classroom::interpolationSearch(numbers, 1000, 1501, comparisons) == -1 && comparisons == 1, "Interpolation gap");
        std::cout << "PASS Q1-Q3 searches, counts and edge cases\n";

        classroom::hashTable linear(13);
        const int keys[] = {18, 41, 22, 44, 59, 32, 31, 73};
        for (int key : keys) require(linear.insert(key), "Q4 insert");
        require(linear.search(31, comparisons) == 10 && comparisons == 6, "Q5 collision path");
        require(linear.search(99) == -1, "Q5 miss");
        const int expected[] = {-1, -1, 41, -1, -1, 18, 44, 59, 32, 22, 31, 73, -1};
        for (int i = 0; i < 13; ++i) require(linear.slot(i) == expected[i], "Q4 table arrangement");
        classroom::hashTable full(3);
        require(full.insert(2) && full.insert(5) && full.insert(8), "Wrapped insertion");
        require(full.search(11, comparisons) == -1 && comparisons == 3, "Full-table miss terminates");
        std::ostringstream errors;
        auto* original = std::cerr.rdbuf(errors.rdbuf());
        const bool duplicate = full.insert(5), overflow = full.insert(11), invalid = full.insert(-1);
        std::cerr.rdbuf(original);
        require(!duplicate && !overflow && !invalid && !errors.str().empty(), "Rejected inserts report errors");
        require(full.slot(0) == 5 && full.slot(1) == 8 && full.slot(2) == 2, "Rejected insert changed table");
        std::cout << "PASS Q4-Q5 linear probing, duplicate/full rejection and wraparound\n";

        classroom::chainHashTable chain(7);
        require(chain.insert(50) && chain.insert(85) && chain.insert(92), "Q6 insert");
        std::ostringstream printed;
        chain.print(printed);
        require(printed.str().find("[1]: 92 -> 85 -> 50 -> nullptr") != std::string::npos, "Front insertion order");
        require(chain.search(85, comparisons) && comparisons == 2, "Q7 chain hit");
        require(!chain.search(99, comparisons) && comparisons == 3, "Q7 one-chain miss");
        original = std::cerr.rdbuf(errors.rdbuf());
        const bool chain_duplicate = chain.insert(85);
        std::cerr.rdbuf(original);
        std::ostringstream after;
        chain.print(after);
        require(!chain_duplicate && printed.str() == after.str(), "Duplicate chain insertion changed state");
        std::cout << "PASS Q6-Q7 chaining, front insertion and duplicates\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
