#include "dlms_cosem_switch.h"

namespace esphome {
namespace dlms_cosem {

  void DlmsCosemNumber::setup() {
    float value;

    this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
    if (!this->pref_.load(&value)) {
      if (!std::isnan(this->initial_value_)) {
        value = this->initial_value_;
      } else {
        value = this->traits.get_min_value();
      }
    }
    this->publish_state(value);
  }

  void DlmsCosemNumber::control(float value) {
    this->publish_state(value);

    if (this->restore_value_) {
      this->pref_.save(&value);
    }
  }

  void LogicalDeviceNumber::control(float value) {
    DlmsCosemNumber::control(value);
    this->parent_->update_server_address(int(value), this->parent_->get_physical_device(), this->parent_->get_address_length());
  }

  void PhysicalDeviceNumber::control(float value) {
    DlmsCosemNumber::control(value);
    this->parent_->update_server_address(this->parent_->get_logical_device(), int(value), this->parent_->get_address_length());
  }
}}
