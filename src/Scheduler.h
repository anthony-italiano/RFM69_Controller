#pragma once
#include <Arduino.h>
#include <functional>

struct Task {
  String name;
  uint32_t interval;
  uint32_t nextDue;
  std::function<void()> fn;
  bool enabled;
};

class Scheduler {
public:
  static const uint8_t MAX_TASKS = 10;
  Task tasks[MAX_TASKS];
  uint8_t count = 0;

  void addTask(const char* name, uint32_t interval, std::function<void()> fn, bool enabled = true) {
    if (count < MAX_TASKS) {
      tasks[count] = { String(name), interval, millis() + interval, fn, enabled };
      count++;
    }
  }

  void tick();  // ⬅️ Only declaration now
};
