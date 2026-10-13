#pragma once
#include <boost/json.hpp> 
#include <boost/json/serialize.hpp>
#include <boost/log/trivial.hpp>  
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;


BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value);
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);

 
inline void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
  json::object log_entry;
      
  auto ts = rec[timestamp];
  if (ts) {
    log_entry["timestamp"] = to_iso_extended_string(*ts);
  }
      
  auto data = rec[additional_data];
  if (data) {
    log_entry["data"] = *data;
  }
      
  auto msg = rec[logging::expressions::smessage];
  if (msg) {
    log_entry["message"] = msg.get();
  }
      
  strm << json::serialize(log_entry);
}

inline void InitLogging() {
  logging::add_console_log(std::cout, keywords::format = &JsonFormatter, keywords::auto_flush = true);
}
