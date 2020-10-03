//
//  window.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/30/20.
//

#include "window.hxx"

#include <cctype>
#include <string>
#include <panel.h>
#include <ncurses.h>

#include "screen.hxx"

using namespace ui;
using namespace std;

static inline pair <string::const_iterator, string::const_iterator> find_line_break (string::const_iterator const &start, int const output_maxlen, ptrdiff_t const str_maxlen);

window::window (std::shared_ptr <ui::screen> const &screen, rect frame) noexcept: ui::screen::window (screen) {
	WINDOW *impl = newwin (frame.height, frame.width, frame.y, frame.x);
	box (impl, 0, 0);
	nodelay (impl, true);
	wmove (impl, 1, 1);
	
	PANEL *panel = new_panel (impl);
	this->_panel = panel;
}

window::~window () {
	delwin (this->impl ());
	del_panel (this->_panel);
}

WINDOW *window::impl () const noexcept {
	return panel_window (this->_panel);
}

rect window::frame () const noexcept {
	rect result;
	getbegyx (this->impl (), result.y, result.x);
	getmaxyx (this->impl (), result.height, result.width);
	result.width -= result.x;
	result.height -= result.y;
	return result;
}

void window::print (string const &str) const noexcept {
	this->print (str, false);
}

void window::println (string const &str) const noexcept {
	this->print (str, !str.ends_with ('\n'));
}

void window::print (string const &str, bool append_newline) const noexcept {
	int x, y;
	getyx (this->impl (), y, x);
	rect const frame = this->frame ().inset (1, 1);
	
	for (auto pos = str.begin (); pos < str.end (); ) {
		auto const [line_end, next] = find_line_break (pos, frame.width - x, str.end () - pos);
		
		if (line_end != pos) {
			waddnstr (this->impl (), pos.base (), static_cast <int> (line_end - pos));
		}
		if (append_newline || (next < str.end ())) {
			y++;
			x = 1;
			wmove (this->impl (), y, x);
		}
		
		pos = next;
	}
}

void window::load (class screen &screen) {
	this->assign_to_screen (screen, [] (auto &stack) { return stack.size () - 1; });
	this->window_did_load ();
}

void window::activate () {
	this->window_will_appear ();
	this->_sources.with_value ([this] (sources_set &sources) {
		for (auto source: sources) {
			this->get_run_loop ()->add_event_source (source);
		}
	});
	this->refresh ();
	this->window_did_appear ();
}

void window::deactivate () {
	this->window_will_disappear ();
	this->_sources.with_value ([this] (sources_set &sources) {
		for (auto source: sources) {
			this->get_run_loop ()->remove_event_source (source);
		}
	});
	this->window_did_disappear ();
}

void window::add_mouse_button_handler (long button, mouse::handler_cref handler) {
	if (this->_mouse.expired ()) {
		auto source = make_shared <mouse> (mouse (*this));
		this->_mouse = source;
		this->add_run_loop_event_source (source);
	}
	
	this->_mouse.lock ()->add_handler (button, [&] (long button, int x, int y, int z) {
		if (!wmouse_trafo (this->impl (), &y, &x, false)) {
			return;
		}
		handler (button, x, y, z);
	});
}

void window::add_key_handler (int key, keyboard::handler_cref handler) {
	if (this->_keyboard.expired ()) {
		auto source = make_shared <keyboard> (keyboard (*this));
		this->_keyboard = source;
		this->add_run_loop_event_source (source);
	}
	this->_keyboard.lock ()->add_handler (key, handler);
}

void window::remove_timer (weak_ptr <timer> t) {
	this->remove_run_loop_event_source (t.lock ());
}

void window::remove_timer (shared_ptr <timer> t) {
	this->remove_run_loop_event_source (t);
}

void window::remove_mouse_handler (long button, mouse::handler_cref handler) {
	if (this->_mouse.expired ()) {
		return;
	}
	
	auto mouse = this->_mouse.lock ();
	mouse->remove_handler (button, handler);
	if (mouse->empty ()) {
		this->remove_run_loop_event_source (mouse);
	}
}

void window::remove_key_handler (int key, keyboard::handler_cref handler) {
	if (this->_keyboard.expired ()) {
		return;
	}
	
	auto keyboard = this->_keyboard.lock ();
	keyboard->remove_handler (key, handler);
	if (keyboard->empty ()) {
		this->remove_run_loop_event_source (keyboard);
	}
}

void window::add_run_loop_event_source (std::shared_ptr <event_source> source) noexcept {
	auto [it, success] = this->_sources.with_value ([this, source] (sources_set &sources) {
		return sources.emplace (source);
	});
	if (!success) {
		return;
	}
	if (this->is_top ()) {
		this->get_run_loop ()->add_event_source (*it);
	}
}

void window::remove_run_loop_event_source (std::shared_ptr <event_source> source) noexcept {
	this->_sources.with_value ([this, source] (sources_set &sources) {
		auto it = sources.find (source);
		if (it != sources.end ()) {
			sources.erase (it);
		}
	});

	this->get_run_loop ()->remove_event_source (source);
}

std::function <void (std::function <void (WINDOW *)> const &)> ui::window::with_window_impl_from_this () const noexcept {
	return std::bind (std::mem_fn (&ui::window::with_window_impl), this, std::placeholders::_1);
}

void window::refresh () const {
	wrefresh (this->impl ());
	update_panels ();
}

static inline pair <string::const_iterator, string::const_iterator> find_line_break (string::const_iterator const &start, int const output_maxlen, ptrdiff_t const str_maxlen) {
	if (!output_maxlen) {
		return { start, start };
	}
	if (output_maxlen >= str_maxlen) {
		auto const next = start + str_maxlen;
		auto line = string_view (start.base (), str_maxlen);
		auto const whitespace_start = find_if_not (line.rbegin (), line.rend (), isspace);
		return { start + (line.rend () - whitespace_start), next };
	}
	
	auto const line = string_view (start.base (), output_maxlen);
	auto const newline_pos = line.find ('\n');
	if (newline_pos != string::npos) {
		auto const line_end = start + newline_pos;
		return { line_end, line_end + 1 };
	}
	
	auto const exact_cutoff = start + output_maxlen;
	if (isspace (*exact_cutoff)) {
		auto const whitespace_begin = exact_cutoff - (find_if_not (line.rbegin (), line.rend (), isspace) - line.rbegin ());
		auto const tail = string_view (exact_cutoff.base (), str_maxlen - output_maxlen);
		auto const whitespace_end = exact_cutoff + (find_if_not (tail.begin (), tail.end (), isspace) - tail.begin ());
		return { whitespace_begin, whitespace_end };
	} else {
		auto const prev_whitespace = find_if (line.rbegin (), line.rend (), isspace);
		if (prev_whitespace == line.rend ()) {
			return { exact_cutoff, exact_cutoff };
		}
		auto const whitespace_end = exact_cutoff - (prev_whitespace - line.rbegin ());
		auto const whitespace_begin = whitespace_end - (find_if_not (prev_whitespace, line.rend (), isspace) - prev_whitespace);
		return { whitespace_begin, whitespace_end };
	}
}
