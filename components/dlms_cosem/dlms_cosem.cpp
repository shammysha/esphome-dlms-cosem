
#include "dlms_cosem.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <sstream>

namespace esphome {
namespace dlms_cosem {

static const char *TAG0 = "dlms_cosem_";

#define TAG (this->tag_.c_str())

static constexpr uint8_t SOH = 0x01;
static constexpr uint8_t STX = 0x02;
static constexpr uint8_t ETX = 0x03;
static constexpr uint8_t EOT = 0x04;
static constexpr uint8_t ENQ = 0x05;
static constexpr uint8_t ACK = 0x06;
static constexpr uint8_t CR = 0x0D;
static constexpr uint8_t LF = 0x0A;
static constexpr uint8_t NAK = 0x15;

static constexpr uint8_t HDLC_FLAG = 0x7E;

static const uint8_t CMD_ACK_SET_BAUD_AND_MODE[] = {ACK, '0', '5', '1', CR, LF};
static const uint8_t CMD_CLOSE_SESSION[] = {SOH, 0x42, 0x30, ETX, 0x75};

static constexpr uint8_t BOOT_WAIT_S = 15;  // 10;

static char empty_str[] = "";

/*
static char format_hex_char(uint8_t v) { return v >= 10 ? 'A' + (v - 10) : '0' + v; }

static std::string format_frame_pretty(const uint8_t *data, size_t length) {
  if (length == 0)
    return "";
  std::string ret;
  ret.resize(3 * length - 1);
  std::ostringstream ss(ret);

  for (size_t i = 0; i < length; i++) {
    switch (data[i]) {
      case 0x00:
        ss << "<NUL>";
        break;
      case 0x01:
        ss << "<SOH>";
        break;
      case 0x02:
        ss << "<STX>";
        break;
      case 0x03:
        ss << "<ETX>";
        break;
      case 0x04:
        ss << "<EOT>";
        break;
      case 0x05:
        ss << "<ENQ>";
        break;
      case 0x06:
        ss << "<ACK>";
        break;
      case 0x0d:
        ss << "<CR>";
        break;
      case 0x0a:
        ss << "<LF>";
        break;
      case 0x15:
        ss << "<NAK>";
        break;
      case 0x20:
        ss << "<SP>";
        break;
      case 0x7E:
        ss << "<FLAG>";
        break;
      default:
        if (data[i] <= 0x20 || data[i] >= 0x7f) {
          ss << "<" << format_hex_char((data[i] & 0xF0) >> 4) << format_hex_char(data[i] & 0x0F) << ">";
        } else {
          ss << (char) data[i];
        }
        break;
    }
  }
  if (length > 4)
    ss << " (" << length << ")";
  return ss.str();
}

uint8_t baud_rate_to_byte(uint32_t baud) {
  constexpr uint16_t BAUD_BASE = 300;
  constexpr uint8_t BAUD_MULT_MAX = 6;

  uint8_t idx = 0;  // 300
  for (size_t i = 0; i <= BAUD_MULT_MAX; i++) {
    if (baud == BAUD_BASE * (1 << i)) {
      idx = i;
      break;
    }
  }
  return idx + '0';
}
*/

void DlmsCosemComponent::set_baud_rate_(uint32_t baud_rate) {
  ESP_LOGV(TAG, "Setting baud rate %u bps", baud_rate);
  iuart_->update_baudrate(baud_rate);
}

void DlmsCosemComponent::set_server_address(uint16_t address) { this->server_address_ = address; };

uint16_t DlmsCosemComponent::set_server_address(uint16_t logicalAddress, uint16_t physicalAddress,
                                                unsigned char addressSize) {
  this->server_address_ = cl_getServerAddress(logicalAddress, physicalAddress, addressSize);

  ESP_LOGD(TAG, "Server address = %d (based on logical_address=%d, physical_address=%d, address_size=%d)",
           this->server_address_, logicalAddress, physicalAddress, addressSize);
  return this->server_address_;
}

void DlmsCosemComponent::update_server_address(uint16_t addr) {
  this->server_address_ = addr;
  cl_clear(&dlms_settings_);
  cl_init(&dlms_settings_, true, this->client_address_, this->server_address_,
          this->auth_required_ ? DLMS_AUTHENTICATION_LOW : DLMS_AUTHENTICATION_NONE,
          this->auth_required_ ? this->password_.c_str() : NULL, DLMS_INTERFACE_TYPE_HDLC);

  this->update();
}

uint16_t DlmsCosemComponent::update_server_address(uint16_t logicalAddress, uint16_t physicalAddress,
                                                   unsigned char addressSize) {
  this->set_server_address(logicalAddress, physicalAddress, addressSize);
  this->update_server_address(this->server_address_);
  return this->server_address_;
}

void DlmsCosemComponent::setup() {
  ESP_LOGD(TAG, "setup");

  cl_init(&dlms_settings_, true, this->client_address_, this->server_address_,
          this->auth_required_ ? DLMS_AUTHENTICATION_LOW : DLMS_AUTHENTICATION_NONE,
          this->auth_required_ ? this->password_.c_str() : NULL, DLMS_INTERFACE_TYPE_HDLC);

  this->buffers_.init();

  this->indicate_transmission(false);

#ifdef USE_ESP32_FRAMEWORK_ARDUINO
  iuart_ = make_unique<DlmsCosemUart>(*static_cast<uart::ESP32ArduinoUARTComponent *>(this->parent_));
#endif

#ifdef USE_ESP_IDF
  iuart_ = make_unique<DlmsCosemUart>(*static_cast<uart::IDFUARTComponent *>(this->parent_));
#endif

#if USE_ESP8266
  iuart_ = make_unique<DlmsCosemUart>(*static_cast<uart::ESP8266UartComponent *>(this->parent_));
#endif
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->setup();
  }

  this->set_baud_rate_(this->baud_rate_handshake_);
  this->set_timeout(BOOT_WAIT_S * 1000, [this]() {
    ESP_LOGD(TAG, "Boot timeout, component is ready to use");
    this->clear_rx_buffers_();
    this->set_next_state_(State::IDLE);
  });
}

void DlmsCosemComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DLMS-COSEM (SPODES):");
  LOG_UPDATE_INTERVAL(this);
  LOG_PIN("  Flow Control Pin: ", this->flow_control_pin_);
  ESP_LOGCONFIG(TAG, "  Receive Timeout: %ums", this->receive_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Supported Meter Types: DLMS/COSEM (SPODES)");
  ESP_LOGCONFIG(TAG, "  Client address: %d", this->client_address_);
  ESP_LOGCONFIG(TAG, "  Server address: %d", this->server_address_);
  ESP_LOGCONFIG(TAG, "  Authentication: %s", this->auth_required_ == DLMS_AUTHENTICATION_NONE ? "None" : "Low");
  ESP_LOGCONFIG(TAG, "  P*ssword: %s", this->password_.c_str());
  ESP_LOGCONFIG(TAG, "  Sensors:");
  for (const auto &sensors : sensors_) {
    auto &s = sensors.second;
    ESP_LOGCONFIG(TAG, "    OBIS code: %s, Name: %s", s->get_obis_code().c_str(), s->get_sensor_name().c_str());
  }
}

void DlmsCosemComponent::register_sensor(DlmsCosemSensorBase *sensor) {
  this->sensors_.insert({sensor->get_obis_code(), sensor});
}

void DlmsCosemComponent::abort_mission_() {
  // try close connection ?
  ESP_LOGE(TAG, "Abort mission. Closing session");
  this->set_next_state_(State::MISSION_FAILED);
}

void DlmsCosemComponent::report_failure(bool failure) {
  if (!failure) {
    this->stats_.failures_ = 0;
    return;
  }

  this->stats_.failures_++;
  if (this->failures_before_reboot_ > 0 && this->stats_.failures_ > this->failures_before_reboot_) {
    ESP_LOGE(TAG, "Too many failures in a row. Let's try rebooting device.");
    delay(100);
    App.safe_reboot();
  }
}

void DlmsCosemComponent::loop() {
  if (!this->is_ready() || this->state_ == State::NOT_INITIALIZED)
    return;

  switch (this->state_) {
    case State::IDLE: {
      this->update_last_rx_time_();
      this->indicate_transmission(false);
      this->indicate_session(false);
    } break;

    case State::TRY_LOCK_BUS: {
      this->log_state_();
      if (this->try_lock_uart_session_()) {
        this->indicate_session(true);
        this->set_next_state_(State::OPEN_SESSION);
        this->indicate_connection(true);

      } else {
        ESP_LOGV(TAG, "UART Bus is busy, waiting ...");
        this->set_next_state_delayed_(1000, State::TRY_LOCK_BUS);

      }
    } break;

    case State::WAIT:
      if (this->check_wait_timeout_()) {
        this->set_next_state_(this->wait_.next_state);
        this->update_last_rx_time_();
      }
      break;

    case State::COMMS_TX: {
      this->log_state_();
      this->indicate_transmission(true);
      if (buffers_.has_more_messages_to_send()) {
        send_dlms_messages_();
      } else {
        this->set_next_state_(State::COMMS_RX);
      }
    } break;

    case State::COMMS_RX: {
      this->log_state_();

      if (this->check_rx_timeout_()) {
        ESP_LOGE(TAG, "RX timeout.");
        this->has_error = true;

        this->indicate_connection(false);
        this->indicate_transmission(false);

        this->dlms_reading_state_.last_error = DLMS_ERROR_CODE_HARDWARE_FAULT;
        this->stats_.invalid_frames_ += reading_state_.err_invalid_frames;
        // if mission critical
        if (reading_state_.mission_critical) {
          ESP_LOGE(TAG, "Mission critical RX timeout.");
          this->abort_mission_();
        } else {
          // if not move forward
          reading_state_.err_invalid_frames++;
          this->set_next_state_(reading_state_.next_state);
        }
        return;
      }

      // the folowing basic algorithm to be implemented to read DLMS packet
      // first version, no retries
      // 1. receive proper hdlc frame
      // 2. get data from hdlc frame
      // 3. if ret = 0 or ret = DLMS_ERROR_CODE_FALSE then stop
      // 4. check reply->complete. if it is 0 then continue reading, go to 1
      //
      // read hdlc frame
      received_frame_size_ = this->receive_frame_hdlc_();

      if (received_frame_size_ == 0) {
        // keep reading until proper frame is received
        return;
      }

      this->update_last_rx_time_();

      // this->set_next_state_(reading_state_.next_state);

      auto ret = dlms_getData2(&dlms_settings_, &buffers_.in, &buffers_.reply, 0);
      if (ret != DLMS_ERROR_CODE_OK || buffers_.reply.complete == 0) {
        ESP_LOGVV(TAG, "dlms_getData2 ret = %d %s reply.complete = %d", ret, this->dlms_error_to_string(ret),
                  buffers_.reply.complete);
      }

      if (ret != DLMS_ERROR_CODE_OK && ret != DLMS_ERROR_CODE_FALSE) {
        ESP_LOGE(TAG, "dlms_getData2 failed. ret %d %s", ret, this->dlms_error_to_string(ret));
        this->reading_state_.err_invalid_frames++;
        this->set_next_state_(reading_state_.next_state);
        return;
      }

      if (buffers_.reply.complete == 0) {
        ESP_LOGD(TAG, "DLMS Reply not complete, need more HDLC frames. Continue reading.");
        // data in multiple frames.
        // we just keep reading until full reply is received.
        return; //keep reading
      }

      this->update_last_rx_time_();
      this->set_next_state_(reading_state_.next_state);

      auto parse_ret = this->dlms_reading_state_.parser_fn();
      this->dlms_reading_state_.last_error = parse_ret;

      if (parse_ret == DLMS_ERROR_CODE_OK) {
        //        ESP_LOGD(TAG, "DLSM parser fn result == DLMS_ERROR_CODE_OK");

      } else {
        ESP_LOGE(TAG, "DLMS parser fn error %d %s", this->dlms_error_to_string(parse_ret));

        // if not - just move forward
        if (reading_state_.mission_critical) {
          this->abort_mission_();
        }
        // if not critical - just move forward
        // set_next_state_(State::IDLE);
      }
    } break;

    case State::MISSION_FAILED: {
      //  this->send_frame_(CMD_CLOSE_SESSION, sizeof(CMD_CLOSE_SESSION));
      this->unlock_uart_session_();
      this->set_next_state_(State::IDLE);
      this->report_failure(true);
      this->stats_dump();
    } break;

    case State::OPEN_SESSION: {
      this->stats_.connections_tried_++;
      this->loop_state_.session_started_ms = millis();
      this->log_state_();
      this->clear_rx_buffers_();
      this->loop_state_.request_iter = this->sensors_.begin();

      this->set_next_state_(State::BUFFERS_REQ);

      // if (false) {
      //   // TODO. check if IEC handshake is needed

      //   uint8_t open_cmd[32]{0};
      //   uint8_t open_cmd_len = snprintf((char *) open_cmd, 32, "/?%s!\r\n",
      //   this->meter_address_.c_str()); request_iter = this->sensors_.begin();
      //   this->send_frame_(open_cmd, open_cmd_len);
      //   this->set_next_state_(State::OPEN_SESSION_GET_ID);
      //   auto read_fn = [this]() { return this->receive_frame_ascii_(); };
      //   // mission crit, no crc
      //   this->read_reply_and_go_next_state_(read_fn,
      //   State::OPEN_SESSION_GET_ID, 0, true, false);
      // }

    } break;

      // case State::OPEN_SESSION_GET_ID:
      //   this->log_state_();

      //   if (received_frame_size_) {
      //     char *id = nullptr;  //
      //     this->extract_meter_id_(received_frame_size_); if (id == nullptr) {
      //       ESP_LOGE(TAG, "Invalid meter identification frame");
      //       this->stats_.invalid_frames_++;
      //       this->abort_mission_();
      //       return;
      //     }

      //     this->update_last_rx_time_();
      //   }
      //   break;

    case State::BUFFERS_REQ: {
      this->log_state_();
      this->prepare_and_send_dlms_buffers();

    } break;

    case State::BUFFERS_RCV: {
      this->log_state_();
      // check the reply and go to next stage
      // todo smth with buffers reply
      this->set_next_state_(State::ASSOCIATION_REQ);

    } break;

    case State::ASSOCIATION_REQ: {
      this->log_state_();
      // this->start_comms_and_next(&aarq_rr_, State::ASSOCIATION_RCV);
      this->prepare_and_send_dlms_aarq();
    } break;

    case State::ASSOCIATION_RCV: {
      // check the reply and go to next stage
      // todo smth with aarq reply
      this->set_next_state_(State::DATA_ENQ_UNIT);
    } break;

    case State::DATA_ENQ_UNIT: {
      this->log_state_();
      if (this->loop_state_.request_iter == this->sensors_.end()) {
        ESP_LOGD(TAG, "All requests done");
        this->set_next_state_(State::SESSION_RELEASE);
        break;
      } else {
        auto req = this->loop_state_.request_iter->first;
        auto sens = this->loop_state_.request_iter->second;
        auto type = sens->get_obis_class();

        ESP_LOGD(TAG, "OBIS code: %s, Sensor: %s", req.c_str(), sens->get_sensor_name().c_str());

        // request units for numeric sensors only and only once
        if (sens->get_type() == SensorType::SENSOR && type == DLMS_OBJECT_TYPE_REGISTER &&
            !sens->has_got_scale_and_unit()) {
          // if (type == DLMS_OBJECT_TYPE_REGISTER)
          //        if (sens->get_attribute() != 2) {
          this->buffers_.gx_attribute = 3;
          this->prepare_and_send_dlms_data_unit_request(req.c_str(), type);
        } else {
          // units not working so far... so we are requesting just data
          this->set_next_state_(State::DATA_ENQ);
        }
      }
    } break;

    case State::DATA_ENQ: {
      this->log_state_();
      if (this->loop_state_.request_iter == this->sensors_.end()) {
        ESP_LOGD(TAG, "All requests done");
        this->set_next_state_(State::SESSION_RELEASE);
        break;
      } else {
        auto req = this->loop_state_.request_iter->first;
        auto sens = this->loop_state_.request_iter->second;
        auto type = sens->get_obis_class();
        auto units_were_requested = (sens->get_type() == SensorType::SENSOR && type == DLMS_OBJECT_TYPE_REGISTER &&
                                     !sens->has_got_scale_and_unit());
        if (units_were_requested) {
          auto ret = this->set_sensor_scale_and_unit(static_cast<DlmsCosemSensor *>(sens));
        }

        this->buffers_.gx_attribute = 2;
        this->prepare_and_send_dlms_data_request(req.c_str(), type, !units_were_requested);
      }
    } break;

    case State::DATA_RECV: {
      this->log_state_();
      this->set_next_state_(State::DATA_NEXT);

      auto req = this->loop_state_.request_iter->first;
      auto sens = this->loop_state_.request_iter->second;
      auto ret = this->set_sensor_value(sens, req.c_str());

    } break;

    case State::DATA_NEXT:
      this->log_state_();
      this->loop_state_.request_iter = this->sensors_.upper_bound(this->loop_state_.request_iter->first);
      if (this->loop_state_.request_iter != this->sensors_.end()) {
        this->set_next_state_delayed_(this->delay_between_requests_ms_, State::DATA_ENQ_UNIT);
      } else {
        this->set_next_state_delayed_(this->delay_between_requests_ms_, State::SESSION_RELEASE);
      }
      break;

    case State::SESSION_RELEASE:
      this->loop_state_.sensor_iter = this->sensors_.begin();

      this->log_state_();
      ESP_LOGD(TAG, "Session release request");
      if (this->auth_required_) {
        this->prepare_and_send_dlms_release();
        break;
      }
      // slip to next state if no auth required
      // no break;

    case State::DISCONNECT_REQ:
      this->log_state_();
      ESP_LOGD(TAG, "Disconnect request");
      this->prepare_and_send_dlms_disconnect();
      break;

    case State::PUBLISH:
      this->log_state_();
      ESP_LOGD(TAG, "Publishing data");
      this->update_last_rx_time_();

      if (this->loop_state_.sensor_iter != this->sensors_.end()) {
        if (this->loop_state_.sensor_iter->second->shall_we_publish()) {
          this->loop_state_.sensor_iter->second->publish();
        }
        this->loop_state_.sensor_iter++;
      } else {
        this->stats_dump();
        if (this->crc_errors_per_session_sensor_ != nullptr) {
          this->crc_errors_per_session_sensor_->publish_state(this->stats_.crc_errors_per_session());
        }
        this->report_failure(false);
        this->unlock_uart_session_();
        this->set_next_state_(State::IDLE);
        ESP_LOGD(TAG, "Total time: %u ms", millis() - this->loop_state_.session_started_ms);
      }
      break;

    default:
      break;
  }
}

void DlmsCosemComponent::update() {
  if (this->state_ != State::IDLE) {
    ESP_LOGD(TAG, "Starting data collection impossible - component not ready");
    return;
  }
  ESP_LOGD(TAG, "Starting data collection");
  this->has_error = false;
  this->set_next_state_(State::TRY_LOCK_BUS);
}

bool char2float(const char *str, float &value) {
  char *end;
  value = strtof(str, &end);
  return *end == '\0';
}

void DlmsCosemComponent::set_next_state_delayed_(uint32_t ms, State next_state) {
  if (ms == 0) {
    set_next_state_(next_state);
  } else {
    ESP_LOGV(TAG, "Short delay for %u ms", ms);
    set_next_state_(State::WAIT);
    wait_.start_time = millis();
    wait_.delay_ms = ms;
    wait_.next_state = next_state;
  }
}

void DlmsCosemComponent::InOutBuffers::init() {
  BYTE_BUFFER_INIT(&in);
  bb_capacity(&in, DEFAULT_IN_BUF_SIZE);
  mes_init(&out_msg);
  reply_init(&reply);
  this->reset();
}

void DlmsCosemComponent::InOutBuffers::reset() {
  mes_clear(&out_msg);
  reply_clear(&reply);
  reply.complete = 1;
  out_msg_index = 0;
  out_msg_data_pos = 0;
  in.size = 0;
  in.position = 0;
  //  amount_in = 0;
}

void DlmsCosemComponent::InOutBuffers::check_and_grow_input(uint16_t more_data) {
  const uint16_t GROW_EPSILON = 20;
  if (in.size + more_data > in.capacity) {
    ESP_LOGVV(TAG0, "Growing input buffer from %d to %d", in.capacity, in.size + more_data + GROW_EPSILON);
    bb_capacity(&in, in.size + more_data + GROW_EPSILON);
  }
}

void DlmsCosemComponent::prepare_and_send_dlms_buffers() {
  auto make = [this]() {
    ESP_LOGD(TAG0, "cl_snrmRequest %p ", this->buffers_.out_msg.data);
    return cl_snrmRequest(&this->dlms_settings_, &this->buffers_.out_msg);
  };
  auto parse = [this]() { return cl_parseUAResponse(&this->dlms_settings_, &this->buffers_.reply.data); };
  this->send_dlms_req_and_next(make, parse, State::BUFFERS_RCV, true);
}

void DlmsCosemComponent::prepare_and_send_dlms_aarq() {
  auto make = [this]() { return cl_aarqRequest(&this->dlms_settings_, &this->buffers_.out_msg); };
  auto parse = [this]() { return cl_parseAAREResponse(&this->dlms_settings_, &this->buffers_.reply.data); };
  this->send_dlms_req_and_next(make, parse, State::ASSOCIATION_RCV);
}

void DlmsCosemComponent::prepare_and_send_dlms_data_unit_request(const char *obis, int type) {
  auto ret = cosem_init(BASE(this->buffers_.gx_register), (DLMS_OBJECT_TYPE) type, obis);
  if (ret != DLMS_ERROR_CODE_OK) {
    ESP_LOGE(TAG, "cosem_init error %d '%s'", ret, this->dlms_error_to_string(ret));
    this->set_next_state_(State::DATA_ENQ);
    return;
  }

  auto make = [this]() {
    return cl_read(&this->dlms_settings_, BASE(this->buffers_.gx_register), this->buffers_.gx_attribute,
                   &this->buffers_.out_msg);
  };
  auto parse = [this]() {
    return cl_updateValue(&this->dlms_settings_, BASE(this->buffers_.gx_register), this->buffers_.gx_attribute,
                          &this->buffers_.reply.dataValue);
  };
  this->send_dlms_req_and_next(make, parse, State::DATA_ENQ, false, false);
}

void DlmsCosemComponent::prepare_and_send_dlms_data_request(const char *obis, int type, bool reg_init) {
  int ret = DLMS_ERROR_CODE_OK;
  if (reg_init) {
    ret = cosem_init(BASE(this->buffers_.gx_register), (DLMS_OBJECT_TYPE) type, obis);
  }
  if (ret != DLMS_ERROR_CODE_OK) {
    ESP_LOGE(TAG, "cosem_init error %d '%s'", ret, this->dlms_error_to_string(ret));
    this->set_next_state_(State::DATA_NEXT);
    return;
  }

  auto make = [this]() {
    return cl_read(&this->dlms_settings_, BASE(this->buffers_.gx_register), this->buffers_.gx_attribute,
                   &this->buffers_.out_msg);
  };
  auto parse = [this]() {
    return cl_updateValue(&this->dlms_settings_, BASE(this->buffers_.gx_register), this->buffers_.gx_attribute,
                          &this->buffers_.reply.dataValue);
  };
  this->send_dlms_req_and_next(make, parse, State::DATA_RECV);
}

void DlmsCosemComponent::prepare_and_send_dlms_release() {
  auto make = [this]() { return cl_releaseRequest(&this->dlms_settings_, &this->buffers_.out_msg); };
  auto parse = []() { return DLMS_ERROR_CODE_OK; };
  this->send_dlms_req_and_next(make, parse, State::DISCONNECT_REQ);
}

void DlmsCosemComponent::prepare_and_send_dlms_disconnect() {
  auto make = [this]() { return cl_disconnectRequest(&this->dlms_settings_, &this->buffers_.out_msg); };
  auto parse = []() { return DLMS_ERROR_CODE_OK; };
  this->send_dlms_req_and_next(make, parse, State::PUBLISH);
}

void DlmsCosemComponent::send_dlms_req_and_next(DlmsRequestMaker maker, DlmsResponseParser parser, State next_state,
                                                bool mission_critical, bool clear_buffer) {
  dlms_reading_state_.maker_fn = maker;
  dlms_reading_state_.parser_fn = parser;
  dlms_reading_state_.next_state = next_state;
  dlms_reading_state_.mission_critical = mission_critical;
  dlms_reading_state_.reply_is_complete = false;
  dlms_reading_state_.last_error = DLMS_ERROR_CODE_OK;

  // if (clear_buffer) {
  buffers_.reset();
  // }
  int ret = DLMS_ERROR_CODE_OK;
  if (maker != nullptr) {
    ret = maker();
    if (ret != DLMS_ERROR_CODE_OK) {
      ESP_LOGE(TAG, "Error in DLSM request maker function %d '%s'", ret, dlms_error_to_string(ret));
      this->set_next_state_(State::IDLE);
      return;
    }
  }

  reading_state_ = {};
  //  reading_state_.read_fn = read_fn;
  reading_state_.mission_critical = mission_critical;
  reading_state_.tries_max = 1;  // retries;
  reading_state_.tries_counter = 0;
  //  reading_state_.check_crc = check_crc;
  reading_state_.next_state = next_state;
  received_frame_size_ = 0;

  received_complete_reply_ = false;

  set_next_state_(State::COMMS_TX);
}

int DlmsCosemComponent::set_sensor_scale_and_unit(DlmsCosemSensor *sensor) {
  ESP_LOGD(TAG, "set_sensor_scale_and_unit");
  if (!buffers_.reply.complete)
    return DLMS_ERROR_CODE_FALSE;
  auto vt = buffers_.reply.dataType;
  ESP_LOGD(TAG, "DLMS_DATA_TYPE: %s (%d)", this->dlms_data_type_to_string(vt), vt);
  if (vt != 0) {
    return DLMS_ERROR_CODE_FALSE;
  }

  auto scal = this->buffers_.gx_register.scaler;
  auto unit = this->buffers_.gx_register.unit;
  auto unit_s = obj_getUnitAsString(unit);
  sensor->set_scale_and_unit(scal, unit, unit_s);

  return DLMS_ERROR_CODE_OK;
}

int DlmsCosemComponent::set_sensor_value(DlmsCosemSensorBase *sensor, const char *obis) {
  if (!buffers_.reply.complete || !sensor->shall_we_publish()) {
    return this->dlms_reading_state_.last_error;
  }

  auto vt = buffers_.reply.dataType;
  ESP_LOGD(TAG, "OBIS code: %s, DLMS_DATA_TYPE: %s (%d)", obis, this->dlms_data_type_to_string(vt), vt);

  //      if (cosem_rr_.result().has_value()) {
  if (this->dlms_reading_state_.last_error == DLMS_ERROR_CODE_OK) {
    // result is okay, value shall be there

    if (sensor->get_type() == SensorType::SENSOR) {
      if (sensor->get_obis_class() == DLMS_OBJECT_TYPE_REGISTER) {
        auto var = &this->buffers_.gx_register.value;
        auto scale = static_cast<DlmsCosemSensor *>(sensor)->get_scale();
        auto unit = static_cast<DlmsCosemSensor *>(sensor)->get_unit();
        if (vt == DLMS_DATA_TYPE_FLOAT32 || vt == DLMS_DATA_TYPE_FLOAT64) {
          float val = var_toDouble(var);
          ESP_LOGD(TAG, "OBIS code: %s, Value: %f, Scale: %f, Unit: %s", obis, val, scale, unit);
          static_cast<DlmsCosemSensor *>(sensor)->set_value(val);
        } else {
          int val = var_toInteger(var);
          ESP_LOGD(TAG, "OBIS code: %s, Value: %d, Scale: %f, Unit: %s", obis, val, scale, unit);
          static_cast<DlmsCosemSensor *>(sensor)->set_value(val);
        }
      } else {
        ESP_LOGW(TAG, "Wrong OBIS class. Regular numberic sensors can only handle Registers (class = 3)");
      }
    }

#ifdef USE_TEXT_SENSOR
    if (sensor->get_type() == SensorType::TEXT_SENSOR) {
      auto var = &this->buffers_.gx_register.value;
      if (var && var->byteArr && var->byteArr->size > 0) {
        auto arr = var->byteArr;

        ESP_LOGV(TAG, "data size=%d", arr->size);

        bb_setInt8(arr, 0);     // add null-termination
        if (arr->size > 128) {  // clip the string
          ESP_LOGW(TAG, "String is too long %d, clipping to 128 bytes", arr->size);
          arr->data[127] = '\0';
        }
        ESP_LOGV(TAG, "DATA: %s", format_hex_pretty(arr->data, arr->size).c_str());
        auto type = sensor->get_obis_class();

        if (type == DLMS_OBJECT_TYPE_DATA) {
          static_cast<DlmsCosemTextSensor *>(sensor)->set_value(reinterpret_cast<const char *>(arr->data),
                                                                this->cp1251_conversion_required_);
        } else if (type == DLMS_OBJECT_TYPE_CLOCK) {
          ESP_LOGD(TAG, "Clock sensor");
        }
      }
    }
#endif
  } else {
    ESP_LOGD(TAG, "OBIS code: %s, result != DLMS_ERROR_CODE_OK = %d", obis, this->dlms_reading_state_.last_error);
  }
  return this->dlms_reading_state_.last_error;
}

void DlmsCosemComponent::indicate_transmission(bool transmission_on) {
#ifdef USE_BINARY_SENSOR
  if (this->transmission_binary_sensor_) {
    this->transmission_binary_sensor_->publish_state(transmission_on);
  }
#endif
}

void DlmsCosemComponent::indicate_session(bool session_on) {
#ifdef USE_BINARY_SENSOR
  if (this->session_binary_sensor_) {
    this->session_binary_sensor_->publish_state(session_on);
  }
#endif
}

void DlmsCosemComponent::indicate_connection(bool connection_on) {
#ifdef USE_BINARY_SENSOR
  if (this->connection_binary_sensor_) {
    this->connection_binary_sensor_->publish_state(connection_on);
  }
#endif
}

void DlmsCosemComponent::send_dlms_messages_() {
  const int MAX_BYTES_IN_ONE_SHOT = 64;
  gxByteBuffer *buffer = buffers_.out_msg.data[buffers_.out_msg_index];

  int bytes_to_send = buffer->size - buffers_.out_msg_data_pos;
  if (bytes_to_send > 0) {
    if (bytes_to_send > MAX_BYTES_IN_ONE_SHOT)
      bytes_to_send = MAX_BYTES_IN_ONE_SHOT;

    if (this->flow_control_pin_ != nullptr)
      this->flow_control_pin_->digital_write(true);

    this->write_array(buffer->data + buffers_.out_msg_data_pos, bytes_to_send);

    if (this->flow_control_pin_ != nullptr)
      this->flow_control_pin_->digital_write(false);

    ESP_LOGVV(TAG, "TX: %s", format_hex_pretty(buffer->data + buffers_.out_msg_data_pos, bytes_to_send).c_str());

    this->update_last_rx_time_();
    buffers_.out_msg_data_pos += bytes_to_send;
  }
  if (buffers_.out_msg_data_pos >= buffer->size) {
    buffers_.out_msg_index++;
  }
}

size_t DlmsCosemComponent::receive_frame_(FrameStopFunction stop_fn) {
  const uint32_t read_time_limit_ms = 45;
  size_t ret_val;

  auto count_available = this->available();
  if (count_available <= 0)
    return 0;

  uint32_t read_start = millis();
  uint8_t *p;

  // ESP_LOGVV(TAG, "avail RX: %d", count_available);
  buffers_.check_and_grow_input(count_available);

  while (count_available-- > 0) {
    if (millis() - read_start > read_time_limit_ms) {
      return 0;
    }

    p = &this->buffers_.in.data[this->buffers_.in.size];
    if (!iuart_->read_one_byte(p)) {
      return 0;
    }
    this->buffers_.in.size++;
    // this->buffers_.amount_in++;

    if (stop_fn(this->buffers_.in.data, this->buffers_.in.size)) {
      //      ESP_LOGVV(TAG, "RX: %s", format_frame_pretty(this->buffers_.in.data, this->buffers_.in.size).c_str());
      ESP_LOGVV(TAG, "RX: %s", format_hex_pretty(this->buffers_.in.data, this->buffers_.in.size).c_str());
      ret_val = this->buffers_.in.size;

      // this->buffers_.amount_in = 0;
      this->update_last_rx_time_();
      return ret_val;
    }

    yield();
    App.feed_wdt();
  }
  return 0;
}

size_t DlmsCosemComponent::receive_frame_hdlc_() {
  // HDLC frame: <FLAG>data<FLAG>
  auto frame_end_check_hdlc = [](uint8_t *b, size_t s) {
    auto ret = s >= 2 && b[0] == HDLC_FLAG && b[s - 1] == HDLC_FLAG;
    // if (ret) {
    //   ESP_LOGVV(TAG, "Frame HDLC Stop");
    // }
    return ret;
  };
  return receive_frame_(frame_end_check_hdlc);
}

#ifdef IEC_HANDSHAKE
size_t DlmsCosemComponent::receive_frame_ascii_() {
  // "data<CR><LF>"
  ESP_LOGVV(TAG, "Waiting for ASCII frame");
  auto frame_end_check_crlf = [](uint8_t *b, size_t s) {
    auto ret = s >= 2 && b[s - 1] == '\n' && b[s - 2] == '\r';
    if (ret) {
      ESP_LOGVV(TAG, "Frame CRLF Stop");
    }
    return ret;
  };
  return receive_frame_(frame_end_check_crlf);
}
#endif

void DlmsCosemComponent::clear_rx_buffers_() {
  int available = this->available();
  if (available > 0) {
    ESP_LOGVV(TAG, "Cleaning garbage from UART input buffer: %d bytes", available);
  }

  int len;
  while (available > 0) {
    len = std::min(available, (int) buffers_.in.capacity);
    this->read_array(this->buffers_.in.data, len);
    available -= len;
  }
  memset(this->buffers_.in.data, 0, buffers_.in.capacity);
  this->buffers_.in.size = 0;
}

const char *DlmsCosemComponent::state_to_string(State state) {
  switch (state) {
    case State::NOT_INITIALIZED:
      return "NOT_INITIALIZED";
    case State::IDLE:
      return "IDLE";
    case State::TRY_LOCK_BUS:
      return "TRY_LOCK_BUS";
    case State::WAIT:
      return "WAIT";
    case State::COMMS_TX:
      return "COMMS_TX";
    case State::COMMS_RX:
      return "COMMS_RX";
    case State::MISSION_FAILED:
      return "MISSION_FAILED";
    case State::OPEN_SESSION:
      return "OPEN_SESSION";
    case State::BUFFERS_REQ:
      return "BUFFERS_REQ";
    case State::BUFFERS_RCV:
      return "BUFFERS_RCV";
    case State::ASSOCIATION_REQ:
      return "ASSOCIATION_REQ";
    case State::ASSOCIATION_RCV:
      return "ASSOCIATION_RCV";
    case State::DATA_ENQ_UNIT:
      return "DATA_ENQ_UNIT";
    case State::DATA_ENQ:
      return "DATA_ENQ";
    case State::DATA_RECV:
      return "DATA_RECV";
    case State::DATA_NEXT:
      return "DATA_NEXT";
    case State::SESSION_RELEASE:
      return "SESSION_RELEASE";
    case State::DISCONNECT_REQ:
      return "DISCONNECT_REQ";
    case State::PUBLISH:
      return "PUBLISH";
    default:
      return "UNKNOWN";
  }
}

void DlmsCosemComponent::log_state_(State *next_state) {
  if (this->state_ != this->last_reported_state_) {
    if (next_state == nullptr) {
      ESP_LOGV(TAG, "State::%s", state_to_string(this->state_));
    } else {
      ESP_LOGV(TAG, "State::%s -> %s", state_to_string(this->state_), state_to_string(*next_state));
    }
    this->last_reported_state_ = this->state_;
  }
}

void DlmsCosemComponent::stats_dump() {
  ESP_LOGV(TAG, "============================================");
  ESP_LOGV(TAG, "Data collection and publishing finished.");
  ESP_LOGV(TAG, "Total number of sessions ............. %u", this->stats_.connections_tried_);
  ESP_LOGV(TAG, "Total number of invalid frames ....... %u", this->stats_.invalid_frames_);
  ESP_LOGV(TAG, "Total number of CRC errors ........... %u", this->stats_.crc_errors_);
  ESP_LOGV(TAG, "Total number of CRC errors recovered . %u", this->stats_.crc_errors_recovered_);
  ESP_LOGV(TAG, "CRC errors per session ............... %f", this->stats_.crc_errors_per_session());
  ESP_LOGV(TAG, "Number of failures ................... %u", this->stats_.failures_);
  ESP_LOGV(TAG, "============================================");
}

const char *DlmsCosemComponent::dlms_error_to_string(int error) {
  switch (error) {
    case DLMS_ERROR_CODE_OK:
      return "DLMS_ERROR_CODE_OK";
    case DLMS_ERROR_CODE_HARDWARE_FAULT:
      return "DLMS_ERROR_CODE_HARDWARE_FAULT";
    case DLMS_ERROR_CODE_TEMPORARY_FAILURE:
      return "DLMS_ERROR_CODE_TEMPORARY_FAILURE";
    case DLMS_ERROR_CODE_READ_WRITE_DENIED:
      return "DLMS_ERROR_CODE_READ_WRITE_DENIED";
    case DLMS_ERROR_CODE_UNDEFINED_OBJECT:
      return "DLMS_ERROR_CODE_UNDEFINED_OBJECT";
    case DLMS_ERROR_CODE_ACCESS_VIOLATED:
      return "DLMS_ERROR_CODE_ACCESS_VIOLATED";
    default:
      return "";
  }
}

const char *DlmsCosemComponent::dlms_data_type_to_string(DLMS_DATA_TYPE vt) {
  switch (vt) {
    case DLMS_DATA_TYPE_NONE:
      return "DLMS_DATA_TYPE_NONE";
    case DLMS_DATA_TYPE_BOOLEAN:
      return "DLMS_DATA_TYPE_BOOLEAN";
    case DLMS_DATA_TYPE_BIT_STRING:
      return "DLMS_DATA_TYPE_BIT_STRING";
    case DLMS_DATA_TYPE_INT32:
      return "DLMS_DATA_TYPE_INT32";
    case DLMS_DATA_TYPE_UINT32:
      return "DLMS_DATA_TYPE_UINT32";
    case DLMS_DATA_TYPE_OCTET_STRING:
      return "DLMS_DATA_TYPE_OCTET_STRING";
    case DLMS_DATA_TYPE_STRING:
      return "DLMS_DATA_TYPE_STRING";
    case DLMS_DATA_TYPE_BINARY_CODED_DESIMAL:
      return "DLMS_DATA_TYPE_BINARY_CODED_DESIMAL";
    case DLMS_DATA_TYPE_STRING_UTF8:
      return "DLMS_DATA_TYPE_STRING_UTF8";
    case DLMS_DATA_TYPE_INT8:
      return "DLMS_DATA_TYPE_INT8";
    case DLMS_DATA_TYPE_INT16:
      return "DLMS_DATA_TYPE_INT16";
    case DLMS_DATA_TYPE_UINT8:
      return "DLMS_DATA_TYPE_UINT8";
    case DLMS_DATA_TYPE_UINT16:
      return "DLMS_DATA_TYPE_UINT16";
    case DLMS_DATA_TYPE_INT64:
      return "DLMS_DATA_TYPE_INT64";
    case DLMS_DATA_TYPE_UINT64:
      return "DLMS_DATA_TYPE_UINT64";
    case DLMS_DATA_TYPE_ENUM:
      return "DLMS_DATA_TYPE_ENUM";
    case DLMS_DATA_TYPE_FLOAT32:
      return "DLMS_DATA_TYPE_FLOAT32";
    case DLMS_DATA_TYPE_FLOAT64:
      return "DLMS_DATA_TYPE_FLOAT64";
    case DLMS_DATA_TYPE_DATETIME:
      return "DLMS_DATA_TYPE_DATETIME";
    case DLMS_DATA_TYPE_DATE:
      return "DLMS_DATA_TYPE_DATE";
    case DLMS_DATA_TYPE_TIME:
      return "DLMS_DATA_TYPE_TIME";
    case DLMS_DATA_TYPE_ARRAY:
      return "DLMS_DATA_TYPE_ARRAY";
    case DLMS_DATA_TYPE_STRUCTURE:
      return "DLMS_DATA_TYPE_STRUCTURE";
    case DLMS_DATA_TYPE_COMPACT_ARRAY:
      return "DLMS_DATA_TYPE_COMPACT_ARRAY";
    case DLMS_DATA_TYPE_BYREF:
      return "DLMS_DATA_TYPE_BYREF";
    default:
      return "DMS_DATA_TYPE UNKNOWN";
  }
}

bool DlmsCosemComponent::try_lock_uart_session_() {
  if (AnyObjectLocker::try_lock(this->parent_)) {
    ESP_LOGV(TAG, "UART bus %p locked by %s", this->parent_, this->tag_.c_str());
    return true;
  }
  ESP_LOGV(TAG, "UART bus %p busy", this->parent_);
  return false;
}

void DlmsCosemComponent::unlock_uart_session_() {
  AnyObjectLocker::unlock(this->parent_);
  ESP_LOGV(TAG, "UART bus %p released by %s", this->parent_, this->tag_.c_str());
}

uint8_t DlmsCosemComponent::next_obj_id_ = 0;

std::string DlmsCosemComponent::generateTag() { return str_sprintf("%s%03d", TAG0, ++next_obj_id_); }

}  // namespace dlms_cosem
}  // namespace esphome
