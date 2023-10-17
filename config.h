#ifndef SRC_UTIL_CONTEXT_H_
#define SRC_UTIL_CONTEXT_H_

#include <boost/program_options.hpp>
#include <thread>

namespace po = boost::program_options;
class configuration {
 public:
  string kDBPath = "/data/level_new";
  string meeting_source = "./meetings/";
  string output_filename = "output.csv";
  int num_threads = 128;
  bool reconstruction = false;
  uint begin_second = 300;
  uint seconds_duration = 3600;

  void print() {
    fprintf(stderr, "configuration:\n");
    if (reconstruction) {
      fprintf(stderr, "reconstruct database\n");
    }
    fprintf(stderr, "num threads:\t%d\n", num_threads);
    fprintf(stderr, "DBPath:\t%s\n", kDBPath.c_str());
    fprintf(stderr, "meeting_source:\t%s\n", meeting_source.c_str());
    fprintf(stderr, "output_filename:\t%s\n", output_filename.c_str());
    fprintf(stderr, "begin_second:\t%u\n", begin_second);
    fprintf(stderr, "input file duration seconds:\t%u\n", seconds_duration);
  }
};

inline int get_num_threads() { return std::thread::hardware_concurrency(); }

inline configuration get_parameters(int argc, char** argv) {
  configuration config;
  // config.num_threads = get_num_threads();

  po::options_description desc("query usage");
  desc.add_options()("help,h", "produce help message")(
      "reconstruction,r", "reconstruct the database")(
      "DBPath,d", po::value<string>(&config.kDBPath), "path to the database")(
      "meeting_source,m", po::value<string>(&config.meeting_source),
      "path of meeting_source")("output_filename,x",
                                po::value<string>(&config.output_filename),
                                "name of output_filename")(
      "begin_second,b", po::value<uint>(&config.begin_second),
      "input file begin second, start")(
      "seconds_duration,s", po::value<uint>(&config.seconds_duration),
      "input file count, duration");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  if (vm.count("help")) {
    cout << desc << "\n";
    exit(0);
  }
  po::notify(vm);
  if (vm.count("reconstruction")) {
    config.reconstruction = true;
  }
  config.print();
  return config;
}

#endif /* SRC_UTIL_CONTEXT_H_ */
