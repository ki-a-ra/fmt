#ifndef _fmt_h_included
#define _fmt_h_included

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

	typedef unsigned char u8;
	typedef unsigned long u64;
	typedef char8_t c8;

	struct byte_slice {
		const u8 *data;
		u64 length;
	};

	template<typename T>
	concept Write = requires(T t, const c8 *str, u64 length) {
		{ t.write_str(str, length) } -> concepts::same<void>;
	};

	struct formatter {
	};

	template<typename T>
	concept Display = requires(T t, formatter &fmt) {
		// NOTE(freya): We force display to return void to avoid users expecting that its return value (for example the
		// amount of written bytes) might be used by the formatting functions.
		{ t.display(fmt) } -> concepts::same<void>;
	};

	template<Display T>
	// Dummy function which does nothing except to allow us to easily print built-in types.
	void display(formatter &fmt, T &argument) {
		argument.display(fmt);
	}

	// NOTE(kiara): This is pretty hackish, but it should work fine.
	void display(formatter &fmt, int argument) {
		// formatter.write(int_as_string(argument));
	}

	void display(formatter &fmt, const char *argument) {
		// formatter.write(argument);
	}

	template<typename... Ts>
	// Writes a newline-terminated string into the output specified by the formatter.
	void print(formatter &fmt, const char *format_string, const Ts &...arguments) {
	}
} // namespace fmt

#endif // _fmt_h_included
