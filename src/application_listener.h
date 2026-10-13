#pragma once
#include <chrono>
#include <pqxx/zview>
#include <pqxx/pqxx>
#include <pqxx/connection>
#include <pqxx/transaction>
#include "connection_pool.h"
#include "tagged_uuid.h"
#include "model.h"
#include "players.h"
#include "boost_log.h"

namespace app_handler {
  class ApplicationHandler;
}

using milliseconds = std::chrono::milliseconds;

class ApplicationListener {
public:
  explicit ApplicationListener(milliseconds save_period) : save_period_(save_period) {}
  virtual ~ApplicationListener() = default;
    
  virtual void OnTick(milliseconds delta, app_handler::ApplicationHandler* handler) = 0;
  virtual void Save(app_handler::ApplicationHandler* handler) = 0;
  virtual void Restore(app_handler::ApplicationHandler* handler) = 0;

protected:
  milliseconds time_since_save_{0};
  milliseconds save_period_;
};

class Application {
public:
  Application();
  void SetAppHandler(app_handler::ApplicationHandler* handler);
  void SetApplicationListener(ApplicationListener* listener);
  void InsertScore(model::Dog* dog);
  void Tick(milliseconds delta);
  void Restore();
  app_handler::ApplicationHandler* GetAppHandler();
  void Save();
  std::vector<std::tuple<std::string, int, double>> GetRecords(int start, int maxItems);

private:
  bool IsAppHandlerExist() const;    
  ApplicationListener* listener_ = nullptr;
  app_handler::ApplicationHandler* app_handler_ = nullptr;
  ConnectionPool conn_pool_;
};
