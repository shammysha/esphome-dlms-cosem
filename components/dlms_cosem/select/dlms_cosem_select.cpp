#include "dlms_cosem_switch.h"

namespace esphome {
namespace dlms_cosem {

  void DlmsCosemSelect::setup() {
    std::string value;

    size_t index;
    this->pref_ = global_preferences->make_preference<size_t>(this->get_object_id_hash());
    if (!this->pref_.load(&index)) {
      value = this->initial_option_;
    } else if (!this->has_index(index)) {
      value = this->initial_option_;
    } else {
      value = this->at(index).value();
    }

    this->publish_state(value);
  }

  void DlmsCosemSelect::control(const std::string &value) {
    this->publish_state(value);

    auto index = this->index_of(value);
    this->pref_.save(&index.value());
  }

  void AddressLengthSelect::control(const std::string &value) {
    DlmsCosemSelect::control(value);

    this->parent_->update_server_address(this->parent_->get_logical_device(), this->parent_->get_physical_device(), (unsigned char) value[0]);
  }

}}
