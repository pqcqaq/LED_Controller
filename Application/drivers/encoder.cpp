/**
 * @file encoder.cpp
 * @brief Object-Oriented Rotary Encoder Driver Implementation
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "encoder.h"
#include "usart.h"
#include "utils/custom_types.h"
#include <stdio.h>

/* Public methods ------------------------------------------------------------*/

/**
 * @brief Constructor
 */
RotaryEncoder::RotaryEncoder(GPIO_TypeDef *pin_a_port, uint16_t pin_a,
                             GPIO_TypeDef *pin_b_port, uint16_t pin_b)
    : pin_a_port_(pin_a_port), pin_a_(pin_a), pin_b_port_(pin_b_port),
      pin_b_(pin_b), prev_next_code_(0), state_store_(0), position_(0),
      delta_(0), last_rotation_time_(0), rotation_active_(false),
      interval_index_(0), current_speed_(ENCODER_SPEED_SLOW),
      acceleration_enabled_(false), acceleration_threshold_ms_(100),
      acceleration_factor_(2),
      debounce_time_ms_(ENCODER_DEFAULT_DEBOUNCE_TIME_MS), reversed_(false),
      interrupt_mode_enabled_(false) {
  // Initialize rotation intervals array
  for (int i = 0; i < 4; i++) {
    rotation_intervals_[i] = 0;
  }
}

/**
 * @brief Initialize encoder
 */
void RotaryEncoder::init() {
  // Reset all state variables
  prev_next_code_ = 0;
  state_store_ = 0;
  position_ = 0;
  delta_ = 0;
  last_rotation_time_ = 0;
  rotation_active_ = false;

  interval_index_ = 0;
  current_speed_ = ENCODER_SPEED_SLOW;

  for (int i = 0; i < 4; i++) {
    rotation_intervals_[i] = 0;
  }

  serial_printf("RotaryEncoder: Initialized\r\n");
  serial_printf("  Pin A: Port %p Pin %u\r\n", pin_a_port_, pin_a_);
  serial_printf("  Pin B: Port %p Pin %u\r\n", pin_b_port_, pin_b_);
}

/**
 * @brief Process encoder events
 */
void RotaryEncoder::process() {
  if (interrupt_mode_enabled_) {
    // In interrupt mode, only handle timeouts and speed updates
    uint32_t current_time = HAL_GetTick();
    if (rotation_active_ && (current_time - last_rotation_time_ > 500)) {
      rotation_active_ = false;
      current_speed_ = ENCODER_SPEED_SLOW;
    }
  } else {
    // In polling mode, do full processing
    int8_t rotation_result = processRotation();

    if (rotation_result != 0) {
      handleRotation(rotation_result);
    } else {
      // Check for rotation timeout
      uint32_t current_time = HAL_GetTick();
      if (rotation_active_ && (current_time - last_rotation_time_ > 500)) {
        rotation_active_ = false;
        current_speed_ = ENCODER_SPEED_SLOW;
      }
    }
  }
}

/**
 * @brief Enable interrupt-driven mode
 */
void RotaryEncoder::setInterruptMode(bool enabled) {
  interrupt_mode_enabled_ = enabled;

  if (enabled) {
    serial_printf("RotaryEncoder: Interrupt mode enabled\r\n");
  } else {
    serial_printf("RotaryEncoder: Polling mode enabled\r\n");
  }
}

/**
 * @brief Set general event callback
 */
void RotaryEncoder::setEventCallback(EventCallback callback) {
  event_callback_ = callback;
}

/**
 * @brief Set rotation-specific callback
 */
void RotaryEncoder::setRotationCallback(RotationCallback callback) {
  rotation_callback_ = callback;
}

/**
 * @brief Set position change callback
 */
void RotaryEncoder::setPositionCallback(PositionCallback callback) {
  position_callback_ = callback;
}

/**
 * @brief Get current encoder position
 */
int32_t RotaryEncoder::getPosition() const { return position_; }

/**
 * @brief Get encoder delta since last read
 */
int32_t RotaryEncoder::getDelta() {
  int32_t delta = delta_;
  delta_ = 0; // Reset delta after reading
  return delta;
}

/**
 * @brief Reset encoder position to zero
 */
void RotaryEncoder::resetPosition() {
  position_ = 0;
  delta_ = 0;
}

/**
 * @brief Set encoder position to specific value
 */
void RotaryEncoder::setPosition(int32_t position) { position_ = position; }

/**
 * @brief Get current rotation speed
 */
EncoderSpeed_t RotaryEncoder::getSpeed() const { return current_speed_; }

/**
 * @brief Enable/disable acceleration feature
 */
void RotaryEncoder::setAcceleration(bool enabled, uint32_t threshold_ms,
                                    uint8_t factor) {
  acceleration_enabled_ = enabled;
  acceleration_threshold_ms_ = threshold_ms;
  acceleration_factor_ = (factor < 2) ? 2 : ((factor > 10) ? 10 : factor);

  serial_printf(
      "RotaryEncoder: Acceleration %s (threshold: %u ms, factor: %u)\r\n",
      enabled ? "enabled" : "disabled", (unsigned int)threshold_ms,
      acceleration_factor_);
}

/**
 * @brief Reverse encoder direction
 */
void RotaryEncoder::setReversed(bool reversed) {
  reversed_ = reversed;
  serial_printf("RotaryEncoder: Direction %s\r\n",
                reversed ? "reversed" : "normal");
}

/**
 * @brief Set debounce time
 */
void RotaryEncoder::setDebounceTime(uint32_t time_ms) {
  debounce_time_ms_ = time_ms;
  serial_printf("RotaryEncoder: Debounce time set to %u ms\r\n",
                (unsigned int)time_ms);
}

/**
 * @brief GPIO EXTI callback
 */
void RotaryEncoder::onGpioInterrupt(uint16_t gpio_pin) {
  // Check if this interrupt is for our pins
  if (gpio_pin == pin_a_ || gpio_pin == pin_b_) {
    if (interrupt_mode_enabled_) {
      // In interrupt mode, process rotation immediately
      uint32_t current_time = HAL_GetTick();
      if (current_time - last_rotation_time_ > debounce_time_ms_) {
        int8_t rotation_result = processRotation();
        if (rotation_result != 0) {
          handleRotation(rotation_result);
        }
      }
    } else {
      // In polling mode, just update timestamp for debouncing
      uint32_t current_time = HAL_GetTick();
      if (current_time - last_rotation_time_ > debounce_time_ms_) {
        // Mark for processing in main loop
      }
    }
  }
}

/* Private methods -----------------------------------------------------------*/

/**
 * @brief Read encoder pin state
 */
bool RotaryEncoder::readPin(GPIO_TypeDef *port, uint16_t pin) const {
  return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET;
}

/**
 * @brief Process encoder rotation using robust table decode method
 * This method uses the improved table decode algorithm for stable operation
 */
int8_t RotaryEncoder::processRotation() {
  // Robust rotary encoder table for quadrature decoding
  // This table handles switch bounce and invalid states
  static const int8_t rot_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1,
                                         1, 0, 0, 1, 0, 1, 1, 0};

  // Read current pin states
  bool pin_a_state = readPin(pin_a_port_, pin_a_);
  bool pin_b_state = readPin(pin_b_port_, pin_b_);

  // Build current state (shift previous state and add new state)
  prev_next_code_ <<= 2;
  if (pin_b_state)
    prev_next_code_ |= 0x02; // Pin B is MSB
  if (pin_a_state)
    prev_next_code_ |= 0x01; // Pin A is LSB
  prev_next_code_ &= 0x0f;   // Keep only 4 bits

  // Check if this is a valid state transition
  if (rot_enc_table[prev_next_code_]) {
    // Valid transition - store in 16-bit state history
    state_store_ <<= 4;
    state_store_ |= prev_next_code_;

    // Check for complete rotation patterns
    // These patterns indicate a complete detent-to-detent movement
    if ((state_store_ & 0xff) == 0x2b) {
      return -1; // Counter-clockwise
    }
    if ((state_store_ & 0xff) == 0x17) {
      return 1; // Clockwise
    }
  }

  return 0; // No valid rotation detected
}

/**
 * @brief Process button state machine
 */

/**
 * @brief Handle rotation result (common logic for both modes)
 */
void RotaryEncoder::handleRotation(int8_t rotation_result) {
  uint32_t current_time = HAL_GetTick();

  // Update speed detection
  updateSpeed();

  // Calculate steps with acceleration if enabled
  int32_t steps = 1;
  if (acceleration_enabled_) {
    steps *= calculateAcceleration();
  }

  // Apply direction reversal if configured
  EncoderDirection_t direction =
      (rotation_result > 0) ? ENCODER_DIR_CW : ENCODER_DIR_CCW;
  if (reversed_) {
    direction =
        (direction == ENCODER_DIR_CW) ? ENCODER_DIR_CCW : ENCODER_DIR_CW;
    rotation_result = -rotation_result;
  }

  // Update position and delta
  position_ += (rotation_result * steps);
  delta_ += (rotation_result * steps);

  // Trigger callbacks
  if (direction == ENCODER_DIR_CW) {
    triggerEvent(ENCODER_EVENT_ROTATE_CW, direction, steps);
  } else {
    triggerEvent(ENCODER_EVENT_ROTATE_CCW, direction, steps);
  }

  triggerRotation(direction, steps, current_speed_);
  triggerPosition();

  last_rotation_time_ = current_time;
  rotation_active_ = true;
}

/**
 * @brief Update rotation speed based on timing
 */
void RotaryEncoder::updateSpeed() {
  uint32_t current_time = HAL_GetTick();

  if (last_rotation_time_ != 0) {
    uint32_t interval = current_time - last_rotation_time_;

    // Store interval in circular buffer
    rotation_intervals_[interval_index_] = interval;
    interval_index_ = (interval_index_ + 1) % 4;

    // Calculate average interval
    uint32_t avg_interval = 0;
    uint8_t valid_intervals = 0;
    for (int i = 0; i < 4; i++) {
      if (rotation_intervals_[i] > 0) {
        avg_interval += rotation_intervals_[i];
        valid_intervals++;
      }
    }

    if (valid_intervals > 0) {
      avg_interval /= valid_intervals;

      // Determine speed based on average interval
      if (avg_interval <= this->acceleration_threshold_ms_) {
        current_speed_ = ENCODER_SPEED_FAST;
      } else if (avg_interval <= this->acceleration_threshold_ms_ * 2) {
        current_speed_ = ENCODER_SPEED_MEDIUM;
      } else {
        current_speed_ = ENCODER_SPEED_SLOW;
      }
    }
  }
}

/**
 * @brief Calculate acceleration factor based on speed
 */
uint8_t RotaryEncoder::calculateAcceleration() const {
  if (!acceleration_enabled_)
    return 1;

  switch (current_speed_) {
  case ENCODER_SPEED_FAST:
    return acceleration_factor_ * 2;
  case ENCODER_SPEED_MEDIUM:
    return acceleration_factor_;
  case ENCODER_SPEED_SLOW:
  default:
    return 1;
  }
}

/**
 * @brief Trigger event callback
 */
void RotaryEncoder::triggerEvent(EncoderEvent_t event,
                                 EncoderDirection_t direction, int32_t steps) {
  if (event_callback_) {
    event_callback_(event, direction, steps);
  }
}

/**
 * @brief Trigger rotation callback
 */
void RotaryEncoder::triggerRotation(EncoderDirection_t direction, int32_t steps,
                                    EncoderSpeed_t speed) {
  if (rotation_callback_) {
    rotation_callback_(direction, steps, speed);
  }
}

/**
 * @brief Trigger position callback
 */
void RotaryEncoder::triggerPosition() {
  if (position_callback_) {
    position_callback_(position_, delta_);
  }
}
