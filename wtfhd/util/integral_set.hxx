//
//  integral_set.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 8/8/20.
//

#ifndef integral_set_hxx
#define integral_set_hxx

#include <map>
#include <bitset>
#include <vector>
#include <climits>
#include <type_traits>

#include "cxx_argpacks.hxx"

namespace util {
	template <std::size_t _N>
	struct is_power_of_two {
		static constexpr bool value = !(_N & 1) && is_power_of_two <_N / 2>::value;
	};
	template <>
	struct is_power_of_two <1> {
		static constexpr bool value = true;
	};
	template <>
	struct is_power_of_two <0> {
		static constexpr bool value = false;
	};
	template <std::size_t _N>
	constexpr bool is_power_of_two_v = is_power_of_two <_N>::value;
	
	template <typename _Tp, std::size_t _N>
	struct integral_set_storage_small;
	template <typename _Tp, std::size_t _N>
	struct integral_set_storage_normal;
	template <typename _Tp, std::size_t _N = sizeof (_Tp) * CHAR_BIT>
	using integral_set_storage = std::conditional_t <(_N > 16), integral_set_storage_normal <_Tp, _N>, integral_set_storage_small <_Tp, _N>>;
	
	template <typename _Tp>
	struct integral_set_base;
	template <typename _Tp, typename ..._Types>
	struct integral_set_multi;
	template <typename ..._Types>
	struct integral_set_wrapper {
		typedef integral_set_multi <_Types...> type;
	};
	template <typename _Tp>
	struct integral_set_wrapper <_Tp> {
		typedef integral_set_base <_Tp> type;
	};
	template <>
	struct integral_set_wrapper <> {};

	template <typename ..._Types>
	using integral_set = typename integral_set_wrapper <_Types...>::type;

	template <typename _Tp>
	struct integral_set_base {
	public:
		static_assert (std::is_integral_v <_Tp>, "value_type must be of integral type");
		static_assert (is_power_of_two_v <sizeof (_Tp)>, "sizeof (value_type) must be a power of 2");
		
		typedef _Tp value_type;
		
		integral_set_base (): _storage {} {}
		integral_set_base (std::initializer_list <_Tp> const &values): integral_set_base (values.begin (), values.end ()) {}

		~integral_set_base () = default;
		
		bool contains (value_type const value) const {
			return this->_storage.contains (value);
		}
		
		bool insert (value_type const value) {
			return this->_storage.insert (value);
		}
		
		bool remove (value_type const value) {
			return this->_storage.remove (value);
		}
		
	protected:
		template <typename _It>
		integral_set_base (_It values_begin, _It values_end): _storage (values_begin, values_end) {}
		
	private:
		using storage = integral_set_storage <_Tp>;
		storage _storage;
	};
		
	template <typename _Tp, typename ..._Types>
	struct integral_set_multi {
		static_assert (sizeof... (_Types), "invalid use of integral_set_multi");
		
	public:
		integral_set_multi (): _storage {} {}
		integral_set_multi (std::initializer_list <std::tuple <_Tp, _Types...>> const &values): integral_set_multi (values.begin (), values.end ()) {}
		
		~integral_set_multi () = default;
		
		bool contains (_Tp const value_head, _Types const ...value_tail) const {
			auto const &it = this->_storage.find (value_head);
			return (it != this->_storage.end ()) && it->second.contains (value_tail...);
		}
				
		bool insert (_Tp const value_head, _Types const ...value_tail) {
			auto const &it = this->_storage.find (value_head);
			if (it != this->_storage.end ()) {
				return it->second.insert (value_tail...);
			} else {
				this->_storage.insert ({ value_head, { value_tail... } });
				return true;
			}
		}
		
		bool remove (_Tp const value_head, _Types const ...value_tail) {
			auto const &it = this->_storage.find (value_head);
			if (it == this->_storage.end ()) {
				return false;
			}
			return it->second.remove (value_tail...);
		}
		
	protected:
		template <typename _It>
		integral_set_multi (_It values_begin, _It values_end) {
			for (auto it = values_begin; it != values_end; it++) {
				std::apply (&integral_set_multi::insert, std::tuple_cat (std::make_tuple (this), *it));
			}
		}

	private:
		using storage_key = _Tp;
		using storage_mapped = integral_set <_Types...>;
		using storage = std::map <storage_key, storage_mapped>;
		
		storage _storage;
	};
	
	template <typename _Tp, std::size_t _N>
	struct integral_set_storage_small {
	public:
		integral_set_storage_small () {}
		integral_set_storage_small (std::initializer_list <_Tp> const &values): integral_set_storage_small (values.begin (), values.end ()) {}
		
		template <typename _It>
		integral_set_storage_small (_It start, _It end) {
			for (auto &it = start; it != end; it++) {
				this->insert (*it);
			}
		}

		bool contains (_Tp const value) const {
			return this->_bits [value];
		}
		
		bool insert (_Tp const value) {
			bool result = true;
			std::swap (this->_bits [value], result);
			return !result;
		}
		
		bool remove (_Tp const value) {
			bool result = false;
			std::swap (this->_bits [value], result);
			return result;
		}
		
	private:
		std::bitset <(1 << _N)> _bits;
	};
	
	template <typename _Tp, std::size_t _N>
	struct integral_set_storage_normal {
		integral_set_storage_normal () {}
		integral_set_storage_normal (std::initializer_list <_Tp> const &values): integral_set_storage_normal (values.begin (), values.end ()) {}

		template <typename _It>
		integral_set_storage_normal (_It start, _It end) {
			for (auto &it = start; it != end; it++) {
				this->insert (*it);
			}
		}
		
		bool contains (_Tp const value) const {
			auto const &it = std::lower_bound (this->_prefixes.begin (), this->_prefixes.end (), value, prefix::is_less);
			return (it != this->_prefixes.end ()) && (it->is_equal (value)) && this->_suffixes [it->index].contains (value & suffix_mask);
		}
		
		bool insert (_Tp const value) {
			auto const &it = std::lower_bound (this->_prefixes.begin (), this->_prefixes.end (), value, prefix::is_less);
			if ((it != this->_prefixes.end ()) && it->is_equal (value)) {
				return this->_suffixes [it->index].insert (value & suffix_mask);
			} else {
				this->_prefixes.insert (it, prefix (value, static_cast <typename prefix::value_type> (this->_suffixes.size ())));
				this->_suffixes.push_back ({ value & suffix_mask });
				return true;
			}
		}
		
		bool remove (_Tp const value) {
			auto const &it = std::lower_bound (this->_prefixes.begin (), this->_prefixes.end (), value, prefix::is_less);
			if ((it == this->_prefixes.end ()) || !it->is_equal (value)) {
				return false;
			}
			return this->_suffixes [it->index].remove (value & suffix_mask);
		}
		
	private:
		static std::size_t constexpr component_bits = _N / 2;
		static constexpr _Tp suffix_mask = ~(_Tp (0)) >> component_bits;
		
		struct prefix {
			typedef _Tp value_type;
			
			static std::size_t constexpr bit_width = component_bits;
			
			_Tp value: bit_width;
			_Tp index: bit_width;
			
			static constexpr bool is_less (prefix const prefix, _Tp const value) {
				return prefix.value < (value >> bit_width);
			}
			
			prefix (_Tp const value, _Tp const index): value (value >> bit_width), index (index) {}

			constexpr bool is_equal (_Tp const value) const {
				return this->value == (value >> bit_width);
			}
		};
		
		std::vector <prefix> _prefixes;
		std::vector <integral_set_storage <_Tp, component_bits>> _suffixes;
	};
}

#endif /* integral_set_hxx */
