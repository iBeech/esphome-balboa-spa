#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "../balboaspa.h"

namespace esphome
{
  namespace balboa_spa
  {

    class BalboaSpaThermostat : public climate::Climate, public Component
    {
    public:
      BalboaSpaThermostat()
      {
        spa = nullptr;
        last_update_time = 0;
      };

      void setup() override;
      void update(SpaState *spaState);
      void set_parent(BalboaSpa *parent);
      void set_legacy_hvac_modes(bool legacy) { legacy_hvac_modes_ = legacy; }
      void set_dynamic_temperature_limits(bool enabled) { dynamic_temperature_limits_ = enabled; }
      void set_high_range_min(float v) { high_range_min_ = v; }
      void set_high_range_max(float v) { high_range_max_ = v; }
      void set_low_range_min(float v) { low_range_min_ = v; }
      void set_low_range_max(float v) { low_range_max_ = v; }

    protected:
      void control(const climate::ClimateCall &call) override;
      climate::ClimateTraits traits() override;
      void apply_range_limits_to_traits(climate::ClimateTraits &traits, bool high_range);

    private:
      BalboaSpa *spa;
      uint32_t last_update_time;
      bool legacy_hvac_modes_ = false;
      bool dynamic_temperature_limits_ = false;
      bool last_published_high_range_known_ = false;
      bool last_published_high_range_ = false;
      float high_range_min_ = 26.0f;
      float high_range_max_ = 40.0f;
      float low_range_min_ = 10.0f;
      float low_range_max_ = 26.0f;
    };

  } // namespace balboa_spa
} // namespace esphome
