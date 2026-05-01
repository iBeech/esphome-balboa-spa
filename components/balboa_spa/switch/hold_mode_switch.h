#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../balboaspa.h"

namespace esphome
{
  namespace balboa_spa
  {

    class HoldModeSwitch : public switch_::Switch
    {
    public:
      HoldModeSwitch() {};
      void update(SpaState *spaState);
      void set_parent(BalboaSpa *parent);
      void set_max_toggle_attempts(uint8_t value) { max_toggle_attempts_ = value; }
      void set_discard_updates(uint8_t value) { discard_updates_config_ = value; }

    protected:
      void write_state(bool state) override;

    private:
      BalboaSpa *spa = nullptr;
      bool desired_state_set_ = false;
      bool desired_state_ = false;
      uint8_t toggle_attempts_ = 0;
      uint8_t max_toggle_attempts_ = 5;
      uint8_t discard_updates_ = 0;
      uint8_t discard_updates_config_ = 20;
    };

  } // namespace balboa_spa
} // namespace esphome
