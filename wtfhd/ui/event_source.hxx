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

#include "misc_types.hxx"
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
	
	template <typename _Impl>
	class impl;

protected:
	event_source () = default;
	event_source (event_source const &) = default;
	event_source (event_source &&) = delete;
	virtual ~event_source () = default;
	
	using run_loop::event_source::is_active;
	using run_loop::event_source::has_event;
	using run_loop::event_source::next_event_interval;
	using run_loop::event_source::activate;
	using run_loop::event_source::deactivate;
	
	virtual priority constexpr priority_class () const = 0;
	
	virtual bool compare (run_loop::event_source const *other) const override final {
		if (auto const other_source = dynamic_cast <event_source const *> (other)) {
			return this->priority_class () < other_source->priority_class ();
		}
		return run_loop::event_source::compare (other);
	}

private:
	template <typename _Tp>
	struct impl_priority;
};

template <>             struct ui::event_source::impl_priority <ui::timer>         { static constexpr priority value = priority::timers;   };
template <>             struct ui::event_source::impl_priority <ui::mouse>         { static constexpr priority value = priority::mouse;    };
template <>             struct ui::event_source::impl_priority <ui::keyboard>      { static constexpr priority value = priority::keyboard; };
template <>             struct ui::event_source::impl_priority <ui::run_loop_idle> { static constexpr priority value = priority::idle;     };

template <typename _Impl>
class ui::event_source::impl: public ui::event_source, public std::enable_shared_from_this <_Impl> {
public:
	impl (): event_source (), std::enable_shared_from_this <_Impl> () {}
	~impl () = default;
	
protected:
	priority constexpr priority_class () const override final {
		return impl_priority <_Impl>::value;
	}
};

template <>
struct std::hash <ui::event_source> {
	std::size_t operator () (ui::event_source const &value) const {
		return reinterpret_cast <uintptr_t> (&value);
	}
};

class ui::timer: public event_source::impl <ui::timer> {
public:
	typedef std::function <void (std::weak_ptr <timer>)> handler_type;
	typedef handler_type &handler_ref;
	typedef handler_type const &handler_cref;

	timer (duration interval): timer (clock_type::now () + interval, oneshot) {}
	timer (time_point deadline): timer (deadline, oneshot) {}
	timer (time_point deadline, duration interval): timer (deadline, interval, {}) {}
	timer (duration interval, handler_cref handler): timer (clock_type::now () + interval, handler) {}
	timer (time_point deadline, handler_cref handler): timer (deadline, oneshot, handler) {}
	timer (time_point deadline, duration interval, handler_cref handler): impl <ui::timer> (), _deadline (deadline), _interval (interval), _handler (handler) {}

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
	static constexpr duration oneshot = duration::max ();
	
	duration const _interval;
	time_point _deadline;
	handler_type _handler;
};

typedef struct _win_st WINDOW;

template <typename _Impl, typename _Subevent, typename ..._Args>
class input_device: public ui::event_source::impl <_Impl> {
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

class ui::mouse: public input_device <ui::mouse, long, int, int, int> {
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

class ui::keyboard: public input_device <ui::keyboard, int> {
public:
	keyboard (ui::window const &parent) noexcept: input_device (parent) {}

	virtual bool has_event (time_point const &now) const override;
	virtual void process_event (time_point const &now) override;
	virtual duration next_event_interval (time_point const &now) const override;
};

class ui::run_loop_idle: public event_source::impl <ui::run_loop_idle> {
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
