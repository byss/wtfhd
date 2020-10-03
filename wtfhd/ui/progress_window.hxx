//
//  progress_window.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 8/8/20.
//

#ifndef progress_window_hxx
#define progress_window_hxx

#include <memory>

#include "window.hxx"
#include "tree_builder.hxx"

namespace ui {
	class progress_window: public ui::window {
	public:
		progress_window (std::shared_ptr <fs::tree_builder> builder): _builder (builder) {}
		~progress_window () = default;
		
	private:
		void window_did_load () override;
		void window_did_appear () override;

		void builder_progress_did_update ();
		void builder_did_finish ();
		void abort_builder ();
		
		std::shared_ptr <fs::tree_builder> _builder;
	};
}

#endif /* progress_window_hxx */
