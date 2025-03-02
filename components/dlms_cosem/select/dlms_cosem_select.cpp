#include "dlms_cosem_switch.h"

namespace esphome {
namespace dlms_cosem {

  void AddressLengthSelect::control(const std::string &value) {
    this->parent_->update_server_address(this->parent_->get_logical_device(), this->parent_->get_physical_device(), (unsigned char) value[0]);
  }

}}
