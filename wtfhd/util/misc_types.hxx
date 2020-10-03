//
//  misc_types.h
//  wtfhd
//
//  Created by Kirill Bystrov on 9/25/20.
//

#ifndef misc_types_hxx
#define misc_types_hxx

#include <cmath>
#include <mutex>
#include <string>
#include <climits>
#include <cassert>
#include <optional>
#include <functional>

namespace util {
	typedef std::function <void ()> callback_t;
	typedef std::uintptr_t callback_id_t;
	
	template <typename _Tp>
	std::size_t constexpr digits_count (_Tp value) {
		static_assert (std::is_unsigned_v <_Tp>, "invalid value");
		
		if (value < 1000) {
			if (value < 10) {
				return 1;
			} else if (value >= 100) {
				return 3;
			} else {
				return 2;
			}
		} else if (value < 1000000) {
			if (value < 10000) {
				return 4;
			} else if (value >= 100000) {
				return 6;
			} else {
				return 5;
			}
		} else if (value < 1000000000) {
			if (value < 10000000) {
				return 7;
			} else if (value >= 100000000) {
				return 9;
			} else {
				return 8;
			}
		} else {
			return 9 + digits_count (value / 1000000000);
		}
	}

	struct progress_t {
		progress_t (std::size_t ready, std::size_t total): ready (ready), total (total) {}
		
		std::string percents () const {
			assert (this->ready <= this->total);
			auto const promille = std::lrint (std::round (static_cast <double> (this->ready) / static_cast <double> (this->total) * 1000.0));
			char buffer [7];
			std::snprintf (buffer, std::extent_v <decltype (buffer)>, "%ld.%ld%%", promille / 10, promille % 10);
			return buffer;
		}
		
		std::string ratio () const {
			assert (this->ready <= this->total);
			auto const buffer_len = digits_count (this->ready) + digits_count (this->total) + 2;
			char buffer [buffer_len];
			std::snprintf (buffer, buffer_len, "%ld/%ld", this->ready, this->total);
			return buffer;
		}
		
		std::size_t const ready;
		std::size_t const total;
	};
	
	template <std::size_t _Sz = sizeof (std::uintptr_t) * CHAR_BIT>
	struct tristate_bool_base {
	public:
		static_assert (_Sz > 1, "tristate_bool requires at least 2 bits of storage");
		
		tristate_bool_base (): _storage { false, false } {}
		tristate_bool_base (std::nullopt_t): tristate_bool_base () {}
		tristate_bool_base (std::nullptr_t): tristate_bool_base () {}
		tristate_bool_base (bool value): _storage { true, value } {}
		~tristate_bool_base () = default;
		
		bool has_value () const {
			return this->_storage.ready ();
		}
		
		bool value () const {
			assert (this->has_value ());
			return this->_storage.value ();
		}
		
		bool operator* () const {
			return this->value ();
		}
		
		operator std::optional <bool> () const {
			return this->has_value () ? std::optional (this->_storage.value ()) : std::nullopt;
		}
		
		bool operator== (tristate_bool_base const &other) const {
			return this->_storage == other.storage;
		}
		
	private:
		union storage {
		private:
			struct bit_v {
			public:
				constexpr bit_v (bool const b): _value (b ? 1 : 0) {}
				
				uintmax_t constexpr value () const {
					return this->_value;
				}
				
			private:
				uintmax_t _value;
			};
			
		public:
			constexpr storage (bool ready, bool value): _bits { .ready = bit_v (ready).value (), .value = bit_v (value).value () } {}
			~storage () = default;
			
			bool ready () const {
				return this->_bits.ready;
			}
			
			bool value () const {
				return this->_bits.value;
			}

			bool operator== (storage const &other) const {
				return this->_bits == other._bits;
			}
			
		private:
			uintmax_t: _Sz;
			
			struct {
				uintmax_t ready: 1;
				uintmax_t value: 1;
			} _bits;
		} _storage;
	};
	
	template <typename _Tp>
	struct threadsafe {
	public:
		template <typename = std::enable_if_t <std::is_default_constructible_v <_Tp>>>
		threadsafe (): threadsafe (_Tp {}) {}
		
		threadsafe (_Tp const &value): _value (value) {}
		threadsafe (_Tp &&value): _value (value) {}
		
		~threadsafe () {
			std::scoped_lock lock (this->_mutex);
			
			if constexpr (std::is_invocable_v <decltype (&_Tp::clear), _Tp *>) {
				this->_value.clear ();
			} else if constexpr (std::is_default_constructible_v <_Tp> && std::is_swappable_v <_Tp>) {
				_Tp empty {};
				std::swap (this->_value, empty);
			}
		}
		
		template <typename _Fp, typename ..._Args, typename = std::enable_if_t <std::is_invocable_v <_Fp, _Tp &, _Args...>>>
		auto with_value (_Fp const &action, _Args &&...args) {
			std::scoped_lock lock (this->_mutex);
			return std::invoke (action, this->_value, std::forward <_Args> (args)...);
		}

	private:
		std::mutex _mutex;
		_Tp _value;
	};
	
	template <std::size_t _L, std::size_t _R>
	bool operator== (tristate_bool_base <_L> const &lhs, tristate_bool_base <_R> const &rhs) {
		if (lhs.has_value ()) {
			return rhs.has_value () && (*lhs == *rhs);
		} else {
			return !rhs.has_value ();
		}
	}
	
	template <>
	struct tristate_bool_base <sizeof (std::optional <bool>) * CHAR_BIT>: public std::optional <bool> {};
	
	typedef tristate_bool_base <> tristate_bool;
	typedef tristate_bool_base <8> tristate_bool_8;
	typedef tristate_bool_base <32> tristate_bool_32;
	typedef tristate_bool_base <64> tristate_bool_64;
}

#endif /* misc_types_hxx */
