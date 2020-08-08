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
	template <typename _Subevent, ui::event_source::priority _Priority, typename ..._Args>
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

	virtual void window_will_appear (void) {}
	virtual void window_did_appear (void) {}
	virtual void window_will_disappear (void) {}
	virtual void window_did_disappear (void) {}
	
	void add_timer (timer &t);
	void add_mouse_button_handler (long button, mouse::handler_cref handler);
	void add_key_handler (int key, keyboard::handler_cref handler);
	void remove_timer (timer &t);
	void remove_mouse_handler (long button, mouse::handler_cref handler);
	void remove_key_handler (int key, keyboard::handler_cref handler);
		
private:
	using screen::window::screen, screen::window::current_stack, screen::window::get_run_loop, screen::window::stack_pos;
	
	virtual void activate () override final {
		this->window_will_appear ();
		for (auto source: this->_sources) {
			this->get_run_loop ().add_event_source (source);
		}
		this->window_did_appear ();
	}
	
	virtual void deactivate () override final {
		this->window_will_disappear ();
		for (auto source: this->_sources) {
			this->get_run_loop ().remove_event_source (source);
		}
		this->window_did_disappear ();
	}
	
	void add_run_loop_event_source (std::shared_ptr <event_source> source) noexcept {
		auto [it, success] = this->_sources.emplace (source);
		if (!success) {
			return;
		}
		if (this->is_top ()) {
			this->get_run_loop ().add_event_source (*it);
		}
	}
	
	void remove_run_loop_event_source (std::shared_ptr <event_source> source) noexcept {
		auto it = this->_sources.find (source);
		if (it != this->_sources.end ()) {
			this->_sources.erase (it);
			this->get_run_loop ().remove_event_source (source);
		}		
	}
	
	std::function <void (std::function <void (WINDOW *)> const &)> with_window_impl_from_this () const noexcept;
	
	void with_window_impl (std::function <void (WINDOW *)> const &action) const noexcept {
		std::invoke (action, this->impl ());
	}

	WINDOW *impl () const noexcept;
	void print (std::string const &, bool) const noexcept;
	
	PANEL *_panel;
	
	std::weak_ptr <mouse> _mouse;
	std::weak_ptr <keyboard> _keyboard;
	std::unordered_set <std::shared_ptr <event_source>> _sources;
};

template <typename _Subevent, ui::event_source::priority _Priority, typename ..._Args>
::input_device <_Subevent, _Priority, _Args...>::input_device (ui::window const &parent) noexcept: _with_window_impl (parent.with_window_impl_from_this ()) {}

#endif /* window_hxx */
