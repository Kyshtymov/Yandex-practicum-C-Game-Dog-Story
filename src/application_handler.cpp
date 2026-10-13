#include "application_handler.h"

namespace app_handler {

  void Ticker::Start() {
    last_tick_ = Clock::now();
    net::dispatch(strand_, [self = shared_from_this()] {
      self->ScheduleTick();
    });
  }

  void Ticker::ScheduleTick() {
    assert(strand_.running_in_this_thread());
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
      self->OnTick(ec);
    });
  }

  void Ticker::OnTick(sys::error_code ec) {
    using namespace std::chrono;
    assert(strand_.running_in_this_thread());

    if (!ec) {
      auto this_tick = Clock::now();
      auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
      last_tick_ = this_tick;
      try {
	handler_(delta);
      } catch (...) {
      }
      ScheduleTick();
    }
  }

  std::string ApplicationHandler::GetStaticPath() const {
    return static_path_;
  }

  const std::vector<model::Map>& ApplicationHandler::GetMaps() const {
    return game_->GetMaps();
  }

  const model::Map* ApplicationHandler::FindMap(model::Map::Id id_map) const {
    return game_->FindMap(id_map);
  }

  model::Map* ApplicationHandler::FindMap(model::Map::Id id_map) {
    return game_->FindMap(id_map);
  }

  model::GameSession* ApplicationHandler::GetOrCreateSession(model::Map* map) const {
    return game_->GetOrCreateSession(map);  
  }

  void ApplicationHandler::UpdateGameState(double time_delta_seconds) {
    game_->UpdateGameState(time_delta_seconds);
  }

  api::Player* ApplicationHandler::FindByToken(Token token) {
    return players_->FindByToken(token);
  }

  api::Player* ApplicationHandler::FindByDogIdAndMapId(unsigned int dog_id, const model::Map::Id& map_id) {
    return players_->FindByDogIdAndMapId(dog_id, map_id);
  }

  bool ApplicationHandler::IsNameTakenOnMap(const std::string& name, const model::Map::Id& map_id) {
    return players_->IsNameTakenOnMap(name, map_id); 
  }

  api::Player& ApplicationHandler::Add(const std::string dog_name, model::GameSession* session) {
    return players_->Add(dog_name, session, random_spawn_); 
  }

  std::optional<Token> ApplicationHandler::GetTokenByPlayer(const api::Player* player) {
    return players_->GetTokenByPlayer(player); 
  }

  std::optional<double> ApplicationHandler::GetTickPeriod() const{
    return tick_period_;
  }
  const extra::ExtraItems& ApplicationHandler::GetLootItems() const{
    return game_->GetLootItems();
  }
  const std::deque<model::GameSession>& ApplicationHandler::GetSessions() const{
    return game_->GetSessions();
  }
  const model::Game* ApplicationHandler::GetGame() const{
    return game_;
  }
  model::Game* ApplicationHandler::GetGame(){
    return game_;
  }
  const api::Players* ApplicationHandler::GetPlayers() const{
    return players_;
  }
  api::Players* ApplicationHandler::GetPlayers(){
    return players_;
  }
    
  std::vector<model::GameSession*> ApplicationHandler::GetRetiredDogsSessions(){
    return game_->GetRetiredDogsSessions();
  }
}  
