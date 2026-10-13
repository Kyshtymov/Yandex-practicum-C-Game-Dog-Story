#include "model.h"
#include "boost_log.h"
#include "tagged.h"
#include <cmath>
#include <iostream>
#include <random>

namespace model {
  using namespace std::literals;

  bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
  }

  bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
  }

  Point Road::GetStart() const noexcept {
    return start_;
  }

  Point Road::GetEnd() const noexcept {
    return end_;
  }
  
  const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
  }

  const Office::Id& Office::GetId() const noexcept {
    return id_;
  }

  Point Office::GetPosition() const noexcept {
    return position_;
  }

  Offset Office::GetOffset() const noexcept {
    return offset_;
  }

  const Map::Id& Map::GetId() const{
    return id_;
  }

  const std::string& Map::GetName() const noexcept {
    return name_;
  }

  const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
  }

  const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
  }

  const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
  }

  void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
  }

  void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
  }

  void Map::AddOffice(const Office& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
      throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(office);
    try {
      warehouse_id_to_index_.emplace(o.GetId(), index);
    }
    catch (const std::bad_alloc& e) {
      BOOST_LOG_TRIVIAL(error) << "AddOffice: Memory allocation failed: " << e.what();
      offices_.pop_back();
      throw;
    } catch (const std::exception& e) {
      BOOST_LOG_TRIVIAL(error) << "AddOffice: Failed to insert warehouse: " << e.what();
      offices_.pop_back();
      throw;
    }
  }

  void Map::SetDogSpeed(double dog_speed){
    dog_speed_ = dog_speed;
  }

  double Map::GetDogSpeed() const{
    
    return dog_speed_;
  }

  void Map::InitRoadIndex(){
    for (auto& road:roads_){
      road_index_.AddRoad(road);
    }
  }
  double Game::GetDogSpeed() const {
    return dog_speed_;
  }

  void Game::SetDogSpeed(double dog_speed) {
    dog_speed_ = dog_speed;
  }
  
  std::pair<Road*, Road*> Map::FindRoadAt(Coord x, Coord y){
    std::pair<Road*, Road*> result = road_index_.FindRoadAt(x, y);
    return result;
  }

  
  const unsigned int Dog::GetId() const noexcept {
    return id_;
  }

  const std::string& Dog::GetName() const {
    return name_;
  }

  
  void Dog::IncreaseTime(double time){
    if (!retired_){
      active_time_ += time;
      if (dog_stay_)
	stay_time_ += time;
    }
  }
    
  double Dog::GetActiveTime() const{
    return active_time_;
  }
    
  double Dog::GetStayTime() const{
    return stay_time_;
  }

  void Dog::Retire(){
    retired_ = true;
  }

  bool Dog::IsDogRetired() const{
    return retired_;
  }
  bool Dog::IsDogStay() const {
    return dog_stay_;
  }
  void Dog::SetActiveTime(double time) {
    active_time_ = time;
  }
  
  void Dog::SetStayTime(double time) {
    stay_time_ = time;
  }
    
  void Dog::SetRetireCond(bool retired){
    retired_ = retired;
  }
  void Dog::SetStayCond(bool stay){
    dog_stay_ = stay;
  }

  
  void Dog::InitDog(Point pos){
    pos_.x = pos.x;
    pos_.y = pos.y;
    dir_ = Direction::NORTH;
    vel_ = {0, 0};
  }
    
  Velocity Dog::GetVelocity() const{
    return vel_;
  }
  Position Dog::GetPosition() const{
    return pos_;
  }
  Direction Dog::GetDirection() const{
    return dir_;
  }
  void Dog::SetVelocity(Velocity vel){
    vel_ = vel;
    
    if (vel_.vx == 0.0 && vel_.vy == 0.0) {
      dog_stay_ = true;
    }
    else{
      stay_time_ = 0.0;
      dog_stay_ = false;
    }
  }
  
  void Dog::SetPosition(Position pos){
    pos_ = pos;
  }
  void Dog::SetDirection(Direction dir){
    dir_ = dir;
  }
     
  Dog& GameSession::AddDog(const Dog& dog) {
    dogs_.push_back(dog);
    return dogs_.back();
  }
    
 
  const Map::Id& GameSession::GetMapId() const {
    if (!map_) {
      throw std::runtime_error("GameSession::GetMapId() - map_ is nullptr");
    }
    return map_->GetId();
  }

  const std::deque<Dog>& GameSession::GetDogs() const {
    return dogs_;
  }
    
  std::deque<Dog>& GameSession::GetDogs() {
    return dogs_;
  }
    
  model::Map* GameSession::GetMap() const {
    return map_;
  }

  int GameSession::GetRandomInt(int min, int max) {    
    std::uniform_int_distribution<int> dist(min, max);  
    return dist(gen_);
  }

  void GameSession::SetDogRetirementTime(double time){
    dog_retirement_time_ = time;
  }

  double GameSession::GetDogRetirementTime() const{
    return dog_retirement_time_;
  }

  std::vector<Dog*> GameSession::GetRetiredDogs(){
    return retired_dogs_;
  }

  void GameSession::ClearRetiredDogs(){
    retired_dogs_.clear();
  }
  void GameSession::RemoveDog(Dog* dog){
    auto dog_iter = std::find_if(dogs_.begin(), dogs_.end(), [dog](const model::Dog& dog_session) {  
      return dog_session.GetId() == dog->GetId();
    });
  
    if (dog_iter == dogs_.end()) 
      throw std::runtime_error("Can't find the retired dog");    
  
    dogs_.erase(dog_iter);
  }

  
  model::Dog* GameSession::MakeDog(std::string name, bool random_spawn){
    const auto& roads = map_->GetRoads();
    if (roads.empty())
      throw std::runtime_error("Can't find the road");

    Point dog_pos = roads[0].GetStart();

    if (random_spawn){
      
      int random_road = GetRandomInt(0, static_cast<int>(roads.size()) - 1);
    
      int road_left = std::min(roads[random_road].GetStart().x, roads[random_road].GetEnd().x);
      int road_right = std::max(roads[random_road].GetStart().x, roads[random_road].GetEnd().x);
      
      int road_up = std::min(roads[random_road].GetStart().y, roads[random_road].GetEnd().y);
      int road_down = std::max(roads[random_road].GetStart().y, roads[random_road].GetEnd().y);
      
      dog_pos.x = GetRandomInt(road_left, road_right);
      dog_pos.y = GetRandomInt(road_up, road_down);

    } else {
      dog_pos = roads[0].GetStart();
    }
    
    dogs_.emplace_back(name, dogs_number_++, dog_pos, GetMap()->GetBagCapacity());
    return &dogs_.back();
  }
      
  const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
  }

  const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
      return &maps_.at(it->second);
    }
    return nullptr;
  }

  Map* Game::FindMap(const Map::Id& id) noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
      return &maps_.at(it->second);
    }
    return nullptr;
  }

  GameSession* Game::FindSession(const std::string& dog_name, const std::string& map_id_string){
    Map::Id map_id_to_find{map_id_string};

    auto session_it = std::find_if(sessions_.begin(), sessions_.end(), [&map_id_to_find, &dog_name](const auto& session) {
      if (session.GetMapId() != map_id_to_find)
	return false;
      
      const auto& dogs = session.GetDogs();
      
      auto dog_iter = std::find_if(dogs.begin(), dogs.end(), [&dog_name](const auto& dog) {
	return dog.GetName() == dog_name;
      });
      return dog_iter != dogs.end();
    });
    
    return (session_it != sessions_.end()) ? &(*session_it) : nullptr;
  }

  void Game::MakeGameSession(model::Map* map) {
    if (!map) {
      throw std::runtime_error("MakeGameSession: map is nullptr");
    }
    
    sessions_.emplace_back(map, extra_, GetDogRetirementTime());
  }    

  GameSession* Game::GetOrCreateSession(model::Map* map) {
    auto session_it = std::find_if(sessions_.begin(), sessions_.end(), [map](const auto& session) {
      return session.GetMapId() == map->GetId();
    });

    if (session_it != sessions_.end())
      return &(*session_it);

    MakeGameSession(map);
    return &sessions_.back();
  }

  void Game::AddMap(Map&& map) {
    auto map_id = map.GetId();
    
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map_id, index); !inserted) {
      throw std::invalid_argument("Map with id "s + *map_id + " already exists"s);
    }
  
    try {
      map.InitRoadIndex();
      maps_.emplace_back(std::move(map));
    }
    catch (const std::exception& e) {
      map_id_to_index_.erase(map_id);
      BOOST_LOG_TRIVIAL(error) << "AddMap failed: " << e.what();
      throw;
    }
  }  
  
  double DistanceCalculation(const Position& pos, double min, double max, double road_pos, bool horizontal){

    double pos_first;
    double pos_second;
    if (horizontal){
      pos_first = pos.x;
      pos_second = pos.y;
    }
    else{
      pos_first = pos.y;
      pos_second = pos.x;
    }
    
    if (pos_first >= min && pos_first <= max) 
      return std::abs(pos_second - road_pos);
    
    double dx1 = pos_first - min;
    double dy1 = pos_second - road_pos;
    double dx2 = pos_first - max;
    double dy2 = pos_second - road_pos;
    
    return std::min(std::sqrt(dx1*dx1 + dy1*dy1), std::sqrt(dx2*dx2 + dy2*dy2));
  }
  
  double CalculateDistanceToRoad(const Position& pos, const Road& road) {

    if (road.IsHorizontal()) {
      double road_y = road.GetStart().y;
      double min_x = std::min(road.GetStart().x, road.GetEnd().x);
      double max_x = std::max(road.GetStart().x, road.GetEnd().x);
      return DistanceCalculation(pos, min_x, max_x, road_y, true);
    }
    
    double road_x = road.GetStart().x;
    double min_y = std::min(road.GetStart().y, road.GetEnd().y);
    double max_y = std::max(road.GetStart().y, road.GetEnd().y);
    return DistanceCalculation(pos, min_y, max_y, road_x, false);
  }

  
  Road* RoadIndex::FindNearestRoad(const Position& pos) const {
    Coord x_int = static_cast<Coord>(std::round(pos.x));
    Coord y_int = static_cast<Coord>(std::round(pos.y));

    double min_distance = std::numeric_limits<double>::max();
    Road* nearest_road = nullptr;

    for (Road* road : all_roads_) {
      double distance = CalculateDistanceToRoad(pos, *road);
      if (distance < min_distance) {
	min_distance = distance;
	nearest_road = road;
      }
    }
    return nearest_road;
  }


  Road* Map::FindNearestRoad(const Position& pos) const {
    return road_index_.FindNearestRoad(pos);
  }

  
  void CheckBoundaries(const model::Road* road, bool check_x_axis, bool velocity_along_road, Position& new_pos, Velocity& vel) {
    
    auto get_coord = [check_x_axis](const Point& pos) -> double {
      return check_x_axis ? pos.x : pos.y;
    };
    
    auto set_coord = [check_x_axis](Position& pos) -> double& {
      return check_x_axis ? static_cast<double&>(pos.x) : static_cast<double&>(pos.y);
    };
    
    auto set_vel = [check_x_axis](Velocity& v) -> double& {
      return check_x_axis ? static_cast<double&>(v.vx) : static_cast<double&>(v.vy);
    };
    
    double road_start = get_coord(road->GetStart());
    double road_end = get_coord(road->GetEnd());
    
    double road_left = std::min(road_start, road_end);
    double road_right = std::max(road_start, road_end);
    
    double left_boundary = road_left;
    double right_boundary = road_right;
    
    if (!velocity_along_road) {
      left_boundary -= Road::ROAD_WIDTH / 2;
      right_boundary += Road::ROAD_WIDTH / 2;
    }
    
    double& pos_coord = set_coord(new_pos);
    double& vel_coord = set_vel(vel);
    
    if (pos_coord < left_boundary) {
      pos_coord = left_boundary;
      vel_coord = 0.0;
    }
    else if (pos_coord > right_boundary) {
      pos_coord = right_boundary;
      vel_coord = 0.0;
    }
  }

  double Dog::GetDogWidth() const{
    return dog_width_;
  }

  void GameSession::UpdateDogsState(double time_delta, collision_detector::Provider& provider) {
    
    for (auto& dog : dogs_) {
      if (dog.IsDogRetired()){
      	continue;
      }
      
      Velocity vel = dog.GetVelocity();
      Position pos = dog.GetPosition();
      
      dog.IncreaseTime(time_delta);

      double stay_time = dog.GetStayTime();
      double retirement_time = GetDogRetirementTime();
      double active_time = dog.GetActiveTime();
        
        
      if (stay_time >= retirement_time){
	dog.Retire();
	retired_dogs_.push_back(&dog);
	continue;
      }

      
      if (vel.vx == 0.0 && vel.vy == 0.0) {
	continue;
      }
      
      Position new_pos = pos;
      new_pos.x += vel.vx * time_delta;
      new_pos.y += vel.vy * time_delta;

      Coord x_pos = static_cast<int>(std::round(pos.x));
      Coord y_pos = static_cast<int>(std::round(pos.y));
      std::pair<Road*, Road*> roads = map_->FindRoadAt(x_pos, y_pos);
      Road* road_hor = roads.first;
      Road* road_ver = roads.second;
      if (road_hor == nullptr && road_ver == nullptr){
	Road* road_nearest = map_->FindNearestRoad(new_pos);

	if (road_nearest == nullptr){
	  dog.SetVelocity(vel);
	  return;
	}
	  
	if (road_nearest->IsHorizontal())
	  road_hor = road_nearest;
	else
	  road_ver = road_nearest;
      }
	      
      if (vel.vx != 0){
	if (road_hor != nullptr) {
	  CheckBoundaries(road_hor, true, true, new_pos, vel);
	}
	else{
	  if (road_ver != nullptr) {
	    CheckBoundaries(road_ver, true, false, new_pos, vel);
	  }
	}
      }
      if (vel.vy != 0){
	if (road_ver != nullptr) {
	  CheckBoundaries(road_ver, false, true, new_pos, vel);
	}
	else{
	  if (road_hor != nullptr) {
	    CheckBoundaries(road_hor, false, false, new_pos, vel);
	  }
	}
      }

      provider.AddGatherer({pos.x, pos.y}, {new_pos.x, new_pos.y}, dog.GetDogWidth());

      dog.SetPosition(new_pos);
      dog.SetVelocity(vel);
    }
  }

  void GameSession::UpdateItemsState(double time_delta, collision_detector::Provider& provider){
    int new_loot = loot_gen_.Generate(std::chrono::milliseconds(static_cast<int>(time_delta)), item_on_roads_.size(), dogs_.size());
    
    if (new_loot == 0)
      return;
      
    const auto& roads = map_->GetRoads();
    if (roads.empty())
      throw std::runtime_error("Can't find the road");
      
    for (int i = 0; i < new_loot; i++){
      int random_type = GetRandomInt(0, items_number_ - 1);
      int random_road = GetRandomInt(0, static_cast<int>(roads.size()) - 1);
    
      int road_left = std::min(roads[random_road].GetStart().x, roads[random_road].GetEnd().x);
      int road_right = std::max(roads[random_road].GetStart().x, roads[random_road].GetEnd().x);
      
      int road_up = std::min(roads[random_road].GetStart().y, roads[random_road].GetEnd().y);
      int road_down = std::max(roads[random_road].GetStart().y, roads[random_road].GetEnd().y);

      ItemsOnRoads item;
      item.pos.x = GetRandomInt(road_left, road_right);
      item.pos.y = GetRandomInt(road_up, road_down);
      item.type = random_type;
      item.id = items_id_++;
      try{
	item.value = map_values_.at(random_type);
      }
      catch (const std::out_of_range&) {
	item.value = 0;
      }
      item_on_roads_.push_back(item);
    }
    
    for (auto& item:item_on_roads_){
      geom::Point2D pos2D = {item.pos.x, item.pos.y};
      provider.AddItem(pos2D, item_width_);
    }
  }

  
  const std::vector<ItemsOnRoads>& GameSession::GetItems() const{
    return item_on_roads_;
  }
  std::vector<ItemsOnRoads>& GameSession::GetItems() {
    return item_on_roads_;
  }

  void Dog::IncreaseScore(int value){
    score_ += value;
  }
  void Dog::CleanBag(){
    const std::vector<model::BagContent>& bag = GetBag();
    for (const auto& item:bag){
      IncreaseScore(item.value);
    }
    bag_.clear();
  }
  
  int Dog::GetScore() const{
    return score_;
  }
  
  void Dog::AddScore(int score){
    score_ = score;
  }
  void UpdateDogBags(std::vector<collision_detector::GatheringEvent> col_events, model::GameSession& session){
    std::deque<model::Dog>& dogs = session.GetDogs();
    std::vector<model::ItemsOnRoads>& items = session.GetItems();
    std::vector<model::ItemsOnRoads> items_copy = items;
    for (auto& event : col_events) {
      if (event.gatherer_id >= dogs.size()) {
	continue;
      }
      
      model::Dog& dog = dogs[event.gatherer_id];
      
        
      if (event.item_id >= items.size()) {
	dog.CleanBag();
	continue;
      }
      
      model::BagContent item;
      item.id = items_copy[event.item_id].id;
      item.type = items_copy[event.item_id].type;
      item.value = items_copy[event.item_id].value;
	
      if (dog.AddItem(item)) {
	auto it = std::find_if(items.begin(), items.end(), [item_id = item.id](const model::ItemsOnRoads& i) {
	  return i.id == item_id;
	});
	  
	if (it != items.end()) {
	  items.erase(it);
	}
      }
    }
  }
  
  
  void Game::UpdateGameState(double time_delta) {
    for (auto& session : sessions_) {
      if (session.GetDogs().size() == 0)
	continue;
      
      if (!session.GetMap()) {
	continue;
      }

      collision_detector::Provider provider;
      session.UpdateDogsState(time_delta, provider);
      session.UpdateItemsState(time_delta, provider);

      std::vector<Dog*> retired_dogs = session.GetRetiredDogs();

      if (retired_dogs.size() > 0){
      	retired_dogs_sessions_.push_back(&session);
      }
      
      const std::vector<model::Office>& vect_office = session.GetMap()->GetOffices();
      for (const auto& office:vect_office){
	model::Point pos = office.GetPosition();
	geom::Point2D office_pos;
	office_pos.x = static_cast<double>(pos.x);
	office_pos.y = static_cast<double>(pos.y);
	provider.AddItem(office_pos, office.GetWidth());
      }
      
      std::vector<collision_detector::GatheringEvent> col_events = FindGatherEvents(provider);
      UpdateDogBags(col_events, session);
    }
  }
    
  void RoadIndex::AddRoad(Road& road) {
    if (road.IsHorizontal()) {
      Coord y = road.GetStart().y;
      Coord start_x = std::min(road.GetStart().x, road.GetEnd().x);
      Coord end_x = std::max(road.GetStart().x, road.GetEnd().x);
            
      for (Coord x = start_x; x <= end_x; ++x) {
	CoordKey key = MakeHorizontalKey(x, y);
	horizontal_roads_[key] = &road;
      }
    }
    else {
      Coord x = road.GetStart().x;
      Coord start_y = std::min(road.GetStart().y, road.GetEnd().y);
      Coord end_y = std::max(road.GetStart().y, road.GetEnd().y);
      
      for (Coord y = start_y; y <= end_y; ++y) {
	CoordKey key = MakeVerticalKey(x, y);
	vertical_roads_[key] = &road;
      }
    }
    all_roads_.push_back(&road);
  }
  
  const std::pair<Road*, Road*> RoadIndex::FindRoadAt(Coord x, Coord y) const {
    std::pair<Road*, Road*> result{nullptr, nullptr};
    
    RoadIndex::CoordKey key_hor = MakeHorizontalKey(x, y);
    auto it_hor = horizontal_roads_.find(key_hor);
    if (it_hor != horizontal_roads_.end()) {
      result.first = it_hor->second; 
    }
    
    RoadIndex::CoordKey key_ver = MakeVerticalKey(x, y);
    auto it_ver = vertical_roads_.find(key_ver);
    if (it_ver != vertical_roads_.end()) {
      result.second = it_ver->second;
    }
    
    return result;
  }


  void Game::SetLootItems(extra::ExtraItems extra){
    extra_ = extra;
  }

  void Game::ClearMaps() {
    maps_.clear();
    map_id_to_index_.clear();
  }
  
  const std::deque<GameSession>& Game::GetSessions() const{
    return sessions_;
  }

  void Game::SetDogRetirementTime(double time){
    dog_retirement_time_ = time;
  }

  double Game::GetDogRetirementTime() const{
    return dog_retirement_time_;
  }
    
  std::vector<GameSession*> Game::GetRetiredDogsSessions(){
    return retired_dogs_sessions_;
  }
    
  void Game::ClearRetiredDogsSessions(){
    retired_dogs_sessions_.clear();
  }
  
  const extra::ExtraItems& Game::GetLootItems() const{
    return extra_;
  }

  extra::ExtraItems& Game::GetLootItems(){
    return extra_;
  }
  
  void Game::SetBagCapacity(int capacity){
    bag_capacity_ = capacity;
  }
  int Game::GetBagCapacity() const{
    return bag_capacity_;
  }
  void Game::SetBagCapacityDefault(){
    SetBagCapacity(DEFAULT_BAG_CAPACITY);
  }

  
  void Map::SetBagCapacity(int capacity){
    bag_capacity_ = capacity;
  }
  
  int Map::GetBagCapacity() const{
    return bag_capacity_;
  }
  
  int Dog::GetBagCapacity() const{
    return bag_capacity_;
  }

  void Dog::SetBagCapacity(int bag_capacity){
    bag_capacity_ = bag_capacity;
  }

  const std::vector<model::BagContent>& Dog::GetBag() const{
    return bag_;
  }
  
  bool Dog::AddItem(model::BagContent item){
    if (bag_.size() < bag_capacity_){
      bag_.push_back(item);
      return true;
    }

    return false;
  }

  int GameSession::GetDogsNumber() const{
    return dogs_number_;
  }
  int GameSession::GetItemsNumber() const{
    return items_number_;
  }

  const std::map<int, int>& GameSession::GetMapValues() const{
    return map_values_;
  }

  std::map<int, int>& GameSession::GetMapValues(){
    return map_values_;
  }

  int GameSession::GetItemsId() const{
    return items_id_;
  }
  void GameSession::SetDogsNumber(int dogs_number){
    dogs_number_ = dogs_number;
  }
  void GameSession::SetItemsNumber(int items_number){
    items_number_ = items_number;
  }
  void GameSession::SetItemsId(int items_id){
    items_id_ = items_id;
  }
  model::GameSession* model::Game::GetSession(model::Map* map) {
    if (!map) return nullptr;
    
    auto& map_id = map->GetId();
    for (auto& session : sessions_) {
      if (session.GetMapId() == map_id) {
	return &session;
      }
    }
    return nullptr;
  }
}  // namespace model
