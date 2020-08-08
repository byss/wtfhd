//
//  children_policy.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/23/20.
//

#include "children_policy.hxx"

#include <map>
#include <set>
#include <filesystem>
#include <unordered_set>

using namespace fs;
using namespace std;
using namespace std::filesystem;

template <>
struct hash <path> {
	size_t operator () (path const &value) const {
		return filesystem::hash_value (value);
	}
};

class children_policy::impl: public fs::children_policy {
public:
	virtual void add_root (path root) override;
	virtual bool is_path_allowed (path const &child) const override;

	virtual bool contains (std::pair <::ino_t, ::dev_t> const &node_id) const override;
	virtual void add_node (std::pair <::ino_t, ::dev_t> node_id) override;

	impl (): children_policy () {}
	
private:
	unordered_set <path> _roots;
	map <dev_t, set <ino_t>> _processed;
};

children_policy &children_policy::make () {
	return *new children_policy::impl ();
}

bool children_policy::impl::is_path_allowed (path const &child) const {
	if (this->_roots.contains (child)) {
		return true;
	} else if (child.has_relative_path ()) {
		return this->is_path_allowed (child.parent_path ());
	} else {
		return false;
	}
}

void children_policy::impl::add_root (path root) {
	auto canonical_root = canonical (root);
	if (!this->is_path_allowed (canonical_root)) {
		this->_roots.insert (canonical_root);
	}
}

bool children_policy::impl::contains (std::pair <::ino_t, ::dev_t> const &node_id) const {
	return this->_processed.contains (node_id.second) && this->_processed.at (node_id.second).contains (node_id.first);
}

void children_policy::impl::add_node (std::pair <::ino_t, ::dev_t> node_id) {
	auto const &it = this->_processed.find (node_id.second);
	if (it != this->_processed.end ()) {
		it->second.insert (node_id.first);
	} else {
		this->_processed.insert ({ node_id.second, { node_id.first } });
	}
}
