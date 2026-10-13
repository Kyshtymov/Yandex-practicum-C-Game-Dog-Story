#include <cmath>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <vector>
#include <map>
#include <set>
#include <random>

#include "../src/loot_generator.h"
#include "../src/model.h"
#include "../src/extra_objects.h"
#include "../src/json_loader.h"

using namespace std::literals;
namespace json = boost::json;

SCENARIO("Loot generation") {
  using loot_gen::LootGenerator;
  using TimeInterval = LootGenerator::TimeInterval;

  GIVEN("a loot generator") {
    LootGenerator gen{1s, 1.0};

    constexpr TimeInterval TIME_INTERVAL = 1s;

    WHEN("loot count is enough for every looter") {
      THEN("no loot is generated") {
        for (unsigned looters = 0; looters < 10; ++looters) {
          for (unsigned loot = looters; loot < looters + 10; ++loot) {
            INFO("loot count: " << loot << ", looters: " << looters);
            REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == 0);
          }
        }
      }
    }

    WHEN("number of looters exceeds loot count") {
      THEN("number of loot is proportional to loot difference") {
        for (unsigned loot = 0; loot < 10; ++loot) {
          for (unsigned looters = loot; looters < loot + 10; ++looters) {
            INFO("loot count: " << loot << ", looters: " << looters);
            REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == looters - loot);
          }
        }
      }
    }
  }

  GIVEN("a loot generator with some probability") {
    constexpr TimeInterval BASE_INTERVAL = 1s;
    LootGenerator gen{BASE_INTERVAL, 0.5};

    WHEN("time is greater than base interval") {
      THEN("number of generated loot is increased") {
        CHECK(gen.Generate(BASE_INTERVAL * 2, 0, 4) == 3);
      }
    }

    WHEN("time is less than base interval") {
      THEN("number of generated loot is decreased") {
        const auto time_interval
          = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
              1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
        CHECK(gen.Generate(time_interval, 0, 4) == 1);
      }
    }
  }

  GIVEN("a loot generator with custom random generator") {
    LootGenerator gen{1s, 0.5, [] {
      return 0.5;
    }};
    WHEN("loot is generated") {
      THEN("number of loot is proportional to random generated values") {
        const auto time_interval
	  = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
	      1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
        CHECK(gen.Generate(time_interval, 0, 4) == 0);
        CHECK(gen.Generate(time_interval, 0, 4) == 1);
      }
    }
  }
}


SCENARIO("JSON loader - loot types fields") {
  GIVEN("a JSON with map containing loot types with missing fields") {
    std::string json_str = R"({
            "defaultDogSpeed": 1.0,
            "lootGeneratorConfig": {
                "period": 1.0,
                "probability": 0.5
            },
            "maps": [
                {
                    "id": "map1",
                    "name": "Map 1",
                    "lootTypes": [
                        {
                            "name": "key",
                            "file": "assets/key.obj",
                            "type": "obj",
                            "rotation": 90,
                            "color": "#338844",
                            "scale": 0.03
                        },
                        {
                            "name": "wallet",
                            "file": "assets/wallet.obj",
                            "type": "obj",
                            "rotation": 0,
                            "color": "#883344",
                            "scale": 0.01
                        }
                    ],
                    "roads": [{"x0": 0, "y0": 0, "x1": 10}],
                    "buildings": [],
                    "offices": []
                },
                {
                    "id": "town",
                    "name": "Town",
                    "lootTypes": [
                        {
                            "name": "key",
                            "file": "assets/key.obj",
                            "type": "obj",
                            "scale": 0.03
                        },
                        {
                            "name": "wallet",
                            "file": "assets/wallet.obj",
                            "type": "obj",
                            "scale": 0.01
                        }
                    ],
                    "roads": [{"x0": 0, "y0": 0, "x1": 10}],
                    "buildings": [],
                    "offices": []
                }
            ]
        })";
        
    std::string temp_file = "temp_test_loot_fields.json";
    {
      std::ofstream f(temp_file);
      f << json_str;
    }
        
    WHEN("loading the game") {
      model::Game game = json_loader::LoadGame(std::filesystem::path(temp_file));
      collision_detector::Provider provider;
      THEN("map1 has all loot fields with values") {
	auto& loot_items = game.GetLootItems();
	model::MapId map1_id("map1");
	const auto& items = loot_items.GetItems(map1_id);
                
	REQUIRE(items.size() == 2);
                
	const auto& item1 = items[0];
	CHECK(item1.name.value() == "key");
	CHECK(item1.file.value() == "assets/key.obj");
	CHECK(item1.type.value() == "obj");
	CHECK(item1.color.value() == "#338844");
	CHECK(item1.rotation.value() == 90);
	CHECK(item1.scale.value() == 0.03);
                
	const auto& item2 = items[1];
	CHECK(item2.name.value() == "wallet");
	CHECK(item2.file.value() == "assets/wallet.obj");
	CHECK(item2.type.value() == "obj");
	CHECK(item2.color.value() == "#883344");
	CHECK(item2.rotation.value() == 0);
	CHECK(item2.scale.value() == 0.01);
      }
            
      THEN("town has loot fields with defaults for missing fields") {
	auto& loot_items = game.GetLootItems();
	model::MapId town_id("town");
	const auto& items = loot_items.GetItems(town_id);
                
	REQUIRE(items.size() == 2);
                
	const auto& item1 = items[0];
	CHECK(item1.name.value() == "key");
	CHECK(item1.file.value() == "assets/key.obj");
	CHECK(item1.type.value() == "obj");

	CHECK_FALSE(item1.color.has_value());  
	CHECK_FALSE(item1.rotation.has_value());
	CHECK(item1.scale.value() == 0.03);
                
	const auto& item2 = items[1];
	CHECK(item2.name.value() == "wallet");
	CHECK(item2.file.value() == "assets/wallet.obj");
	CHECK(item2.type.value() == "obj");
	CHECK_FALSE(item2.color.has_value());  
	CHECK_FALSE(item2.rotation.has_value());
	CHECK(item2.scale.value() == 0.01);
      }
    }
        
    std::filesystem::remove(temp_file);
  }
}


SCENARIO("Loot generation - type range validation") {
  using namespace model;
  collision_detector::Provider provider;
    
  GIVEN("a game session with 2 loot types") {
    MapId map_id("test_map");
    Map map(map_id, "Test Map");
    map.AddRoad(Road(Road::HORIZONTAL, Point{0, 0}, 10));
    map.InitRoadIndex();
        
    extra::ExtraItems extra;
    extra.SetPeriod(1.0);
    extra.SetProbability(1.0);
        
    std::deque<extra::Item> items;
    for (int i = 0; i < 2; ++i) {
      extra::Item item;
      item.name = "item" + std::to_string(i);
      item.id = i;
      items.push_back(item);
    }
    extra.SetItem(map_id, items);
        
    GameSession session(&map, extra);
    session.MakeDog("Dog1", true);
    session.MakeDog("Dog2", true);
        
    WHEN("generating many loot items") {
      std::set<int> seen_types;
      const int ITERATIONS = 100;
            
      for (int i = 0; i < ITERATIONS; ++i) {
	session.UpdateItemsState(1.0, provider);
	const auto& loot_items = session.GetItems();
                
	for (const auto& item : loot_items) {
	  CHECK(item.type >= 0);
	  CHECK(item.type < 2);
	  seen_types.insert(item.type);
	}
      }
            
      THEN("only types 0 and 1 are generated") {
	CHECK(seen_types.count(0) >= 0);
	CHECK(seen_types.count(1) >= 0);
	CHECK(seen_types.count(2) == 0);
      }
    }
  }
    
  GIVEN("a game session with 5 loot types") {
    MapId map_id("test_map");
    Map map(map_id, "Test Map");
    map.AddRoad(Road(Road::HORIZONTAL, Point{0, 0}, 10));
    map.InitRoadIndex();
        
    extra::ExtraItems extra;
    extra.SetPeriod(1.0);
    extra.SetProbability(1.0);
        
    std::deque<extra::Item> items;
    for (int i = 0; i < 5; ++i) {
      extra::Item item;
      item.name = "item" + std::to_string(i);
      item.id = i;
      items.push_back(item);
    }
    extra.SetItem(map_id, items);
        
    GameSession session(&map, extra);
    session.MakeDog("Dog1", true);
        
    WHEN("generating loot") {
      session.UpdateItemsState(1.0, provider);
      const auto& items = session.GetItems();
            
      THEN("all generated types are between 0 and 4") {
	for (const auto& item : items) {
	  CHECK(item.type >= 0);
	  CHECK(item.type < 5);
	}
      }
    }
  }
}


SCENARIO("Loot generation - respect looter count") {
  using namespace model;
  collision_detector::Provider provider;
    
  GIVEN("a game session with 2 dogs") {
    MapId map_id("test_map");
    Map map(map_id, "Test Map");
    map.AddRoad(Road(Road::HORIZONTAL, Point{0, 0}, 10));
    map.InitRoadIndex();
        
    extra::ExtraItems extra;
    extra.SetPeriod(1.0);
    extra.SetProbability(1.0);
        
    std::deque<extra::Item> items;
    extra::Item item;
    item.name = "key";
    item.id = 0;
    items.push_back(item);
    extra.SetItem(map_id, items);
        
    GameSession session(&map, extra);
    session.MakeDog("Dog1", true);
    session.MakeDog("Dog2", true);
        
    WHEN("generating loot") {
      session.UpdateItemsState(1.0, provider);
      session.UpdateItemsState(1.0, provider);
      session.UpdateItemsState(1.0, provider);
            
      THEN("loot count never exceeds dog count") {
	CHECK(session.GetItems().size() <= 2);
      }
    }
  }
    
  GIVEN("a game session with 1 dog") {
    MapId map_id("test_map");
    Map map(map_id, "Test Map");
    map.AddRoad(Road(Road::HORIZONTAL, Point{0, 0}, 10));
    map.InitRoadIndex();
        
    extra::ExtraItems extra;
    extra.SetPeriod(1.0);
    extra.SetProbability(1.0);
        
    std::deque<extra::Item> items;
    extra::Item item;
    item.name = "key";
    item.id = 0;
    items.push_back(item);
    extra.SetItem(map_id, items);
        
    GameSession session(&map, extra);
    session.MakeDog("Dog1", true);
        
    WHEN("generating loot") {
      session.UpdateItemsState(1.0, provider);
      session.UpdateItemsState(1.0, provider);
      session.UpdateItemsState(1.0, provider);
            
      THEN("loot count never exceeds 1") {
	CHECK(session.GetItems().size() <= 1);
      }
    }
  }
}


SCENARIO("Loot placement - on roads only") {
  using namespace model;
  collision_detector::Provider provider;
    
  GIVEN("a map with horizontal road from (0,0) to (10,0)") {
    MapId map_id("test_map");
    Map map(map_id, "Test Map");
    map.AddRoad(Road(Road::HORIZONTAL, Point{0, 0}, 10));
    map.InitRoadIndex();
        
    extra::ExtraItems extra;
    extra.SetPeriod(1.0);
    extra.SetProbability(1.0);
        
    std::deque<extra::Item> items;
    extra::Item item;
    item.name = "key";
    item.id = 0;
    items.push_back(item);
    extra.SetItem(map_id, items);
        
    GameSession session(&map, extra);
    session.MakeDog("Dog1", true);
        
    WHEN("loot is generated") {
      session.UpdateItemsState(1.0, provider);
      const auto& loot_items = session.GetItems();
            
      THEN("all loot is placed on the road") {
	for (const auto& loot : loot_items) {
	  auto roads = map.FindRoadAt(static_cast<Coord>(std::round(loot.pos.x)), static_cast<Coord>(std::round(loot.pos.y)));
	  CHECK(roads.first != nullptr); 
	  CHECK(roads.second == nullptr);
	  CHECK(loot.pos.y == 0.0);
	  CHECK(loot.pos.x >= 0.0);
	  CHECK(loot.pos.x <= 10.0);
	}
      }
    }
  }
    
  GIVEN("a map with vertical road from (0,0) to (0,10)") {
    MapId map_id("test_map");
    Map map(map_id, "Test Map");
    map.AddRoad(Road(Road::VERTICAL, Point{0, 0}, 10));
    map.InitRoadIndex();
        
    extra::ExtraItems extra;
    extra.SetPeriod(1.0);
    extra.SetProbability(1.0);
        
    std::deque<extra::Item> items;
    extra::Item item;
    item.name = "key";
    item.id = 0;
    items.push_back(item);
    extra.SetItem(map_id, items);
        
    GameSession session(&map, extra);
    session.MakeDog("Dog1", true);
        
    WHEN("loot is generated") {
      session.UpdateItemsState(1.0, provider);
      const auto& loot_items = session.GetItems();
            
      THEN("all loot is placed on the road") {
	for (const auto& loot : loot_items) {
	  auto roads = map.FindRoadAt(static_cast<Coord>(std::round(loot.pos.x)), static_cast<Coord>(std::round(loot.pos.y)));
	  CHECK(roads.first == nullptr);
	  CHECK(roads.second != nullptr); 
	  CHECK(loot.pos.x == 0.0); 
	  CHECK(loot.pos.y >= 0.0);
	  CHECK(loot.pos.y <= 10.0);
	}
      }
    }
  }
}


SCENARIO("Loot generation - edge cases") {
  using namespace model;
  collision_detector::Provider provider;
    
  GIVEN("a game session with zero dogs") {
    MapId map_id("test_map");
    Map map(map_id, "Test Map");
    map.AddRoad(Road(Road::HORIZONTAL, Point{0, 0}, 10));
    map.InitRoadIndex();
        
    extra::ExtraItems extra;
    extra.SetPeriod(1.0);
    extra.SetProbability(1.0);
        
    std::deque<extra::Item> items;
    extra::Item item;
    item.name = "key";
    item.id = 0;
    items.push_back(item);
    extra.SetItem(map_id, items);
        
    GameSession session(&map, extra);
        
    WHEN("no dogs are added") {
      session.UpdateItemsState(1.0, provider);
            
      THEN("no loot is generated") {
	CHECK(session.GetItems().size() == 0);
      }
    }
  }    
}
