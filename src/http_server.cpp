#include "http_server.h"
#include <boost/asio/dispatch.hpp>
#include <iostream>

using namespace std::literals;

// Ядро асинхронного HTTP-сервера будет располагаться в пространстве имён http_server
namespace http_server {
  
  void ReportError(beast::error_code ec, std::string_view where, std::string_view what) {
    json::value custom_data = {
      {"code", ec.value()},
      {"text", what},
      {"where", where}
    };
    
    BOOST_LOG_TRIVIAL(error) 
      << logging::add_value(additional_data, custom_data)
      << "error"sv;
  }

  std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
      return std::nullopt;
    }
    if (ec) {
      ReportError(ec, "read", "Failed to read request");
      throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }
    return req;
  }
  
  void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
  }

  void SessionBase::Read() {
    using namespace std::literals;
    request_ = {};
    stream_.expires_after(30s);
    http::async_read(stream_, buffer_, request_,
		     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
  }

  void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
      return Close();
    }
    if (ec) {
      return ReportError(ec, "read", "Failed to read request");
    }
    HandleRequest(std::move(request_));
  }

  void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
  }
    
  void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
      return ReportError(ec, "write", "Failed to write respond");
    }

    if (close) {
      return Close();
    }

    Read();
  }

}  // namespace http_server
