//
//  run_loop.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#include "run_loop.hxx"

#include <map>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>
#include <compare>
#include <optional>
#include <algorithm>
#include <functional>
#include <type_traits>

#include "misc_types.hxx"
#include "event_source.hxx"

using namespace ui;
using namespace std;
using namespace util;

namespace ui::impl {
	struct compare_source_ptr_base {
	public:
		typedef function <bool (run_loop::event_source const *, run_loop::event_source const *)> comparator;

		static bool impl (weak_ptr <run_loop::event_source> const &lhs_ptr, weak_ptr <run_loop::event_source> const &rhs_ptr, comparator const &impl) {
			if (!(lhs_ptr.use_count () && rhs_ptr.use_count ())) {
				
			}
			if (lhs_ptr.expired ()) {
				return false;
			} else if (rhs_ptr.expired ()) {
				return true;
			}
			auto const &lhs = lhs_ptr.lock (), &rhs = rhs_ptr.lock ();
			return impl (lhs.get (), rhs.get ());
		}
	};
	
	template <typename ..._Args>
	struct compare_source_ptr: private compare_source_ptr_base {
	public:
		compare_source_ptr (_Args &&...args): _impl (forward <_Args> (args)...) {}
		~compare_source_ptr () = default;
		
		bool operator () (weak_ptr <run_loop::event_source> const &lhs, weak_ptr <run_loop::event_source> const &rhs) const {
			return compare_source_ptr::impl (lhs, rhs, this->_impl);
		}
		
	private:
		comparator const _impl;
	};
	
	template <>
	struct compare_source_ptr <>: private compare_source_ptr_base {
	public:
		compare_source_ptr () {}
		~compare_source_ptr () = default;
		
		bool operator () (weak_ptr <run_loop::event_source> const &lhs, weak_ptr <run_loop::event_source> const &rhs) const {
			return compare_source_ptr::impl (lhs, rhs, &run_loop::run_loop::event_source::compare);
		}
	};
	
	class run_loop: public ui::run_loop {
	public:
		run_loop (): ui::run_loop () {}
		~run_loop () = default;
		
		virtual bool is_main_thread () const override;
		
		virtual int run (void) override;
		virtual void exit (int) override;

		virtual void add_pending_callback_invocation (callback_t const &) override;
		virtual void add_event_source (weak_ptr <event_source> const &) override;
		virtual void remove_event_source (weak_ptr <event_source> const &) override;

	private:
		typedef map <weak_ptr <event_source> const, bool, compare_source_ptr <>> pending_sources_t;
		
		run_loop::time_point next_iteration (time_point const &now);
		
		void handle_emitted_events (time_point const &now);
		void process_pending_sources (pending_sources_t &pending);
		time_point idle_deadline (time_point const &now) const;
		
		int _rc;
		thread::id _main_thread_id;
		timed_mutex _exit_mutex;
		
		vector <weak_ptr <event_source>> _sources;
		threadsafe <pending_sources_t> _pending;
		threadsafe <vector <callback_t>> _callback_invocations;
	};
};

unique_ptr <ui::run_loop> ui::run_loop::make_unique () {
	return std::make_unique <impl::run_loop> ();
}

bool impl::run_loop::is_main_thread () const {
	return this->_main_thread_id == this_thread::get_id ();
}

int impl::run_loop::run () {
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

void impl::run_loop::exit (int rc) {
	this->_rc = rc;
	this->_exit_mutex.unlock ();
}

void impl::run_loop::add_pending_callback_invocation (callback_t const &callback) {
	this->_callback_invocations.with_value ([&callback] (vector <callback_t> &callbacks) { callbacks.push_back (callback); });
}

void impl::run_loop::add_event_source (weak_ptr <event_source> const &source) {
	this->_pending.with_value ([&source] (pending_sources_t &pending) { pending.insert_or_assign (source, true); });
}

void impl::run_loop::remove_event_source (weak_ptr <event_source> const &source) {
	this->_pending.with_value ([&source] (pending_sources_t &pending) { pending.erase (source); });
}

run_loop::time_point impl::run_loop::next_iteration (time_point const &now) {
	this->_callback_invocations.with_value ([] (vector <callback_t> &callbacks) {
		for (auto callback: callbacks) {
			invoke (callback);
		}
		callbacks.clear ();
	});
	
	this->handle_emitted_events (now);
	
	return this->_pending.with_value ([this, &now] (pending_sources_t &pending) {
		this->process_pending_sources (pending);
		return this->idle_deadline (now);
	});
}

void impl::run_loop::handle_emitted_events (time_point const &now) {
	vector <shared_ptr <event_source>> fired_sources;
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

void impl::run_loop::process_pending_sources (pending_sources_t &pending) {
	erase_if (this->_sources, [&] (auto const &source_ptr) {
		if (source_ptr.expired ()) {
			return true;
		}
		auto const &source = source_ptr.lock ();
		auto const &it = pending.find (source);
		if (it != pending.end () && !it->second) {
			source->deactivate ();
			pending.erase (it);
			return true;
		}
		return false;
	});
	
	if (pending.empty ()) {
		return;
	}
	
	erase_if (pending, [&] (auto const &source) {
		if (!source.second || source.first.expired ()) {
			return true;
		}
		if (binary_search (this->_sources.begin (), this->_sources.end (), source.first, compare_source_ptr ())) {
			return true;
		}
		return false;
	});
	
	this->_sources.reserve (this->_sources.size () + pending.size ());
	auto const &sorted_count = this->_sources.size ();
	for (auto source: pending) {
		this->_sources.emplace_back (move (source.first)).lock ()->activate ();
	}
	pending.clear ();
	inplace_merge (this->_sources.begin (), this->_sources.begin () + sorted_count, this->_sources.end (), compare_source_ptr ());
}

run_loop::time_point impl::run_loop::idle_deadline (time_point const &now) const {
	auto const it_min = min_element (this->_sources.begin (), this->_sources.end (), compare_source_ptr ([&now] (auto const &lhs, auto const &rhs) {
		return lhs->next_event_interval (now) < rhs->next_event_interval (now);
	}));
	if (it_min != this->_sources.end ()) {
		return now + it_min->lock ()->next_event_interval (now);
	} else {
		return time_point::max ();
	}
}
