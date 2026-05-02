#include "esphome.h"
#include "esphome/core/log.h"
#include "spa_thermostat.h"
#include "esphome/components/climate/climate_mode.h"

namespace esphome
{
    namespace balboa_spa
    {
        static const char *TAG = "balboa_spa.climate";

        // Heating mode raw values (from Flag Byte 5 bits 0x03)
        static const uint8_t HEATING_MODE_READY = 0;
        static const uint8_t HEATING_MODE_REST = 1;
        static const uint8_t HEATING_MODE_READY_IN_REST = 3;

        static const char *const PRESET_READY = "Ready";
        static const char *const PRESET_REST = "Rest";
        static const char *const PRESET_READY_IN_REST = "Ready in Rest";

        void BalboaSpaThermostat::apply_range_limits_to_traits(climate::ClimateTraits &traits, bool high_range)
        {
            if (high_range)
            {
                traits.set_visual_min_temperature(this->high_range_min_);
                traits.set_visual_max_temperature(this->high_range_max_);
            }
            else
            {
                traits.set_visual_min_temperature(this->low_range_min_);
                traits.set_visual_max_temperature(this->low_range_max_);
            }
            traits.set_visual_temperature_step(0.5f);
        }

        void BalboaSpaThermostat::setup()
        {
            // Custom presets live on the Climate base, not on traits. Set once at component setup.
            this->set_supported_custom_presets({PRESET_READY, PRESET_REST, PRESET_READY_IN_REST});
        }

        climate::ClimateTraits BalboaSpaThermostat::traits()
        {
            auto traits = climate::ClimateTraits();

            if (this->legacy_hvac_modes_)
            {
                // Legacy: HEAT (Ready) + OFF (Rest) HVAC modes, plus HOME/ECO presets for high/low range.
                // New custom presets are also exposed so users can begin migrating without losing functionality.
                traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
                traits.set_supported_presets({climate::ClimatePreset::CLIMATE_PRESET_HOME, climate::ClimatePreset::CLIMATE_PRESET_ECO});
            }
            else
            {
                // New: HEAT only (Hold lives on the dedicated switch). Heating mode via custom_preset.
                traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
            }

            traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION | climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);

            if (this->dynamic_temperature_limits_)
            {
                bool high_range = false;
                if (this->spa != nullptr && this->spa->get_current_state() != nullptr)
                {
                    high_range = this->spa->get_current_state()->highrange == 1;
                }
                else
                {
                    high_range = this->last_published_high_range_;
                }
                this->apply_range_limits_to_traits(traits, high_range);
            }

            return traits;
        }

        void BalboaSpaThermostat::control(const climate::ClimateCall &call)
        {
            if (call.get_target_temperature().has_value())
            {
                spa->set_temp(*call.get_target_temperature());
            }

            // custom_preset: Ready / Rest are settable. Ready in Rest is read-only — selecting it is rejected.
            if (call.has_custom_preset())
            {
                StringRef requested = call.get_custom_preset();
                uint8_t current = spa->get_heating_mode_raw();

                if (requested == PRESET_READY_IN_REST)
                {
                    ESP_LOGW(TAG, "'Ready in Rest' is a derived state — start Pump 1 from Rest to enter it. Ignoring.");
                }
                else if (requested == PRESET_READY)
                {
                    if (current != HEATING_MODE_READY && current != HEATING_MODE_READY_IN_REST)
                    {
                        ESP_LOGD(TAG, "Toggling heating mode to Ready");
                        spa->toggle_heat();
                    }
                }
                else if (requested == PRESET_REST)
                {
                    if (current != HEATING_MODE_REST)
                    {
                        ESP_LOGD(TAG, "Toggling heating mode to Rest");
                        spa->toggle_heat();
                    }
                }
                else
                {
                    ESP_LOGW(TAG, "Unknown custom_preset '%s'", requested.c_str());
                }
            }

            // Legacy HOME/ECO preset still controls highrange when legacy_hvac_modes is enabled.
            if (this->legacy_hvac_modes_ && call.get_preset().has_value())
            {
                spa->set_highrange(*call.get_preset() == climate::ClimatePreset::CLIMATE_PRESET_HOME);
            }

            // Legacy HVAC mode mapping kept only when legacy flag is enabled.
            if (this->legacy_hvac_modes_ && call.get_mode().has_value())
            {
                auto requested_mode = *call.get_mode();
                bool is_in_rest = spa->get_restmode();

                if (requested_mode == climate::CLIMATE_MODE_HEAT && is_in_rest)
                {
                    ESP_LOGD(TAG, "Legacy HVAC: toggle from Rest to Heat (Ready)");
                    spa->toggle_heat();
                }
                else if (requested_mode == climate::CLIMATE_MODE_OFF && !is_in_rest)
                {
                    ESP_LOGD(TAG, "Legacy HVAC: toggle from Heat to Rest");
                    spa->toggle_heat();
                }
            }
        }

        void BalboaSpaThermostat::set_parent(BalboaSpa *parent)
        {
            spa = parent;
            parent->register_listener([this](SpaState *spaState)
                                      { this->update(spaState); });
        }

        bool inline is_diff_no_nan(float a, float b)
        {
            return !std::isnan(a) && !std::isnan(b) && b != a;
        }

        void BalboaSpaThermostat::update(SpaState *spaState)
        {
            bool needs_update = false;

            if (!spa->is_communicating())
            {
                this->target_temperature = NAN;
                this->current_temperature = NAN;
                return;
            }

            float target_temp = spaState->target_temp;
            needs_update = is_diff_no_nan(target_temp, this->target_temperature) || needs_update;
            this->target_temperature = !std::isnan(target_temp) ? target_temp : this->target_temperature;

            auto current_temp = spaState->current_temp;
            needs_update = is_diff_no_nan(current_temp, this->current_temperature) || needs_update;
            this->current_temperature = !std::isnan(current_temp) ? current_temp : this->current_temperature;

            // Action mapping:
            //   HEATING — heater is actively firing (any mode, including filter-cycle bursts in Rest)
            //   OFF     — heating mode is Rest and heater is not firing (visually "off" on the tile)
            //   IDLE    — Ready / Ready in Rest, heater not currently firing
            climate::ClimateAction new_action;
            if (spaState->heat_state == 1)
            {
                new_action = climate::CLIMATE_ACTION_HEATING;
            }
            else if (spaState->rest_mode == HEATING_MODE_REST)
            {
                new_action = climate::CLIMATE_ACTION_OFF;
            }
            else
            {
                new_action = climate::CLIMATE_ACTION_IDLE;
            }
            needs_update = new_action != this->action || needs_update;
            this->action = new_action;

            // HVAC mode reflects the legacy mapping when legacy flag is enabled; otherwise always HEAT.
            climate::ClimateMode new_mode;
            if (this->legacy_hvac_modes_)
            {
                new_mode = spaState->rest_mode == HEATING_MODE_REST ? climate::CLIMATE_MODE_OFF : climate::CLIMATE_MODE_HEAT;
            }
            else
            {
                new_mode = climate::CLIMATE_MODE_HEAT;
            }
            needs_update = new_mode != this->mode || needs_update;
            this->mode = new_mode;

            // Legacy HOME/ECO preset based on highrange.
            if (this->legacy_hvac_modes_)
            {
                auto legacy_preset = spaState->highrange == 1 ? climate::ClimatePreset::CLIMATE_PRESET_HOME : climate::ClimatePreset::CLIMATE_PRESET_ECO;
                needs_update = legacy_preset != this->preset || needs_update;
                this->preset = legacy_preset;
            }

            // New custom_preset reflecting Balboa heating mode.
            const char *new_custom_preset = nullptr;
            switch (spaState->rest_mode)
            {
            case HEATING_MODE_READY:
                new_custom_preset = PRESET_READY;
                break;
            case HEATING_MODE_REST:
                new_custom_preset = PRESET_REST;
                break;
            case HEATING_MODE_READY_IN_REST:
                new_custom_preset = PRESET_READY_IN_REST;
                break;
            default:
                break;
            }
            if (new_custom_preset != nullptr)
            {
                StringRef current_preset = this->get_custom_preset();
                if (!this->has_custom_preset() || current_preset != new_custom_preset)
                {
                    this->set_custom_preset_(new_custom_preset);
                    needs_update = true;
                }
            }

            // Track range changes so dynamic temp limits can re-publish traits if enabled.
            bool high_range = spaState->highrange == 1;
            if (this->dynamic_temperature_limits_)
            {
                if (!this->last_published_high_range_known_ || high_range != this->last_published_high_range_)
                {
                    this->last_published_high_range_ = high_range;
                    this->last_published_high_range_known_ = true;
                    needs_update = true;
                    ESP_LOGD(TAG, "Range changed (high=%d); next publish will carry updated temperature bounds.", high_range);
                }
            }

            needs_update = this->last_update_time + 300000 < millis() || needs_update;

            if (needs_update)
            {
                this->publish_state();
                this->last_update_time = millis();
            }
        }

    }
}
