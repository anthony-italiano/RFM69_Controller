#include "Scheduler.h"
#include "Config.h"

void Scheduler::tick() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < count; i++) {
    if (tasks[i].enabled && (int32_t)(now - tasks[i].nextDue) >= 0) {
      if (DEBUG_LEVEL & SCHED_DEBUG) {
        Serial.print(F("[SCHED] Running task: "));
        Serial.println(tasks[i].name);
      }
      tasks[i].fn();
      tasks[i].nextDue = now + tasks[i].interval;
    }
  }
}
