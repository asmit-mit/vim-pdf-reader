#pragma once
#include <any>
#include <functional>
#include <mutex>
#include <queue>
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
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_.push({topic, typeid(Event), std::any(event)});
  }

  void update() {
    std::queue<Entry> current;
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      std::swap(current, queue_);
    }

    while (!current.empty()) {
      auto &[topic, type, payload] = current.front();

      auto topic_it = listeners_.find(topic);
      if (topic_it != listeners_.end()) {
        auto type_it = topic_it->second.find(type);
        if (type_it != topic_it->second.end()) {
          for (auto &listener : type_it->second)
            listener(payload);
        }
      }

      current.pop();
    }
  }

private:
  using Listener = std::function<void(const std::any &)>;

  struct Entry {
    std::string topic;
    std::type_index type;
    std::any payload;
  };

  std::unordered_map<std::string, std::unordered_map<std::type_index, std::vector<Listener>>>
      listeners_;

  std::queue<Entry> queue_;
  std::mutex queue_mutex_;
};

} // namespace core
