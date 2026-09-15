export module greeter;
import std;

// On Apple platforms mcpp compiles every unit of a graph whose C++ runtime is
// this package with hidden visibility, so that the runtime's instantiations are
// never coalesced with the system's libc++. A shared library therefore states
// the symbols it exports; on ELF the attribute restates the default.
export [[gnu::visibility("default")]] std::string greet(int n);
export [[gnu::visibility("default")]] void greet_throw();
