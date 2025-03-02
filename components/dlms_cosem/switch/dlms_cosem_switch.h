#pragma once

#include "esphome/components/switch/switch.h"
#include "../dlms_cosem.h"

namespace esphome {
namespace dlms_cosem {

class ScanningSwitch : public switch_::Switch, public Parented<LD2410Component> {
 public:
    void write_state(bool state) override;
};

}  // namespace ld2410
}  // namespace esphome
