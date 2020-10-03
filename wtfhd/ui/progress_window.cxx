//
//  progress_window.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 8/8/20.
//

#include "progress_window.hxx"

using namespace ui;

void progress_window::window_did_load () {
	window::window_did_load ();
	
	this->add_key_handler ('q', std::bind (&progress_window::abort_builder, this));
	this->_builder->add_progress_callback (std::bind (&progress_window::builder_progress_did_update, this));
}

void progress_window::window_did_appear () {
	window::window_did_appear ();
	
	if (!this->_builder->started ()) {
		this->_builder->start (std::bind (&progress_window::builder_did_finish, this));
	}
}

void progress_window::builder_progress_did_update () {
	this->invoke_callback (&progress_window::println, this, this->_builder->progress ().ratio ());
}

void progress_window::builder_did_finish () {
	this->invoke_callback (&progress_window::pop, this);
}

void progress_window::abort_builder () {
	this->_builder->cancel ();
}
