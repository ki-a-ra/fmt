#include "fmt.hh"

typedef unsigned long u64;

struct address_wrapper {
	u64 address;

	void display(fmt::formatter &fmt) {
	}
};

int main() {
	fmt::formatter fmt = {};
	const address_wrapper address = { 42 };
	fmt::print(fmt, "hello, {}", address);
	fmt::display(fmt, 42);
	fmt::display(fmt, "nyan");
	fmt::print(fmt, "uwu, {}", "owo");
	return 0;
}
