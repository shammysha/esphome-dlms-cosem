#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "dlms_cosem.h"

namespace esphome {
namespace dlms_cosem {

template<typename... Ts> class DlmsCosemScanStartAction : public Action<Ts...>, public Parented<DlmsCosemComponent> {
  public:
    TEMPLATABLE_VALUE(int, server_address)
    TEMPLATABLE_VALUE(int, logical_device)
    TEMPLATABLE_VALUE(int, physical_device)
    TEMPLATABLE_VALUE(int, address_length)

    void play(Ts... x) override {
      if (this->server_address_.value(x...)) {
        this->parent_->scan_start(this->server_address_.value(x...));
      } else {
        this->parent_->scan_start(this->logical_device.value(x...), this->logical_device.value(x...), this->address_length.value(x...));
      }
    }
};

template<typename... Ts> class DlmsCosemScanStop : public Action<Ts...>, public Parented<DlmsCosemComponent> {
  public:

    void play(Ts... x) override {
      this->parent_->scan_stopt();
    }
};

}}
