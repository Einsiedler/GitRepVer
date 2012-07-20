#include "../inc/stdafx.h"

#include <boost/program_options.hpp>

#include <iostream>


bool extract_cl_variables(int argc, char* argv[], boost::program_options::variables_map& r_vm);

int _tmain(int argc, char* argv[])
{
    namespace po = boost::program_options;

    po::variables_map vm;

    if(!extract_cl_variables(argc, argv, vm))
        return -1;

    if (vm.count("compression")) 
        std::cout << "Compression level was set to " << vm["compression"].as<int>() << std::endl;
    else
        std::cout << "Compression level was not set." << std::endl;

	return 0;
}


bool extract_cl_variables(int argc, char* argv[], boost::program_options::variables_map& r_vm)
{
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");

    desc.add_options()
        ("help", "produce help message")
        ("compression", po::value<int>(), "set compression level")
        ;

    po::store (po::parse_command_line(argc, argv, desc), r_vm);
    po::notify(r_vm);    

    if (r_vm.count("help"))
    {
        std::cout << desc << std::endl;

        return false;
    }

    return true;
}