//
//  main_window.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#ifndef main_window_hxx
#define main_window_hxx

#include "window.hxx"

namespace ui {
	class main_window;
}

class ui::main_window: public ui::window {
public:
	main_window (): window () {
	}
	
	virtual ~main_window () = default;
};

#endif /* main_window_hxx */
