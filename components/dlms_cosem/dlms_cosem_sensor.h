#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome {
namespace dlms_cosem {

static constexpr uint8_t MAX_TRIES = 10;

stdstring CONNECTION = "CONNECTION"

enum SensorType { SENSOR, TEXT_SENSOR };

// const char * UNIT_STR_UNKNOWN = "Unknown unit";
#define UNIT_STR_UNKNOWN_NOT_YET "Unknown unit / not yet known"
#define UNIT_STR_UNKNOWN "Unknown unit"

class DlmsCosemSensorBase {
 public:
  static const uint8_t MAX_REQUEST_SIZE = 15;

  virtual SensorType get_type() const = 0;
  virtual const StringRef &get_sensor_name() = 0;
  virtual EntityBase *get_base() = 0;
  virtual void publish() = 0;

  void set_obis_code(const char *obis_code) { this->obis_code_ = obis_code; }
  const std::string &get_obis_code() const { return this->obis_code_; }

  void set_dont_publish(bool dont_publish) { this->we_shall_publish_ = !dont_publish; }
  bool shall_we_publish() const { return this->we_shall_publish_; }

  void set_obis_class(int obis_class) { this->obis_class_ = obis_class; }
  int get_obis_class() { return this->obis_class_; }

  void reset() {
    has_value_ = false;
    tries_ = 0;
  }

  bool has_value() { return has_value_; }
  virtual bool has_got_scale_and_unit() { return scale_and_unit_detected_; }

  void record_failure() {
    if (tries_ < MAX_TRIES) {
      tries_++;
    } else {
      has_value_ = false;
    }
  }
  bool is_failed() { return tries_ == MAX_TRIES; }

 protected:
  std::string obis_code_;
  int obis_class_{3 /*DLMS_OBJECT_TYPE_REGISTER*/};
  bool has_value_;
  uint8_t tries_{0};
  bool we_shall_publish_{true};
  bool scale_and_unit_detected_{false};
};

class DlmsCosemSensor : public DlmsCosemSensorBase, public sensor::Sensor {
 public:
  SensorType get_type() const override { return SENSOR; }
  const StringRef &get_sensor_name() { return this->get_name(); }
  EntityBase *get_base() { return this; }
  void publish() override { publish_state(this->value_); }

  void set_scale_and_unit(int8_t scaler, uint8_t unit, const char *unit_s) {
    this->scale_and_unit_detected_ = true;
    this->scaler_ = scaler;
    this->scale_f_ = std::pow(10, scaler);
    this->unit_ = unit;
    this->unit_s_ = unit_s ? unit_s : UNIT_STR_UNKNOWN;
    this->log_scale_and_unit();
  }

  void log_scale_and_unit() {
    ESP_LOGW("dlms_cosem_sensor", "name: %s, obis: %s, unit: %d (%s), scaler pow: %d, scale_f: %f",
             this->get_sensor_name().c_str(), this->get_obis_code().c_str(), this->unit_, unit_s_, scaler_, scale_f_);
  }

  float get_scale() const { return scale_f_; }

  const char *get_unit() const { return unit_s_; }

  void set_multiplier(float multiplier) { this->multiplier_ = multiplier; }

  void set_value(float value) {
    this->value_ = value * scale_f_ * multiplier_;
    this->has_value_ = true;
    this->tries_ = 0;
  }

 protected:
  float value_;
  float multiplier_{1.0f};
  int8_t scaler_{0};
  float scale_f_{1.0f};
  uint8_t unit_{0};
  const char *unit_s_ = UNIT_STR_UNKNOWN_NOT_YET;
};

#ifdef USE_TEXT_SENSOR
class DlmsCosemTextSensor : public DlmsCosemSensorBase, public text_sensor::TextSensor {
  public:
    SensorType get_type() const override { return TEXT_SENSOR; }
    const StringRef &get_sensor_name() { return this->get_name(); }
    EntityBase *get_base() { return this; }
    void publish() override { publish_state(value_); }

    bool has_got_scale_and_unit() override { return true; }

    void set_value(const char *value) {
      char res[std::strlen(value)*3+1]; 
      cp1251_to_utf8(res, value);
      value_ = std::string(res);
      has_value_ = true;
      tries_ = 0;
    }

 protected:
   std::string value_;
  
   void cp1251_to_utf8(char *out, const char *in) {
     static const int table[128] = {                    
       0x82D0,0x83D0,0x9A80E2,0x93D1,0x9E80E2,0xA680E2,0xA080E2,0xA180E2,
       0xAC82E2,0xB080E2,0x89D0,0xB980E2,0x8AD0,0x8CD0,0x8BD0,0x8FD0,    
       0x92D1,0x9880E2,0x9980E2,0x9C80E2,0x9D80E2,0xA280E2,0x9380E2,0x9480E2,
       0,0xA284E2,0x99D1,0xBA80E2,0x9AD1,0x9CD1,0x9BD1,0x9FD1,               
       0xA0C2,0x8ED0,0x9ED1,0x88D0,0xA4C2,0x90D2,0xA6C2,0xA7C2,              
       0x81D0,0xA9C2,0x84D0,0xABC2,0xACC2,0xADC2,0xAEC2,0x87D0,              
       0xB0C2,0xB1C2,0x86D0,0x96D1,0x91D2,0xB5C2,0xB6C2,0xB7C2,              
       0x91D1,0x9684E2,0x94D1,0xBBC2,0x98D1,0x85D0,0x95D1,0x97D1,            
       0x90D0,0x91D0,0x92D0,0x93D0,0x94D0,0x95D0,0x96D0,0x97D0,
       0x98D0,0x99D0,0x9AD0,0x9BD0,0x9CD0,0x9DD0,0x9ED0,0x9FD0,
       0xA0D0,0xA1D0,0xA2D0,0xA3D0,0xA4D0,0xA5D0,0xA6D0,0xA7D0,
       0xA8D0,0xA9D0,0xAAD0,0xABD0,0xACD0,0xADD0,0xAED0,0xAFD0,
       0xB0D0,0xB1D0,0xB2D0,0xB3D0,0xB4D0,0xB5D0,0xB6D0,0xB7D0,
       0xB8D0,0xB9D0,0xBAD0,0xBBD0,0xBCD0,0xBDD0,0xBED0,0xBFD0,
       0x80D1,0x81D1,0x82D1,0x83D1,0x84D1,0x85D1,0x86D1,0x87D1,
       0x88D1,0x89D1,0x8AD1,0x8BD1,0x8CD1,0x8DD1,0x8ED1,0x8FD1
     };
     while (*in)
       if (*in & 0x80) {
         int v = table[(int)(0x7f & *in++)];
         if (!v) {
           continue;
         }
         *out++ = (char)v;
         *out++ = (char)(v >> 8);
         if (v >>= 16)
           *out++ = (char)v;
       } else {
         *out++ = *in++;
       }
       *out = 0;
   }

};
#endif


}  // namespace dlms_cosem
}  // namespace esphome
