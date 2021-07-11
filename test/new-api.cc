#include <windows.h>
#include <stdint.h> // NOLINT

// To avoid having to include entire stdio.h.
extern "C" __inline int __cdecl sprintf(char* const buffer, char const* const format, ...);

namespace fmt {
	struct writable {
		virtual void write(const char *str, size_t length) {
			// No useful implementation.
		}
	};

	struct stdout : writable {
		HANDLE handle;

		explicit stdout(HANDLE handle) : handle(handle) {}

		void write(const char *str, size_t length) override {
			WriteFile(this->handle, str, length, nullptr, nullptr);
		}
	};

	struct formatter {
		writable &buffer;
	};

	struct parse_state {
		const char *format_string;
		size_t fs_length;
	};

	void _flush_string(formatter &fmt, const char *str, size_t length) {
		fmt.buffer.write(str, length);
	}

	template<typename T>
	inline void display(formatter &, const T &) {
#ifdef NDEBUG
		WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), "fmt::display not implemented!\n", 30, nullptr, nullptr);
#endif
	}

	template<typename T>
	void _parse_fmts(parse_state &state, const T &argument) {
		size_t offset = 0;
		stdout out{GetStdHandle(STD_OUTPUT_HANDLE)};
		formatter fmt{out};

		for (size_t i = 0; i < state.fs_length; ++i) {
			auto c = state.format_string[i];

			if (c == '%') {
				_flush_string(fmt, state.format_string, i);
				display(fmt, argument);
				++offset;

				break;
			}

			++offset;
		}

		state.format_string += offset;
	}

	template<typename... Ts>
	void print(const char *format_string, const Ts &...arguments) {
		parse_state state{format_string, strlen(format_string)};
		stdout out{GetStdHandle(STD_OUTPUT_HANDLE)};
		formatter fmt{out};

		(_parse_fmts(state, arguments), ...);
		_flush_string(fmt, state.format_string, strlen(state.format_string));
	}

	inline void display(formatter &fmt, uint32_t value) {
		char buffer[16] = {0};
		// Replace this with a custom implementation.
		sprintf(buffer, "%d", value);
		fmt.buffer.write(buffer, strlen(buffer));
	}

	inline void display(formatter &fmt, const char *value) {
		fmt.buffer.write(value, strlen(value));
	}
}

struct foo {
	uint32_t bar;
};

namespace fmt {
	inline void display(formatter &fmt, const foo &value) {
		fmt::display(fmt, value.bar);
	}
}

int main() {
	fmt::print("hello %\n", "world");
	fmt::print("foo is %\n", foo{42});
	return 0;
}
