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
            ESP_LOGD(TAG, "update(): publishing %s (highrange raw=%d)", high ? "High" : "Low", spaState->highrange);
            this->last_published_state_known_ = true;
            this->last_published_high_ = high;
            this->publish_state(high ? OPTION_HIGH : OPTION_LOW);
        }

        void TemperatureRangeSelect::set_parent(BalboaSpa *parent)
        {
            spa = parent;
            ESP_LOGD(TAG, "set_parent() called, registering listener");
            parent->register_listener([this](SpaState *spaState)
                                      { this->update(spaState); });
        }

        void TemperatureRangeSelect::control(size_t index)
        {
            ESP_LOGD(TAG, "control(size_t %u) invoked", static_cast<unsigned>(index));
            // Defer to base which converts via option_at and calls control(const std::string&).
            select::Select::control(index);
        }

        void TemperatureRangeSelect::control(const std::string &value)
        {
            ESP_LOGD(TAG, "control(string '%s') invoked", value.c_str());
            bool desired_high = (value == OPTION_HIGH);
            spa->set_highrange(desired_high);
            ESP_LOGD(TAG, "control(): set_highrange(%d) called", desired_high);
        }

    } // namespace balboa_spa
} // namespace esphome
