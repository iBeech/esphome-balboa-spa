#include "esphome/core/log.h"
#include "temperature_range_select.h"

namespace esphome
{
    namespace balboa_spa
    {
        static const char *TAG = "balboa_spa.temperature_range";

        static const std::string OPTION_HIGH = "High";
        static const std::string OPTION_LOW = "Low";

        void TemperatureRangeSelect::update(SpaState *spaState)
        {
            bool high = spaState->highrange == 1;
            if (this->last_published_state_known_ && high == this->last_published_high_)
            {
                return;
            }
            this->last_published_state_known_ = true;
            this->last_published_high_ = high;
            this->publish_state(high ? OPTION_HIGH : OPTION_LOW);
        }

        void TemperatureRangeSelect::set_parent(BalboaSpa *parent)
        {
            spa = parent;
            parent->register_listener([this](SpaState *spaState)
                                      { this->update(spaState); });
        }

        void TemperatureRangeSelect::control(const std::string &value)
        {
            bool desired_high = (value == OPTION_HIGH);
            spa->set_highrange(desired_high);  // gated internally — sends 0x50 only if state differs
            ESP_LOGD(TAG, "Temperature range change requested: %s", value.c_str());
        }

    } // namespace balboa_spa
} // namespace esphome
