#pragma once
#include "sdk.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include "boost_log.h"
#include <optional>
#include <iostream>
#include <functional>
#include <variant> 

namespace http_server {

  namespace net = boost::asio;
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace sys = boost::system;
  
  using namespace std::literals;
  using tcp = net::ip::tcp;
  using StringResponse = http::response<http::string_body>;
  using FileResponse = http::response<http::file_body>;  
  using EmptyResponse = http::response<http::empty_body>;
  
  using AnyResponse = std::variant<StringResponse, FileResponse, EmptyResponse>;  
  using StringRequest = http::request<http::string_body>;

  template <typename RequestHandler>
  concept HandlerConcept = requires(RequestHandler handler, StringRequest req) {
    { handler(std::move(req)) } -> std::same_as<AnyResponse>;
  };

  void ReportError(beast::error_code ec, std::string_view where, std::string_view what);
  std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer);

  class SessionBase {
  protected:
    using HttpRequest = http::request<http::string_body>;

    ~SessionBase() = default;
    
  public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    
    void Run();
    
  protected:
    explicit SessionBase(tcp::socket&& socket)
      : stream_(std::move(socket)) {
    }
    
    void Write(AnyResponse&& response) {
      std::visit([this](auto&& resp) {
        using ResponseType = std::decay_t<decltype(resp)>;
        auto safe_response = std::make_shared<ResponseType>(std::move(resp));
        
        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response,
			  [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
			    self->OnWrite(safe_response->need_eof(), ec, bytes_written);
			  });
      }, std::move(response));
    }

    void Read();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
    void Close();
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);
    
    virtual void HandleRequest(HttpRequest&& request) = 0;
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
  };

  template <typename RequestHandler>
  class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
  public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
      : SessionBase(std::move(socket))
      , request_handler_(std::forward<Handler>(request_handler)) {
    }
    
  private:
    std::shared_ptr<SessionBase> GetSharedThis() override {
      return this->shared_from_this();
    }
    
    void HandleRequest(HttpRequest&& request) override {
      std::string client_ip = stream_.socket().remote_endpoint().address().to_string();
      auto self = this->shared_from_this();
      request_handler_(std::move(request), client_ip, 
		       [self](AnyResponse&& response) {
			 self->Write(std::move(response));
		       });
    }
    
    RequestHandler request_handler_;
  };
  
  template <typename RequestHandler>
  class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
  public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
      : ioc_(ioc)
      , acceptor_(net::make_strand(ioc))
      , request_handler_(std::forward<Handler>(request_handler)) {
      beast::error_code ec;
      acceptor_.open(endpoint.protocol(), ec);
      if (ec) {
	ReportError(ec, "accept", "Can't open acceptor");
        return;
      }
      
      acceptor_.set_option(net::socket_base::reuse_address(true), ec);
      if (ec) {
	ReportError(ec, "accept", "Can't set option on the acceptor");
        return;
      }
        
      acceptor_.bind(endpoint, ec);
      if (ec) {
	ReportError(ec, "accept", "Can't bind the acceptor");
        return;
      }
      
      acceptor_.listen(net::socket_base::max_listen_connections, ec);
      if (ec) {
	ReportError(ec, "accept", "Can't listen the acceptor");
        return;
      }
    }
  
    void Run() {
      DoAccept();
    }
    
  private:
    void AsyncRunSession(tcp::socket&& socket) {
      auto session = std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_);
      session->Run();
    }
    
    void DoAccept() {
      acceptor_.async_accept(
			     net::make_strand(ioc_),
			     beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }
    
    void OnAccept(beast::error_code ec, tcp::socket socket) {
      using namespace std::literals;
      if (ec) {
        return ReportError(ec, "accept", "Can't make accept");
      }

      AsyncRunSession(std::move(socket));
      DoAccept();
    }
    
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
  };

  template <typename RequestHandler>
  void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
  }

}  // namespace http_server
