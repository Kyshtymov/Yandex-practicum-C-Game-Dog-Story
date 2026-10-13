#pragma once
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/optional.hpp>
#include "model.h"

namespace serialization {

  template <typename T>
  struct TaggedSerializer {
    std::string value;
    
    TaggedSerializer() = default;
    explicit TaggedSerializer(const util::Tagged<std::string, T>& tagged) 
      : value(*tagged) {}
    
    util::Tagged<std::string, T> Restore() const {
      return util::Tagged<std::string, T>(value);
    }
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & value;
    }
  };
  
  struct PointRepr {
    model::Coord x, y;
    
    PointRepr();
    explicit PointRepr(model::Point p);
    model::Point Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & x & y;
    }
  };

  struct PositionRepr {
    double x, y;
    
    PositionRepr();
    explicit PositionRepr(model::Position p);
    model::Position Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & x & y;
    }
  };

  struct VelocityRepr {
    double vx, vy;
    
    VelocityRepr();
    explicit VelocityRepr(model::Velocity v);
    model::Velocity Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & vx & vy;
    }
  };

  struct OffsetRepr {
    model::Dimension dx, dy;
    
    OffsetRepr();
    explicit OffsetRepr(model::Offset o);
    model::Offset Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & dx & dy;
    }
  };

  struct SizeRepr {
    model::Dimension width, height;
    
    SizeRepr();
    explicit SizeRepr(model::Size s);
    model::Size Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & width & height;
    }
  };

  struct RectangleRepr {
    PointRepr position;
    SizeRepr size;
    
    RectangleRepr();
    explicit RectangleRepr(model::Rectangle r);
    model::Rectangle Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & position & size;
    }
  };

  struct BagContentRepr {
    int id;
    int type;
    int value;
    
    BagContentRepr();
    explicit BagContentRepr(const model::BagContent& bc);
    model::BagContent Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & id & type & value;
    }
  };

  struct ItemsOnRoadsRepr {
    int type;
    int id;
    PositionRepr pos;
    int value;
    
    ItemsOnRoadsRepr();
    explicit ItemsOnRoadsRepr(const model::ItemsOnRoads& item);
    model::ItemsOnRoads Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & type & id & pos & value;
    }
  };

  class RoadRepr {
  public:
    RoadRepr();
    explicit RoadRepr(const model::Road& road);
    model::Road Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & start_.x & start_.y & end_.x & end_.y;
    }

  private:
    model::Point start_;
    model::Point end_;
  };

  class BuildingRepr {
  public:
    BuildingRepr();
    explicit BuildingRepr(const model::Building& building);
    model::Building Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & bounds_.position.x & bounds_.position.y 
	& bounds_.size.width & bounds_.size.height;
    }

  private:
    model::Rectangle bounds_;
  };

  class OfficeRepr {
  public:
    OfficeRepr();
    explicit OfficeRepr(const model::Office& office);
    model::Office Restore() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & id_ & position_.x & position_.y & offset_.dx & offset_.dy;
    }

  private:
    std::string id_;
    model::Point position_;
    model::Offset offset_;
  };

  class MapRepr {
  public:
    MapRepr();
    
    explicit MapRepr(const model::Map& map);
    model::Map Restore() const;
    const std::string& GetId() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & id_ & name_ & dog_speed_ & bag_capacity_ 
	& roads_ & buildings_ & offices_;
    }


  private:
    std::string id_;
    std::string name_;
    double dog_speed_ = 0.0;
    int bag_capacity_ = 0;
    std::vector<RoadRepr> roads_;
    std::vector<BuildingRepr> buildings_;
    std::vector<OfficeRepr> offices_;
  };

  class DogRepr {
  public:
    DogRepr();

    explicit DogRepr(const model::Dog& dog);
    model::Dog Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & id_ & name_ & pos_ & vel_ & dir_ 
	& score_ & bag_capacity_ & bag_ & active_time_ & stay_time_ & retired_ & dog_stay_;;
    }

  private:
    unsigned int id_;
    std::string name_;
    PositionRepr pos_;
    VelocityRepr vel_;
    int dir_ = 0;
    int score_ = 0;
    int bag_capacity_ = 0;
    std::vector<BagContentRepr> bag_;
    double active_time_ = 0;
    double stay_time_ = 0;
    bool retired_ = false;
    bool dog_stay_ = true;
  };

  class GameSessionRepr {
  public:
    GameSessionRepr();

    explicit GameSessionRepr(const model::GameSession& session);
    void Restore(model::GameSession& session) const;
    const std::string& GetMapId() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & map_id_ & dogs_ & dogs_number_ & items_ 
	& items_number_ & items_id_ & map_values_ & dog_retirement_time_;
    }

  private:
    std::string map_id_;
    std::vector<DogRepr> dogs_;
    unsigned int dogs_number_ = 0;
    std::vector<ItemsOnRoadsRepr> items_;
    int items_number_ = 0;
    int items_id_ = 0;
    std::map<int, int> map_values_;
    double dog_retirement_time_;
  };

  class GameRepr {
  public:
    GameRepr();

    explicit GameRepr(const model::Game& game);
    void Restore(model::Game& game) const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & dog_speed_ & bag_capacity_ & maps_ & sessions_ & extra_;
    }

  private:
    double dog_speed_ = 0.0;
    int bag_capacity_ = 0;
    std::vector<MapRepr> maps_;
    std::vector<GameSessionRepr> sessions_;
    extra::ExtraItems extra_;
  };

} // namespace serialization


namespace extra {

  template <typename T>
  struct OptionalField {
    bool has_value = false;
    T value = T{};
    
    OptionalField() = default;
    explicit OptionalField(const std::optional<T>& opt) {
      if (opt.has_value()) {
	has_value = true;
	value = opt.value();
      }
    }
    
    std::optional<T> ToOptional() const {
      if (has_value) {
	return value;
      }
      return std::nullopt;
    }
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
      ar & has_value;
      if (has_value) {
	ar & value;
      }
    }
  };

  template <typename Archive>
  void serialize(Archive& ar, Item& item, [[maybe_unused]] const unsigned int version) {
    OptionalField<std::string> name(item.name);
    OptionalField<std::string> file(item.file);
    OptionalField<std::string> type(item.type);
    OptionalField<int> rotation(item.rotation);
    OptionalField<std::string> color(item.color);
    OptionalField<double> scale(item.scale);
    OptionalField<int> value(item.value);
    
    ar & name & file & type & rotation & color & scale & value & item.id;
  }

  template <typename Archive>
  void serialize(Archive& ar, ExtraItems& obj, [[maybe_unused]] const unsigned int version) {
    double period = obj.GetPeriod();
    double probability = obj.GetProbability();
    ar & period & probability;
    
    obj.SetPeriod(period);
    obj.SetProbability(probability);
    
    std::map<std::string, std::deque<Item>> items_for_serialization;
    
    const auto& items = obj.GetItems();
    for (const auto& [map_id, item_deque] : items) {
      items_for_serialization[*map_id] = item_deque;
    }
    
    ar & items_for_serialization;
    
    for (const auto& [map_id_str, item_deque] : items_for_serialization) {
      model::MapId map_id(map_id_str);
      obj.SetItem(map_id, item_deque);
    }
    // namespace extra
  }
}
