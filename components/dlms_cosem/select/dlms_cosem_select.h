#pragma once

#include "esphome/components/select/select.h"
#include "../dlms_cosem.h"

namespace esphome {
namespace dlms_cosem {

class AddressLengthSelect : public select::Select, public Parented<DlmsCosemComponent> {
  public:
    AddressLengthSelect() = default;

  protected:
    void control(const std::string &value) override;

};

}  // namespace ld2410
}  // namespace esphome
