#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <map>
#include "model.h"  
#include "extra_objects.h"


namespace extra{
    
  const std::deque<Item>& ExtraItems::GetItems(model::MapId id_map) const{
    auto it = items_.find(id_map);
    if (it != items_.end()) {
      return it->second;
    }
    static const std::deque<Item> empty;
    return empty;
  }

  double ExtraItems::GetPeriod() const{
    return period_;
  }

  void ExtraItems::SetPeriod(double period){
    period_ = period;
  }
    
  void ExtraItems::SetProbability(double probability){
    probability_ = probability;
  }

  void ExtraItems::SetItem(model::MapId id_map, std::deque<Item> map_item){
    items_[id_map] = map_item;
  }

  void ExtraItems::SetItem(model::MapId id_map, Item map_item){
    items_[id_map].push_back(map_item);
  }
    
  double ExtraItems::GetProbability() const{
    return probability_;
  }

  int ExtraItems::GetNumberOfItems(model::MapId id_map) const{
    auto it = items_.find(id_map);
    if (it != items_.end()) {
      return it->second.size();
    }
    return 0;
  }

  std::map<int, int> ExtraItems::GetTypeValues(model::MapId map_id) const{
    std::map<int, int> values;
    auto it = items_.find(map_id);
    if (it != items_.end()) {
      for (int i = 0; i < it->second.size(); i++){
	if (it->second[i].value.has_value())
	  values[i] = it->second[i].value.value();
      }
    }
    return values;
  }
    
  std::map<model::MapId, std::deque<Item>> ExtraItems::GetItems(){
    return items_;
  }
    
  const std::map<model::MapId, std::deque<Item>> ExtraItems::GetItems() const{
    return items_;
  }    
}
