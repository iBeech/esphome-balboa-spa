#include "filter2_switch.h"

namespace esphome
{
    namespace balboa_spa
    {
        static const char *TAG = "balboa_spa.switch";

        void Filter2Switch::update(SpaFilterSettings *filterSettings)
        {
            if (this->state != filterSettings->filter2_enable)
            {
                this->publish_state(filterSettings->filter2_enable);
            }
        }

        void Filter2Switch::set_parent(BalboaSpa *parent)
        {
            spa = parent;
            // Subscribe to filter settings updates to sync switch state
            parent->register_filter_listener([this](SpaFilterSettings *filterSettings) {
                this->update(filterSettings);
            });
        }

        void Filter2Switch::write_state(bool state)
        {
            if (state)
            {
                if (!spa->has_filter2_duration_configured())
                {
                    ESP_LOGE(TAG, "Cannot enable Filter 2: no duration configured. Set Filter 2 duration via the text field first.");
                    this->publish_state(false);
                    return;
                }
                spa->enable_filter2();
                ESP_LOGI(TAG, "Filter 2 enabled");
                spa->request_filter_settings_update();
            }
            else
            {
                spa->disable_filter2();
                ESP_LOGI(TAG, "Filter 2 disabled");
                spa->request_filter_settings_update();
            }
        }

    } // namespace balboa_spa
} // namespace esphome
