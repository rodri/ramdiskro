// c 2018,2019,2020



#include "ramdiskro.h"
#include <string>
#include <vector>



class inode;



class file
{
public:
	std::string name;
	size_t inode_num;
};



class inode
{
public:
	rdro_t type; // file/dir and attributes: size_t not enough for 32 bit host and 64 bit image
	size_t inode_parent; // useful for dirs
	size_t start; // bytes
	std::vector<char> content; // if file
	std::vector<file> files; // if dir
};



class builder
{
public:
	builder();
	void add(const std::string &dir, size_t padding, bool print);
	void save(const std::string &file) const;
protected:
	void add_item(size_t parent_inode, const std::string &namein, const std::string &nameout, bool print);
	void build_dirs();
	std::vector<inode> inodes;
	size_t space;
	size_t padding;
};
