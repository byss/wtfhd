//
//  main.cpp
//  wtfhd
//
//  Created by Kirill Bystrov on 7/19/20.
//

#include <iostream>

#include "node_info.hxx"
#include "children_policy.hxx"

using namespace fs;
using namespace std;

template <typename _Tp>
struct size_info {
	static char const *const type_name;
	static auto constexpr type_size = sizeof (_Tp);
};

template <typename _Tp>
auto const size_info <_Tp>::type_name = typeid (_Tp).name ();


template <typename _Tp>
ostream &operator << (ostream &stream, size_info <_Tp> info) {
	return stream << "sizeof (" << decltype (info)::type_name  << ") = " << decltype (info)::type_size;
}

int main (int argc, const char *argv []) {
	cout << size_info <node_info> () << endl;
	cout << size_info <file_info> () << endl;
	cout << size_info <dir_info> () << endl;
	cout << size_info <link_info> () << endl;

	try {
		vector <filesystem::path> roots (argv + 1, argv + argc);
		if (roots.empty ()) {
			roots.emplace_back (filesystem::current_path ());
		}
		
		auto &policy = children_policy::make ();
		for (auto &path: roots) {
			policy.add_root (path);
		}

		for (auto const &path: roots) {
			auto di = node_info::make (filesystem::path (path), policy);
			di->load_info (policy);
			cout << di->name () << '\t' << di->size () << endl;
		}
		policy.set_fs_boundaries_policy (boundaries_policy::transparent);
		return EXIT_SUCCESS;
	} catch (system_error const &e) {
		auto const &code = e.code ();
		cerr << "Unhandled " << code.category ().name () << " error: " << code.message () << " (" << code.value () << ")" << endl;
		return EXIT_FAILURE;
	} catch (exception const &e) {
		cerr << "Unhandled exception: " << e.what () << endl;
		return EXIT_FAILURE;
	}
}
