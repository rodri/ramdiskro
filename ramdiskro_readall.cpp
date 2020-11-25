// c 2018,2019,2020



#include "ramdiskro.h"
#include <iostream>
#include <vector>



int reread(const void *super, rdro_t inode, std::string path)
{
	const rdro_inode *ino;
	rdro_data data;
	const char *err = rdro_get_inode(super, inode, &ino);
	if (err)
	{
		std::cerr << err << std::endl;
		return 10;
	}
	err = rdro_get_data_from_ino(super, ino, &data);
	if (err)
	{
		std::cerr << err << std::endl;
		return 11;
	}
	if (RDRO_IS_DIR(data.type))
	{
		if (path == "/")
			std::cout << path << "    " << data.size << " bytes" << std::endl;
		else
			std::cout << path << "/    " << data.size << " bytes" << std::endl;
	}
	else
	{
		std::cout << path << "    " << data.size << " bytes" << std::endl;
		return 1;
	}
	rdro_t entries = 0;
	err = rdro_get_dir_entry_count(super, ino, &entries);
	if (err)
	{
		std::cerr << err << std::endl;
		return 12;
	}
	if (entries < 2)
	{
		std::cerr << "warning: entries < 2" << std::endl;
	}
	for (rdro_t e = 2 ; e < entries ; ++e)
	{
		rdro_t inode2 = -1;
		const char *name = nullptr;
		num_t name_size = 0;
		err = rdro_get_dir_entry(super, ino, e, &inode2, &name, &name_size);
		if (err)
		{
			std::cerr << err << std::endl;
			return 13;
		}
		std::string np;
		if (path == "/")
			np = path + std::string(name, name+name_size);
		else
			np = path + "/" + std::string(name, name+name_size);
		int r = reread(super, inode2, np);
		if (r > 1)
			return 20;
	}
	return 0;
}




int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "usage: ramdiskro_readall input_file" << std::endl;
		std::cerr << "input_file: ramdiskro image to read" << std::endl;
		std::cerr << "error: number of arguments is not 1" << std::endl;
		return 1;
	}

	std::vector<char> buffer(16*1024);
	std::vector<char> file_data;

	FILE *fi = fopen(argv[1], "rb");
	if (!fi)
	{
		std::cerr << "error: cannot open file: " << argv[1] << std::endl;
		return 2;
	}
	while (true)
	{
		auto ret = fread(&buffer.front(), 1, buffer.size(), fi);
		if (ret == 0)
			break;
		file_data.insert(file_data.end(), buffer.begin(), buffer.begin() + ret);
	}
	if (ferror(fi))
	{
		std::cerr << "error: cannot read file: " << argv[1] << std::endl;
		return 2;
	}
	if (fclose(fi))
	{
		std::cerr << "error: cannot close file: " << argv[1] << std::endl;
		return 2;
	}

	std::cerr << "loaded      " << file_data.size() << " bytes" << std::endl;

	std::cerr << "checking... ";
	const char *ret = rdro_check((&file_data.front()), file_data.size(), RDRO_SIZE_EXACT);
	if (ret == 0)
	{
		std::cerr << "ok" << std::endl;
	}
	else
	{
		std::cerr << "error: " << ret << std::endl;
		return 3;
	}

	const char *root = "/";

	struct rdro_data r;
	ret = rdro_get_data_from_path((&file_data.front()), root, &r);

	if (ret == 0)
	{
		if (!RDRO_IS_DIR(r.type))
		{
			std::cerr << "error: " + std::string(root) + " is not a directory" << std::endl;
			return 5;
		}
		return reread((&file_data.front()), r.inode, root);
	}
	else
	{
		std::cerr << "file or directory not found: error: " << ret << std::endl;
		return 4;
	}
}
