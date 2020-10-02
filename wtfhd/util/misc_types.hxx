//
//  misc_types.h
//  wtfhd
//
//  Created by Kirill Bystrov on 9/25/20.
//

#ifndef misc_types_hxx
#define misc_types_hxx

#include <cmath>
#include <string>
#include <climits>
#include <cassert>
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
		
		tristate_bool_base (): tristate_bool_base (storage_v <false, false>) {}
		tristate_bool_base (std::nullopt_t): tristate_bool_base () {}
		tristate_bool_base (std::nullptr_t): tristate_bool_base () {}
		tristate_bool_base (bool value): tristate_bool_base (value ? storage_v <true, true> : storage_v <true, false>) {}
		~tristate_bool_base () = default;
		
		bool has_value () const {
			return this->_storage;
		}
		
		bool value () const {
			assert (this->has_value ());
			return this->raw_value ();
		}
		
		operator std::optional <bool> () const {
			return this->has_value () ? std::optional (this->raw_value ()) : std::nullopt;
		}
		
	private:
		static uintmax_t constexpr ready_mask = 0b01;
		static uintmax_t constexpr value_mask = 0b10;
		
		template <bool ready, bool value>
		static uintmax_t constexpr storage_v = (ready ? ready_mask : 0x00) | (value ? value_mask : 0x00);
		
		explicit tristate_bool_base (uintmax_t const storage): _storage (storage) {}
		
		bool raw_value () const {
			return (this->_storage & value_mask) != 0;
		}
		
		uintmax_t _storage: _Sz;
	};
	
	template <>
	struct tristate_bool_base <sizeof (std::optional <bool>) * CHAR_BIT>: public std::optional <bool> {};
	
	typedef tristate_bool_base <> tristate_bool;
	typedef tristate_bool_base <8> tristate_bool_8;
	typedef tristate_bool_base <32> tristate_bool_32;
	typedef tristate_bool_base <64> tristate_bool_64;
}

#endif /* misc_types_hxx */
