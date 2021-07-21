// fmt.hh
// Copyright 2021 by Takanashi Kiara.
// Released under 0-clause BSD license, see LICENSE file for more information.
// See also https://fmt-cxx.rtfd.io for documentation.

#ifndef kiara_fmt_hh_included
#define kiara_fmt_hh_included

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef stdout
#pragma push_macro("stdout")
#undef stdout
#define _old_stdout
#endif // stdout

// Simple custom formatting and printing library for the native-vulkan project.
// We probably can move it to a separate project with its own documentation,
// releases, etc.
namespace fmt {
	// Thread-local storage for type-erased arguments that are going to be parsed
	// by format_args, and then passed into proper formatting functions, interspersed
	// with string slices. 8 is probably the optimal size, taking "only" 64 bytes,
	// but accounting for pretty much almost all cases I can imagine.
	thread_local const void *_type_erased_args[8];

	typedef unsigned long long uint;

	struct _write {
		virtual void write_str(const char *string, uint length) = 0;
	};

	struct stdout : _write {
		inline void write_str(const char *string, uint length) override {
			WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), string, length, nullptr, nullptr);
		}
	};

	struct formatter {
		// Current printing destination.
		_write &buffer;
	};

	namespace traits {
		// Decays an array (float []) into a pointer (float *).
		template<typename T> struct decay_array {
			typedef T type;
		};

		template<typename T> struct decay_array<T[]> {
			typedef T *type;
		};

		template<typename T, uint N> struct decay_array<T[N]> {
			typedef T *type;
		};

		// If-else for types.
		template<bool B, typename T, typename U> struct either {
			typedef T type;
		};

		template<typename T, typename U> struct either<false, T, U> {
			typedef U type;
		};

		// Checks for whether two types are equivalent (without any type decay or
		// implicit conversions.)
		template<typename T, typename U> struct same {
			static constexpr bool value = false;
		};

		template<typename T> struct same<T, T> {
			static constexpr bool value = true;
		};

		// Helper traits for working with the variety of integer types in C++.
		template<typename T> struct is_integral {
			static constexpr bool value = false;
		};

		template<> struct is_integral<unsigned int> {
			static constexpr bool value = true;
		};

		template<> struct is_integral<unsigned long long> {
			static constexpr bool value = true;
		};

		// Fundamental types can't implement display concept, so we need to have a
		// fallback trait for checking whether they are still suitable (i.e. const
		// char *, int, etc.)
		template<typename T> struct is_printable {
			static constexpr bool value = false;
		};

		template<typename T> concept display = requires(T &t, const formatter &f) {
			{ t.display(f) };
		};

		// Every integer type is printable.
		template<typename T> requires is_integral<T>::value
		struct is_printable<T> {
			static constexpr bool value = true;
		};

		// Nul-terminated strings are always printable.
		template<> struct is_printable<char *> {
			static constexpr bool value = true;
		};
	}

	constexpr const char *decimal_lookup_table =
		"00010203040506070809"
		"10111213141516171819"
		"20212223242526272829"
		"30313233343536373839"
		"40414243444546474849"
		"50515253545556575859"
		"60616263646566676869"
		"70717273747576777879"
		"80818283848586878889"
		"90919293949596979899";

	// We need to put "display" function into a struct so we can overload it from
	// outside the namespace, but we don't want to keep that struct in the general
	// namespace, so we just put it into a separate namespace.
	namespace impl {
		// Catch-all and the base case. If there goes something wrong that type passes
		// checks, but resolves into this, we will know because of the error of the
		// lack of fmt method.
		template<typename T> struct display { };

		template<typename T> requires traits::display<T>
		struct display<T> {
			inline static void fmt(const T &t, const formatter &f) {
				t.display(f);
			}
		};

		template<> struct display<const char *> {
			inline static void fmt(const char *t, const formatter &f) {
				f.buffer.write_str(t, strlen(t));
			}
		};

		template<> struct display<char *> : display<const char *> { };

		// Writes a decimal representation of the integer to the output.
		template<typename T> requires traits::is_integral<T>::value
		struct display<T> {
			static void fmt(const T &t, const formatter &f) {
				T number = t;
				// To format number in decimal we need at most 20 digits, but we use 32
				// here to optimise for the cache size.
				constexpr uint buffer_size = 32;
				char buffer[buffer_size] = { };
				char *end = buffer + buffer_size;

				while (number >= 100) {
					memcpy(end -= 2, &decimal_lookup_table[(number % 100) * 2], 2);
					number /= 100;
				}

				while (number >= 10) {
					*(--end) = '0' + (number % 10);
					number /= 10;
				}

				if (number < 10) {
					*(--end) = '0' + (number % 10);
				}

				const auto length = buffer_size - (end - (char *)buffer);
				f.buffer.write_str(buffer + (buffer_size - length), length);
			}
		};
	}

	struct _print_state {
		const char *string;
		uint offset, length;
		_write &buffer;
	};

	// We need to check whether T is a kind-of a char array, and if so, we decay it,
	// otherwise we just leave it as is, to allow for easy passing of the arrays
	// without having to wrap them in a separate type, such as std::array.
	//
	//     float nyoom[3] = { 0.1f, 0.2f, 0.3f };
	//     fmt::print(nyoom); // impl::display<float[3]>
	//
	//     char meow[] = "nyooom";
	//     fmt::print(meow); // impl::display<char *>
	template<typename T> using decayed_type = typename traits::either<traits::same<typename traits::decay_array<T>::type, char *>::value, typename traits::decay_array<T>::type, T>::type;

	template<typename T> void _parse_fmts(_print_state &state, const T &t) {
		uint length = 0;

		for (uint i = state.offset; i < state.length; i++) {
			auto c = state.string[i];

			if (c == '%') {
				// Dump the buffer before trying to display an argument.
				state.buffer.write_str(state.string + state.offset, length);

				formatter f{state.buffer};
				// Call display function on t, break the loop, update the offsets.
				impl::display<decayed_type<T>>::fmt(t, f);
				++length;
				break;
			}

			// Otherwise, move the offset (we use a temporary value to avoid referencing
			// memory.)
			++length;
		}

		state.offset += length;
	}

	struct char_traits {
		static constexpr uint length(const char *string) noexcept {
			uint length = 0;
			while(*string++) ++length;
			return length;
		}
	};

	inline void write(const char *string) {
		stdout buffer{};
		buffer.write_str(string, char_traits::length(string));
	}

	inline void write(const char *string, uint length) {
		stdout buffer{};
		buffer.write_str(string, length);
	}

	// Ugly hack to support having print declarations without any arguments. This
	// is because a) we cannot have references to void, which empty argument pack
	// coerces into, and b) void does not support any of the required traits. This
	// does mean though that we need to do print<void>(...) to properly dispatch
	// into a correct overload. Maybe we should add something like fmt::write?
	// DEPRECATED: Use fmt::write instead.
	template<typename T> inline void print(const char *string) {
		fmt::write(string);
	}

	// Yay so long!
	template<typename... Ts> requires (traits::display<Ts>, ...) || (traits::is_printable<decayed_type<Ts>>::value, ...)
	void print(const char *format_string, const Ts &...ts) {
		stdout buffer{};
		_print_state state{format_string, 0, strlen(format_string), buffer};
		(_parse_fmts(state, ts), ...);

		// If there is anything left in the state buffer (offset < length), dump it.
		if (state.offset < state.length) {
			buffer.write_str(state.string + state.offset, state.length - state.offset);
		}
	}
}

#ifdef _old_stdout
#pragma pop_macro("stdout")
#undef _old_stdout
#endif // _old_stdout

#endif // kiara_fmt_hh_included
