#pragma once

#include "esphome/components/switch/switch.h"
#include "../dlms_cosem.h"

namespace esphome {
namespace dlms_cosem {

  class LogicalDeviceNumber : public number::Number, public Parented<DlmsCosemComponent> {
    public:
      LogicalDeviceNumber() = default;

    protected:
      void control(float value) override;
  };

  class PhysicalDeviceNumber : public number::Number, public Parented<DlmsCosemComponent> {
    public:
      PhysicalDeviceNumber() = default;

    protected:
      void control(float value) override;
  };


}  // namespace ld2410
}  // namespace esphome
