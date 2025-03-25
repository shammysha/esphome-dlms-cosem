#pragma once

#include "esphome/components/number.h"
#include "../dlms_cosem.h"
#include "esphome/core/preferences.h"

namespace esphome {
namespace dlms_cosem {

  class DlmsCosemNumber: public number::Number {
    public:
      void setup() override;
      void set_initial_value(float initial_value) { initial_value_ = initial_value; }
      void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

    protected:
      void control(float value) override;

      bool optimistic_{true};
      float initial_value_{NAN};
      bool restore_value_{true};

      ESPPreferenceObject pref_;
  }

  class LogicalDeviceNumber : public dlms_cosem::DlmsCosemNumber, public Parented<DlmsCosemComponent> {
    public:
      LogicalDeviceNumber() = default;

    protected:
      void control(float value) override;
  };

  class PhysicalDeviceNumber : public dlms_cosem::DlmsCosemNumber, public Parented<DlmsCosemComponent> {
    public:
      PhysicalDeviceNumber() = default;

    protected:
      void control(float value) override;
  };


}  // namespace ld2410
}  // namespace esphome
