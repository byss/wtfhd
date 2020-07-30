//
//  children_policy.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/23/20.
//

#ifndef children_policy_hxx
#define children_policy_hxx

#include <utility>
#include <filesystem>
#include <sys/types.h>

namespace fs {
	enum struct boundaries_policy;
	class children_policy;
};

enum struct fs::boundaries_policy {
	ignore = 0,
	transparent,
	stay_within,
};

class fs::children_policy {
public:
	virtual ~children_policy () = default;

	void set_fs_boundaries_policy (boundaries_policy policy) {
		this->_fs_policy = policy;
	}
	
	static children_policy &make ();
	
	virtual void add_root (std::filesystem::path root) = 0;
	virtual bool is_path_allowed (std::filesystem::path const &child) const = 0;
	
	virtual bool contains (std::pair <::ino_t, ::dev_t> const &node_id) const = 0;
	virtual void add_node (std::pair <::ino_t, ::dev_t> node_id) = 0;

protected:
	children_policy () = default;

private:
	class impl;
	boundaries_policy _fs_policy;
};

#endif /* children_policy_hxx */

