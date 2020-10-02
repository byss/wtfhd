//
//  main_window.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/31/20.
//

#ifndef main_window_hxx
#define main_window_hxx

#include "window.hxx"

namespace fs {
	class children_policy;
	class tree_builder;
}

namespace ui {
	class main_window;
}

class ui::main_window: public ui::window {
public:
	main_window (std::unique_ptr <fs::children_policy const> &&policy);
	
private:
	void window_did_appear () override;
	
	std::shared_ptr <fs::tree_builder> _builder;
};

#endif /* main_window_hxx */
