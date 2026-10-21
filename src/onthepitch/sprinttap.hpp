#ifndef _HPP_FOOTBALL_ONTHEPITCH_SPRINTTAP
#define _HPP_FOOTBALL_ONTHEPITCH_SPRINTTAP

// A knock-on is a short-lived double tap, never a stored future touch.
class SprintTap {
 public:
  void Update(unsigned long now_ms, bool pressed, bool eligible) {
    if (!eligible) {
      Reset();
      return;
    }
    if (pending && now_ms - pendingAt_ms >= 280)
      pending = false;
    if (pressed) {
      // The press that completes a pair is not also the first press of the
      // next one, so steady tapping knocks on every other tap, not every tap.
      const bool completesPair = hasPrevious && now_ms - previousAt_ms < 280;
      pending = completesPair;
      pendingAt_ms = now_ms;
      previousAt_ms = now_ms;
      hasPrevious = !completesPair;
    }
  }

  bool Consume() {
    const bool result = pending;
    pending = false;
    return result;
  }

  void Reset() {
    hasPrevious = false;
    pending = false;
  }

 private:
  unsigned long previousAt_ms = 0;
  unsigned long pendingAt_ms = 0;
  bool hasPrevious = false;
  bool pending = false;
};

#endif
