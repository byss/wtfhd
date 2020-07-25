//
//  node_info.hxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/19/20.
//

#ifndef node_info_hxx
#define node_info_hxx

#include <chrono>
#include <vector>
#include <cinttypes>
#include <filesystem>
#include <sys/types.h>

class node_info {
public:
	::dev_t dev () const {
		return this->_dev;
	}

	::ino_t inode () const {
		return this->_inode;
	}

	auto mtime () const {
		return std::chrono::file_clock::time_point (std::chrono::seconds (this->_mtime.tv_sec) + std::chrono::nanoseconds (this->_mtime.tv_nsec));
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
	
	virtual void load_info () = 0;

protected:
	friend std::unique_ptr <node_info> make_node_info (std::filesystem::path const &);
	
	std::filesystem::path const &path () const {
		return this->_path;
	}
	
	node_info (node_info &&) = default;
	node_info (node_info const &) = delete;
	node_info &operator = (node_info const &) = delete;

	node_info (std::filesystem::path, struct stat const &);

	static void throw_errno_if (bool condition);
	
private:
	std::filesystem::path _path;
	
	::dev_t _dev;
	::ino_t _inode;
	::timespec _mtime;
	::off_t _size;
};

class file_info: public node_info {
public:
	file_info (file_info &&) = default;
	file_info (file_info const &) = delete;
	file_info &operator = (file_info const &) = delete;

	file_info (std::filesystem::path path, struct stat const &info): node_info (path, info) {}

	virtual void load_info () override {}
};

class dir_info: public node_info {
public:
	dir_info (dir_info &&) = default;
	dir_info (dir_info const &) = delete;
	dir_info &operator = (dir_info const &) = delete;

	dir_info (std::filesystem::path, struct stat const &);

	virtual void load_info () override;
	
	virtual bool is_dir () const override {
		return true;
	}

private:
	std::uintmax_t _total_size;
	std::vector <std::unique_ptr <node_info>> _children;
};

class link_info: public node_info {
public:
	link_info (link_info &&) = default;
	link_info (link_info const &) = delete;
	link_info &operator = (link_info const &) = delete;

	link_info (std::filesystem::path path, struct stat const &info): node_info (path, info) {}
	
	virtual void load_info () override;
	
	virtual bool is_symlink () const override {
		return true;
	}

private:
	std::unique_ptr <node_info> _target;
};

std::unique_ptr <node_info> make_node_info (std::filesystem::path const &);

#endif /* node_info_hxx */
