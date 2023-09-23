/*
 * context.h
 *
 *  Created on: Jan 16, 2021
 *      Author: teng
 */

#ifndef SRC_UTIL_CONTEXT_H_
#define SRC_UTIL_CONTEXT_H_

#include <boost/program_options.hpp>
#include <thread>

namespace po = boost::program_options;
class configuration{
public:
    string kDBPath = "/data/rocks_new";
    string input_filename = "meetings_0.in";
    string output_filename = "output.csv";
    int num_threads = 128;

	void print(){
		fprintf(stderr,"configuration:\n");
		fprintf(stderr,"num threads:\t%d\n",num_threads);
		fprintf(stderr,"DBPath:\t%s\n",kDBPath.c_str());
        fprintf(stderr,"input_filename:\t%s\n",input_filename.c_str());
        fprintf(stderr,"output_filename:\t%s\n",output_filename.c_str());
	}
};

inline int get_num_threads(){
    return std::thread::hardware_concurrency();
}

inline configuration get_parameters(int argc, char **argv){
	configuration config;
	//config.num_threads = get_num_threads();

	po::options_description desc("query usage");
	desc.add_options()
		("help,h", "produce help message")
        ("DBPath,d", po::value<string>(&config.kDBPath), "path to the database")
        ("input_filename,i", po::value<string>(&config.input_filename), "name of input_filename")
        ("output_filename,x", po::value<string>(&config.output_filename), "name of output_filename")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	if (vm.count("help")) {
		cout << desc << "\n";
		exit(0);
	}
	po::notify(vm);
	config.print();
	return config;
}




#endif /* SRC_UTIL_CONTEXT_H_ */
