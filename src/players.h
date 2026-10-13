#pragma once
#include <random>
#include <sstream>
#include <iomanip>
#include <string>
#include <functional> 
#include <set> 
#include <algorithm>
#include <optional>
#include "model.h"
#include <iostream>
#include <deque>

namespace detail {
  struct TokenTag {};
}

using Token = util::Tagged<std::string, detail::TokenTag>;

namespace api {

  class Player;  

  bool IsValidToken(const std::string& token_str);
  
  class PlayerTokens {
    using TokenHasher = util::TaggedHasher<Token>;
    using TokenToPlayer = std::unordered_map<Token, Player*, TokenHasher>;
    using PlayerToToken = std::unordered_map<Player*, Token>;

  private:
    std::random_device random_device_;
    std::mt19937_64 generator1_;
    std::mt19937_64 generator2_;
    TokenToPlayer tokens_;
    PlayerToToken players_;

  public:
    PlayerTokens() : 
      generator1_(random_device_()), 
      generator2_(random_device_()) {
    }

    std::string MakeHexString();
    Token AddPlayer(Player* player);
    Player* FindPlayerByToken(const Token& token);
    std::optional<Token> FindTokenByPlayer(const Player* player);
    const Player* FindPlayerByToken(const Token& token) const;
    std::optional<Token> FindTokenByPlayer(const Player* player) const;
    void AddPlayerForRestore(Player* player, const Token& token);
    void Clear();
    void RemoveToken(Player* player);
  };
  
  class Player {
  public:
    Player(model::Dog* dog, model::GameSession* session, unsigned int player_id) 
      : dog_(dog), session_(session), id_(player_id) {
      name_ = dog->GetName();
    }
    
    const unsigned int GetDogId() const;
    const std::string& GetDogName() const;
    const model::Map::Id& GetMapId() const;
    unsigned int GetId() const;
    const std::deque<model::Dog>& GetDogsInSession() const;
    model::Dog* GetDog() const;
    model::GameSession* GetSession() const;
    std::string GetName() const;
    
  private:
    std::string name_;
    model::Dog* dog_;
    model::GameSession* session_;
    unsigned int id_;
  };
  
  class Players {
    using TokenHasher = util::TaggedHasher<Token>;

  public:
    Player& Add(const std::string dog_name, model::GameSession* session, bool random_spawn);
    std::optional<Token> GetTokenByPlayer(const Player* player);
    bool IsNameTakenOnMap(const std::string& name, const model::Map::Id& map_id);
    Player* FindByDogNameAndMapId(const std::string& dog_name, const model::Map::Id& map_id);
    Player* FindByDogIdAndMapId(const unsigned int dog_id, const model::Map::Id& map_id);
    Player* FindByToken(const Token& token);
    const std::deque<Player>& GetPlayers() const;
    unsigned int GetPlayerNumber() const;
    void AddForRestore(model::Dog* dog, model::GameSession* session, unsigned int id);
    PlayerTokens& GetPlayerTokens();
    const PlayerTokens& GetPlayerTokens() const;
    void Clear();
    void RemovePlayer(model::Dog* dog);
    
  private:
    std::deque<Player> players_deq_;
    unsigned int player_number_ = 0;
    PlayerTokens player_tokens_;
  };
}  // namespace api
