import std;
import greeter;

// The first line is the claim. The second is a reading: with a private copy of
// the runtime in each image, a standard exception thrown in the shared library
// is not caught by its class here.
int main() {
    std::cout << greet(3) << std::endl;
    try { greet_throw(); }
    catch (const std::runtime_error&) { std::cout << "runtime_error caught by its class" << std::endl; }
    catch (...) { std::cout << "runtime_error not matched by its class" << std::endl; }
    return 0;
}
