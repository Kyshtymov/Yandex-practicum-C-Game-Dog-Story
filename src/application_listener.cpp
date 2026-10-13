#include "application_listener.h"
#include "application_handler.h" 
#include "model.h"
#include "players.h"
#include "boost_log.h"

Application::Application() 
  : conn_pool_(4, [] { 
    const char* db_url = std::getenv("GAME_DB_URL");
    if (!db_url) {
      throw std::runtime_error("DB URL is not specified");
    }
    auto conn = std::make_shared<pqxx::connection>(db_url);
    
    pqxx::work tx{*conn};
    tx.exec0(
	     "CREATE TABLE IF NOT EXISTS retired_players ("
	     "    id UUID PRIMARY KEY,"
	     "    name VARCHAR(100) UNIQUE NOT NULL,"
	     "    score DOUBLE PRECISION NOT NULL,"
	     "    playtime DOUBLE PRECISION NOT NULL"
	     ");"
	     );
    
    tx.exec0(
	     "CREATE INDEX IF NOT EXISTS idx_retired_players_score ON retired_players(score DESC);"
	     );
    tx.exec0(
	     "CREATE INDEX IF NOT EXISTS idx_retired_players_playtime ON retired_players(playtime ASC);"
	     );
    tx.exec0(
	     "CREATE INDEX IF NOT EXISTS idx_retired_players_name ON retired_players(name ASC);"
	     );
    
    tx.commit();
    
    return conn;
  }) {
}

void Application::SetAppHandler(app_handler::ApplicationHandler* handler) {
  app_handler_ = handler;
}

void Application::SetApplicationListener(ApplicationListener* listener) {
  listener_ = listener;
  listener_->Restore(app_handler_);
}

void Application::InsertScore(model::Dog* dog) {
  try {
    struct IdTag {};
    using uuid_Id = util::TaggedUUID<IdTag>;
        
    auto conn = conn_pool_.GetConnection();
        
    uuid_Id dog_id = uuid_Id::New();
        
    pqxx::work tx{*conn};
    tx.exec0(
	     "INSERT INTO retired_players (id, name, score, playtime) VALUES (" +
	     tx.quote(dog_id.ToString()) + ", " +
	     tx.quote(dog->GetName()) + ", " +
	     tx.quote(dog->GetScore()) + ", " +
	     tx.quote(dog->GetActiveTime()) + ")"
	     );
    tx.commit();
        
  } catch (const pqxx::sql_error& e) {
    BOOST_LOG_TRIVIAL(error) << "SQL error inserting retired dog: " << e.what();
  } catch (const std::exception& e) {
    BOOST_LOG_TRIVIAL(error) << "Error inserting retired dog: " << e.what();
  }
}

void Application::Tick(milliseconds delta) {
  double delta_seconds = std::chrono::duration<double>(delta).count();
  app_handler_->UpdateGameState(delta_seconds);

  std::vector<model::GameSession*> retired_dogs_sessions = app_handler_->GetRetiredDogsSessions();

  if (!retired_dogs_sessions.empty()) {
    api::Players* players = app_handler_->GetPlayers();
        
    std::vector<model::Dog*> all_retired_dogs;
    for (auto* session : retired_dogs_sessions) { 
      std::vector<model::Dog*> retired_dogs = session->GetRetiredDogs();
      all_retired_dogs.insert(all_retired_dogs.end(), retired_dogs.begin(), retired_dogs.end());
    }
        
    for (auto* dog : all_retired_dogs) {
      try {
	InsertScore(dog);
	for (auto* session : retired_dogs_sessions) {
	  try {
	    session->RemoveDog(dog);
	  } catch (const std::exception& ex) {
	    BOOST_LOG_TRIVIAL(debug) << "The dog has already been removed: " << ex.what();
	  }
	}
	
	players->RemovePlayer(dog);
        
      } catch (const std::exception& e) {
	BOOST_LOG_TRIVIAL(error) << "Error processing retired dog: " << e.what();
      }
    }

    for (auto* session : retired_dogs_sessions) {
      session->ClearRetiredDogs();
    }
        
    app_handler_->GetGame()->ClearRetiredDogsSessions();
  }
        
  if (listener_) {
    listener_->OnTick(delta, app_handler_);
  }
}
void Application::Restore() {
  if (listener_) {
    listener_->Restore(app_handler_);
  }
}
    
app_handler::ApplicationHandler* Application::GetAppHandler() {   
  return app_handler_;
}

void Application::Save() {
  BOOST_LOG_TRIVIAL(debug) << "SAVE";
  if (listener_ && IsAppHandlerExist()) {
    listener_->Save(app_handler_);
  }
}

bool Application::IsAppHandlerExist() const {
  return (app_handler_ != nullptr);
}


std::vector<std::tuple<std::string, int, double>> Application::GetRecords(int start = 0, int maxItems = 100) {
  std::vector<std::tuple<std::string, int, double>> records;
        
  try {
    auto conn = conn_pool_.GetConnection();
    pqxx::read_transaction tx{*conn};
            
    auto result = tx.exec(
			  "SELECT name, score, playtime FROM retired_players "
			  "ORDER BY score DESC, playtime ASC, name ASC "
			  "LIMIT " + std::to_string(maxItems) + " OFFSET " + std::to_string(start)
			  );
            
    for (const auto& row : result) {
      records.emplace_back(
			   row[0].as<std::string>(), 
			   row[1].as<int>(),         
			   row[2].as<double>()       
			   );
    }
            
  } catch (const std::exception& e) {
    BOOST_LOG_TRIVIAL(error) << "Error getting records: " << e.what();
  }
        
  return records;
}
