//
//  children_policy.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/23/20.
//

#include "children_policy.hxx"

using namespace fs;
using namespace std;
using namespace std::filesystem;

template <>
struct hash <path> {
	size_t operator () (path const &value) const {
		return hash_value (value);
	}
};

namespace fs::impl {
	class children_policy: public ::children_policy {
	public:
		children_policy (): ::children_policy () {}
		
		virtual unique_ptr <::children_policy> copy () const override;
		virtual bool contains (path const &child) const override;
		virtual unordered_set <path> const &roots () const override;
		virtual void add_root (path const &root) override;
		
	private:
		unordered_set <path> _roots;
	};
}

unique_ptr <children_policy> children_policy::make_unique () {
	return std::make_unique <impl::children_policy> ();
}

std::unique_ptr <children_policy> impl::children_policy::copy () const {
	auto result = std::make_unique <children_policy> ();
	result->set_fs_boundaries_policy (this->fs_boundaries_policy ());
	result->_roots = this->_roots;
	return result;
}

bool impl::children_policy::contains (path const &child) const {
	if (this->_roots.contains (child)) {
		return true;
	} else if (child.has_relative_path ()) {
		return this->contains (child.parent_path ());
	} else {
		return false;
	}
}

unordered_set <path> const &impl::children_policy::roots () const {
	return this->_roots;
}

void impl::children_policy::add_root (path const &root) {
	auto canonical_root = canonical (root);
	if (!this->contains (canonical_root)) {
		this->_roots.insert (canonical_root);
	}
}
