// c 2018,2019,2020



#include "ramdiskro.h"
#include <iostream>
#include <vector>



void usage()
{
	std::cerr << "usage: ramdiskro_read [-p] input_file path" << std::endl;
	std::cerr << "-p: also print file contents to stdout" << std::endl;
	std::cerr << "input_file: ramdiskro image to read" << std::endl;
	std::cerr << "path: path to print information of" << std::endl;
}



int main(int argc, char *argv[])
{
	#define ARGN 2
	size_t argn = 0;
	const char *args[ARGN] = {nullptr,nullptr};
	bool print = false;

	for (int i = 1 ; i < argc ; ++i)
	{
		if (argv[i] == std::string("-p"))
		{
			print = true;
		}
		else
		{
			if (argn < ARGN)
			{
				args[argn] = argv[i];
			}
			else
			{
				usage();
				std::cerr << "error: too many arguments" << std::endl;
				return 1;
			}
			++argn;
		}
	}

	if (argn < ARGN)
	{
		usage();
		std::cerr << "error: too few arguments" << std::endl;
		return 1;
	}

	std::vector<char> buffer(16*1024);
	std::vector<char> file_data;

	FILE *fi = fopen(args[0], "rb");
	if (!fi)
	{
		std::cerr << "error: cannot open file: " << args[0] << std::endl;
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
		std::cerr << "error: cannot read file: " << args[0] << std::endl;
		return 2;
	}
	if (fclose(fi))
	{
		std::cerr << "error: cannot close file: " << args[0] << std::endl;
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

	struct rdro_data r;
	ret = rdro_get_data_from_path((&file_data.front()), args[1], &r);

	if (ret == 0)
	{
		std::cerr << "item.offset " << (uint8_t*)r.data - (uint8_t*)&file_data.front() << " bytes" << std::endl;
		std::cerr << "item.size   " << r.size << " bytes" << std::endl;
		std::cerr << "item.inode  " << r.inode << std::endl;
		std::cerr << "item.type   0x" << std::hex << r.type;
		if (RDRO_IS_DIR(r.type))
			std::cerr << " (is dir)";
		else
			std::cerr << " (is file)";
		std::cerr << std::endl;
		if (print)
		{
			std::cerr << std::endl;
			std::cout << std::string((uint8_t*)r.data, (uint8_t*)r.data+r.size);
		}
		return 0;
	}
	else
	{
		std::cerr << "error: file or directory not found: " << ret << std::endl;
		return 2;
	}
}
