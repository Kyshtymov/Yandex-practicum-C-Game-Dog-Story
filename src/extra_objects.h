#pragma once
#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <map>

namespace util {
  template <typename Tag, typename T>
  class Tagged;
}

namespace model {
  class Map;  
  using MapId = util::Tagged<std::string, Map>;
}

namespace extra{
  
  struct Item{
    std::optional<std::string> name;
    std::optional<std::string> file;
    std::optional<std::string> type;
    std::optional<int> rotation;
    std::optional<std::string> color;
    std::optional<double> scale;
    std::optional<int> value;
    int id;
  };

    
  class ExtraItems{
  public:
    ExtraItems() = default;
    const std::deque<Item>& GetItems(model::MapId id_map) const;
    double GetPeriod() const;
    void SetPeriod(double period);
    void SetProbability(double probability);
    void SetItem(model::MapId id_map, std::deque<Item> map_item);
    void SetItem(model::MapId id_map, Item map_item);
    double GetProbability() const;
    int GetNumberOfItems(model::MapId id_map) const;
    std::map<int, int> GetTypeValues(model::MapId map_id) const;
    std::map<model::MapId, std::deque<Item>> GetItems();
    const std::map<model::MapId, std::deque<Item>> GetItems() const;
    
  private:
    std::map<model::MapId, std::deque<Item>> items_;
    double period_;
    double probability_;
  };
}
