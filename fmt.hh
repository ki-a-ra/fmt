#ifndef _fmt_h_included
#define _fmt_h_included

#include <windows.h>

namespace fmt {
	// Helper types for working with types. Mostly a re-implementation of STL `<type_traits>` header, except putting it in
	// its own namespace, and not having to deal with discrepancies between different implementations.
	namespace type_traits {
		template<typename T, typename U>
		struct same {
			static constexpr bool value = false;
		};

		template<typename T>
		struct same<T, T> {
			static constexpr bool value = true;
		};
	} // namespace traits

	// Helper concepts for working with types. Mostly required because C++20 requires concepts instead of values in its
	// `requires`-block declarations.
	namespace concepts {
		template<typename T, typename U>
		// Checks whether two types denote the same type.
		// Essentially just `type_traits::same<T, U>::value`, but required to be a concept by C++20.
		concept same = type_traits::same<T, U>::value;
	} // namespace concepts

	typedef unsigned char _u8;
	typedef unsigned short _u16;
	typedef unsigned long _u64;
	typedef char8_t c8;

	struct string_slice {
		const c8 *data;
		_u64 length;
	};

	template<typename T>
	concept Write = requires(T t, const string_slice str) {
		{ t.write_str(str) } -> concepts::same<void>;
	};

	// Information about the formatter state that we are working with, i.e. file, standard output, error, etc.
	struct formatter {
		HANDLE handle;

		void write_str(const string_slice str) const {
			WriteFile(this->handle, str.data, str.length, nullptr, nullptr);
		}
	};

	// Information about a single parsed argument.
	struct argument {
		// Required for Display implementations to know what formatter they are printing into.
		formatter &_fmt;

		bool debug;
		bool pretty;

		_u16 width;
		_u16 precision;
	};

	template<typename T>
	concept Display = requires(T t, argument &arg) {
		// NOTE(kiara): We force display to return void to avoid users expecting that its return value (for example the
		// amount of written bytes) might be used by the formatting functions.
		{ t.display(arg) } -> concepts::same<void>;
	};

	template<Display T>
	// Dummy function which does nothing except to allow us to easily print built-in types.
	inline void display(argument &arg, T &value) {
		value.display(arg);
	}

	// Replace this with a better, faster procedure.
	void format_integral_as_string(int number, c8 buffer[16], _u64 *out_length) {
		if (number == 0) {
			buffer[0] = '0';
			*out_length = 1;
			return;
		}

		// We will need it later during reversing, as in the "stringification" loop we move the buffer pointer forwards, but
		// during reversing we need the whole buffer.
		c8 *old_buffer = buffer;
		_u64 length = 0;

		// Can we somehow refactor this to avoid having to duplicate so much code? But I want to avoid having a branch on
		// every loop iteration.
		if (number < 0) {
			while (number != 0) {
				*buffer = '0' - (number % 10);
				number /= 10;
				++buffer;
				++length;
			}

			*buffer = '-';
			++length;
		} else {
			while (number != 0) {
				// Simplest possible int-to-string algorithm. However, it gives a "backwards" representation of the integer (i.e.
				// least to most significant), so we have to reverse it after.
				*buffer = '0' + (number % 10);
				number /= 10;
				++buffer;
				++length;
			}
		}

		// This it the actual reversing step. It simply loops over all pairs of characters and swaps them.
		for (_u64 i = 0; i < (length / 2); ++i) {
			c8 t = old_buffer[length - 1 - i];
			old_buffer[length - 1 - i] = old_buffer[i];
			old_buffer[i] = t;
		}

		*out_length = length;
	}

	// NOTE(kiara): This is pretty hackish, but it should work fine.
	void display(argument &arg, int value) {
		c8 buffer[16];
		_u64 length = 0;
		format_integral_as_string(value, buffer, &length);
		arg._fmt.write_str({ .data = buffer, .length = length });
	}

	void display(argument &arg, const char *value) {
		arg._fmt.write_str({ .data = reinterpret_cast<const c8 *>(value), .length = static_cast<_u64>(strlen(value)) });
	};

	namespace utf8 {
		// Returns length of a UTF-8 encoded string in BYTES.
		_u64 strlen(const c8 *str) {
			_u64 length = 0;
			// Since UTF-8 allows strings to contain null byte, we should handle them. Fortunately for us, null byte in C++
			// strings means an end of the string literal. Thus, we don't need to handle them at all!
			while (*str++ != 0) ++length;
			return length;
		}
	} // namespace utf8

	void display(argument &arg, const c8 *value) {
		arg._fmt.write_str({ .data = value, .length = utf8::strlen(value) });
	}

	// Information about the current printing state, required for passing information between handling each argument
	// separately.
	struct state {
		const c8 *str;
		_u64 offset;
		// To avoid recomputing length every time we want to use it.
		_u64 length;
		// To know into which formatter are we writing to.
		formatter &fmt;
	};

	template<typename T>
	void process(state &st, T &value) {
		_u64 buffer_length = 0;

		for (_u64 i = st.offset; i < st.length; ++i) {
			const c8 c = st.str[i];

			if (c == '{') {
				WriteFile(st.fmt.handle, st.str + st.offset, buffer_length, nullptr, nullptr);
				const c8 next_c = st.str[i + 1];

				if (next_c == '}') {
					argument arg = { ._fmt = st.fmt };
					display(arg, value);
					st.offset = i + 2;
					return;
				}

				WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), "invalid character after {\n", 26, nullptr, nullptr);
				ExitThread(1);
			}

			++buffer_length;
		}
	}

	template<typename... Ts>
	// Writes a newline-terminated string into the output specified by the formatter.
	void print(const c8 *format_string, const Ts &...arguments) {
		HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		if (stdout == nullptr) {
			// We can't even print any information because we have no handle to the standard output.
			ExitThread(1);
		}

		formatter fmt = { stdout };
		state st = { .str = format_string, .offset = 0, .length = utf8::strlen(format_string), .fmt = fmt };

		(process(st, arguments), ...);

		// Flush any remaining text.
		if (st.offset < st.length) {
			WriteFile(stdout, st.str + st.offset, st.length - st.offset, nullptr, nullptr);
		}
	}
} // namespace fmt

#endif // _fmt_h_included
