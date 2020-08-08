//
//  event_source.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#include "event_source.hxx"

#include <algorithm>
#include <ncurses.h>

using namespace ui;
using namespace std;
using namespace chrono;

bool timer::has_event (time_point const &now) const {
	return this->_deadline >= now;
}

void timer::process_event (time_point const &now) {
	invoke (this->_handler, *this);
	if (this->_interval < duration::max ()) {
		while ((this->_deadline += this->_interval) <= now);
	}
}

timer::duration timer::next_event_interval (time_point const &now) const {
	return max (duration (), this->_deadline - now);
}

static inline void mmask_for_each (mmask_t mask, function <void (mmask_t const &)> const &action) {
	for (mmask_t mask_item; mask && (mask_item = 1 << __builtin_ctzl (mask)); mask &= ~mask_item) {
		invoke (action, mask_item);
	}
}

void mouse::add_handler (subevent_type subevent, handler_cref handler) {
	mmask_for_each (subevent, [&] (auto primitive_subevent) {
		input_device::add_handler (primitive_subevent, handler);
	});
	
	if (~this->_buttons_mask & subevent) {
		this->_buttons_mask |= subevent;
	}
}

void mouse::remove_handler (subevent_type subevent, handler_cref handler) {
	mmask_for_each (subevent, [&] (auto primitive_subevent) {
		input_device::remove_handler (primitive_subevent, handler);
	});
	
	if (!this->contains_handler (subevent) && (this->_buttons_mask & subevent)) {
		this->_buttons_mask &= ~subevent;
	}
}

void mouse::activate () {
	event_source::activate ();
	mousemask (this->_buttons_mask, NULL);
}

void mouse::deactivate () {
	mousemask (0, NULL);
	event_source::deactivate ();
}

bool mouse::has_event (time_point const &now) const {
	auto const c = this->with_window_impl <int> ([] (auto impl) { return wgetch (impl); });
	switch (c) {
	case KEY_MOUSE:
		return true;
	case ERR:
		break;
	default:
		ungetch (c);
	}
	return false;
}

void mouse::process_event (time_point const &now) {
	MEVENT event;
	if ((getmouse (&event) == ERR) || !(this->_buttons_mask & event.bstate)) {
		return;
	}
	;
	if (this->with_window_impl <bool> ([&] (auto impl) { return wmouse_trafo (impl, &event.y, &event.x, false); })) {
	}
	
	mmask_for_each (event.bstate, [&] (auto subevent) {
		this->invoke_handlers (subevent, event.x, event.y, event.z);
	});
}

mouse::duration mouse::next_event_interval (const time_point &now) const {
	return milliseconds (mouseinterval (-1));
}

bool keyboard::has_event (time_point const &now) const {
	auto const c = getch ();
	if ((c != ERR) && (this->contains_handler (c) || this->contains_handler (ERR))) {
		ungetch (c);
		return true;
	}
	return false;
}

void keyboard::process_event (time_point const &now) {
	auto const c = getch ();
	if (c == ERR) {
		return;
	}

	this->invoke_handlers (c);
	this->for_each_subevent_handler (ERR, [=] (auto const &it) {
		invoke (it->second, c);
	});
}

keyboard::duration keyboard::next_event_interval (const time_point &now) const {
	return milliseconds (50);
}

bool run_loop_idle::has_event (time_point const &now) const {
	return this->_last_invocation < now;
}

void run_loop_idle::process_event (time_point const &now) {
	invoke (this->_handler);
	this->_last_invocation = now;
}

run_loop_idle::duration run_loop_idle::next_event_interval (const time_point &now) const {
	return duration::zero ();
}
