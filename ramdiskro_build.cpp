// c 2018,2019,2020



#include "ramdiskro_builder.hpp"
#include <iostream>



void usage()
{
	std::cerr << "usage: ramdiskro_build [-a bytes] [-v] output_file input_dir" << std::endl;
	std::cerr << "-a bytes: set alignment of start of files (default is 16)" << std::endl;
	std::cerr << "-v: print names of files being added" << std::endl;
	std::cerr << "output_file: ramdiskro image to write" << std::endl;
	std::cerr << "input_dir: directory to read from" << std::endl;
}



int main(int argc, char *argv[])
{
	#define ARGN 2
	size_t argn = 0;
	const char *args[ARGN] = {nullptr,nullptr};
	long alignment = 16;
	bool print = false;

	for (int i = 1 ; i < argc ; ++i)
	{
		if (argv[i] == std::string("-v"))
		{
			print = true;
		}
		else if (argv[i] == std::string("-a"))
		{
			if (i + 1 < argc)
			{
				alignment = atol(argv[i + 1]);
				++i;
			}
			else
			{
				usage();
				std::cerr << "error: missing argument to -a" << std::endl;
				return 1;
			}
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

	if (alignment < 0)
	{
                std::cerr << "error: provided alignment is < 0" << std::endl;
		return 1;
	}

	builder b;

	b.add(args[1], alignment, print);

	b.save(args[0]);

	return 0;
}
