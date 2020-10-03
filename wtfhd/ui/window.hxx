//
//  window.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/30/20.
//

#ifndef window_hxx
#define window_hxx

#include <functional>
#include <unordered_set>

#include "ui_common.hxx"
#include "screen.hxx"
#include "event_source.hxx"

namespace ui {
	struct base_window;
	class window;
}

typedef struct panel PANEL;

class ui::window: public ui::screen::window {
	template <typename _Impl, typename _Subevent, typename ..._Args>
	friend class ::input_device;
public:
	static std::shared_ptr <window> root ();
	
	~window ();
	
	bool is_root () const {
		return this->stack_pos () == 0;
	}
	
	bool is_top () const {
		return this->stack_pos () == this->current_stack ().size () - 1;
	}
	
	ui::window const &presenting () const {
		return static_cast <ui::window const &> (*this->current_stack () [this->stack_pos () - 1]);
	}
	
	ui::window const &presented () const {
		return static_cast <ui::window const &> (*this->current_stack () [this->stack_pos () + 1]);
	}
	
protected:
	window (): window (ui::screen::shared ()) {}
	window (rect frame): window (ui::screen::shared (), frame) {}
	window (std::shared_ptr <ui::screen> const &screen): window (screen, screen->bounds ()) {}
	window (std::shared_ptr <ui::screen> const &screen, rect frame) noexcept;
	
	rect frame () const noexcept;
	
	virtual void print (std::string const &) const noexcept;
	virtual void println (std::string const &) const noexcept;
	void refresh () const;

	virtual void window_did_load (void) {}
	virtual void window_will_appear (void) {}
	virtual void window_did_appear (void) {}
	virtual void window_will_disappear (void) {}
	virtual void window_did_disappear (void) {}
	
	void add_mouse_button_handler (long button, mouse::handler_cref handler);
	void add_key_handler (int key, keyboard::handler_cref handler);
	void remove_timer (std::weak_ptr <timer> t);
	void remove_timer (std::shared_ptr <timer> t);
	void remove_mouse_handler (long button, mouse::handler_cref handler);
	void remove_key_handler (int key, keyboard::handler_cref handler);

protected:
	template <typename _Window, typename ..._Args>
	_Window &push (_Args ...args) {
		return this->screen ().push <_Window> (std::forward <_Args> (args)...);
	}
	
	void pop () {
		return this->screen ().pop ();
	}
	
	void add_timer (timer &timer) {
		this->add_timer_impl (timer);
	}
	
	timer &add_timer (timer::duration interval, timer::handler_cref handler) {
		return this->add_timer_impl (interval, handler);
	}
	
	timer &add_timer (timer::time_point deadline, timer::handler_cref handler) {
		return this->add_timer_impl (deadline, handler);
	}
	
	timer &add_timer (timer::time_point deadline, timer::duration interval, timer::handler_cref handler) {
		return this->add_timer_impl (deadline, interval, handler);
	}
	
	template <typename _Fp, typename ..._Args>
	void invoke_callback (_Fp const &f, _Args &&...args) {
		auto &run_loop = this->get_run_loop ();
		if (run_loop->is_main_thread ()) {
			return std::invoke (f, std::forward <_Args> (args)...);
		} else {
			run_loop->add_pending_callback_invocation (std::bind (f, std::forward <_Args> (args)...));
		}
	}
		
private:
	using screen::window::screen, screen::window::current_stack, screen::window::get_run_loop, screen::window::stack_pos;
	
	typedef std::unordered_set <std::shared_ptr <event_source>> sources_set;
	
	virtual void load (class screen &) override final;
	virtual void activate () override final;
	virtual void deactivate () override final;

	
	void add_run_loop_event_source (std::shared_ptr <event_source> source) noexcept;
	void remove_run_loop_event_source (std::shared_ptr <event_source> source) noexcept;
	std::function <void (std::function <void (WINDOW *)> const &)> with_window_impl_from_this () const noexcept;

	template <typename ..._Args>
	timer &add_timer_impl (_Args &&...args) {
		auto result = std::make_shared <timer> (std::forward <_Args> (args)...);
		this->add_run_loop_event_source (result);
		return *result;
	}
		
	void with_window_impl (std::function <void (WINDOW *)> const &action) const noexcept {
		std::invoke (action, this->impl ());
	}

	WINDOW *impl () const noexcept;
	void print (std::string const &, bool) const noexcept;
	
	PANEL *_panel;
	
	std::weak_ptr <mouse> _mouse;
	std::weak_ptr <keyboard> _keyboard;
	util::threadsafe <sources_set> _sources;
};

template <typename _Impl, typename _Subevent, typename ..._Args>
::input_device <_Impl, _Subevent, _Args...>::input_device (ui::window const &parent) noexcept:
	ui::event_source::impl <_Impl> (), _with_window_impl (parent.with_window_impl_from_this ()) {}

#endif /* window_hxx */
