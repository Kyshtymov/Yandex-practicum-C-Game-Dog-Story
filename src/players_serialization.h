#pragma once
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include "players.h"
#include "model_serialization.h"

namespace serialization {

  using TokenSerializer = serialization::TaggedSerializer<detail::TokenTag>;

  class PlayerRepr {
  public:
    PlayerRepr();
    explicit PlayerRepr(const api::Player& player);
    api::Player Restore(model::Dog* dog, model::GameSession* session) const;
    unsigned int GetId() const;
    unsigned int GetDogId() const;
    const std::string& GetMapId() const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & id_ & name_ & dog_id_ & map_id_;
    }

  private:
    unsigned int id_;
    std::string name_;
    unsigned int dog_id_;
    std::string map_id_;
  };

  class PlayerTokensRepr {
  public:
    using TokenToPlayerMap = std::vector<std::pair<TokenSerializer, unsigned int>>; 
    using PlayerToTokenMap = std::vector<std::pair<unsigned int, TokenSerializer>>; 

    PlayerTokensRepr();

    explicit PlayerTokensRepr(const api::PlayerTokens& tokens, 
                              const std::deque<api::Player>& players);
    void Restore(api::PlayerTokens& tokens, const std::deque<api::Player>& players) const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & token_to_player_ & player_to_token_;
    }
    
  private:
    TokenToPlayerMap token_to_player_;
    PlayerToTokenMap player_to_token_;
  };

  class PlayersRepr {
  public:
    PlayersRepr();

    explicit PlayersRepr(const api::Players& players);
    void Restore(api::Players& players, model::Game& game) const;
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
      ar & player_number_ & players_ & tokens_repr_;
    }


  private:
    unsigned int player_number_ = 0;
    std::vector<PlayerRepr> players_;
    PlayerTokensRepr tokens_repr_;
  };

} // namespace serialization
