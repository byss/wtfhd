//
//  screen.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#include "screen.hxx"

namespace ncurses {
	#include <ncurses.h>
}

#include "window.hxx"

using namespace ui;
using namespace std;

shared_ptr <screen> screen::shared () {
	static thread_local weak_ptr <screen> instance;
	if (instance.expired ()) {
		auto result = shared_ptr <screen> (new screen ());
		instance = result;
		return result;
	} else {
		return instance.lock ();
	}
}

int screen::run_main () {
	auto const result = screen::shared ()->_run_loop.run ();
	for (auto const shared = screen::shared (); !shared->_windows.empty (); shared->pop ());
	return result;
}

void screen::exit (int rc) {
	screen::shared ()->_run_loop.exit (rc);
}

screen::screen () {
	using namespace ncurses;
	initscr ();
	cbreak ();
	noecho ();
	timeout (0);
	nonl ();
	keypad (stdscr, true);
}

screen::~screen () {
	while (!this->_windows.empty ()) {
		this->pop ();
	}
	using namespace ncurses;
	endwin ();
}

rect screen::bounds () const {
	using namespace ncurses;
	return rect (0, 0, COLS, LINES);
}
