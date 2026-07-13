#pragma once

#include <any>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace core {

class EventBus {
public:
  template <typename Event> using Callback = std::function<void(const Event &)>;

  template <typename Event> void subscribe(const std::string &topic, Callback<Event> callback) {
    listeners_[topic][typeid(Event)].push_back([callback](const std::any &event) {
      callback(std::any_cast<const Event &>(event));
    });
  }

  template <typename Event> void emit(const std::string &topic, const Event &event) {
    auto topic_it = listeners_.find(topic);
    if (topic_it == listeners_.end())
      return;

    auto type_it = topic_it->second.find(typeid(Event));
    if (type_it == topic_it->second.end())
      return;

    std::any e = event;

    for (auto &listener : type_it->second)
      listener(e);
  }

private:
  using Listener = std::function<void(const std::any &)>;

  std::unordered_map<std::string, std::unordered_map<std::type_index, std::vector<Listener>>>
      listeners_;
};

} // namespace core
