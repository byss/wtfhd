//
//  children_policy.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/23/20.
//

#ifndef children_policy_hxx
#define children_policy_hxx

#include <memory>
#include <filesystem>
#include <sys/types.h>
#include <unordered_set>

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
	static std::unique_ptr <children_policy> make_unique ();	
	virtual ~children_policy () = default;
	
	boundaries_policy fs_boundaries_policy () const {
		return this->_fs_policy;
	}

	void set_fs_boundaries_policy (boundaries_policy policy) {
		this->_fs_policy = policy;
	}
	
	virtual std::unique_ptr <children_policy> copy () const = 0;
	virtual bool contains (std::filesystem::path const &path) const = 0;
	virtual std::unordered_set <std::filesystem::path> const &roots () const = 0;
	virtual void add_root (std::filesystem::path const &root) = 0;

protected:
	children_policy () = default;

private:
	boundaries_policy _fs_policy;
};

#endif /* children_policy_hxx */

