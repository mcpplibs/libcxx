module greeter;
import std;

std::string greet(int n) { return std::format("greeter-{}", n); }
void greet_throw() { throw std::runtime_error("from the shared library"); }
