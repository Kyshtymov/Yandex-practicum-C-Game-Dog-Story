#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <sstream>
#include <vector>
#include <algorithm>
#include "../src/collision_detector.h"


using namespace Catch::Matchers;

namespace collision_detector {
  inline bool operator<(const GatheringEvent& lhs, const GatheringEvent& rhs) {
    if (lhs.time != rhs.time) return lhs.time < rhs.time;
    if (lhs.gatherer_id != rhs.gatherer_id) return lhs.gatherer_id < rhs.gatherer_id;
    return lhs.item_id < rhs.item_id;
  }
}

namespace Catch {
  template<>
  struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
      std::ostringstream tmp;
      tmp << "(g:" << value.gatherer_id << ", i:" << value.item_id 
	  << ", d:" << value.sq_distance << ", t:" << value.time << ")";
      return tmp.str();
    }
  };
}

template<typename T>
class IsPermutationMatcher : public Catch::Matchers::MatcherBase<std::vector<T>> {
public:
  IsPermutationMatcher(std::vector<T> expected)
    : expected_(std::move(expected)) {
    std::sort(expected_.begin(), expected_.end());
  }
    
  bool match(std::vector<T> const& actual) const override {
    std::vector<T> actual_sorted = actual;
    std::sort(actual_sorted.begin(), actual_sorted.end());
    return actual_sorted == expected_;
  }
    
  std::string describe() const override {
    std::ostringstream ss;
    ss << "is permutation of: [";
    for (size_t i = 0; i < expected_.size(); ++i) {
      if (i > 0) ss << ", ";
      ss << expected_[i];
    }
    ss << "]";
    return ss.str();
  }
    
private:
  std::vector<T> expected_;
};

template<typename T>
IsPermutationMatcher<T> IsPermutation(std::vector<T> expected) {
  return IsPermutationMatcher<T>(std::move(expected));
}

class TestProvider : public collision_detector::ItemGathererProvider {
public:
  void AddGatherer(geom::Point2D start, geom::Point2D end, double width) {
    collision_detector::Gatherer g;
    g.start_pos = start;
    g.end_pos = end;
    g.width = width;
    gatherers_.push_back(g);
  }
    
  void AddItem(geom::Point2D pos, double width) {
    collision_detector::Item i;
    i.position = pos;
    i.width = width;
    items_.push_back(i);
  }
    
  size_t ItemsCount() const override {
    return items_.size();
  }
  collision_detector::Item GetItem(size_t idx) const override {
    return items_[idx];
  }
  size_t GatherersCount() const override {
    return gatherers_.size();
  }
  collision_detector::Gatherer GetGatherer(size_t idx) const override {
    return gatherers_[idx];
  }
    
private:
  std::vector<collision_detector::Gatherer> gatherers_;
  std::vector<collision_detector::Item> items_;
};

geom::Point2D Point(double x, double y) {
  return geom::Point2D{x, y};
}


TEST_CASE("Should use sum of widths, not just gatherer width") {
  TestProvider provider;
    
  // Собиратель шириной 2, предмет шириной 4 - сумма = 6
  provider.AddGatherer(Point(0, 0), Point(10, 0), 2.0);
  provider.AddItem(Point(5, 2.5), 4.0);  // расстояние 2.5 < 3 (полусумма)
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  // Должно быть событие, так как 2.5 < (2+4)/2 = 3
  REQUIRE(events.size() == 1);
}

TEST_CASE("Should use sum of widths, not just item width") {
  TestProvider provider;
    
  // Собиратель шириной 4, предмет шириной 2 - сумма = 6
  provider.AddGatherer(Point(0, 0), Point(10, 0), 4.0);
  provider.AddItem(Point(5, 2.5), 2.0);  // расстояние 2.5 < 3 (полусумма)
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  // Должно быть событие, так как 2.5 < (4+2)/2 = 3
  REQUIRE(events.size() == 1);
}

TEST_CASE("Events must be sorted chronologically") {
  TestProvider provider;
    
  provider.AddGatherer(Point(0, 0), Point(10, 0), 2.0);
    
  // Предметы на разных расстояниях
  provider.AddItem(Point(8, 0), 1.0);  // время 0.8
  provider.AddItem(Point(2, 0), 1.0);  // время 0.2
  provider.AddItem(Point(5, 0), 1.0);  // время 0.5
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  REQUIRE(events.size() == 3);
    
  // Проверяем хронологический порядок
  for (size_t i = 1; i < events.size(); ++i) {
    CHECK(events[i-1].time <= events[i].time);
  }
    
  // Конкретные времена
  CHECK_THAT(events[0].time, WithinAbs(0.2, 1e-10));
  CHECK_THAT(events[1].time, WithinAbs(0.5, 1e-10));
  CHECK_THAT(events[2].time, WithinAbs(0.8, 1e-10));
}

TEST_CASE("Should calculate distance correctly - horizontal movement") {
  TestProvider provider;
  
  // Горизонтальное движение
  provider.AddGatherer(Point(0, 0), Point(10, 0), 2.0);
  // Предмет смещен по вертикали на 1
  provider.AddItem(Point(5, 1), 1.0);
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  REQUIRE(events.size() == 1);
  // Правильное расстояние: 1.0, квадрат = 1.0
  CHECK_THAT(events[0].sq_distance, WithinAbs(1.0, 1e-10));
}

TEST_CASE("Should calculate distance correctly - vertical movement") {
  TestProvider provider;
    
  // Вертикальное движение
  provider.AddGatherer(Point(0, 0), Point(0, 10), 2.0);
  // Предмет смещен по горизонтали на 1
  provider.AddItem(Point(1, 5), 1.0);
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  REQUIRE(events.size() == 1);
  // Правильное расстояние: 1.0, квадрат = 1.0
  CHECK_THAT(events[0].sq_distance, WithinAbs(1.0, 1e-10));
}

TEST_CASE("Should calculate distance correctly - diagonal movement") {
  TestProvider provider;
    
  // Диагональное движение под 45 градусов
  provider.AddGatherer(Point(0, 0), Point(10, 10), 2.0);
  // Предмет на расстоянии 1 от диагонали
  provider.AddItem(Point(6, 5), 1.0);  // точка (6,5) отстоит от прямой x=y на |6-5|/√2 = 1/√2
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  REQUIRE(events.size() == 1);
  // Квадрат расстояния = 0.5
  CHECK_THAT(events[0].sq_distance, WithinAbs(0.5, 1e-10));
}


TEST_CASE("Stationary gatherer should not generate events") {
  TestProvider provider;
    
  // Неподвижный собиратель
  provider.AddGatherer(Point(5, 5), Point(5, 5), 2.0);
  provider.AddItem(Point(5, 5), 1.0);  // предмет в той же точке
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  CHECK(events.empty());
}

TEST_CASE("Items outside movement segment should not be collected") {
  TestProvider provider;
    
  provider.AddGatherer(Point(0, 0), Point(10, 0), 2.0);
    
  // Предмет перед началом движения
  provider.AddItem(Point(-2, 0), 1.0);
  // Предмет после конца движения
  provider.AddItem(Point(12, 0), 1.0);
  // Предмет на пути
  provider.AddItem(Point(5, 0), 1.0);
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  REQUIRE(events.size() == 1);
  CHECK(events[0].item_id == 2);  // только предмет на пути
}

TEST_CASE("Touching items should be collected") {
  TestProvider provider;
    
  // Сумма радиусов = (2+1)/2 = 1.5
  provider.AddGatherer(Point(0, 0), Point(10, 0), 2.0);
  // Предмет касается собирателя
  provider.AddItem(Point(5, 1.5), 1.0);
    
  auto events = collision_detector::FindGatherEvents(provider);
    
  REQUIRE(events.size() == 1);
  CHECK_THAT(events[0].sq_distance, WithinAbs(2.25, 1e-10));  // 1.5^2
}
