CXX ?= c++
CXXFLAGS ?= -O3 -DNDEBUG -Wall -Wextra -Wpedantic
N ?= 5000
QUERIES ?= 100000
UPDATES ?= $(N)
SEED ?= 42
SANITIZE = -std=c++17 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined

.PHONY: all run app lab compare bench test sanitize submission clean
all: build/findfast

build:
	mkdir -p build

build/findfast: findfast.cpp | build
	$(CXX) $(CXXFLAGS) -std=c++17 $< -o $@

build/lab_demo: examples/main.cpp examples/lab.hpp | build
	$(CXX) $(CXXFLAGS) -std=c++17 $< -o $@

run: build/findfast
	./build/findfast --demo

app: build/findfast
	./build/findfast --app --data data

lab: build/lab_demo
	./build/lab_demo

compare: build/lab_demo
	./build/lab_demo --compare

bench: build/findfast
	./build/findfast --bench --n $(N) --queries $(QUERIES) --updates $(UPDATES) --seed $(SEED)

build/index_tests: tests/index.cpp findfast.cpp | build
	$(CXX) $(CXXFLAGS) -std=c++17 $< -o $@

build/lab_tests: tests/lab.cpp examples/lab.hpp | build
	$(CXX) $(CXXFLAGS) -std=c++17 $< -o $@

test: build/findfast build/index_tests build/lab_tests
	./build/findfast --test all --scale 10000
	./build/index_tests
	./build/lab_tests
	python3 tests/check.py

build/findfast_sanitize: findfast.cpp | build
	$(CXX) $(SANITIZE) $< -o $@

build/index_tests_sanitize: tests/index.cpp findfast.cpp | build
	$(CXX) $(SANITIZE) $< -o $@

build/lab_tests_sanitize: tests/lab.cpp examples/lab.hpp | build
	$(CXX) $(SANITIZE) $< -o $@

sanitize: build/findfast_sanitize build/index_tests_sanitize build/lab_tests_sanitize
	./build/findfast_sanitize --test all --scale 10000
	./build/index_tests_sanitize
	./build/lab_tests_sanitize

submission:
	python3 tests/check.py --submission

clean:
	$(RM) -r build
