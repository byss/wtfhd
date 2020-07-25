//
//  children_policy.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/23/20.
//

#ifndef children_policy_hxx
#define children_policy_hxx

#include <map>
#include <vector>
#include <variant>
#include <algorithm>
#include <filesystem>

class children_policy {
private:
	struct filesystem_paths_trie {
	public:
		struct impl;
		
		filesystem_paths_trie () = default;
		filesystem_paths_trie (filesystem_paths_trie &&other) = default;
		virtual ~filesystem_paths_trie () = default;

		filesystem_paths_trie (filesystem_paths_trie const &other) = delete;
		filesystem_paths_trie &operator = (filesystem_paths_trie const &other) = delete;

		virtual bool contains (std::filesystem::path const &value) const = 0;
		virtual bool insert (std::filesystem::path value) = 0;
	};
	
public:
	enum struct fs_boundaries_policy {
		ignore = 0,
		transparent,
		stay_within,
	};
	
	children_policy ();
	children_policy (children_policy const &) = delete;
	children_policy &operator = (children_policy const &) = delete;
	
	void add_root (std::filesystem::path root) {
		this->_roots->insert (root);
	}
	
	void set_fs_boundaries_policy (fs_boundaries_policy policy) {
		this->_fs_policy = policy;
	}
	
private:
	std::unique_ptr <filesystem_paths_trie> _roots;
	fs_boundaries_policy _fs_policy;
};

#endif /* children_policy_hxx */
