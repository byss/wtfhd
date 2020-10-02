//
//  cxx_argpacks.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 9/25/20.
//

#ifndef cxx_argpacks_h
#define cxx_argpacks_h

#include <utility>

namespace util {
	template <typename ..._Types>
	struct pack {
	private:
		template <template <typename ...> typename _Base, typename _Head, typename ..._Tail>
		static _Base <_Tail...> tail_helper (_Base <> &, _Head &, _Tail &&...);
		
	public:
		typedef std::tuple_element_t <0, std::tuple <_Types...>> head_type;
		
		template <template <typename ...> typename _Base>
		using tail_type = std::invoke_result_t <decltype (tail_helper), _Base <>, _Types...> ();
	};
	template <typename _Tp>
	struct pack <_Tp> {
		typedef _Tp head_type;
		
		template <template <typename ...> typename _Base>
		using tail_type = _Base <>;
	};
	template <>
	struct pack <> {};

	template <typename ..._Types>
	using pack_head_t = typename pack <_Types...>::head_type;
	template <template <typename ...> typename _Base, typename ..._Types>
	using pack_tail_t = typename pack <_Types...>::template tail_type <_Base>;
}

#endif /* cxx_argpacks_h */
