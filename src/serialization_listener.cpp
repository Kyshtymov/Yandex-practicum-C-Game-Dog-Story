#include "serialization_listener.h"


SerializationListener::SerializationListener(milliseconds save_period, std::string path, app_handler::ApplicationHandler* handler): ApplicationListener(save_period), path_(std::move(path)), app_handler_(handler) {
  if (!path_.empty()) {
    Restore(handler);
  }
}
  
void SerializationListener::OnTick(milliseconds delta, app_handler::ApplicationHandler* handler) {
  time_since_save_ += delta;
  if (time_since_save_ < save_period_)
    return;
      
  time_since_save_ = milliseconds(0);
  Save(handler);
}
  
void SerializationListener::Save(app_handler::ApplicationHandler* handler) {
  if (path_.empty()) return;
  AtomicSave(path_, *handler->GetGame(), *handler->GetPlayers());
}

  
void SerializationListener::Restore(app_handler::ApplicationHandler* handler){
  if (path_.empty()) return;

  if (RestoreFromFile(path_, handler)) {
    return;
  }
    
  throw std::runtime_error("Failed to restore the data");
}

  
SerializationListener::~SerializationListener() {
  if (!path_.empty() && app_handler_) {
    Save(app_handler_);
  }
}
  
  
bool SerializationListener::AtomicSave(const std::string& filename, const model::Game& game, const api::Players& players) {
  fs::path temp_path = fs::path(filename).string() + ".tmp";
  {
    std::ofstream ofs(temp_path);
    if (!ofs) {
      BOOST_LOG_TRIVIAL(error) << "Cannot create temporary file: " << temp_path;
      return false;
    }
      
    boost::archive::text_oarchive oa(ofs);
    serialization::GameRepr game_repr(game);
    serialization::PlayersRepr players_repr(players);
	
    oa << game_repr << players_repr;
    ofs.flush();
  }
    
  fs::rename(temp_path, filename);  
  return true;
    
}

bool SerializationListener::RestoreFromFile(const std::string& filepath, app_handler::ApplicationHandler* handler) {
  if (already_restored) {
    return true;
  }
  try {
    std::ifstream ifs(filepath);
    if (!ifs)
      return true;
      
    boost::archive::text_iarchive ia(ifs);
       
    serialization::GameRepr game_repr;
    serialization::PlayersRepr players_repr;
      
    ia >> game_repr >> players_repr;
      
    model::Game* game = const_cast<model::Game*>(handler->GetGame());
    api::Players* players = const_cast<api::Players*>(handler->GetPlayers());
      
    if (game && players) {
      game_repr.Restore(*game);
      players_repr.Restore(*players, *game);
      already_restored = true;
      return true;
    }
  } catch (const std::exception& e) {
    BOOST_LOG_TRIVIAL(error) << "Failed to restore from " << filepath << ": " << e.what();
  }
  return false;
}
