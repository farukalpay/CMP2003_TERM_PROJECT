#include "lab.hpp"
#include <cstring>
int main(int argc, char** argv) {
    try {
        if (argc == 1) classroom::run_demo(std::cout);
        else if (argc == 2 && std::strcmp(argv[1], "--compare") == 0)
            classroom::run_comparison(std::cout);
        else {
            std::cerr << "Use lab_demo [--compare]\n";
            return 2;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
