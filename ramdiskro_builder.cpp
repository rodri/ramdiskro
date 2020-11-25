// c 2018,2019,2020



#include "ramdiskro_builder.hpp"
#include <iostream>
#include <cstdio>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>



size_t pad(size_t size, size_t padding)
{
	if (size == 0 || padding < 2)
		return size;
	size_t rem = size % padding;
	if (rem == 0)
		return size;
	return size + padding - rem;
}



void writepad(size_t &size, size_t padding, FILE *f, const std::string &file, const std::vector<char> &zeros)
{
	size_t sizenew = pad(size, padding);
	if (sizenew != size)
	{
		auto ret = fwrite(&zeros.front(), 1, sizenew-size, f);
		if (ret != (sizenew-size))
			throw std::runtime_error(__FILE__": cannot write padding to: " + file);
		size = sizenew;
	}
}



rdro_t get_stat(const std::string &item)
{
	struct stat buf;
	int s = stat(item.c_str(), &buf);
	if (s)
	{
		std::cerr << "error: " << s << ": " << errno << ": " << strerror(errno) << std::endl;
		throw std::runtime_error(__FILE__": error in stat: " + item);
	}
	if (buf.st_mode & S_IFDIR)
		return buf.st_mode | RDRO_DIR;
	else
		return buf.st_mode & ~RDRO_DIR;
}



builder::builder()
: space(0)
, padding(0)
{
}



void builder::add(const std::string &dir, size_t padding_, bool print)
{
	space = 0;
	padding = padding_;
	inodes.clear();
	add_item(RDRO_ROOT_INODE, "/", dir, print);
	build_dirs();
}



void builder::add_item(size_t parent_inode, const std::string &namein, const std::string &nameout, bool print)
{
	auto st = get_stat(nameout);

	size_t current_inode = inodes.size();

	inode i;
	i.type = st;
	i.inode_parent = parent_inode;
	i.start = 0;
	inodes.push_back(i);

	if (parent_inode != current_inode)
	{
		file f;
		f.name = namein;
		f.inode_num = current_inode;
		inodes[parent_inode].files.push_back(f);
	}

	if (RDRO_IS_DIR(st))
	{
		if (print)
		{
			if (nameout.size() == 0 || nameout.back() != '/')
				std::cout << nameout << "/" << std::endl;
			else
				std::cout << nameout << std::endl;
		}

		DIR *dirp = opendir(nameout.c_str());
		struct dirent *dp;

		if (dirp)
		{
			file f;
			f.name = ".";
			f.inode_num = current_inode;
			inodes[current_inode].files.push_back(f);
			f.name = "..";
			f.inode_num = parent_inode;
			inodes[current_inode].files.push_back(f);
		}
		else
			throw std::runtime_error(__FILE__": open error in dir: " + nameout);

		while (dirp)
		{
			errno = 0;
			if ((dp = readdir(dirp)) != NULL)
			{
				std::string entry = dp->d_name;
				if (entry == "." || entry == "..")
					continue;
				std::string item;
				if (nameout.size() == 0)
					item = dp->d_name;
				else if (nameout.back() == '/')
					item = nameout + dp->d_name;
				else
					item = nameout + "/" + dp->d_name;

				add_item(current_inode, entry, item, print);
			}
			else
			{
				if (errno == 0)
				{
					closedir(dirp); // here: do not alter errno
					break;
				}
				else
				{
					closedir(dirp);
					throw std::runtime_error(__FILE__": read error in dir: " + nameout);
				}
			}
		}
	}
	else
	{
		if (print)
		{
			std::cout << nameout << std::endl;
		}

		if (current_inode == RDRO_ROOT_INODE)
			throw std::runtime_error(__FILE__": root inode is not a dir: " + nameout);

		FILE *fi = fopen(nameout.c_str(), "rb");
		if (!fi)
			throw std::runtime_error(__FILE__": cannot open file: " + nameout);
		std::vector<char> v(1024*16); // FIX GLOBAL ??? // ACCUMULATED
		while (true)
		{
			auto ret = fread(&v.front(), 1, v.size(), fi);
			if (ret == 0)
				break;
			inodes[current_inode].content.insert(inodes[current_inode].content.end(),
				v.begin(), v.begin() + ret);
			if (inodes[current_inode].content.size() > RDRO_MAX)
				throw std::runtime_error(__FILE__": file is too big: " + nameout);
		}
		if (ferror(fi))
			throw std::runtime_error(__FILE__": cannot read file: " + nameout);
		if (fclose(fi))
			throw std::runtime_error(__FILE__": cannot close file: " + nameout);
	}
}



void builder::build_dirs()
{
	space = pad(sizeof(struct rdro_super) + inodes.size() * sizeof(struct rdro_inode), padding);
	if (space > RDRO_MAX)
		throw std::runtime_error(__FILE__": total content size is too big");
	for (size_t in = 0 ; in != inodes.size() ; ++in)
	{
		auto &i = inodes[in];
		i.start = space;
		if (RDRO_IS_DIR(i.type))
		{
			// sort files for later binary search
			// do not sort . and .. (keep them at the beginning)
			if (i.files.size() >= 2)
				std::sort(i.files.begin() + 2, i.files.end(), [](file &a, file &b) {
					return rdro_strcmp_n(a.name.c_str(), a.name.size(),
						b.name.c_str(), b.name.size()) < 0; });

			rdro_diri rdi;
			if (i.files.size() > RDRO_MAX)
				throw std::runtime_error(__FILE__": directory is too big");
			rdi.entries = RDRO_HTOX(i.files.size());
			i.content.resize(sizeof(rdro_diri) + sizeof(rdro_dire) * i.files.size(), 0);
			size_t pos = 0;
			std::copy((const char*)&rdi, (const char*)(&rdi+1), i.content.begin() + pos);
			pos += sizeof(rdi);
			for (const auto f : i.files)
			{
				rdro_dire e;
				e.inode = RDRO_HTOX(f.inode_num);
				e.name = RDRO_HTOX(i.content.size());
				std::copy((const char*)&e, (const char*)(&e+1), i.content.begin() + pos);
				pos += sizeof(e);
				unsigned char s = 0;
				if (f.name.size() > 255)
				{
//					std::cerr << "warning: name truncated: " << f.name << std::endl;
					throw std::runtime_error(__FILE__": file name is too big (>255): " + f.name);
					s = 255;
				}
				else
					s = f.name.size();
				i.content.push_back(s); // filename size
				i.content.insert(i.content.end(), f.name.begin(), f.name.begin() + s); // filename
				i.content.push_back(0); // null terminate
			}
		}
		if (i.content.size() > RDRO_MAX)
			throw std::runtime_error(__FILE__": file/dir content size is too big");
		space += pad(i.content.size(), padding);
		if (space > RDRO_MAX)
			throw std::runtime_error(__FILE__": total content size is too big");
	}
	if (space > RDRO_MAX)
		throw std::runtime_error(__FILE__": total content size is too big");
	if (space != pad(space, padding))
		throw std::runtime_error(__FILE__": space not correctly padded");
}



void builder::save(const std::string &file) const
{
	FILE *fi = fopen(file.c_str(), "wb");
	if (!fi)
		throw std::runtime_error(__FILE__": cannot open file: " + file);

	std::vector<char> zeros(padding, 0); // 0 or (padding - 1) might be also

	rdro_super super;
	for (size_t l = 0 ; l != RDRO_MAGIC_SIZE ; ++l)
		super.magic[l] = rdro_magic[l];
	super.rdro_size = sizeof(rdro_t);
	super.version = 1;
	for (size_t t = 0 ; t != sizeof(super.pad) / sizeof(super.pad[0]) ; ++t)
		super.pad[t] = 0;
	super.endian = RDRO_HTOX(1);
	super.inodes = RDRO_HTOX(inodes.size());
	super.size = RDRO_HTOX(space);

	auto ret = fwrite(&super, 1, sizeof(super), fi);
	if (ret != sizeof(super))
		throw std::runtime_error(__FILE__": cannot write superblock to: " + file);
	size_t used = sizeof(super);

	rdro_inode ino;
	for (size_t i = 0 ; i != inodes.size() ; ++i)
	{
		ino.type = RDRO_HTOX(inodes[i].type);
		ino.start = RDRO_HTOX(inodes[i].start);
		ino.size = RDRO_HTOX(inodes[i].content.size());
		ret = fwrite(&ino, 1, sizeof(ino), fi);
		if (ret != sizeof(ino))
			throw std::runtime_error(__FILE__": cannot write inode to: " + file);
		used += sizeof(ino);
	}

	for (size_t in = 0 ; in != inodes.size() ; ++in)
	{
		writepad(used, padding, fi, file, zeros);
		auto &i = inodes[in];
		if (used != i.start)
			throw std::runtime_error(__FILE__": inconsistency in inode start position");
		ret = fwrite(&i.content.front(), 1, i.content.size(), fi);
		if (ret != i.content.size())
			throw std::runtime_error(__FILE__": cannot write content to: " + file);
		used += i.content.size();
	}

	writepad(used, padding, fi, file, zeros);

	if (ferror(fi))
		throw std::runtime_error(__FILE__": cannot write file: " + file);
	if (fclose(fi))
		throw std::runtime_error(__FILE__": cannot close file: " + file);

	if (space != used)
		throw std::runtime_error(__FILE__": error in calculated space: " +
			std::to_string(space) + " != " + std::to_string(used));

	std::cerr << "written " << used << " bytes in " << inodes.size() << " inodes" << std::endl;
}
