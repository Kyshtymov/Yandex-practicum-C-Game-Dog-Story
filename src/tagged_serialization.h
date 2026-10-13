#pragma once
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include "tagged.h"

namespace boost {
namespace serialization {

template <typename Archive, typename Value, typename Tag>
void serialize(Archive& ar, util::Tagged<Value, Tag>& tagged, 
               [[maybe_unused]] const unsigned int version) {
    Value value = *tagged;
    ar & value;
    if constexpr (Archive::is_loading::value) {
        tagged = util::Tagged<Value, Tag>(std::move(value));
    }
}

} // namespace serialization
} // namespace boost
