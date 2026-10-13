#pragma once
#include "body_maker.h"
#include <string>
#include <variant>
#include <unordered_map>
#include <boost/beast/http.hpp>
#include "application_handler.h"
#include "application_listener.h"

namespace http_handler {
  namespace beast = boost::beast;
  namespace http = beast::http;
    
  using StringResponse = http::response<http::string_body>;
  using FileResponse = http::response<http::file_body>;
  using EmptyResponse = http::response<http::empty_body>;
  using AnyResponse = std::variant<StringResponse, FileResponse, EmptyResponse>;

  struct Request;
  
  inline int HexCharToValue(char hex_char) {
    hex_char = static_cast<char>(std::tolower(hex_char));
    if (hex_char >= '0' && hex_char <= '9') {
      return hex_char - '0';
    } else if (hex_char >= 'a' && hex_char <= 'f') {
      return 10 + (hex_char - 'a');
    }
    return -1; 
  }

  inline std::string UrlDecode(std::string_view input) {
    std::string result;
    result.reserve(input.size());
    
    for (size_t i = 0; i < input.size(); ++i) {
      char ch = input[i];
        
      if (ch == '%' && i + 2 < input.size()) {
        int digit1 = HexCharToValue(input[i + 1]);
        int digit2 = HexCharToValue(input[i + 2]);
            
        if (digit1 != -1 && digit2 != -1) {
          char decoded_char = static_cast<char>(digit1 * 16 + digit2);
          result += decoded_char;
          i += 2;
        } else {
          result += ch;
        }
      }
      else if (ch == '+') {
        result += ' ';
      }
      else {
        result += ch;
      }
    }
    return result;
  }
  
  inline std::filesystem::path MakeStaticPath(std::string_view body, std::string_view static_path){
    std::string decoded_url = UrlDecode(body);
    std::filesystem::path static_fs_path(static_path);
    if (decoded_url.empty() || decoded_url == "/"){
      return static_fs_path / "index.html";
    }
    
    if (decoded_url[0] == '/') {
      decoded_url = decoded_url.substr(1);
    }
    
    std::filesystem::path decoded_fs_path(decoded_url);
    std::filesystem::path full_path = static_fs_path / decoded_fs_path;
    
    if (!std::filesystem::exists(full_path)) {
      if (std::filesystem::is_directory(full_path) || full_path.extension().empty()) {
        full_path /= "index.html";
      }
    }
    
    return full_path;  
  }

  
  inline std::string GetMimeType(const std::string& extension) {
    static const std::unordered_map<std::string, std::string> mime_types = {
      {"html", "text/html"},
      {"css", "text/css"},
      {"js", "text/javascript"},
      {"json", "application/json"},
      {"xml", "application/xml"},
      {"txt", "text/plain"},
      {"png", "image/png"},
      {"jpg", "image/jpeg"},
      {"jpe", "image/jpeg"},
      {"jpeg", "image/jpeg"},
      {"gif", "image/gif"},
      {"bmp", "image/bmp"},
      {"ico", "image/vnd.microsoft.icon"},
      {"svg", "image/svg+xml"},
      {"tiff", "image/tiff"},
      {"mp3", "audio/mpeg"},
      {"", "application/octet-stream"}
    };
  
    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
      return it->second;
    }
  
    return "application/octet-stream";
  }
  
  inline std::string GetContentType(std::string_view path, std::string_view static_path){
    std::string file_path = MakeStaticPath(path, static_path).string();
    size_t dot_pos = file_path.rfind('.');
    if (dot_pos == std::string_view::npos) {
      return GetMimeType(""); 
    }
    
    std::string extension;
    for (size_t i = dot_pos + 1; i < file_path.size(); ++i) {
      extension += static_cast<char>(std::tolower(file_path[i]));
    }
    
    if (extension.empty()) {
      return GetMimeType("");
    }
    
    return GetMimeType(extension);
  }
  
  class RespondMaker{
  public:
    RespondMaker(Application* application)
      : body_{application}, app_handler_(application->GetAppHandler()){}
    
    template <typename Body, typename Allocator>
    AnyResponse HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req){
      try{
	auto auth_field = req[http::field::authorization];
	Request request;
    
	auto target_sv = req.target();
	request.target = std::string(target_sv.data(), target_sv.size());
       
	auto method_sv = req.method_string();
	request.method = std::string(method_sv.data(), method_sv.size());
      
	auto ct_sv = req[http::field::content_type];
	request.content_type = ct_sv.empty() ? "" : std::string(ct_sv.data(), ct_sv.size());
      
	auto auth_sv = req[http::field::authorization];
	request.authorization = auth_sv.empty() ? "" : std::string(auth_sv.data(), auth_sv.size());
      
	request.body = req.body();
	request.version = req.version();
	request.keep_alive = req.keep_alive();

	
	std::string method_upper = request.method;
	std::transform(method_upper.begin(), method_upper.end(), method_upper.begin(), ::toupper);
	bool is_maps = (request.target == Endpoints::MAPS);
	bool is_map_by_id = (request.target.substr(0, Endpoints::MAP_BY_ID.size()) == Endpoints::MAP_BY_ID);
	
	if (is_maps || is_map_by_id) {
	  if (method_upper != "GET" && method_upper != "HEAD") {
	    return MakeWrongMethodResponse("GET, HEAD", req.version(), req.keep_alive());
	  }
	}
	
	if (request.target == Endpoints::API_GAME_JOIN){
	  if (method_upper != "POST")
	    return MakeWrongMethodResponse("POST", req.version(), req.keep_alive());
	}
	
	if (request.target == Endpoints::API_GAME_PLAYERS) {
	  if (method_upper != "GET" && method_upper != "HEAD")
	    return MakeWrongMethodResponse("GET, HEAD", req.version(), req.keep_alive());
	}
	
	if (request.target == Endpoints::API_GAME_STATE) {
	  if (method_upper != "GET" && method_upper != "HEAD") 
	    return MakeWrongMethodResponse("GET, HEAD", req.version(), req.keep_alive());
	}
	
	if (request.target == Endpoints::API_GAME_PLAYER_ACTION) {
	  if (method_upper != "POST") 
	    return MakeWrongMethodResponse("POST", req.version(), req.keep_alive());
	}

	if (request.target == Endpoints::API_GAME_TICK) {
	  if (method_upper != "POST") 
	    return MakeWrongMethodResponse("POST", req.version(), req.keep_alive());
	}

	if (req.method() == http::verb::get) {
	  return GetStringResponse(request);
	}
	
	if (req.method() == http::verb::head) {
	  return HeadStringResponse(request);
	}
	
	if (req.method() == http::verb::post) {
	  return PostStringResponse(request);
	}
    
	return ElseResponse(request);
      } catch (const std::exception& ex) {
	return ReportServerError(req.version(),	req.keep_alive());
      }
    }

    template <typename Body, typename Allocator>
    AnyResponse HandleFileRequest(http::request<Body, http::basic_fields<Allocator>>&& req){
      try{
	Request request;
	auto auth_field = req[http::field::authorization];
	auto target_sv = req.target();
	request.target = std::string(target_sv.data(), target_sv.size());
	auto method_sv = req.method_string();
	request.method = std::string(method_sv.data(), method_sv.size());
	auto ct_sv = req[http::field::content_type];
	request.content_type = ct_sv.empty() ? "" : std::string(ct_sv.data(), ct_sv.size());
	auto auth_sv = req[http::field::authorization];
	request.authorization = auth_sv.empty() ? "" : std::string(auth_sv.data(), auth_sv.size());
      
	request.body = req.body();
	request.version = req.version();
	request.keep_alive = req.keep_alive();
    
	if (req.method() == http::verb::get)
	  return GetFileResponse(request);
	if (req.method() == http::verb::head)
	  return HeadFileResponse(request);
      
	return ElseResponse(request);
      } catch (const std::exception& ex) {
	return ReportServerError(req.version(),	req.keep_alive());
      }
    }

    AnyResponse ReportServerError(unsigned version, bool keep_alive);
    
  private:
    
    StringResponse HeadStringResponse(const Request& request);
    AnyResponse HeadFileResponse(const Request& request);
    StringResponse GetStringResponse(const Request& request);
    AnyResponse GetFileResponse(const Request& request);
    StringResponse PostStringResponse(const Request& request);
    StringResponse MakeWrongMethodResponse(const std::string type_allowed, unsigned version, bool keep_alive);
    StringResponse ElseResponse(const Request& request);
    
    BodyMaker body_;
    app_handler::ApplicationHandler* app_handler_;
  };

}
