#include "model_serialization.h"

namespace serialization {
  
  PointRepr::PointRepr() = default;
  PointRepr::PointRepr(model::Point p) : x(p.x)
				       , y(p.y) {}
  model::Point PointRepr::Restore() const {
    return {x, y};
  }
    

    
  PositionRepr::PositionRepr() = default;
  PositionRepr::PositionRepr(model::Position p) : x(p.x)
						, y(p.y) {}
  model::Position PositionRepr::Restore() const {
    return {x, y};
  }
  
  
  VelocityRepr::VelocityRepr() = default;
  VelocityRepr::VelocityRepr(model::Velocity v) : vx(v.vx)
						, vy(v.vy) {}
  model::Velocity VelocityRepr::Restore() const {
    return {vx, vy};
  }
    

  OffsetRepr::OffsetRepr() = default;
  OffsetRepr::OffsetRepr(model::Offset o) : dx(o.dx)
					  , dy(o.dy) {}
  model::Offset OffsetRepr::Restore() const {
    return {dx, dy};
  } 
    
    
  SizeRepr::SizeRepr() = default;
  SizeRepr::SizeRepr(model::Size s) : width(s.width)
				    , height(s.height) {}
  model::Size SizeRepr::Restore() const {
    return {width, height};
  }
    

  RectangleRepr::RectangleRepr() = default;
  RectangleRepr::RectangleRepr(model::Rectangle r) : position(r.position)
						   , size(r.size) {}
  model::Rectangle RectangleRepr::Restore() const {
    return {position.Restore(), size.Restore()};
  }
    
  BagContentRepr::BagContentRepr() = default;
  BagContentRepr::BagContentRepr(const model::BagContent& bc) : id(bc.id)
							      , type(bc.type)
							      , value(bc.value) {}
  model::BagContent BagContentRepr::Restore() const {
    return {id, type, value};
  }
    
  ItemsOnRoadsRepr::ItemsOnRoadsRepr() = default;
  ItemsOnRoadsRepr::ItemsOnRoadsRepr(const model::ItemsOnRoads& item) : type(item.type)
								      , id(item.id)
								      , pos(item.pos)
								      , value(item.value) {}
    
  model::ItemsOnRoads ItemsOnRoadsRepr::Restore() const { 
    return {type, id, pos.Restore(), value}; 
  }
    

  RoadRepr::RoadRepr() = default;
    
  RoadRepr::RoadRepr(const model::Road& road) {
    start_ = road.GetStart();
    end_ = road.GetEnd();
  }
    
  model::Road RoadRepr::Restore() const {
    if (start_.y == end_.y) {
      return model::Road(model::Road::HORIZONTAL, start_, end_.x);
    } else {
      return model::Road(model::Road::VERTICAL, start_, end_.y);
    }
  }
    
    
  BuildingRepr::BuildingRepr() = default;
    
  BuildingRepr::BuildingRepr(const model::Building& building) 
    : bounds_(building.GetBounds()) {}
    
  model::Building BuildingRepr::Restore() const {
    return model::Building(bounds_);
  }
    
  OfficeRepr::OfficeRepr() = default;
  OfficeRepr::OfficeRepr(const model::Office& office): id_(*office.GetId())
						     , position_(office.GetPosition())
						     , offset_(office.GetOffset()) {}
    
  model::Office OfficeRepr::Restore() const {
    return model::Office(model::Office::Id(id_), position_, offset_);
  }
    
  MapRepr::MapRepr() = default;
    
  MapRepr::MapRepr(const model::Map& map): id_(*map.GetId())
					 , name_(map.GetName())
					 , dog_speed_(map.GetDogSpeed())
					 , bag_capacity_(map.GetBagCapacity()) {
    for (const auto& road : map.GetRoads()) {
      roads_.emplace_back(road);
    }
        
    for (const auto& building : map.GetBuildings()) {
      buildings_.emplace_back(building);
    }
        
    for (const auto& office : map.GetOffices()) {
      offices_.emplace_back(office);
    }
  }
    
  model::Map MapRepr::Restore() const {
    model::Map map(model::Map::Id{id_}, name_);
    for (const auto& road_repr : roads_) {
      map.AddRoad(road_repr.Restore());
    }
        
    for (const auto& building_repr : buildings_) {
      map.AddBuilding(building_repr.Restore());
    }
        
    for (const auto& office_repr : offices_) {
      map.AddOffice(office_repr.Restore());
    }
        
    map.SetDogSpeed(dog_speed_);
    map.SetBagCapacity(bag_capacity_);
    map.InitRoadIndex();
    return map;
  }
    

  DogRepr::DogRepr() = default;

  DogRepr::DogRepr(const model::Dog& dog): id_(dog.GetId()), name_(dog.GetName()), 
					   pos_(dog.GetPosition()), vel_(dog.GetVelocity()), dir_(static_cast<int>(dog.GetDirection())), 
					   score_(dog.GetScore()), bag_capacity_(dog.GetBagCapacity()), active_time_(dog.GetActiveTime()), 
					   stay_time_(dog.GetStayTime()), retired_(dog.IsDogRetired()), dog_stay_(dog.IsDogStay()) {
    for (const auto& item : dog.GetBag()) {
      bag_.emplace_back(item);
    }
  }

  model::Dog DogRepr::Restore() const {
    model::Dog dog{name_, id_, model::Point{static_cast<model::Coord>(std::round(pos_.x)), static_cast<model::Coord>(std::round(pos_.y))}, bag_capacity_};
      
    dog.SetVelocity(vel_.Restore());
    dog.SetDirection(static_cast<model::Direction>(dir_));
    dog.SetPosition(pos_.Restore());
    dog.SetActiveTime(active_time_);
    dog.SetStayTime(stay_time_);
    dog.SetRetireCond(retired_);
    dog.SetStayCond(dog_stay_);
    dog.AddScore(score_);
        
    for (const auto& item : bag_) {
      if (!dog.AddItem(item.Restore())) {
	throw std::runtime_error("Failed to restore bag content");
      }
    }
        
    return dog;
  }

    
  GameSessionRepr::GameSessionRepr() = default;

  GameSessionRepr::GameSessionRepr(const model::GameSession& session): map_id_(*session.GetMapId())
								     , dogs_number_(session.GetDogsNumber())
								     , items_number_(session.GetItemsNumber())
								     , items_id_(session.GetItemsId())
								     , dog_retirement_time_(session.GetDogRetirementTime()) {
    const auto& dogs = session.GetDogs();
    int dog_idx = 0;
    for (const auto& dog : dogs) {
      dogs_.emplace_back(dog);
    }
    
    const auto& items = session.GetItems();
    int item_idx = 0;
    for (const auto& item : items) {
      items_.emplace_back(item);
    }
    
    const auto& map_vals = session.GetMapValues();
    for (const auto& [key, value] : map_vals) {
      map_values_[key] = value;
    }
  }

  void GameSessionRepr::Restore(model::GameSession& session) const {
    if (map_id_ != *session.GetMapId()) {
      throw std::runtime_error("Session map_id mismatch");
    }
        
    session.SetDogsNumber(dogs_number_);
    session.SetItemsNumber(items_number_);
    session.SetItemsId(items_id_);
    session.SetDogRetirementTime(dog_retirement_time_);
        
    auto& dogs = const_cast<std::deque<model::Dog>&>(session.GetDogs());
    dogs.clear();
    for (const auto& dog_repr : dogs_) {
      dogs.push_back(dog_repr.Restore());
    }
        
    auto& items = session.GetItems();
    items.clear();
    for (const auto& item_repr : items_) {
      items.push_back(item_repr.Restore());
    }
        
    auto& map_vals = session.GetMapValues();
    map_vals.clear();
    for (const auto& [key, value] : map_values_) {
      map_vals[key] = value;
    }
  }
    
  const std::string& GameSessionRepr::GetMapId() const {
    return map_id_;
  }

  GameRepr::GameRepr() = default;

  GameRepr::GameRepr(const model::Game& game): dog_speed_(game.GetDogSpeed())
					     , bag_capacity_(game.GetBagCapacity())
					     , extra_(game.GetLootItems()) {
    const auto& maps = game.GetMaps();

    for (const auto& map : maps) {
      maps_.emplace_back(map);
    }

    const auto& sessions = game.GetSessions();

    for (const auto& session : sessions) {
      sessions_.emplace_back(session);
    }
  }

  void GameRepr::Restore(model::Game& game) const {
    game.SetDogSpeed(dog_speed_);
    game.SetBagCapacity(bag_capacity_);
    game.SetLootItems(extra_);
    game.ClearMaps();

    for (const auto& map_repr : maps_) {
      model::Map map = map_repr.Restore();
      game.AddMap(std::move(map));
    }
      
     
    auto& sessions = const_cast<std::deque<model::GameSession>&>(game.GetSessions());
    sessions.clear();
      
    for (const auto& session_repr : sessions_) {
      model::Map* map = game.FindMap(model::Map::Id(session_repr.GetMapId()));
      if (!map) {
	throw std::runtime_error("Map not found for session");
      }
        
      sessions.emplace_back(map, game.GetLootItems(), game.GetDogRetirementTime());
      session_repr.Restore(sessions.back());
    }
  }    
} // namespace serialization

