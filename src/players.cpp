#include "players.h"
#include "model.h"

namespace api {

  bool IsValidToken(const std::string& token_str) {
    if (token_str.length() != 32) {
      return false;
    }
    
    for (char c : token_str) {
      if (!((c >= '0' && c <= '9') || 
	    (c >= 'a' && c <= 'f') || 
	    (c >= 'A' && c <= 'F'))) {
	return false;
      }
    }
    return true;
  }
  
  std::string PlayerTokens::MakeHexString() {
    const int MAX_ATTEMPTS = 1000;
  
    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
      std::stringstream ss;
      ss << std::hex << std::setfill('0') 
	 << std::setw(16) << generator1_() 
	 << std::setw(16) << generator2_();
    
      std::string token_str = ss.str();
    
      std::transform(token_str.begin(), token_str.end(), token_str.begin(),
		     [](unsigned char c) { return std::tolower(c); });
    
      Token token{token_str};
    
      if (tokens_.find(token) == tokens_.end()) {
	return token_str;
      }
    }
  
    throw std::runtime_error("Can't generate unique token");
  }
  void PlayerTokens::AddPlayerForRestore(Player* player, const Token& token) {
    tokens_[token] = player;
    players_[player] = token;
  }
    
  void PlayerTokens::Clear() {
    tokens_.clear();
    players_.clear();
  }

  void PlayerTokens::RemoveToken(Player* player){
    auto token = FindTokenByPlayer(player);

    if (!token.has_value())
      throw std::runtime_error("Can't find the token of the retired dog player");

    players_.erase(player);
    tokens_.erase(token.value());
  }

  
  Token PlayerTokens::AddPlayer(Player* player) {
    std::string token_str = MakeHexString();
    Token token{token_str};
    tokens_[token] = player;
    
    players_.emplace(player, token);
    return token;
  };
  
  Player* PlayerTokens::FindPlayerByToken(const Token& token) {
    auto it = tokens_.find(token);
    if (it != tokens_.end())
      return it->second;
    else
      return nullptr;
  }

  
  const Player* PlayerTokens::FindPlayerByToken(const Token& token) const {
    auto it = tokens_.find(token);
    if (it != tokens_.end())
      return it->second;
    else
      return nullptr;
  }


  std::optional<Token> PlayerTokens::FindTokenByPlayer(const Player* player) const {
    auto it = players_.find(const_cast<Player*>(player));
    if (it != players_.end()) {
      return it->second;
    }
    return std::nullopt;
  }
  
  std::optional<Token> PlayerTokens::FindTokenByPlayer(const Player* player) {
    auto it = players_.find(const_cast<Player*>(player));
    if (it != players_.end())
      return it->second;
    else
      return std::nullopt;
  }
  const unsigned int Player::GetDogId() const {
    return dog_->GetId();
  }
    
  const std::string& Player::GetDogName() const {
    return dog_->GetName();
  }

  const model::Map::Id& Player::GetMapId() const {
    return session_->GetMapId();
  }

  unsigned int Player::GetId() const {
    return id_;
  }

  const std::deque<model::Dog>& Player::GetDogsInSession() const {
    return session_->GetDogs();
  }

  model::Dog* Player::GetDog() const {
    return dog_;
  }

  model::GameSession* Player::GetSession() const {
    return session_;
  }

  std::string Player::GetName() const{
    return name_;
  }
  
  Player& Players::Add(const std::string dog_name, model::GameSession* session, bool random_spawn) {
    model::Dog* dog_to_player = session->MakeDog(dog_name, random_spawn);
    players_deq_.emplace_back(dog_to_player, session, player_number_++);
    player_tokens_.AddPlayer(&players_deq_.back());
    return players_deq_.back();
  }

  std::optional<Token> Players::GetTokenByPlayer(const Player* player) {
    return player_tokens_.FindTokenByPlayer(player);
  }

    
  bool Players::IsNameTakenOnMap(const std::string& name, const model::Map::Id& map_id) {
    auto player_iter = std::find_if(players_deq_.begin(), players_deq_.end(), [&name, &map_id](const Player& player) {
      return player.GetName() == name && player.GetMapId() == map_id;
    });
    
    return player_iter != players_deq_.end();
  }
  
  Player* Players::FindByDogNameAndMapId(const std::string& dog_name, const model::Map::Id& map_id) {
    auto player_iter = std::find_if(players_deq_.begin(), players_deq_.end(), [&dog_name, &map_id](const Player& player) {
      return player.GetDogName() == dog_name && player.GetMapId() == map_id;
    });
    
    if (player_iter != players_deq_.end())
      return &(*player_iter);

    return nullptr;
  }

  Player* Players::FindByDogIdAndMapId(const unsigned int dog_id, const model::Map::Id& map_id) { 
    auto player_iter = std::find_if(players_deq_.begin(), players_deq_.end(), [dog_id, &map_id](const Player& player) {
      return player.GetDogId() == dog_id && player.GetMapId() == map_id;
    });
    
    if (player_iter != players_deq_.end())
      return &(*player_iter);
    
    return nullptr;
  }

  Player* Players::FindByToken(const Token& token) {
    return player_tokens_.FindPlayerByToken(token);
  }

  
  const std::deque<Player>& Players::GetPlayers() const{
    return players_deq_;
  }

  unsigned int Players::GetPlayerNumber() const{
    return player_number_;
  }
  void Players::AddForRestore(model::Dog* dog, model::GameSession* session, unsigned int id) {
    players_deq_.emplace_back(dog, session, id);
    if (id >= player_number_) {
      player_number_ = id + 1;
    }
  }
  PlayerTokens& Players::GetPlayerTokens() {
    return player_tokens_;
  }
  const PlayerTokens& Players::GetPlayerTokens() const { return player_tokens_; }
    
    
  void Players::Clear() {
    players_deq_.clear();
    player_number_ = 0;
    player_tokens_.Clear();
  }

  void Players::RemovePlayer(model::Dog* dog){
    auto player_iter = std::find_if(players_deq_.begin(), players_deq_.end(), [dog](const Player& player) { 
      return player.GetDogId() == dog->GetId();
    });
  
    if (player_iter == players_deq_.end())
      throw std::runtime_error("Can't find the retired player of the retired dog");

    player_tokens_.RemoveToken(&(*player_iter)); 
    players_deq_.erase(player_iter);
  }



}  // namespace api
