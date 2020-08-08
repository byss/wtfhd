//
//  event_source.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#ifndef event_source_hxx
#define event_source_hxx

#include <functional>
#include <unordered_map>

#include "run_loop.hxx"

namespace ui {
	class event_source;
	class timer;
	class mouse;
	class keyboard;
	class run_loop_idle;
	class window;
}

class ui::event_source: public ui::run_loop::event_source {
public:
	enum struct priority {
		timers = 100,
		mouse = 200,
		keyboard = 400,
		idle = 1000,
	};
	
	template <priority _Priority>
	class impl;

protected:
	event_source () = default;
	event_source (event_source const &) = default;
	event_source (event_source &&) = delete;
	virtual ~event_source () = default;
};

template <ui::event_source::priority _Priority>
class ui::event_source::impl: public ui::event_source, public std::enable_shared_from_this <ui::event_source::impl <_Priority>> {
	virtual std::underlying_type_t <priority> priority_value () const final {
		return static_cast <std::underlying_type_t <priority>> (_Priority);
	}
	
	priority constexpr priority_class () const {
		return _Priority;
	}
};

template <>
struct std::hash <ui::event_source> {
	std::size_t operator () (ui::event_source const &value) const {
		return reinterpret_cast <uintptr_t> (&value);
	}
};

class ui::timer: public event_source::impl <event_source::priority::timers> {
public:
	typedef std::function <void (timer const &)> handler_type;
	typedef handler_type &handler_ref;
	typedef handler_type const &handler_cref;

	template <typename _Tp>
	timer (std::chrono::time_point <_Tp> deadline): timer (deadline, std::chrono::time_point <_Tp>::duration::max) {}
	template <typename _Tp>
	timer (std::chrono::time_point <_Tp> deadline, typename std::chrono::time_point <_Tp>::duration interval);
	
	handler_cref handler () const {
		return this->_handler;
	}
	
	void set_handler (handler_cref handler) {
		this->_handler = handler;
	}
	
	virtual duration next_event_interval (time_point const &now) const override;
	
protected:
	virtual bool has_event (time_point const &now) const override;
	virtual void process_event (time_point const &now) override;

private:
	duration const _interval;
	time_point _deadline;
	handler_type _handler;
};

typedef struct _win_st WINDOW;

template <typename _Subevent, ui::event_source::priority _Priority, typename ..._Args>
class input_device: public ui::event_source::impl <_Priority> {
public:
	typedef _Subevent subevent_type;
	typedef void target_type (subevent_type, _Args...);
	typedef std::function <target_type> handler_type;
	typedef handler_type &handler_ref;
	typedef handler_type const &handler_cref;
	
	bool empty () const {
		return this->_handlers.empty ();
	}

	virtual void add_handler (subevent_type subevent, handler_cref handler) {
		if (!handler) {
			return;
		}
		this->_handlers.insert ({ subevent, handler });
	}
	
	virtual void remove_handler (subevent_type subevent, handler_cref handler) {
		if (handler) {
			this->for_each_subevent_handler (subevent, [&] (auto it) {
				if (it->second.template target <target_type> () == handler.template target <target_type> ()) {
					this->_handlers.erase (it);
				}
			});
		} else {
			this->_handlers.erase (subevent);
		}
	}
	
	virtual void invoke_handlers (subevent_type subevent, _Args const &...args) const {
		this->for_each_subevent_handler (subevent, [&] (auto const &it) {
			std::invoke (it->second, subevent, args...);
		});
	}

protected:
	input_device (ui::window const &parent) noexcept;
	
	template <typename _Tp>
	_Tp with_window_impl (std::function <_Tp (WINDOW *)> const &action) const noexcept {
		std::optional <_Tp> result = std::nullopt;
		std::invoke (this->_with_window_impl, [&] (WINDOW *impl) {
			result = std::invoke (action, impl);
		});
		return result.value ();
	}
	
	bool contains_handler (subevent_type subevent) const {
		auto const &[handlers_begin, handlers_end] = this->_handlers.equal_range (subevent);
		return handlers_begin == handlers_end;
	}
	
	void for_each_subevent_handler (subevent_type subevent, std::function <void (typename std::unordered_multimap <subevent_type, handler_type>::iterator const &)> const &action) {
		auto const &[handlers_begin, handlers_end] = this->_handlers.equal_range (subevent);
		for (auto it = handlers_begin; it != handlers_end; it++) {
			std::invoke (action, it);
		}
	}
	
	void for_each_subevent_handler (subevent_type subevent, std::function <void (typename std::unordered_multimap <subevent_type, handler_type>::const_iterator const &)> const &action) const {
		auto const &[handlers_begin, handlers_end] = this->_handlers.equal_range (subevent);
		for (auto it = handlers_begin; it != handlers_end; it++) {
			std::invoke (action, it);
		}
	}
	
private:
	std::function <void (std::function <void (WINDOW *)> const &)> const _with_window_impl;
	std::unordered_multimap <subevent_type, handler_type> _handlers;
};

class ui::mouse: public input_device <long, event_source::priority::mouse, int, int, int> {
public:
	mouse (ui::window const &parent) noexcept: input_device (parent) {}
	
	virtual void add_handler (subevent_type subevent, handler_cref handler) override;
	virtual void remove_handler (subevent_type subevent, handler_cref handler) override;
	
	virtual bool has_event (time_point const &now) const override;
	virtual void process_event (time_point const &now) override;
	virtual duration next_event_interval (time_point const &now) const override;
	
protected:
	virtual void activate () override;
	virtual void deactivate () override;

private:
	long _buttons_mask;
};


class ui::keyboard: public input_device <int, event_source::priority::keyboard> {
public:
	keyboard (ui::window const &parent) noexcept: input_device (parent) {}

	virtual bool has_event (time_point const &now) const override;
	virtual void process_event (time_point const &now) override;
	virtual duration next_event_interval (time_point const &now) const override;
};

class ui::run_loop_idle: public event_source::impl <event_source::priority::idle> {
public:
	typedef std::function <void (void)> handler_type;
	typedef handler_type &handler_ref;
	typedef handler_type const &handler_cref;
	
	run_loop_idle (handler_cref handler): _handler (handler) {}
	~run_loop_idle () = default;

	virtual bool has_event (time_point const &now) const override;
	virtual void process_event (time_point const &now) override;
	virtual duration next_event_interval (time_point const &now) const override;
	
private:
	time_point _last_invocation;
	handler_type _handler;
};

#endif /* event_source_hxx */
