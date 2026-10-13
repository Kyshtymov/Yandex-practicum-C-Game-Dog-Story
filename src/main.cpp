#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>
#include "boost_log.h"
#include "boost_parse_command_line.h"
#include "http_server.h"
#include "json_loader.h"
#include "request_handler.h"
#include "application_handler.h"
#include "model.h"
#include "players.h"
#include "serialization_listener.h"
#include "application_listener.h"


using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;



namespace {
  template <typename Fn>
  void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
      workers.emplace_back(fn);
    }
    fn();
  }
}

int main(int argc, const char* argv[]) {
  
  Application application; 
  try {
    if (auto args = ParseCommandLine(argc, argv)) {
      try {
	logging::add_common_attributes();
	InitLogging();
	
	model::Game game = json_loader::LoadGame(args->config);
	api::Players players;
	app_handler::ApplicationHandler app_handler{&game, &players, args->tick_period, args->static_path, args->random_spawn};
                
	application.SetAppHandler(&app_handler); 
	std::shared_ptr<SerializationListener> serialization_listener;
		
	if (args->state_file.has_value()) {
	  std::chrono::milliseconds save_period_ms{0};
	  if (args->save_state_period.has_value())
	    save_period_ms = std::chrono::milliseconds(static_cast<int>(*args->save_state_period));
		  
	  serialization_listener = std::make_shared<SerializationListener>(save_period_ms, args->state_file.value(), &app_handler);
	  application.SetApplicationListener(serialization_listener.get());
	}                
	const unsigned num_threads = std::thread::hardware_concurrency();
	net::io_context ioc(num_threads);
	auto api_strand = net::make_strand(ioc);
                
	if (args->tick_period.has_value()) {
	  std::chrono::milliseconds tick_ms(static_cast<int>(*args->tick_period));
		  
	  auto ticker = std::make_shared<app_handler::Ticker>(api_strand, tick_ms, [&application](std::chrono::milliseconds delta) {
	    double delta_seconds = std::chrono::duration<double>(delta).count();
	    application.Tick(delta);
	  });
	  ticker->Start();
	}
                
	auto handler = std::make_shared<http_handler::RequestHandler>(&application, api_strand);
	auto log_handler = std::make_shared<http_handler::LoggingRequestHandler<http_handler::RequestHandler>>(handler);
                
	net::signal_set signals(ioc, SIGINT, SIGTERM);
	signals.async_wait([&ioc, &application](const sys::error_code& ec, int) {
	  if (!ec) {
	    application.Save();
	    ioc.stop();
	  }
	});
                
	const auto address = net::ip::make_address("0.0.0.0");
	constexpr net::ip::port_type port = 8080;
                
	http_server::ServeHttp(ioc, {address, port}, [log_handler](auto&& req, std::string client_ip, auto&& send) {
	  (*log_handler)(std::forward<decltype(req)>(req), std::move(client_ip), std::forward<decltype(send)>(send));
	});
	
	json::value custom_data = {
	  {"address", address.to_string()},
	  {"port", port}
	};
                
	BOOST_LOG_TRIVIAL(info) 
	  << logging::add_value(additional_data, custom_data)
	  << "server started";
                
	RunWorkers(num_threads, [&ioc] { ioc.run(); });

	json::value exit_data = {{"code", 0}};
	BOOST_LOG_TRIVIAL(info) 
	  << logging::add_value(additional_data, exit_data)
	  << "server exited";
                
      } catch (const std::exception& ex) {
	json::value custom_data = {
	  {"code", EXIT_FAILURE},
	  {"exception", ex.what()}
	};
	BOOST_LOG_TRIVIAL(fatal) 
	  << logging::add_value(additional_data, custom_data)
	  << "error";
	return EXIT_FAILURE;
      }
    }
	
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
