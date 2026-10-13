#pragma once
#include <string>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <stdexcept>
#include "application_handler.h"
#include "application_listener.h"
#include "extra_objects.h"

namespace http_handler {
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace json = boost::json;  

  class Endpoints {
  public:
    static constexpr std::string_view API_BASE = "/api/";
    static constexpr std::string_view API_V1 = "/api/v1/";
    static constexpr std::string_view MAPS = "/api/v1/maps";
    static constexpr std::string_view MAP_BY_ID = "/api/v1/maps/";
    static constexpr std::string_view API_GAME_JOIN = "/api/v1/game/join";
    static constexpr std::string_view API_GAME_PLAYERS = "/api/v1/game/players";
    static constexpr std::string_view API_GAME_STATE = "/api/v1/game/state";
    static constexpr std::string_view API_GAME_PLAYER_ACTION = "/api/v1/game/player/action";
    static constexpr std::string_view API_GAME_RECORDS = "/api/v1/game/records";
    static constexpr std::string_view API_GAME_TICK = "/api/v1/game/tick";
  };
  

  enum class StatusCode{
    CODE_OK,
    CODE_NOT_FOUND,
    CODE_BAD_REQUEST,
    CODE_UNATHORIZED
  };
  
  struct Request {
    std::string body;
    std::string method;
    std::string content_type;
    std::string authorization;
    std::string target;
    unsigned version;
    bool keep_alive;
  }; 

  class BodyMaker{
  public:
    BodyMaker(Application* application):app_handler_(application->GetAppHandler()), application_(application){};
    std::string MakeBodyStringResponse(const Request& request, StatusCode& code);  
    std::optional<http::file_body::value_type> MakeBodyFileResponse(std::string_view target, StatusCode& code);
    std::string MakeBodyError(std::string code_name, std::string message);

  private:

    template <typename Fn>
    std::string ExecuteAuthorized(StatusCode& code, const Request& request, Fn&& action) {
      try {
	auto token = TryExtractToken(request);
	return action(token, request, code);
      } catch(const std::exception& e) {
	return MakeUnauthorizedError(code, e.what());
      }
    }
    
    json::array MakeBodyForRoads(const std::vector<model::Road>& roads);
    json::array MakeBodyForBuildings(const std::vector<model::Building>& buildings);
    json::array MakeBodyForOffices(const std::vector<model::Office>& offices); 
    bool IsSubPath(std::filesystem::path path, std::filesystem::path base);
    json::array MakeBodyForLoots(const extra::ExtraItems& extra, model::Map::Id id_map);
    std::string MakeBodyForMaps(StatusCode& code);
    std::string MakeBodyForMapID(StatusCode& code, std::string_view target);
    std::optional<json::value> ParseRequestBody(const std::string& body);
    std::string MakeBodyForGameJoin(StatusCode& code, const Request& request);
    std::string MakeBodyForGamePlayers(StatusCode& code, const Request& request, Token token);
    std::string MakeBodyForGameState(StatusCode& code, const Request& request, Token token);
    std::string MakeBodyForGamePlayerAction(StatusCode& code, const Request& request, Token token);
    std::string MakeUnauthorizedError(StatusCode& code, const std::string& message);
    std::string MakeBodyForGameTick(StatusCode& code, const Request& request);
    std::string MakeBodyForGameRecords(StatusCode& code, const Request& request);
    Token TryExtractToken(const Request& request);
    app_handler::ApplicationHandler* app_handler_;
    Application* application_;
  };
}
