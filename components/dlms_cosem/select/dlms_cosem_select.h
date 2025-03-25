#pragma once

#include "esphome/components/select/select.h"
#include "../dlms_cosem.h"

namespace esphome {
namespace dlms_cosem {

class DlmsCosemSelect : public select::Select, public Parented<DlmsCosemComponent> {
  public:
    void setup() override;
    void set_initial_option(const std::string &initial_option) { this->initial_option_ = initial_option; }
    void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

  protected:
   void control(const std::string &value) override;

   bool optimistic_{true};
   std::string initial_option_;
   bool restore_value_ = true;

   ESPPreferenceObject pref_;


};

class AddressLengthSelect : public select::Select, public Parented<DlmsCosemComponent> {
  public:
    AddressLengthSelect() = default;

  protected:
    void control(const std::string &value) override;

};

}  // namespace ld2410
}  // namespace esphome
