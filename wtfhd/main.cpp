//
//  main.cpp
//  wtfhd
//
//  Created by Kirill Bystrov on 7/19/20.
//

#include <iostream>

#include "node_info.hxx"

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
	return stream << "sizeof (" << decltype (info)::type_name  << ") = " << decltype (info)::type_size << endl;
}

int main (int argc, const char *argv []) {
	vector <filesystem::path> roots (argv + 1, argv + argc);
	if (roots.empty ()) {
		roots.emplace_back (filesystem::current_path ());
	}
	for (auto path: roots) {
		auto di = make_node_info (path);
		di->load_info ();
		cout << di->name () << '\t' << di->size ();
	}
	return 0;
}
