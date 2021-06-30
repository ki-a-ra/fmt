#include "fmt.hh"

struct foo {
	const fmt::c8 *value;

	void display(fmt::argument &arg) const {
		arg._fmt.write_str({ .data = this->value, .length = fmt::utf8::strlen(this->value) });
	}
};

int main() {
	fmt::print(u8"foo\n;");
	fmt::print(u8"hello {} nyan\n;", "world");
	foo f = {u8"nyan~ "};
	fmt::print(u8"hello, {}\n;", f);
	fmt::print(u8"it's {}\n;", 2021);
	fmt::print(u8"it's not {}\n;", -1999);

	return 0;
}
