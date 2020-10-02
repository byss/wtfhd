//
//  main.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/19/20.
//

#include <iostream>

#include "node_info.hxx"
#include "children_policy.hxx"

#include "main_window.hxx"
#include "progress_window.hxx"

using namespace fs;
using namespace ui;
using namespace std;

int main (int argc, const char *argv []) {
	vector <filesystem::path> roots (argv + 1, argv + argc);
	if (roots.empty ()) {
		roots.emplace_back (filesystem::current_path ());
	}
	
	auto policy = children_policy::make_unique ();
	policy->set_fs_boundaries_policy (boundaries_policy::transparent);
	for (auto &path: roots) {
		policy->add_root (path);
	}
	
	ui::screen::shared ()->make_root <main_window> (policy->copy ());

	try {
		return ui::main ();
	} catch (system_error const &e) {
		auto const &code = e.code ();
		cerr << "Unhandled " << code.category ().name () << " error: " << code.message () << " (" << code.value () << ")" << endl;
		return EXIT_FAILURE;
	} catch (exception const &e) {
		cerr << "Unhandled exception: " << e.what () << endl;
		return EXIT_FAILURE;
	}
}
