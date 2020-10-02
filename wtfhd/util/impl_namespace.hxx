//
//  impl_namespace.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 8/8/20.
//

#ifndef impl_namespace_hxx
#define impl_namespace_hxx

#define IMPL_MAKE_UNIQUE(_ns, _class) namespace _ns { namespace { \
	class _class; \
	std::unique_ptr <_ns::_class> make_unique_ ## _ns ## _ ## _class (); \
}} \
template <> \
std::unique_ptr <_ns::_class> std::make_unique <_ns::_class> () { return _ns::make_unique_ ## _ns ## _ ## _class (); }

#endif /* impl_namespace_hxx */
