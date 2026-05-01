#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "../balboaspa.h"

namespace esphome
{
  namespace balboa_spa
  {

    class TemperatureRangeSelect : public select::Select, public Component
    {
    public:
      TemperatureRangeSelect() {};
      void update(SpaState *spaState);
      void set_parent(BalboaSpa *parent);

    protected:
      void control(const std::string &value) override;

    private:
      BalboaSpa *spa = nullptr;
      bool last_published_state_known_ = false;
      bool last_published_high_ = false;
    };

  } // namespace balboa_spa
} // namespace esphome
