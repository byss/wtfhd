//
//  run_loop.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#include "run_loop.hxx"

#include <map>
#include <thread>
#include <compare>
#include <optional>
#include <algorithm>

#include "event_source.hxx"

using namespace ui;
using namespace std;

bool run_loop::compare_source_ptr::operator () (weak_ptr <event_source> const &lhs_ptr, weak_ptr <event_source> const &rhs_ptr) const {
	if (lhs_ptr.expired ()) {
		return false;
	} else if (rhs_ptr.expired ()) {
		return true;
	}
	auto const &lhs = lhs_ptr.lock (), &rhs = rhs_ptr.lock ();
	return lhs->compare (rhs.get ());
}

int run_loop::run () {
	this->_main_thread_id = this_thread::get_id ();
	
	this->_exit_mutex.lock ();
	for (;;) {
		auto const now = clock_type::now ();
		this->next_iteration (now);
		if (this->_exit_mutex.try_lock_until (this->idle_deadline (now))) {
			break;
		}
	}
	return this->_rc;
}

void run_loop::exit (int rc) {
	this->_rc = rc;
	this->_exit_mutex.unlock ();
}

void run_loop::next_iteration (time_point const &now) {
	this->handle_emitted_events (now);
	this->process_pending_sources ();
}

void run_loop::handle_emitted_events (time_point const &now) {
	std::vector <std::shared_ptr <event_source>> fired_sources;
	for (auto const &source_ptr: this->_sources) {
		if (source_ptr.expired ()) {
			continue;
		}
		auto const &source = source_ptr.lock ();
		if (source->has_event (now)) {
			fired_sources.push_back (source);
		}
	}
	
	for (auto const &source: fired_sources) {
		do {
			source->process_event (now);
		} while (source->has_event (now));
	}
}

void run_loop::process_pending_sources () {
	if (this->_pending.empty ()) {
		return;
	}
	
	erase_if (this->_sources, [&] (auto &source_ptr) {
		if (source_ptr.expired ()) {
			return true;
		}
		auto const &source = source_ptr.lock ();
		auto const &it = this->_pending.find (source);
		if (it != this->_pending.end () && !it->second) {
			source->deactivate ();
			this->_pending.erase (it);
			return true;
		}
		return false;
	});
	
	erase_if (this->_pending, [&] (auto const &source) {
		if (!source.second || source.first.expired ()) {
			return true;
		}
		if (binary_search (this->_sources.begin (), this->_sources.end (), source.first, compare_source_ptr ())) {
			return true;
		}
		return false;
	});
	
	this->_sources.reserve (this->_sources.size () + this->_pending.size ());
	auto const &sorted_count = this->_sources.size ();
	for (auto source: this->_pending) {
		this->_sources.emplace_back (std::move (source.first)).lock ()->activate ();
	}
	this->_pending.clear ();
	inplace_merge (this->_sources.begin (), this->_sources.begin () + sorted_count, this->_sources.end (), compare_source_ptr ());
}

run_loop::time_point run_loop::idle_deadline (time_point const &now) const {
	auto const it_min = min_element (this->_sources.begin (), this->_sources.end (), [&now] (auto const &lhs_ptr, auto const &rhs_ptr) {
		if (lhs_ptr.expired ()) {
			return true;
		} else if (rhs_ptr.expired ()) {
			return false;
		}
		auto const &lhs = lhs_ptr.lock (), &rhs = rhs_ptr.lock ();
		return lhs->next_event_interval (now) < rhs->next_event_interval (now);
	});
	if (it_min != this->_sources.end ()) {
		return now + it_min->lock ()->next_event_interval (now);
	} else {
		return time_point::max ();
	}
}
