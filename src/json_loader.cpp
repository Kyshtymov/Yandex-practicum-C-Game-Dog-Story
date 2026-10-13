#include "json_loader.h"
#include "boost_log.h"
#include "extra_objects.h"
#include <boost/json.hpp>
#include <fstream>
#include <stdexcept>

namespace json = boost::json;

namespace json_loader {
  const json::object& ExtractObject(const json::object& obj, std::string_view key) {
    auto* ptr = obj.if_contains(key.data()); 
    if (!ptr || !ptr->is_object()) {
      throw std::runtime_error("JSON missing or invalid field: " + std::string(key));
    }
    return ptr->as_object(); 
  }

  const json::array& ExtractArray(const json::object& obj, std::string_view key) {
    auto* ptr = obj.if_contains(key.data()); 
    if (!ptr || !ptr->is_array()) {
      throw std::runtime_error("JSON missing or invalid field: " + std::string(key));
    }
    return ptr->as_array(); 
  }

  
  std::string ExtractString(const json::object& obj, std::string_view key) {
    auto* ptr = obj.if_contains(key.data());  
    if (!ptr || !ptr->is_string()) {
      throw std::runtime_error("Missing or invalid field: " + std::string(key));
    }
    return json::value_to<std::string>(*ptr);
  }
  
  int64_t ExtractInt(const json::object& obj, std::string_view key) {
    auto* ptr = obj.if_contains(key.data());  
    if (!ptr || !ptr->is_int64()) {
      throw std::runtime_error("Missing or invalid field: " + std::string(key));
    }
    return ptr->as_int64();
  }

  double ExtractDouble(const json::object& obj, std::string_view key) {
    auto* ptr = obj.if_contains(key.data());  
    if (!ptr || !ptr->is_double()) {
      throw std::runtime_error("Missing or invalid field: " + std::string(key));
    }
    return ptr->as_double();
  }
  
  void LoadRoadsFromJson(const json::object& map_json, model::Map& map){
    const auto& roads_array = ExtractArray(map_json, "roads");
    for (const auto& road : roads_array) {
      if (!road.is_object())
        throw std::runtime_error("Road must be an object");
      
      auto& road_obj = road.as_object();
      int64_t road_x0 = ExtractInt(road_obj, "x0");
      int64_t road_y0 = ExtractInt(road_obj, "y0");
      
      model::Point p_init{
        static_cast<model::Coord>(road_x0),
        static_cast<model::Coord>(road_y0)
      };


      auto* road_x1 = road_obj.if_contains("x1");  
      auto* road_y1 = road_obj.if_contains("y1");  

      if (!road_x1 && !road_y1)
	throw std::runtime_error("The road must contain x1 or y1");

      
      if (road_x1 && road_y1)
	throw std::runtime_error("The road must not contain x1 and y1");
      
      if (road_x1) {
	model::Coord fin_p = static_cast<model::Coord>(road_x1->as_int64());
        map.AddRoad(model::Road(model::Road::HORIZONTAL, p_init, fin_p));
      }

      if (road_y1) {	
	model::Coord fin_p = static_cast<model::Coord>(road_y1->as_int64());
        map.AddRoad(model::Road(model::Road::VERTICAL, p_init, fin_p));
      }
    }
  }

  void LoadBuildingsFromJson(const json::object& map_json, model::Map& map){
    const auto& buildings_array = ExtractArray(map_json, "buildings"); 
    for (const auto& building : buildings_array) {
      if (!building.is_object())
        throw std::runtime_error("Building must be an object");
            
      auto& building_obj = building.as_object();
      int64_t building_x = ExtractInt(building_obj, "x");
      int64_t building_y = ExtractInt(building_obj, "y");
      int64_t building_w = ExtractInt(building_obj, "w");
      int64_t building_h = ExtractInt(building_obj, "h");
                        
      model::Point p{
        static_cast<model::Coord>(building_x),
        static_cast<model::Coord>(building_y)
      };
            
      model::Size size{
        static_cast<model::Dimension>(building_w),
        static_cast<model::Dimension>(building_h)
      };
            
      model::Rectangle rect{p, size};
      model::Building building_map(rect);
      map.AddBuilding(building_map);
    }
  }

  void LoadOfficesFromJson(const json::object& map_json, model::Map& map){        
    const auto& offices_array = ExtractArray(map_json, "offices");
    
    for (const auto& office : offices_array) {

      if (!office.is_object())
        throw std::runtime_error("Office must be an object");
      
      auto& office_obj = office.as_object();
      std::string office_id = ExtractString(office_obj, "id");
      int64_t office_x = ExtractInt(office_obj, "x");
      int64_t office_y = ExtractInt(office_obj, "y");
      int64_t office_offsetx = ExtractInt(office_obj, "offsetX");
      int64_t office_offsety = ExtractInt(office_obj, "offsetY");
                      
      model::Office::Id id{office_id};
            
      model::Point p{
        static_cast<model::Coord>(office_x),
        static_cast<model::Coord>(office_y)
      };
            
      model::Offset offset{
        static_cast<model::Dimension>(office_offsetx),
        static_cast<model::Dimension>(office_offsety)
      };
      model::Office office_map(id, p, offset);
      map.AddOffice(office_map);
    }
  }
  void LoadLootsFromJson(const json::object& map_json, model::Map& map, extra::ExtraItems& extra, const model::Map::Id& id_map){        
    const auto& loot_array = ExtractArray(map_json, "lootTypes");
    int id = 0;
    std::deque<extra::Item> map_items;
    for (const auto& loot : loot_array) {
      if (!loot.is_object())
        throw std::runtime_error("lootTypes must be an object");
      
      auto& loot_obj = loot.as_object();

      extra::Item item;

      auto* item_name = loot_obj.if_contains("name");
      auto* item_file = loot_obj.if_contains("file");
      auto* item_type = loot_obj.if_contains("type");
      auto* item_color = loot_obj.if_contains("color");
      auto* item_rotation = loot_obj.if_contains("rotation");
      auto* item_scale = loot_obj.if_contains("scale");
      auto* item_value = loot_obj.if_contains("value");
      
      if (item_name)
        item.name = json::value_to<std::string>(*item_name);
      if (item_file)
        item.file = json::value_to<std::string>(*item_file);
      if (item_type)
        item.type = json::value_to<std::string>(*item_type);
      if (item_color)
        item.color = json::value_to<std::string>(*item_color);
      if (item_rotation)
        item.rotation = static_cast<int>(item_rotation->as_int64());
      if (item_scale)
        item.scale = item_scale->as_double();
      if (item_value)
        item.value = item_value->as_int64();
      
      item.id = id++;
      map_items.push_back(item);
    }
    extra.SetItem(id_map, map_items);
  }

  void LoadMapsFromJson(const json::object& obj_maps, model::Game& game){
    const auto& maps_array = ExtractArray(obj_maps, "maps");
    for (const auto& value_map : maps_array) {
      if (!value_map.is_object())
        throw std::runtime_error("Map element must be an object");
      
      auto& map_obj = value_map.as_object();
      std::string map_id = ExtractString(map_obj, "id");
      std::string map_name = ExtractString(map_obj, "name");
       
      model::Map::Id id_map{map_id};
      model::Map map(id_map, map_name);
      try {
	LoadRoadsFromJson(map_obj, map);
	LoadBuildingsFromJson(map_obj, map);
	LoadOfficesFromJson(map_obj, map);
	LoadLootsFromJson(map_obj, map, game.GetLootItems(), id_map);
	try{
	  map.SetDogSpeed(ExtractDouble(map_obj, "dogSpeed"));
	}
	catch(const std::runtime_error&){
	  map.SetDogSpeed(game.GetDogSpeed());
	}

	try{
	  map.SetBagCapacity(static_cast<int>(ExtractInt(map_obj, "bagCapacity")));
	}
	catch(const std::runtime_error&){
	  map.SetBagCapacity(game.GetBagCapacity());
	}

      } catch (const std::runtime_error& e) {
	BOOST_LOG_TRIVIAL(error) << "LoadMapsFromJson: Error in map '" << map_name << "': " << e.what();
	throw std::runtime_error("Error in map '" + map_name + "': " + e.what());
      }
      
      game.AddMap(std::move(map));
    }
  }
  
  model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
      throw std::runtime_error("Cannot open file: " + json_path.string());
    }

    std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    json::value json_value;
    try{
      json_value = json::parse(json_str);
      if (!json_value.is_object())
	throw std::runtime_error("Root JSON element must be an object");
    }
    catch (const boost::system::system_error& e) {
      BOOST_LOG_TRIVIAL(error) << "LoadGame: JSON parse error: " << e.what();
      throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
      BOOST_LOG_TRIVIAL(error) << "LoadGame: Unexpected error: " << e.what();
      throw std::runtime_error(std::string("Unexpected error: ") + e.what());
    }
    auto& obj_maps = json_value.as_object();
    model::Game game;
    try{
      game.SetDogSpeed(ExtractDouble(obj_maps, "defaultDogSpeed"));
    }
    catch(const std::runtime_error&){
      game.SetDogSpeed(1);
    }

    extra::ExtraItems extra;
    
    try{
      const auto& loot_obj = ExtractObject(obj_maps, "lootGeneratorConfig");
      extra.SetPeriod(ExtractDouble(loot_obj, "period"));
      extra.SetProbability(ExtractDouble(loot_obj, "probability"));
    }
    catch (const std::runtime_error& e) {
      BOOST_LOG_TRIVIAL(error) << "LoadGame: Unexpected error: " << e.what();
      throw;
    }

    game.SetLootItems(extra);

    
    try{
      game.SetDogRetirementTime(ExtractDouble(obj_maps, "dogRetirementTime"));
    }
    catch(const std::runtime_error&){
    }
    
    try{
      game.SetBagCapacity(static_cast<int>(ExtractInt(obj_maps, "defaultBagCapacity")));
    }
    catch(const std::runtime_error&){
      game.SetBagCapacityDefault();
    }
    
    LoadMapsFromJson(obj_maps, game);
    return game;
  }
}
