#include "respond_maker.h"
#include "boost_log.h"
#include <iostream>

namespace http_handler {
  namespace beast = boost::beast;
  namespace http = beast::http;
  using namespace std::literals;

  using StringResponse = http::response<http::string_body>;
  using FileResponse = http::response<http::file_body>;
  using EmptyResponse = http::response<http::empty_body>;
  using AnyResponse = std::variant<StringResponse, FileResponse, EmptyResponse>;

  namespace json = boost::json;

  
  http::status IdentifyCode(StatusCode code){
    switch(code){
    case StatusCode::CODE_OK:
      return http::status::ok;
    case StatusCode::CODE_NOT_FOUND:
      return http::status::not_found;       
    case StatusCode::CODE_BAD_REQUEST:
      return http::status::bad_request;     
    case StatusCode::CODE_UNATHORIZED:
      return http::status::unauthorized;    
    default:
      return http::status::bad_request;
    }
  }
 

  AnyResponse RespondMaker::ReportServerError(unsigned version, bool keep_alive){
    StringResponse response(http::status::internal_server_error, version);
    response.set(http::field::content_type, GetMimeType("json"));
    response.set(http::field::cache_control, "no-cache");
    response.body() = R"({"code": "serverError", "message": "Internal server error"})";
    response.content_length(response.body().size());
    response.keep_alive(keep_alive);
    return response;
  }
    
  StringResponse RespondMaker::HeadStringResponse(const Request& request) {
    StatusCode code;
    std::string body_new = body_.MakeBodyStringResponse(request, code);
    StringResponse response(IdentifyCode(code), request.version);
    response.set(http::field::content_type, GetMimeType("json"));
    response.content_length(body_new.size());
    response.keep_alive(request.keep_alive);
    response.set(http::field::cache_control, "no-cache");
    
    return response; 
  }
  
  AnyResponse RespondMaker::HeadFileResponse(const Request& request) {
    StatusCode code;
    std::optional<http::file_body::value_type> file_opt = body_.MakeBodyFileResponse(request.target, code);    

    if (code == StatusCode::CODE_NOT_FOUND || code == StatusCode::CODE_BAD_REQUEST) {
      StringResponse response(IdentifyCode(code), request.version);
      response.set(http::field::content_type, GetMimeType("txt"));
      response.keep_alive(request.keep_alive);
      std::string body_new;
      if (code == StatusCode::CODE_NOT_FOUND)
	body_new = body_.MakeBodyError("fileNotFound", "File not found");
      else
	body_new = body_.MakeBodyError("badRequest", "Invalid file path");
      response.body() = body_new;
      response.content_length(body_new.size());
      return response;
    }
    
    EmptyResponse response;
    response.version(request.version);
    response.keep_alive(request.keep_alive);
    response.result(IdentifyCode(code));
    response.set(http::field::content_type, GetContentType(request.target, app_handler_->GetStaticPath()));
    if (file_opt.has_value()) {
      try {
	auto size = file_opt->size(); 
	response.content_length(size);
      } catch (const std::filesystem::filesystem_error& e) {
	BOOST_LOG_TRIVIAL(error) << "HeadFileResponse: Filesystem error getting file size: " << e.what();
      } catch (const std::system_error& e) {
	BOOST_LOG_TRIVIAL(error) << "HeadFileResponse: System error getting file size: " << e.what();
      } catch (const std::exception& e) {
	BOOST_LOG_TRIVIAL(error) << "HeadFileResponse: Unexpected error getting file size: " << e.what();
      }
    }
    
    response.prepare_payload();
    return response;  
  }
  
  StringResponse RespondMaker::MakeWrongMethodResponse(const std::string type_allowed, unsigned version, bool keep_alive){
    StringResponse response(http::status::method_not_allowed, version);
    response.set(http::field::content_type, GetMimeType("json"));
    response.set(http::field::allow, type_allowed);
    response.set(http::field::cache_control, "no-cache");
    response.body() = R"({"code": "invalidMethod", "message": "Invalid method"})";
    response.content_length(response.body().size());
    response.keep_alive(keep_alive);
    return response;

  }
  
  StringResponse RespondMaker::GetStringResponse(const Request& request) {  
    StatusCode code;

    std::string body_content = body_.MakeBodyStringResponse(request, code);
    StringResponse response(IdentifyCode(code), request.version);
    response.set(http::field::content_type, GetMimeType("json"));
    response.keep_alive(request.keep_alive);
    response.set(http::field::cache_control, "no-cache");
    response.body() = body_content;
    response.content_length(body_content.size());
    return response;
  }
  
  AnyResponse RespondMaker::GetFileResponse(const Request& request) {    
    StatusCode code;
    std::optional<http::file_body::value_type> file_opt = body_.MakeBodyFileResponse(request.target, code);
    
    if (code == StatusCode::CODE_NOT_FOUND || code == StatusCode::CODE_BAD_REQUEST) {
      StringResponse response(IdentifyCode(code), request.version);
      response.set(http::field::content_type, GetMimeType("txt"));
      response.keep_alive(request.keep_alive);
      std::string body_new;
      if (code == StatusCode::CODE_NOT_FOUND)
	body_new = body_.MakeBodyError("fileNotFound", "File not found");
      else
	body_new = body_.MakeBodyError("badRequest", "Invalid file path");
      response.body() = body_new;
      response.content_length(body_new.size());
      return response;
    }
        
    FileResponse response;
    response.version(request.version);
    response.result(http::status::ok);
    response.keep_alive(request.keep_alive);
    response.set(http::field::content_type, GetContentType(request.target, app_handler_->GetStaticPath()));
    
    if (file_opt.has_value()) {
      response.body() = std::move(*file_opt);
    }
    
    response.prepare_payload();
    return response;
  }

  StringResponse RespondMaker::PostStringResponse(const Request& request) {
    StatusCode code;
    std::string body_new = body_.MakeBodyStringResponse(request, code);
    StringResponse response(IdentifyCode(code), request.version);
    response.set(http::field::content_type, GetMimeType("json"));
    response.keep_alive(request.keep_alive);
    response.set(http::field::cache_control, "no-cache");
    response.body() = body_new;
    response.content_length(body_new.size());
    return response;
  }
  
  StringResponse RespondMaker::ElseResponse(const Request& request) {
    StringResponse response(http::status::method_not_allowed, request.version);

    std::string allow;
    
    if (request.target == Endpoints::MAPS || request.target.substr(0, Endpoints::MAP_BY_ID.size()) == Endpoints::MAP_BY_ID) {
      allow = "GET, HEAD";
    } else if (request.target == Endpoints::API_GAME_JOIN) {
      allow = "POST";
    } else if (request.target == Endpoints::API_GAME_PLAYERS || request.target == Endpoints::API_GAME_STATE) {
      allow = "GET, HEAD";
    } else if (request.target == Endpoints::API_GAME_PLAYER_ACTION || request.target == Endpoints::API_GAME_TICK) {
      allow = "POST";
    } else {
      allow = "GET, HEAD";
    }
    
    response.set(http::field::allow, allow); 
    response.set(http::field::content_type, GetMimeType("json"));
     
    json::object error_obj;
    error_obj["code"] = "invalidMethod";
    error_obj["message"] = "Invalid method";
    response.body() = json::serialize(error_obj);
    
    response.set(http::field::cache_control, "no-cache");    
    response.content_length(response.body().size());
    response.keep_alive(request.keep_alive);
    return response;
  }
}
