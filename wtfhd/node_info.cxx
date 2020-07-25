//
//  node_info.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 7/19/20.
//

#include "node_info.hxx"

#include <array>
#include <tuple>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;
using namespace filesystem;

unique_ptr <node_info> make_node_info (path const &path) {
	struct stat info;
	node_info::throw_errno_if (::lstat (path.c_str (), &info));
	switch (info.st_mode & S_IFMT) {
	case S_IFLNK:
		return make_unique <link_info> (path, info);
	case S_IFDIR:
		return make_unique <dir_info> (path, info);
	default:
		return make_unique <file_info> (path, info);
	}
}

node_info::node_info (class path path, struct stat const &info): _path (path) {
	this->_dev = info.st_dev;
	this->_inode = info.st_ino;
	this->_mtime = info.st_mtimespec;
	this->_size = info.st_size;
}

dir_info::dir_info (class path path, struct stat const &info): node_info (path, info), _total_size (this->size ()) {}

void dir_info::load_info () {
	DIR *const dirp = opendir (this->path ().c_str ());
	this->throw_errno_if (!dirp);
	
	try {
		struct dirent *result = nullptr;
		do {
			struct dirent entry;
			this->throw_errno_if (::readdir_r (dirp, &entry, &result));
			if (!result) {
				continue;
			}
			
			auto const childName = std::string (entry.d_name, entry.d_name + entry.d_namlen);
			if ((childName == ".") || (childName == "..")) {
				continue;
			}
			
			auto child = make_node_info (this->path () / childName);
			child->load_info ();
			this->_children.push_back (std::move (child));
			this->_total_size += child->size ();
		} while (result);
		closedir (dirp);
		std::sort (this->_children.begin (), this->_children.end (), [] (unique_ptr <node_info> const &lhs, unique_ptr <node_info> const &rhs) {
			return lhs->size () > rhs->size ();
		});
		
	} catch (...) {
		closedir (dirp);
		throw;
	}
}

void link_info::load_info () {
	static thread_local std::array <char, PATH_MAX> target_path {};
	this->throw_errno_if (::readlink (this->path ().c_str (), target_path.data (), target_path.size ()));
}

void node_info::throw_errno_if (bool condition) {
	if (condition) {
		throw system_error (make_error_code (errc (errno)));
	}
}
