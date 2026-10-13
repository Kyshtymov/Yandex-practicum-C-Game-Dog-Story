#pragma once
#include "respond_maker.h"
#include "boost_log.h"
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <variant>

namespace http_handler {
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace net = boost::asio;
  namespace json = boost::json;
  
  using StringResponse = http::response<http::string_body>;
  using FileResponse = http::response<http::file_body>;
  using EmptyResponse = http::response<http::empty_body>;
  using AnyResponse = std::variant<StringResponse, FileResponse, EmptyResponse>;
  
  class RequestHandler;

  template<class DecRequestHandler>
  class LoggingRequestHandler : public std::enable_shared_from_this<LoggingRequestHandler<DecRequestHandler>> {
  public:
    
    LoggingRequestHandler(std::shared_ptr<DecRequestHandler> decorated)
      : decorated_(std::move(decorated)) {
    }
        
    template<typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, 
                    std::string client_ip, Send&& send) {
      
      LogRequest(std::move(req), client_ip);
      auto start_time = std::chrono::steady_clock::now();	

      auto self = this->shared_from_this();
      auto logging_send = [self, send = std::forward<Send>(send), start_time](AnyResponse&& response){
        self->LogResponse(response, start_time);
        send(std::move(response));
      };
      
      (*decorated_)(std::move(req), std::move(client_ip), std::move(logging_send));
    }
  
  private:
    template<typename Body, typename Allocator>
    void LogRequest(http::request<Body, http::basic_fields<Allocator>>&& req, const std::string& client_ip) {
      using namespace std::literals;
            
      json::value custom_data = {
        {"ip", client_ip},
        {"URI", std::string(req.target())},
        {"method", std::string(req.method_string())}
      };
            
      BOOST_LOG_TRIVIAL(info) 
        << logging::add_value(additional_data, custom_data)
        << "request received"sv;
    }
        
    void LogResponse(const AnyResponse& response, std::chrono::steady_clock::time_point start_time) {
      using namespace std::literals;
      auto end_time = std::chrono::steady_clock::now(); 
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
      json::value custom_data = {
        {"response_time", duration.count()},
        {"code", GetResponseCode(response)},
        {"content_type", GetContentType(response)}
      };
            
      BOOST_LOG_TRIVIAL(info) 
	<< logging::add_value(additional_data, custom_data)
        << "response sent"sv;
    }
    
    int GetResponseCode(const AnyResponse& response) const {
      return std::visit([](const auto& resp) {
        return static_cast<int>(resp.result());
      }, response);
    }

    std::string GetContentType(const AnyResponse& response) const {
      return std::visit([](const auto& resp) -> std::string {
        auto ct = resp.find(http::field::content_type);
        if (ct != resp.end()) {
          return std::string(ct->value());
        }
        return "null"; 
      }, response);
    }
    std::shared_ptr<DecRequestHandler> decorated_;
  };

  class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
  public:
    using Strand = net::strand<net::io_context::executor_type>;
    
    RequestHandler(Application* application, Strand api_strand)
      : respond_maker_{application}, api_strand_{std::move(api_strand)} {
    }
        
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, 
                    const std::string& client_ip, Send&& send) {
      try {         
        if (std::string(req.target()).substr(0, Endpoints::API_BASE.size()) == Endpoints::API_BASE) {
          auto handle = [self = shared_from_this(), send = std::forward<Send>(send), req = std::move(req)]() mutable {
            try {
              assert(self->api_strand_.running_in_this_thread());
              send(self->HandleApiRequest(std::move(req)));
            } catch (...) {
              send(self->ReportServerError(req.version(), req.keep_alive()));
            }
          };
          net::dispatch(api_strand_, std::move(handle));
          return;
        }
        
        auto response = HandleFileRequest(std::move(req));
        send(std::move(response));
	
      } catch (const std::exception& ex) {
        send(ReportServerError(req.version(), req.keep_alive()));
      }
    }
    
  private:
    template <typename Body, typename Allocator>
    AnyResponse HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req){
      return respond_maker_.HandleApiRequest(std::move(req));
    }

    template <typename Body, typename Allocator>
    AnyResponse HandleFileRequest(http::request<Body, http::basic_fields<Allocator>>&& req){
      return respond_maker_.HandleFileRequest(std::move(req));
    }
    
    AnyResponse ReportServerError(unsigned version, bool keep_alive){
      return respond_maker_.ReportServerError(version, keep_alive);
    }
    
    RespondMaker respond_maker_;
    Strand api_strand_;    
  };
}  // namespace http_handler
