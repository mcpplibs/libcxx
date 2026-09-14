import std;

// The program of examples/import-std at c++20. `std::print` is C++23, so the
// line is formatted with `std::format` and written to `std::cout`; the two
// paths it exercises are the same.
int main() {
    std::unordered_map<std::string, int> m;
    m["one"] = 1;
    m["two"] = 2;
    std::atomic<int> a{0};
    a.store(3);
    a.notify_all();
    std::vector<int> v{m["one"], m["two"], a.load()};
    std::ranges::sort(v);
    std::cout << std::format("{}-{}-{}", v[0], v[1], v[2]) << std::endl;
    return 0;
}
