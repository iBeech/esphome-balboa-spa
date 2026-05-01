#include "esphome/core/log.h"
#include "hold_mode_switch.h"

namespace esphome
{
    namespace balboa_spa
    {
        static const char *TAG = "balboa_spa.hold_mode";

        void HoldModeSwitch::update(SpaState *spaState)
        {
            if (this->discard_updates_ > 0)
            {
                this->discard_updates_--;
                return;
            }

            bool current = spaState->hold_mode == 1;

            if (!this->desired_state_set_)
            {
                if (current != this->state)
                {
                    this->publish_state(current);
                }
                return;
            }

            if (current == this->desired_state_)
            {
                this->desired_state_set_ = false;
                this->toggle_attempts_ = 0;
                this->publish_state(current);
                ESP_LOGD(TAG, "Hold mode reached target state %d", current);
                return;
            }

            if (this->toggle_attempts_ < this->max_toggle_attempts_)
            {
                this->toggle_attempts_++;
                this->discard_updates_ = this->discard_updates_config_;
                spa->toggle_hold();
                ESP_LOGD(TAG, "Toggling hold mode (attempt %d/%d) current=%d target=%d",
                         this->toggle_attempts_, this->max_toggle_attempts_, current, this->desired_state_);
            }
            else
            {
                ESP_LOGW(TAG, "Hold mode failed to reach target state after %d attempts", this->max_toggle_attempts_);
                this->desired_state_set_ = false;
                this->toggle_attempts_ = 0;
                this->publish_state(current);
            }
        }

        void HoldModeSwitch::set_parent(BalboaSpa *parent)
        {
            spa = parent;
            parent->register_listener([this](SpaState *spaState)
                                      { this->update(spaState); });
        }

        void HoldModeSwitch::write_state(bool state)
        {
            bool current = spa->get_hold_mode();
            if (current == state)
            {
                ESP_LOGD(TAG, "Hold mode already at target state %d", state);
                this->publish_state(state);
                return;
            }
            this->desired_state_ = state;
            this->desired_state_set_ = true;
            this->toggle_attempts_ = 0;
            this->discard_updates_ = this->discard_updates_config_;
            spa->toggle_hold();
            ESP_LOGD(TAG, "Hold mode change requested: current=%d target=%d", current, state);
        }

    } // namespace balboa_spa
} // namespace esphome
