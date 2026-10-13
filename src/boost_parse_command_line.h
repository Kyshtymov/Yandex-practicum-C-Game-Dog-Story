#pragma once
#include <boost/program_options.hpp>
#include <string>
#include <optional>
#include <iostream>

using namespace std::literals;

struct Args {
  std::optional<double> tick_period;
  std::string config;
  std::string static_path;
  bool random_spawn;
  std::optional<std::string> state_file;
  std::optional<double> save_state_period;
};


[[nodiscard]] inline std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
  namespace po = boost::program_options;

  po::options_description desc{"All options"s};

  Args args;
  double tick_period;
  double save_state_period;
  std::string state_file;
  desc.add_options()
    ("help,h", "produce help message")
    ("tick-period,t", po::value(&tick_period)->value_name("milliseconds"s), "set tick period")
    ("config-file,c", po::value(&args.config)->value_name("file"s), "set config file path")
    ("www-root,w", po::value(&args.static_path)->value_name("dir"s), "set static files root")
    ("randomize-spawn-points", "spawn dogs at random positions")
    ("state-file,s", po::value(&state_file)->value_name("state_file"s), "set state file")
    ("save-state-period,p", po::value(&save_state_period)->value_name("state_period"s), "set state save period");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  
  if (vm.contains("help")) {
    std::cout << desc;
    return std::nullopt;
  }
    
  if (vm.contains("tick-period")) {
    args.tick_period = tick_period;
  }

  
  if (vm.contains("state-file")) {
    args.state_file = state_file;
  }
  
  if (vm.contains("save-state-period")) {
    args.save_state_period = save_state_period;
  }
  if (!vm.contains("config-file")) {
    throw std::runtime_error("Config file has not been specified"s);
  }
  if (!vm.contains("www-root")) {
    throw std::runtime_error("Static path has not been specified"s);
  }
  
  args.random_spawn = vm.contains("randomize-spawn-points");

  return args;
}
