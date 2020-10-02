//
//  node_info.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/19/20.
//

#ifndef node_info_hxx
#define node_info_hxx

#include <tuple>
#include <chrono>
#include <vector>
#include <utility>
#include <filesystem>
#include <sys/stat.h>

#include "children_policy.hxx"

namespace fs {
	class node_info;
	class file_info;
	class dir_info;
	class link_info;
};

class fs::node_info {
public:
	struct id {
		typedef std::tuple <::dev_t, ::ino_t> tuple_type;
		
		::dev_t const device;
		::ino_t const inode;
		
		id (::dev_t device, ::ino_t inode): device (device), inode (inode) {}
		id (tuple_type value): id (value, std::make_index_sequence <std::tuple_size_v <tuple_type>> ()) {}
		~id () = default;
		
		tuple_type constexpr as_tuple () const {
			return { this->device, this->inode };
		}
		
	private:
		template <std::size_t ..._Idx>
		id (tuple_type value, std::index_sequence <_Idx...> indices): id (std::get <_Idx> (value)...) {}
	};
	
	static std::unique_ptr <node_info> make (std::filesystem::path &&, children_policy &);

	id identifier () const {
		return { this->_dev, this->_inode };
	}

	auto mtime () const {
		return std::chrono::file_clock::time_point (std::chrono::seconds (this->_mtime_sec) + std::chrono::nanoseconds (this->_mtime_nsec));
	}

	std::string name () const {
		return this->_path.filename ();
	}
	
	virtual std::uintmax_t size () const {
		return this->_size;
	}
	
	virtual bool is_dir () const {
		return false;
	}
	
	virtual bool is_symlink () const {
		return false;
	}

	virtual ~node_info () = default;
	
	virtual void load_info (children_policy &) = 0;
	
	std::filesystem::path const &path () const {
		return this->_path;
	}
	
	node_info (std::filesystem::path &&, struct ::stat const &);

	node_info (node_info &&) = default;
	node_info (node_info const &) = delete;
	node_info &operator = (node_info const &) = delete;
	
protected:
	static void throw_errno_if (bool condition);
	
private:
	std::filesystem::path const _path;
	
	::off_t _size;
	::ino_t _inode;
	::time_t _mtime_sec;
	::uint32_t _mtime_nsec;
	::dev_t _dev;
};

class fs::file_info: public node_info {
public:
	file_info (file_info &&) = default;
	file_info (file_info const &) = delete;
	file_info &operator = (file_info const &) = delete;

	file_info (std::filesystem::path &&path, struct stat const &info): node_info (std::move (path), info) {}

	virtual void load_info (children_policy &) override {}
};

class fs::dir_info: public node_info {
public:
	dir_info (dir_info &&) = default;
	dir_info (dir_info const &) = delete;
	dir_info &operator = (dir_info const &) = delete;

	dir_info (std::filesystem::path &&path, struct stat const &info): node_info (std::move (path), info), _children_size () {}

	virtual void load_info (children_policy &) override;
	
	virtual bool is_dir () const override {
		return true;
	}
	
	virtual std::uintmax_t size () const override {
		return node_info::size () + this->_children_size;
	}
	
private:
	std::uintmax_t _children_size;
	std::vector <std::unique_ptr <node_info>> _children;
};

class fs::link_info: public node_info {
public:
	link_info (link_info &&) = default;
	link_info (link_info const &) = delete;
	link_info &operator = (link_info const &) = delete;

	link_info (std::filesystem::path &&path, struct stat const &info): node_info (std::move (path), info) {}
	
	virtual void load_info (children_policy &) override;
	
	virtual bool is_symlink () const override {
		return true;
	}
	
	virtual std::uintmax_t size () const override {
		auto result = node_info::size ();
		if (this->_target) {
			result += this->_target->size ();
		}
		return result;
	}

private:
	std::unique_ptr <node_info> _target;
};

#endif /* node_info_hxx */
