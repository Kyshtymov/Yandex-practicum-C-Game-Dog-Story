#pragma once
#include <optional>
#include <chrono>
#include <functional>
#include <memory>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include "model.h"
#include "players.h"

namespace app_handler {

  using namespace std::literals;
  namespace net = boost::asio;
  namespace sys = boost::system;

  class Ticker : public std::enable_shared_from_this<Ticker> {
  public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
      : strand_{strand}
      , period_{period}
      , handler_{std::move(handler)} {
    }

    void Start();
    
  private:
    using Clock = std::chrono::steady_clock;
    void ScheduleTick();
    void OnTick(sys::error_code ec);
    Strand strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
  };
  
  class ApplicationHandler {
  public:
    ApplicationHandler(model::Game* game, api::Players* players, std::optional<double> tick_period, std::string static_path, bool random_spawn)
      : game_(game), players_(players), tick_period_(tick_period), static_path_(static_path), random_spawn_(random_spawn) {};

    std::string GetStaticPath() const;
    const std::vector<model::Map>& GetMaps() const;
    const model::Map* FindMap(model::Map::Id id_map) const;
    model::Map* FindMap(model::Map::Id id_map);
    model::GameSession* GetOrCreateSession(model::Map* map) const;
    void UpdateGameState(double time_delta_seconds);
    api::Player* FindByToken(Token token);
    api::Player* FindByDogIdAndMapId(unsigned int dog_id, const model::Map::Id& map_id);
    bool IsNameTakenOnMap(const std::string& name, const model::Map::Id& map_id);
    api::Player& Add(const std::string dog_name, model::GameSession* session);
    std::optional<Token> GetTokenByPlayer(const api::Player* player);
    std::optional<double> GetTickPeriod() const;
    const extra::ExtraItems& GetLootItems() const;
    const std::deque<model::GameSession>& GetSessions() const;
    const model::Game* GetGame() const;
    model::Game* GetGame();
    const api::Players* GetPlayers() const;
    api::Players* GetPlayers();
    std::vector<model::GameSession*> GetRetiredDogsSessions();
    
  private:
    model::Game* game_;
    api::Players* players_;
    std::optional<double> tick_period_;
    std::string static_path_;
    bool random_spawn_;
  };  
}  
