import std;

// The two paths that failed at link on the iOS rows under the payload's
// headers over the SDK's libc++: string hashing (`__hash_memory`) and the
// atomic notification table. Both resolve inside this package.
int main() {
    std::unordered_map<std::string, int> m;
    m["one"] = 1;
    m["two"] = 2;
    std::atomic<int> a{0};
    a.store(3);
    a.notify_all();
    std::vector<int> v{m["one"], m["two"], a.load()};
    std::ranges::sort(v);
    std::print("{}-{}-{}\n", v[0], v[1], v[2]);
    return 0;
}
