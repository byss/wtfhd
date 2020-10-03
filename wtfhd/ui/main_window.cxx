//
//  main_window.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#include "main_window.hxx"

#include "tree_builder.hxx"
#include "progress_window.hxx"

using namespace fs;
using namespace ui;
using namespace std;
using namespace chrono;
using namespace chrono_literals;

main_window::main_window (unique_ptr <children_policy const> &&policy):
	window (), _builder { tree_builder::make_unique (forward <unique_ptr <children_policy const>> (policy)) } {}

void main_window::window_did_appear () {
	window::window_did_appear ();
	
	if (this->_builder->ready ()) {
		this->println (this->_builder->success () ? "Builder finished" : "Builder failed");
		this->refresh ();
		this->add_timer (1s, std::bind (ui::exit, 0));
	} else {
		this->push <progress_window> (this->_builder);
	}
}
