//
//  run_loop.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#ifndef run_loop_hxx
#define run_loop_hxx

#include <chrono>
#include <memory>

#include "misc_types.hxx"

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
		
	private:
		bool _is_active;
	};
	
	static std::unique_ptr <run_loop> make_unique ();
	virtual ~run_loop () = default;
	
	virtual bool is_main_thread () const = 0;
	
	virtual int run (void) = 0;
	virtual void exit (int) = 0;

	virtual void add_pending_callback_invocation (util::callback_t const &) = 0;
	virtual void add_event_source (std::weak_ptr <event_source> const &) = 0;
	virtual void remove_event_source (std::weak_ptr <event_source> const &) = 0;
};

#endif /* run_loop_hxx */
