#include "dlms_cosem_switch.h"

namespace esphome {
namespace dlms_cosem {

  void DlmsCosemScanSwitch::write_state(bool state) {
    if (state) {
      this->parent_->start_scan(this->parent_->get_logical_device(), this->parent_->get_physical_device(), this->parent_->get_address_length());
    } else {
      this->parent_->stop_scan();
    }
  }

}}
