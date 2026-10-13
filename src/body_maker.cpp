#include "body_maker.h"
#include "boost_log.h"
#include <iostream>
#include <deque>
#include <vector>

namespace http_handler {
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace json = boost::json;

  std::filesystem::path MakeStaticPath(std::string_view body, std::string_view static_path);
  
  std::string BodyMaker::MakeBodyStringResponse(const Request& request, StatusCode& code) {
    const std::string& target = request.target;
        
    if (target == Endpoints::MAPS) {
      return MakeBodyForMaps(code);
    }

    if (target.substr(0, Endpoints::MAP_BY_ID.size()) == Endpoints::MAP_BY_ID) {
      std::string map_id = target.substr(Endpoints::MAP_BY_ID.size());
      return MakeBodyForMapID(code, map_id);
    }

    if (target == Endpoints::API_GAME_JOIN) {
      return MakeBodyForGameJoin(code, request);
    }
    
    if (target == Endpoints::API_GAME_PLAYERS) {
      return ExecuteAuthorized(code, request, [this](Token token, const Request& request, StatusCode& code) {
        return MakeBodyForGamePlayers(code, request, token);
      });
    }

    if (target == Endpoints::API_GAME_STATE) {
      return ExecuteAuthorized(code, request, [this](Token token, const Request& request, StatusCode& code) {
        return MakeBodyForGameState(code, request, token);
      });
    }
    
    if (target == Endpoints::API_GAME_PLAYER_ACTION) {
      return ExecuteAuthorized(code, request, [this](Token token, const Request& request, StatusCode& code) {
        return MakeBodyForGamePlayerAction(code, request, token);
      });
    }

    if (target == Endpoints::API_GAME_TICK) {
      if (app_handler_->GetTickPeriod().has_value()) {
	code = StatusCode::CODE_BAD_REQUEST;
	return MakeBodyError("badRequest", "Invalid endpoint");
      }
      return MakeBodyForGameTick(code, request);
    }
    
    if (target.substr(0, Endpoints::API_GAME_RECORDS.size()) == Endpoints::API_GAME_RECORDS) {
      return MakeBodyForGameRecords(code, request);
    }
	
    if (target.substr(0, Endpoints::API_BASE.size()) == Endpoints::API_BASE) {
      code = StatusCode::CODE_BAD_REQUEST;
      return MakeBodyError("badRequest", "Bad request");
    }


      
    code = StatusCode::CODE_NOT_FOUND;
    return MakeBodyError("pathNotFound", "Bad request");
  }
  
  std::optional<http::file_body::value_type> BodyMaker::MakeBodyFileResponse(std::string_view target, StatusCode& code){
    
    http::file_body::value_type file;
    std::filesystem::path static_fs_path(app_handler_->GetStaticPath());    
    std::filesystem::path full_path = MakeStaticPath(target, app_handler_->GetStaticPath());
    
    if (!std::filesystem::exists(full_path) || !std::filesystem::is_regular_file(full_path)) {
      code = StatusCode::CODE_NOT_FOUND;
      return std::nullopt;
    }
    
    if (!IsSubPath(full_path, static_fs_path)){
      code = StatusCode::CODE_BAD_REQUEST;
      return std::nullopt;
    }
    
    boost::system::error_code ec; 
    file.open(full_path.string().c_str(), boost::beast::file_mode::read, ec);
    if (ec) {
      code = StatusCode::CODE_NOT_FOUND;
      return std::nullopt;
    }
    
    return file;
  }

  std::string BodyMaker::MakeBodyError(std::string code_name, std::string message){
    json::object obj;
    obj["code"] = std::move(code_name);
    obj["message"] = std::move(message);
    return json::serialize(obj);
  }

	
  json::array BodyMaker::MakeBodyForRoads(const std::vector<model::Road>& roads){
    json::array road_array;
    for (const auto& road : roads) {
      json::object road_obj;
      model::Point pos_start = road.GetStart();
      model::Point pos_end = road.GetEnd();
      road_obj["x0"] = pos_start.x;
      road_obj["y0"] = pos_start.y;
      if (road.IsHorizontal()) {
	road_obj["x1"] = pos_end.x;
      } else {
	road_obj["y1"] = pos_end.y;
      }
      road_array.emplace_back(std::move(road_obj));
    }
    return road_array;
  }

  json::array BodyMaker::MakeBodyForBuildings(const std::vector<model::Building>& buildings){
    json::array building_array;
    for (const auto& building : buildings) {
      json::object building_obj;
      model::Rectangle rect = building.GetBounds();
      building_obj["x"] = rect.position.x;
      building_obj["y"] = rect.position.y;
      building_obj["w"] = rect.size.width;
      building_obj["h"] = rect.size.height;
      building_array.emplace_back(std::move(building_obj));
    }
    return building_array;
  }

  json::array BodyMaker::MakeBodyForOffices(const std::vector<model::Office>& offices){
    json::array office_array;
    for (const auto& office : offices) {
      json::object office_obj;
      model::Office::Id id = office.GetId();
      model::Point pos = office.GetPosition();
      model::Offset offset = office.GetOffset();
      office_obj["id"] = *id;
      office_obj["x"] = pos.x;
      office_obj["y"] = pos.y;
      office_obj["offsetX"] = offset.dx;
      office_obj["offsetY"] = offset.dy;
      office_array.emplace_back(std::move(office_obj));
    }
    return office_array;
  }

  json::array BodyMaker::MakeBodyForLoots(const extra::ExtraItems& extra, model::Map::Id id_map){
    json::array loot_array;
    std::deque<extra::Item> items = extra.GetItems(id_map);
    for (const auto& item: items) {
      json::object item_obj;

      if (item.color.has_value())
	item_obj["color"] = item.color.value();
      if (item.file.has_value())
	item_obj["file"] = item.file.value();
      if (item.name.has_value())
	item_obj["name"] = item.name.value();
      if (item.rotation.has_value())
	item_obj["rotation"] = item.rotation.value();
      if (item.type.has_value())
	item_obj["type"] = item.type.value();
      if (item.scale.has_value())
	item_obj["scale"] = item.scale.value();
      if (item.value.has_value())
	item_obj["value"] = item.value.value();  
      loot_array.emplace_back(std::move(item_obj));
    }
    return loot_array;
  }
  
  bool BodyMaker::IsSubPath(std::filesystem::path path, std::filesystem::path base) {
    try {
      path = std::filesystem::weakly_canonical(path);
      base = std::filesystem::weakly_canonical(base);
    } catch (const std::filesystem::filesystem_error& e) {
      BOOST_LOG_TRIVIAL(error) << "IsSubPath: Failed find the subpath: " << e.what();
      return false;
    } catch (const std::bad_alloc& e) {
      BOOST_LOG_TRIVIAL(error) << "IsSubPath: Failed find the subpath: " << e.what();
      return false;
    } catch (const std::exception& e) {
      BOOST_LOG_TRIVIAL(error) << "IsSubPath: Failed find the subpath: " << e.what();
      return false;
    }
    
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
      if (p == path.end() || *p != *b) {
	return false;
      }
    }
    return true;
  }
    
  std::string BodyMaker::MakeBodyForMaps(StatusCode& code){
    json::array obj_array;
    const std::vector<model::Map>& maps = app_handler_->GetMaps();
        
    for (const auto& map : maps) {
      json::object obj;
      obj["id"] = *map.GetId();
      obj["name"] = map.GetName();
      obj_array.emplace_back(std::move(obj));
    }
          
    code = StatusCode::CODE_OK;
    std::string result = json::serialize(obj_array);
    return result;
  }
  
  std::string BodyMaker::MakeBodyForMapID(StatusCode& code, std::string_view target){
    model::Map::Id id_map{std::string(target)};
    const model::Map* map = app_handler_->FindMap(id_map);
    if (map == nullptr) {
      code = StatusCode::CODE_NOT_FOUND;
      return MakeBodyError("mapNotFound", "Map not found");
    }
    
    json::object obj;
    obj["id"] = *map->GetId();
    obj["name"] = map->GetName();
    obj["roads"] = MakeBodyForRoads(map->GetRoads());
    obj["buildings"] = MakeBodyForBuildings(map->GetBuildings());
    obj["offices"] = MakeBodyForOffices(map->GetOffices());
    obj["lootTypes"] = MakeBodyForLoots(app_handler_->GetLootItems(), id_map);
    code = StatusCode::CODE_OK;
    return json::serialize(obj);
  }

  std::optional<json::value> BodyMaker::ParseRequestBody(const std::string& body) {
    try {
      return json::parse(body);
    } catch (const boost::system::system_error& e) {
      BOOST_LOG_TRIVIAL(error) << "ParseRequestBody: Failed to parse the json string: " << e.what();
      return std::nullopt;
    } catch (const std::bad_alloc& e) {
      BOOST_LOG_TRIVIAL(error) << "ParseRequestBody: Failed to parse the json string: " << e.what();
      return std::nullopt;
    } catch (const std::exception& e) {
      BOOST_LOG_TRIVIAL(error) << "ParseRequestBody: Failed to parse the json string: " << e.what();
      return std::nullopt;
    }
  }

  Token BodyMaker::TryExtractToken(const Request& request){
    std::string auth_header = request.authorization;
    if (auth_header.empty())
      throw std::runtime_error("Authorization header is missing");
    
    const std::string bearer_prefix = "Bearer ";
    if (auth_header.size() <= bearer_prefix.size() || 
	auth_header.substr(0, bearer_prefix.size()) != bearer_prefix)
      throw std::runtime_error("Invalid authorization format. Expected 'Bearer <token>'");
    
    std::string token_str = auth_header.substr(bearer_prefix.size());
        
    if (!api::IsValidToken(token_str))
      throw std::runtime_error("Invalid token format. Token must be 32 hexadecimal characters");

    return Token{token_str};;
  }
  
  std::string BodyMaker::MakeUnauthorizedError(StatusCode& code, const std::string& message){
    code = StatusCode::CODE_UNATHORIZED;
    return MakeBodyError("invalidToken", message);
  }

  std::string BodyMaker::MakeBodyForGamePlayers(StatusCode& code, const Request& request, Token token) {    
    api::Player* player = app_handler_->FindByToken(token);

    if (player == nullptr) {
      code = StatusCode::CODE_UNATHORIZED;  
      return MakeBodyError("unknownToken", "Player token has not been found");
    }
    
    model::GameSession* session = player->GetSession();
    if (!session) {
      code = StatusCode::CODE_UNATHORIZED;
      return MakeBodyError("unknownToken", "Player session not found");
    }
    
    auto map_id = player->GetMapId();

    const std::deque<model::Dog>& dogs = session->GetDogs();
    json::object obj_resp;

    for (const auto& dog : dogs) {
      auto* dog_player = app_handler_->FindByDogIdAndMapId(dog.GetId(), player->GetMapId());
      if (dog_player != nullptr) {
	json::object player_obj;
	player_obj["name"] = dog_player->GetDogName();
	obj_resp[std::to_string(dog_player->GetId())] = player_obj;
      }
    }
    code = StatusCode::CODE_OK; 
    return json::serialize(obj_resp);
  }
  
  std::string BodyMaker::MakeBodyForGameJoin(StatusCode& code, const Request& request) {
        
    std::string content_type_lower = request.content_type;
    std::transform(content_type_lower.begin(), content_type_lower.end(), 
		   content_type_lower.begin(), ::tolower);
    
    if (content_type_lower.find("application/json") == std::string::npos) {
      code = StatusCode::CODE_BAD_REQUEST;  
      return MakeBodyError("invalidArgument", "Content type must be application/json");
    }
    
    std::optional<json::value> json_data_opt = ParseRequestBody(request.body);
    if (!json_data_opt.has_value()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "Join game request parse error");
    }
    
    if (!json_data_opt->is_object()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "Invalid JSON format");
    }
    
    const auto& obj = json_data_opt->as_object();
    
    if (!obj.contains("userName")) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "userName not found");
    }
    
    auto name_value = obj.at("userName");
    if (!name_value.is_string()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "userName must be a string");
    }
    
    std::string name = name_value.as_string().c_str();
    if (name.empty()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "userName cannot be empty");
    }
    
    if (!obj.contains("mapId")) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "mapId not found");
    }
    
    auto map_id_value = obj.at("mapId");
    if (!map_id_value.is_string()) {
      code = StatusCode::CODE_NOT_FOUND; 
      return MakeBodyError("mapNotFound", "mapId must be a string");
    }
    
    std::string map_id = map_id_value.as_string().c_str();
    model::Map* map = app_handler_->FindMap(model::Map::Id{map_id});
    if (map == nullptr) {
      code = StatusCode::CODE_NOT_FOUND; 
      return MakeBodyError("mapNotFound", "Map doesn't exist");
    }
    
    if (app_handler_->IsNameTakenOnMap(name, map->GetId())) {      
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "Player with this name already exists on the map");
    }
    
    model::GameSession* session = app_handler_->GetOrCreateSession(map);
    api::Player& player = app_handler_->Add(name, session);
    
    json::object obj_resp;
    std::optional<Token> token_opt = app_handler_->GetTokenByPlayer(&player);
    if (!token_opt.has_value()) {
      code = StatusCode::CODE_BAD_REQUEST;  
      return MakeBodyError("serverError", "Failed to generate token");
    }
    
    obj_resp["authToken"] = *(*token_opt);
    obj_resp["playerId"] = player.GetId();
    
    code = StatusCode::CODE_OK; 
    return json::serialize(obj_resp);
  }

  
  std::string BodyMaker::MakeBodyForGameState(StatusCode& code, const Request& request, Token token) {
    
    api::Player* player = app_handler_->FindByToken(token);

    if (player == nullptr) {
      code = StatusCode::CODE_UNATHORIZED;  
      return MakeBodyError("unknownToken", "Player token has not been found");
    }
    
    model::GameSession* session = player->GetSession();
    if (!session) {
      code = StatusCode::CODE_UNATHORIZED;
      return MakeBodyError("unknownToken", "Player session not found");
    }

    auto map_id = player->GetMapId();
    const std::deque<model::Dog>& dogs = session->GetDogs();
    json::object obj_resp;
    
    for (const auto& dog : dogs) {

      json::object player_obj;

      model::Position pos = dog.GetPosition();
      json::array player_info_pos;
      player_info_pos.emplace_back(pos.x);
      player_info_pos.emplace_back(pos.y);

      model::Velocity vel = dog.GetVelocity();
      json::array player_info_vel;
      player_info_vel.emplace_back(vel.vx);
      player_info_vel.emplace_back(vel.vy);

      
      player_obj["pos"] = player_info_pos;
      player_obj["speed"] = player_info_vel;
      player_obj["dir"] = model::GetDirection(dog.GetDirection());
      
      const std::vector<model::BagContent>& bag = dog.GetBag();

      json::array player_info_bag;
      for (const auto& i:bag){
	json::object bag_item;
	bag_item["id"] = i.id;
	bag_item["type"] = i.type;
	player_info_bag.emplace_back(bag_item);
      }

      player_obj["bag"] = player_info_bag;
      player_obj["score"] = dog.GetScore();
      obj_resp[std::to_string(dog.GetId())] = player_obj;
    }
    
    json::object obj_resp_players;
    obj_resp_players["players"] = obj_resp;

    const std::vector<model::ItemsOnRoads>& items = session->GetItems();

    json::object obj_resp_loot;
    int items_number = 0;
    for (const auto& item:items) {
      json::object loot_obj;
      loot_obj["type"] = item.type;
      json::array loot_info_pos;
      loot_info_pos.emplace_back(item.pos.x);
      loot_info_pos.emplace_back(item.pos.y);
      loot_obj["pos"] = loot_info_pos;
      obj_resp_loot[std::to_string(item.id)] = loot_obj;
    }
    
    obj_resp_players["lostObjects"] = obj_resp_loot;
    
    
    code = StatusCode::CODE_OK; 
    return json::serialize(obj_resp_players);
  }
  
  std::string BodyMaker::MakeBodyForGamePlayerAction(StatusCode& code, const Request& request, Token token){
    api::Player* player = app_handler_->FindByToken(token);

    if (player == nullptr) {
      code = StatusCode::CODE_UNATHORIZED;  
      return MakeBodyError("unknownToken", "Player token has not been found");
    }
    
    std::optional<json::value> json_data_opt = ParseRequestBody(request.body);
    if (!json_data_opt.has_value()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "Join game request parse error");
    }
    
    if (!json_data_opt->is_object()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "Invalid JSON format");
    }
    
    const auto& obj = json_data_opt->as_object();
    
    if (!obj.contains("move")) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "move not found");
    }
    
    auto direction_value = obj.at("move");
    if (!direction_value.is_string()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "move must be a string");
    }

    auto map_id = player->GetMapId();
    model::Map* map = app_handler_->FindMap(model::Map::Id{map_id});

    auto* dog = player->GetDog();
    double dog_speed = map->GetDogSpeed();
    
    std::string direction = direction_value.as_string().c_str();
    model::Velocity vel;
    model::Direction dir;

    if (direction != "L" && direction != "R" && direction != "U" && direction != "D" && direction != "") {
      code = StatusCode::CODE_BAD_REQUEST;
      return MakeBodyError("invalidArgument", "Invalid move value");
    }

    if (direction == "L"){
      dir = model::Direction::WEST;
      vel = {-dog_speed, 0};
    }
    if (direction == "R"){
      dir = model::Direction::EAST;
      vel = {dog_speed, 0};
    }
    if (direction == "U"){
      dir = model::Direction::NORTH;
      vel = {0, -dog_speed};
    }
    if (direction == "D"){
      dir = model::Direction::SOUTH;
      vel = {0, dog_speed};
    }
    if (direction == ""){
      vel = {0, 0};
    }
    
    if (!direction.empty())
      dog->SetDirection(dir);
        
    dog->SetVelocity(vel);
    
    json::object obj_resp;

    code = StatusCode::CODE_OK;
    return json::serialize(obj_resp);
  }



  std::string BodyMaker::MakeBodyForGameTick(StatusCode& code, const Request& request) {
    
    std::string content_type_lower = request.content_type;
    std::transform(content_type_lower.begin(), content_type_lower.end(), 
		   content_type_lower.begin(), ::tolower);
    
    if (content_type_lower.find("application/json") == std::string::npos) {
      code = StatusCode::CODE_BAD_REQUEST;  
      return MakeBodyError("invalidArgument", "Content type must be application/json");
    }
    
    std::optional<json::value> json_data_opt = ParseRequestBody(request.body);
    if (!json_data_opt.has_value()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "Failed to parse tick request JSON");
    }
    
    if (!json_data_opt->is_object()) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "Invalid JSON format");
    }
    
    const auto& obj = json_data_opt->as_object();
    
    if (!obj.contains("timeDelta")) {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "timeDelta not found");
    }
    
    auto time_delta_value = obj.at("timeDelta");
    double time_delta_ms;
    if (time_delta_value.is_int64()) {
      time_delta_ms = static_cast<double>(time_delta_value.as_int64());
    } else if (time_delta_value.is_double()) {
      time_delta_ms = time_delta_value.as_double();
    } else {
      code = StatusCode::CODE_BAD_REQUEST; 
      return MakeBodyError("invalidArgument", "timeDelta must be a number");
    }

    if (time_delta_ms < 0) {
      code = StatusCode::CODE_BAD_REQUEST;
      return MakeBodyError("invalidArgument", "timeDelta must be non-negative");
    }
    
    int64_t rounded_ms = static_cast<int64_t>(std::round(time_delta_ms));
    
    try {
      std::chrono::milliseconds delta(rounded_ms);
      application_->Tick(delta);
    } catch (const std::exception& e) {
      code = StatusCode::CODE_BAD_REQUEST;
      BOOST_LOG_TRIVIAL(error) << "MakeBodyForGameTick: Failed to update game state: " << e.what();
      return MakeBodyError("serverError", "Failed to update game state");
    }
    
    json::object obj_resp;
    code = StatusCode::CODE_OK; 
    return json::serialize(obj_resp);
  }

  
  std::string BodyMaker::MakeBodyForGameRecords(StatusCode& code, const Request& request) {
    int start = 0;
    int max_items = 100;
    
    std::string target = request.target;
    size_t query_pos = target.find('?');
    
    if (query_pos != std::string::npos) {
      std::string query_string = target.substr(query_pos + 1);
      std::stringstream ss(query_string);
      std::string param;
        
      while (std::getline(ss, param, '&')) {
	size_t eq_pos = param.find('=');
	if (eq_pos != std::string::npos) {
	  std::string key = param.substr(0, eq_pos);
	  std::string value = param.substr(eq_pos + 1);
                
	  try {
	    if (key == "start") {
	      start = std::stoi(value);
	      if (start < 0) {
		code = StatusCode::CODE_BAD_REQUEST;
		return MakeBodyError("invalidArgument", "start must be non-negative");
	      }
	    } else if (key == "maxItems") {
	      max_items = std::stoi(value);
	      if (max_items < 0) {
		code = StatusCode::CODE_BAD_REQUEST;
		return MakeBodyError("invalidArgument", "maxItems must be non-negative");
	      }
	      if (max_items > 100) {
		code = StatusCode::CODE_BAD_REQUEST;
		return MakeBodyError("invalidArgument", "maxItems cannot exceed 100");
	      }
	    }
	  } catch (const std::exception& e) {
	    code = StatusCode::CODE_BAD_REQUEST;
	    return MakeBodyError("invalidArgument", "Invalid parameter value");
	  }
	}
      }
    }

    std::vector<std::tuple<std::string, int, double>> records = application_->GetRecords(start, max_items);
    
    json::array record_info_array;
    for (const auto& record : records) {
      json::object record_obj;
      record_obj["name"] = std::get<0>(record);
      record_obj["score"] = std::get<1>(record);
      record_obj["playTime"] = std::get<2>(record);
      record_info_array.emplace_back(std::move(record_obj));
    }
    
    code = StatusCode::CODE_OK;
    return json::serialize(record_info_array);
  }
  
}
