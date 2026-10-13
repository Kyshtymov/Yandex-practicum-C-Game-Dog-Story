#include "players_serialization.h"

namespace serialization {
  
  PlayerRepr::PlayerRepr() = default;
  PlayerRepr::PlayerRepr(const api::Player& player) : id_(player.GetId()), name_(player.GetName()), dog_id_(player.GetDogId()), map_id_(*player.GetMapId()){
  }

  api::Player PlayerRepr::Restore(model::Dog* dog, model::GameSession* session) const {
    return api::Player(dog, session, id_);
  }
    
  unsigned int PlayerRepr::GetId() const {
    return id_;
  } 
  unsigned int PlayerRepr::GetDogId() const {
    return dog_id_;
  }
  const std::string& PlayerRepr::GetMapId() const {
    return map_id_;
  }
    
  PlayerTokensRepr::PlayerTokensRepr() = default;

  PlayerTokensRepr::PlayerTokensRepr(const api::PlayerTokens& tokens, 
				     const std::deque<api::Player>& players) {
    for (size_t i = 0; i < players.size(); ++i) {
      const auto& player = players[i];
      auto token_opt = tokens.FindTokenByPlayer(&player);
      if (token_opt) {
	token_to_player_.emplace_back(TokenSerializer(*token_opt), i);
	player_to_token_.emplace_back(i, TokenSerializer(*token_opt));
      }
    }
  }
    
  void PlayerTokensRepr::Restore(api::PlayerTokens& tokens, const std::deque<api::Player>& players) const {
      
    for (const auto& [token_ser, player_idx] : token_to_player_) {
      if (player_idx < players.size()) {
	api::Player* player = const_cast<api::Player*>(&players[player_idx]);
	Token token = token_ser.Restore();
	tokens.AddPlayerForRestore(player, token);	  
      }
    }
  }
    

  PlayersRepr::PlayersRepr() = default;

  PlayersRepr::PlayersRepr(const api::Players& players)
    : player_number_(players.GetPlayerNumber()) {
    for (const auto& player : players.GetPlayers()) {
      players_.emplace_back(player); 
    }  
    tokens_repr_ = PlayerTokensRepr(players.GetPlayerTokens(), players.GetPlayers());
  }
    
  void PlayersRepr::Restore(api::Players& players, model::Game& game) const {
    players.Clear();
    for (const auto& player_repr : players_) {
      model::Map* map = game.FindMap(model::Map::Id(player_repr.GetMapId()));
      if (!map) {
	throw std::runtime_error("Map not found for player: " + player_repr.GetMapId());
      }
        
      model::GameSession* session = game.GetSession(map);
      if (!session) {
	throw std::runtime_error("Session not found for map");
      }
        
      model::Dog* dog = nullptr;
      for (auto& session_dog : const_cast<std::deque<model::Dog>&>(session->GetDogs())) {
	if (session_dog.GetId() == player_repr.GetDogId()) {
	  dog = &session_dog;
	  break;
	}
      }
        
      if (!dog) {
	throw std::runtime_error("Dog not found for player");
      }
      players.AddForRestore(dog, session, player_repr.GetId());
    }
      
    tokens_repr_.Restore(players.GetPlayerTokens(), players.GetPlayers());
  }
    
} // namespace serialization
