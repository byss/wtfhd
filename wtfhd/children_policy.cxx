//
//  children_policy.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/23/20.
//

#include "children_policy.hxx"

using std::map, std::tuple, std::unique_ptr, std::make_unique, std::variant, std::invoke, std::invoke_result_t, std::filesystem::path;

template <typename _Tp>
struct split_head {
	tuple <invoke_result_t <decltype (_Tp::pop_back) (_Tp)>, _Tp> operator () (_Tp source) {
		auto head = source.pop_back ();
		return { head, source };
	}
};

template <>
struct split_head <path> {
	tuple <path, path> operator () (path source) {
		auto head = source.has_root_path () ? source.root_path () : *source.begin ();
		return { head, source.lexically_relative (head) };
	}
};

template <typename _Kp, typename _Vp>
struct trie {
protected:
	using Children = map <_Kp, unique_ptr <trie>>;
	using Split = split_head <_Kp>;

	trie (): impl (Children {}) {}

public:
	trie (_Vp value): impl (value) {}
	trie (_Vp &&value): impl (value) {}
	trie (Children children): impl (children) {}
	trie (Children &&children): impl (children) {}

	~trie () = default;
	
	bool contains (_Kp const &key) const {
		if (this->has_value ()) {
			return key.empty ();
		}
		auto const &children = this->children ();
		auto [head, tail] = invoke (Split (), key);
		return children.contains (head) && children.at (head)->contains (tail);
	}
	
	bool insert (_Kp key, _Vp value) {
		if (this->has_value ()) {
			return false;
		} else if (key.empty ()) {
			this->value () = value;
			return true;
		} else {
			auto [head, tail] = invoke (Split (), key);
			return this->children ().at (head)->insert (tail, value);
		}
	}
	
protected:
	bool has_value () const {
		return this->impl.index ();
	}
	
	Children &children () {
		return get <0> (this->impl);
	}
	
	Children const &children () const {
		return get <0> (this->impl);
	}
	
	_Vp &value () {
		return get <1> (this->impl);
	}
	
private:
	variant <Children /* children */, _Vp /* value */> impl;
};

struct children_policy::filesystem_paths_trie::impl: public trie <path, path>, public children_policy::filesystem_paths_trie {
	impl (): trie <path, path> (), filesystem_paths_trie () {}
	impl (impl const &) = delete;
	impl (impl &&) = default;
	impl &operator = (impl const &) = delete;
	
	bool contains (path const &key) const {
		return trie <path, path>::contains (key);
	}

	bool insert (path key) {
		return trie <path, path>::insert (key, key);
	}
};

children_policy::children_policy (): _roots (unique_ptr <filesystem_paths_trie> (new filesystem_paths_trie::impl ())) {}
