#pragma once
#include <chrono>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "application_listener.h"
#include "model_serialization.h"
#include "players_serialization.h"
#include "application_handler.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class SerializationListener : public ApplicationListener {
public:
  SerializationListener(milliseconds save_period, std::string path, app_handler::ApplicationHandler* handler);
  void OnTick(milliseconds delta, app_handler::ApplicationHandler* handler) override;
  void Save(app_handler::ApplicationHandler* handler) override;
  void Restore(app_handler::ApplicationHandler* handler) override;
  ~SerializationListener();
  
private:
  bool AtomicSave(const std::string& filename, const model::Game& game, const api::Players& players);
  bool RestoreFromFile(const std::string& filepath, app_handler::ApplicationHandler* handler);
  
  bool already_restored = false;
  std::string path_;
  app_handler::ApplicationHandler* app_handler_;
};
