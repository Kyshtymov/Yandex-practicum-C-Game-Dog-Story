#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <deque>
#include <iostream>
#include <random>
#include "tagged.h"
#include "collision_detector.h"
#include "extra_objects.h"
#include "loot_generator.h"
#include "boost_log.h"

namespace model {
  using Dimension = int;
  using Coord = Dimension;
  
  struct Point {
    Coord x, y;
  };

  struct Velocity {
    double vx, vy;
  };

  struct Position {
    double x, y;
  };
  
  struct Size {
    Dimension width, height;
  };

  struct Rectangle {
    Point position;
    Size size;
  };

  struct Offset {
    Dimension dx, dy;
  };

  enum class Direction{
    NORTH,
    SOUTH,
    WEST,
    EAST
  };

  inline std::string GetDirection(const Direction dir){
    switch(dir){
    case Direction::NORTH:
      return "U";
    case Direction::SOUTH:
      return "D";
    case Direction::WEST:
      return "L";
    case Direction::EAST:
      return "R";
    default:
      return "U";
    }
  }

  
  class Road {
    struct HorizontalTag {
      explicit HorizontalTag() = default;
    };

    struct VerticalTag {
      explicit VerticalTag() = default;
    };

  public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    static constexpr double ROAD_WIDTH = 0.8; 
    
    Road(HorizontalTag, Point start, Coord end_x) noexcept
      : start_{start}
      , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
      : start_{start}
      , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;

  private:
    Point start_;
    Point end_;
  };

  class Building {
  public:
    explicit Building(Rectangle bounds) noexcept
      : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept;

  private:
    Rectangle bounds_;
  };

  class Office {
  public:
    using Id = util::Tagged<std::string, Office>;
    constexpr static double DEFAULT_OFFICE_WIDTH = 0.5;

    Office(Id id, Point position, Offset offset) noexcept
      : id_{std::move(id)}
      , position_{position}
      , offset_{offset} {
    }

    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;
    double GetWidth() const{
      return office_width;
    }

  private:
    Id id_;
    Point position_;
    Offset offset_;
    double office_width = DEFAULT_OFFICE_WIDTH;
  };


  class RoadIndex {
  public:
    using CoordKey = int64_t;
    
    void AddRoad(Road& road);
    const std::pair<Road*, Road*> FindRoadAt(Coord x, Coord y) const;
    Road* FindNearestRoad(const Position& pos) const;

  private:

    static CoordKey MakeHorizontalKey(Coord x, Coord y) {
      return (static_cast<int64_t>(y) << 32) | static_cast<int64_t>(x);
    }
    
    static CoordKey MakeVerticalKey(Coord x, Coord y) {
      return (static_cast<int64_t>(x) << 32) | static_cast<int64_t>(y);
    }
    std::vector<Road*> all_roads_;
    std::unordered_map<CoordKey, Road*> horizontal_roads_;
    std::unordered_map<CoordKey, Road*> vertical_roads_;
  };

  
  using MapId = util::Tagged<std::string, Map>;
  class Map {
  public:
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using Id = MapId; 
    Map(Id id, std::string name) noexcept
      : id_(std::move(id))
      , name_(std::move(name)) {
    }
    
    const Id& GetId() const;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(const Office& office);
    void SetDogSpeed(double dog_speed);
    double GetDogSpeed() const;
    void InitRoadIndex();
    std::pair<Road*, Road*> FindRoadAt(Coord x, Coord y);
    Road* FindNearestRoad(const Position& pos) const;
    void SetBagCapacity(int capacity);
    int GetBagCapacity() const;
    
  private:    
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;    
    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    Offices offices_;
    OfficeIdToIndex warehouse_id_to_index_;
    double dog_speed_;
    RoadIndex road_index_;
    int bag_capacity_;
  };

  struct BagContent{
    int id;
    int type;
    int value;
  };
  
  struct ItemsOnRoads{
    int type;
    int id;
    Position pos;
    int value;
  };

  class Dog {    
  public:
    Dog(std::string name, unsigned int id, Point pos, int bag_capacity) : name_(std::move(name)), id_{id}, bag_capacity_(bag_capacity){
      InitDog(pos);
    }
    
    constexpr static double DEFAULT_DOG_WIDTH = 0.6;
    const unsigned int GetId() const noexcept;
    const std::string& GetName() const;
    Velocity GetVelocity() const;
    Position GetPosition() const;
    Direction GetDirection() const;
    void SetVelocity(Velocity vel);
    void SetPosition(Position pos);
    void SetDirection(Direction dir);
    const std::vector<model::BagContent>& GetBag() const;
    bool AddItem(model::BagContent item);
    void IncreaseScore(int value);
    int GetScore() const;
    void AddScore(int score);
    void CleanBag();
    double GetDogWidth() const;
    int GetBagCapacity() const;
    void SetBagCapacity(int bag_capacity);
    void IncreaseTime(double time);
    double GetActiveTime() const;
    double GetStayTime() const;
    void Retire();
    bool IsDogRetired() const;
    bool IsDogStay() const;
    void SetActiveTime(double time);
    void SetStayTime(double time);
    void SetRetireCond(bool retired);
    void SetStayCond(bool stay);
    
  private:
    void InitDog(Point pos);
    std::string name_;
    unsigned int id_;
    Velocity vel_;
    Position pos_;
    Direction dir_;
    double dog_width_ = DEFAULT_DOG_WIDTH;
    std::vector<model::BagContent> bag_;
    int score_ = 0;
    int bag_capacity_;
    double active_time_ = 0;
    double stay_time_ = 0;
    bool retired_ = false;
    bool dog_stay_ = true;
  };


  
  class GameSession {
  public:
    GameSession(model::Map* map, const extra::ExtraItems& extra, double dog_time) : map_(map), loot_gen_{std::chrono::milliseconds(static_cast<int>(extra.GetPeriod() * 1000)), extra.GetProbability()}, items_number_(extra.GetNumberOfItems(map->GetId())), map_values_(extra.GetTypeValues(GetMapId())), dog_retirement_time_(dog_time){
      std::random_device rd;
      gen_ = std::mt19937(rd());
    }

    constexpr static double DEFAULT_ITEM_WIDTH = 0;
    Dog& AddDog(const Dog& dog);
    const Map::Id& GetMapId() const;
    const std::deque<Dog>& GetDogs() const;
    std::deque<Dog>& GetDogs();
    model::Map* GetMap() const;
    model::Dog* MakeDog(std::string name, bool random_spawn);
    void UpdateDogsState(double time_delta, collision_detector::Provider& provider);
    void UpdateItemsState(double time_delta, collision_detector::Provider& provider);
    const std::vector<ItemsOnRoads>& GetItems() const;
    std::vector<ItemsOnRoads>& GetItems();
    int GetDogsNumber() const;
    int GetItemsNumber() const;
    const std::map<int, int>& GetMapValues() const;
    std::map<int, int>& GetMapValues();
    int GetItemsId() const;
    void SetDogsNumber(int dogs_number);
    void SetItemsNumber(int items_number);
    void SetItemsId(int items_id);
    void SetDogRetirementTime(double time);
    double GetDogRetirementTime() const;
    std::vector<Dog*> GetRetiredDogs();
    void ClearRetiredDogs();
    void RemoveDog(Dog* dog);
    
  private:
    int GetRandomInt(int min, int max);
    loot_gen::LootGenerator loot_gen_;
    std::deque<Dog> dogs_;
    std::mt19937 gen_;
    model::Map* map_;
    unsigned int dogs_number_ = 0;
    std::vector<ItemsOnRoads> item_on_roads_;
    int items_number_;
    double item_width_ = DEFAULT_ITEM_WIDTH;
    int items_id_ = 0;
    std::map<int, int> map_values_;
    double dog_retirement_time_;
    std::vector<Dog*> retired_dogs_;
  };
  
  class Game {
  public:
    using Maps = std::vector<Map>;
    constexpr static int DEFAULT_BAG_CAPACITY = 3;
    constexpr static double DEFAULT_DOG_RETIREMENT_TIME = 60;
    void AddMap(Map&& map);
    const Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;
    Map* FindMap(const Map::Id& id) noexcept;
    GameSession* FindSession(const std::string& dog_name, const std::string& map_id_string);
    void MakeGameSession(model::Map* map);
    GameSession* GetSession(model::Map* map);
    GameSession* GetOrCreateSession(model::Map* map);
    void SetDogSpeed(double dog_speed);
    double GetDogSpeed() const;
    void UpdateGameState(double time_delta);
    void SetLootItems(extra::ExtraItems extra);
    const extra::ExtraItems& GetLootItems() const;
    extra::ExtraItems& GetLootItems();
    void SetBagCapacity(int capacity);
    int GetBagCapacity() const;
    void SetBagCapacityDefault();
    void ClearMaps();
    const std::deque<GameSession>& GetSessions() const;
    void SetDogRetirementTime(double time);
    double GetDogRetirementTime() const;
    std::vector<GameSession*> GetRetiredDogsSessions();
    void ClearRetiredDogsSessions();
    
  private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    double dog_speed_;
    int bag_capacity_;
    std::vector<Map> maps_;
    std::deque<GameSession> sessions_;
    MapIdToIndex map_id_to_index_;
    extra::ExtraItems extra_;
    double dog_retirement_time_ = DEFAULT_DOG_RETIREMENT_TIME;
    std::vector<GameSession*> retired_dogs_sessions_;
  };

}  // namespace model
