#include "dlms_cosem_switch.h"

namespace esphome {
namespace dlms_cosem {

  void LogicalDeviceNumber::control(float value) {
    this->parent_->update_server_address(int(value), this->parent_->get_physical_device(), this->parent_->get_address_length());
  }

  void PhysicalDeviceNumber::control(float value) {
    this->parent_->update_server_address(this->parent_->get_logical_device(), int(value), this->parent_->get_address_length());
  }

}}
