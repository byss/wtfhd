//
//  screen.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#ifndef screen_hxx
#define screen_hxx

#include <vector>
#include <memory>

#include "ui_common.hxx"
#include "run_loop.hxx"

namespace ui {
	class screen;
	class window;
}

class ui::screen: public std::enable_shared_from_this <ui::screen> {
public:
	class window {
	public:
		window (window &&) = default;
		virtual ~window () = default;

		virtual void load (screen &) {}
		virtual void activate () {}
		virtual void deactivate () {}
		
	protected:
		window () {}
		window (std::shared_ptr <ui::screen> const &screen): _screen (screen), _stack_pos (screen->_windows.size ()) {}

		ui::screen &screen () const {
			return *this->_screen;
		}

		std::vector <std::unique_ptr <window>> const &current_stack () const {
			return this->_screen->_windows;
		}

		std::unique_ptr <run_loop> const &get_run_loop () const {
			return this->_screen->_run_loop;
		}
	
		size_t stack_pos () const {
			return this->_stack_pos;
		}
		
		void assign_to_screen (ui::screen &screen, std::function <std::size_t (std::vector <std::unique_ptr <window>> &)> const &stack_actions);
		
		std::shared_ptr <ui::screen> _screen;
		std::size_t _stack_pos;
	};
	
	static std::shared_ptr <screen> shared ();
	static int run_main (void);
	static void exit (int);
	
	~screen ();		
	rect bounds () const;
	
	window &top () {
		return *this->_windows.back ();
	}
	
	window &root () {
		return *this->_windows.front ();
	}
	
	window const &top () const {
		return *this->_windows.back ();
	}
	
	window const &root () const {
		return *this->_windows.front ();
	}
	
	template <typename _Window, typename ..._Args>
	_Window &make_root (_Args &&...args) {
		if (!this->_windows.empty ()) {
			this->_windows.back ()->deactivate ();
			this->_windows.clear ();
		}
		return this->push <_Window> (std::forward <_Args> (args)...);
	}
	
	template <typename _Window, typename ..._Args>
	_Window &push (_Args ...args) {
		this->_windows.empty () ? (void) nullptr : this->_windows.back ()->deactivate ();
		auto &result = this->_windows.emplace_back (std::make_unique <_Window> (std::forward <_Args> (args)...));
		result->load (*this);
		result->activate ();
		return static_cast <_Window &> (*this->_windows.back ());
	}
	
	void pop () {
		this->_windows.back ()->deactivate ();
		this->_windows.pop_back ();
		this->_windows.empty () ? (void) nullptr : this->_windows.back ()->activate ();
	}

protected:
	screen ();
	
private:
	std::unique_ptr <run_loop> _run_loop;
	std::vector <std::unique_ptr <window>> _windows;
};

#endif /* screen_hxx */
