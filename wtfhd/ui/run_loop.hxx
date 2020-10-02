//
//  run_loop.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#ifndef run_loop_hxx
#define run_loop_hxx

#include <map>
#include <mutex>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <compare>
#include <utility>

namespace ui {
	class event_source;
	class run_loop;
}

class ui::run_loop {
public:
	typedef std::chrono::steady_clock clock_type;
	typedef clock_type::time_point time_point;
	typedef clock_type::duration duration;
	typedef clock_type::period period;
	
	struct event_source {
		friend class run_loop;
	public:
		typedef run_loop::clock_type clock_type;
		typedef run_loop::time_point time_point;
		typedef run_loop::duration duration;
		typedef run_loop::period period;
		
		virtual ~event_source () {
			if (this->is_active ()) {
				this->deactivate ();
			}
		}
		
		bool is_active () const {
			return this->_is_active;
		}

	protected:
		virtual bool compare (event_source const *other) const {
			return this < other;
		}

		virtual bool has_event (time_point const &now) const = 0;
		virtual void process_event (time_point const &now) = 0;
		
		virtual duration next_event_interval (time_point const &now) const = 0;

		virtual void activate () {
			this->_is_active = true;
		}
		
		virtual void deactivate () {
			this->_is_active = false;
		}
		
		bool _is_active;
	};
	
	bool is_main_thread () const {
		return this->_main_thread_id == std::this_thread::get_id ();
	}
	
	void add_event_source (std::weak_ptr <event_source> source) {
		this->_pending.insert_or_assign (source, true);
	}
	
	void remove_event_source (std::weak_ptr <event_source> source) {
		this->_pending.insert_or_assign (source, false);
	}
	
	int run (void);
	void exit (int);
	
private:	
	void next_iteration (time_point const &now);
	void handle_emitted_events (time_point const &now);
	void process_pending_sources ();
	time_point idle_deadline (time_point const &now) const;
	
	struct compare_source_ptr {
		compare_source_ptr () = default;
		~compare_source_ptr () = default;
		
		bool operator () (std::weak_ptr <event_source> const &, std::weak_ptr <event_source> const &) const;
	};
	
	int _rc;
	std::thread::id _main_thread_id;
	std::timed_mutex _exit_mutex;
	std::vector <std::weak_ptr <event_source>> _sources;
	std::map <std::weak_ptr <event_source>, bool, compare_source_ptr> _pending;
};

#endif /* run_loop_hxx */
