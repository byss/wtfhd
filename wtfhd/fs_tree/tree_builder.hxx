//
//  tree_builder.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 8/8/20.
//

#ifndef tree_builder_hxx
#define tree_builder_hxx

#include <memory>
#include <optional>
#include <sys/types.h>

#include "misc_types.hxx"
#include "node_info.hxx"

namespace fs {
	class children_policy;
	
	class tree_builder {
	public:
		typedef node_info::id node_id_t;
		
		static std::unique_ptr <tree_builder> make_unique (std::unique_ptr <children_policy const> &&policy);
		virtual ~tree_builder () = default;
		
		virtual bool started () const = 0;
		virtual util::progress_t progress () const = 0;
		virtual bool ready () const = 0;
		virtual bool success () const = 0;
		virtual std::optional <bool> result () const = 0;
		
		virtual bool contains (node_id_t const &node_id) const = 0;
		virtual void add_node (node_id_t const &node_id) = 0;
		
		virtual util::callback_id_t add_progress_callback (util::callback_t const &callback) = 0;
		virtual void remove_progress_callback (util::callback_id_t const callback_id) = 0;

		virtual void start (util::callback_t const &callback) = 0;
		virtual void cancel () = 0;
	};
}

#endif /* tree_builder_hxx */
