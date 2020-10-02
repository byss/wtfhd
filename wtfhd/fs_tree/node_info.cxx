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

using namespace fs;
using namespace std;
using namespace filesystem;

unique_ptr <node_info> node_info::make (class path &&path, fs::children_policy &policy) {
	if (!policy.contains (path)) {
		return nullptr;
	}
	
	struct ::stat info;
	node_info::throw_errno_if (::lstat (path.c_str (), &info));
//	TODO
//	if (policy.contains ({ info.st_ino, info.st_dev })) {
//		return nullptr;
//	}
	
	unique_ptr <node_info> result;
	switch (info.st_mode & S_IFMT) {
	case S_IFLNK:
		result = make_unique <link_info> (std::move (path), info);
		break;
	case S_IFDIR:
		result = make_unique <dir_info> (std::move (path), info);
		break;
	default:
		result = make_unique <file_info> (std::move (path), info);
	}
//	TODO
//	policy.add_node (result->identifier ());
	return result;
}

node_info::node_info (class path &&path, struct ::stat const &info): _path (std::move (path)) {
	this->_dev = info.st_dev;
	this->_inode = info.st_ino;
	this->_mtime_sec = info.st_mtimespec.tv_sec;
	this->_mtime_nsec = (uint32_t) info.st_mtimespec.tv_nsec;
	this->_size = info.st_size;
}

void dir_info::load_info (fs::children_policy &policy) {
	DIR *const dirp = ::opendir (this->path ().c_str ());
	this->throw_errno_if (!dirp);
	
	try {
		struct dirent *result = nullptr;
		do {
			struct dirent entry;
			this->throw_errno_if (::readdir_r (dirp, &entry, &result));
			if (!result) {
				continue;
			}
			
			if ((*entry.d_name == '.') && ((entry.d_namlen == 1) || ((entry.d_namlen == 2) && (entry.d_name [1] == '.')))) {
				continue;
			}
			
			auto child_path = this->path ();
			child_path /= string (entry.d_name, entry.d_name + entry.d_namlen);
			auto child = node_info::make (std::move (child_path), policy);
			if (!child) {
				continue;
			}
			child->load_info (policy);
			this->_children_size += child->size ();
			this->_children.push_back (std::move (child));
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

void link_info::load_info (fs::children_policy &policy) {
	static thread_local std::array <char, PATH_MAX> target_path {};
	ssize_t const target_path_len = ::readlink (this->path ().c_str (), target_path.data (), target_path.size ());
	this->throw_errno_if (target_path_len == -1);
	try {
		if (auto target = node_info::make (string (target_path.data (), target_path.data () + target_path_len), policy)) {
			target->load_info (policy);
			this->_target = std::move (target);
		}
	} catch (...) {}
}

void node_info::throw_errno_if (bool condition) {
	if (condition) {
		throw system_error (errno, system_category ());
	}
}
