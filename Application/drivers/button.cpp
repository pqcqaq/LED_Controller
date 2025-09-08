/**
 * @file button.cpp
 * @brief Object-Oriented Button Driver Implementation
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "button.h"
#include "usart.h"
#include "utils/custom_types.h"
#include <stdio.h>

/* Public methods ------------------------------------------------------------*/

/**
 * @brief Constructor
 */
Button::Button(GPIO_TypeDef *port, uint16_t pin, bool active_low)
    : port_(port), pin_(pin), active_low_(active_low),
      current_state_(BUTTON_STATE_RELEASED), last_state_(BUTTON_STATE_RELEASED),
      last_change_time_(0), press_start_time_(0), debounce_active_(false),
      long_press_triggered_(false), click_count_(0), last_click_time_(0),
      multi_click_gap_ms_(BUTTON_DEFAULT_MULTI_CLICK_GAP_MS),
      multi_click_pending_(false),
      long_press_time_ms_(BUTTON_DEFAULT_LONG_PRESS_TIME_MS),
      long_press_enabled_(false), long_press_continuous_(false),
      long_press_interval_ms_(BUTTON_DEFAULT_LONG_PRESS_INTERVAL_MS),
      last_long_press_time_(0), max_multi_clicks_(2),
      multi_click_enabled_(false), interrupt_mode_enabled_(false) {}

/**
 * @brief Initialize button
 */
void Button::init() {
  // Reset all state variables
  current_state_ = BUTTON_STATE_RELEASED;
  last_state_ = BUTTON_STATE_RELEASED;
  last_change_time_ = 0;
  press_start_time_ = 0;
  debounce_active_ = false;
  long_press_triggered_ = false;
  last_long_press_time_ = 0;
  click_count_ = 0;
  last_click_time_ = 0;
  multi_click_pending_ = false;

  serial_printf("Button: Initialized on Port %p Pin %u (Active %s)\r\n", port_,
                pin_, active_low_ ? "Low" : "High");
}

/**
 * @brief Process button events
 */
void Button::process() {
  if (interrupt_mode_enabled_) {
    // In interrupt mode, only process multi-click timeout and long press
    // detection
    processMultiClick();
    checkLongPress();
  } else {
    // In polling mode, do full state machine processing
    stateMachine();
    processMultiClick();
  }
}

/**
 * @brief Enable interrupt-driven mode
 */
void Button::setInterruptMode(bool enabled) {
  interrupt_mode_enabled_ = enabled;

  if (enabled) {
    serial_printf("Button: Interrupt mode enabled\r\n");
  } else {
    serial_printf("Button: Polling mode enabled\r\n");
  }
}

/**
 * @brief Set general event callback
 */
void Button::setEventCallback(EventCallback callback) {
  event_callback_ = callback;
}

/**
 * @brief Register click callback
 */
void Button::handleClick(ClickCallback callback) { click_callback_ = callback; }

/**
 * @brief Register long press callback
 */
void Button::handleLongPress(LongPressCallback callback, uint32_t time_ms) {
  long_press_callback_ = callback;
  long_press_time_ms_ = time_ms;
  long_press_enabled_ = true;

  serial_printf("Button: Long press enabled (%u ms)\r\n",
                (unsigned int)time_ms);
}

/**
 * @brief Set continuous long press mode
 */
void Button::setContinuousLongPress(bool continuous, uint32_t interval_ms) {
  long_press_continuous_ = continuous;
  long_press_interval_ms_ = interval_ms;

  serial_printf("Button: Continuous long press %s (interval %u ms)\r\n",
                continuous ? "enabled" : "disabled", (unsigned int)interval_ms);
}

/**
 * @brief Register multi-click callback
 */
void Button::handleMultiClick(uint8_t max_clicks, MultiClickCallback callback,
                              uint32_t gap_ms) {
  multi_click_callback_ = callback;
  max_multi_clicks_ = max_clicks;
  multi_click_gap_ms_ = gap_ms;
  multi_click_enabled_ = true;

  serial_printf("Button: Multi-click enabled (max %u clicks, gap %u ms)\r\n",
                max_clicks, (unsigned int)gap_ms);
}

/**
 * @brief Get current button state
 */
ButtonState_t Button::getState() const { return current_state_; }

/**
 * @brief Check if button is currently pressed
 */
bool Button::isPressed() const {
  return current_state_ != BUTTON_STATE_RELEASED;
}

/**
 * @brief Get press duration for current press
 */
uint32_t Button::getPressDuration() const {
  if (current_state_ == BUTTON_STATE_RELEASED) {
    return 0;
  }
  return HAL_GetTick() - press_start_time_;
}

/**
 * @brief Disable long press detection
 */
void Button::disableLongPress() {
  long_press_enabled_ = false;
  long_press_continuous_ = false;
  long_press_callback_ = nullptr;
}

/**
 * @brief Disable multi-click detection
 */
void Button::disableMultiClick() {
  multi_click_enabled_ = false;
  multi_click_callback_ = nullptr;
  click_count_ = 0;
  multi_click_pending_ = false;
}

/**
 * @brief GPIO EXTI callback
 */
void Button::onGpioInterrupt() {
  uint32_t current_time = HAL_GetTick();

  if (interrupt_mode_enabled_) {
    // In interrupt mode, do immediate state processing
    if (current_time - last_change_time_ > BUTTON_DEBOUNCE_TIME_MS) {
      bool current_pressed = readPin();

      ButtonState_t new_state =
          current_pressed ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;

      if (new_state != current_state_) {
        last_state_ = current_state_;
        current_state_ = new_state;
        last_change_time_ = current_time;

        if (current_pressed) {
          // Button pressed
          press_start_time_ = current_time;
          long_press_triggered_ = false;
          last_long_press_time_ = 0;
          triggerEvent(BUTTON_EVENT_PRESS);
        } else {
          // Button released
          uint32_t press_duration = current_time - press_start_time_;
          triggerEvent(BUTTON_EVENT_RELEASE);

          // Handle click detection
          if (press_duration < long_press_time_ms_) {
            if (multi_click_enabled_) {
              // Multi-click mode
              click_count_++;
              last_click_time_ = current_time;
              multi_click_pending_ = true;
            } else {
              // Single click mode
              triggerEvent(BUTTON_EVENT_CLICK);
            }
          }
        }
      }
    }
  } else {
    // In polling mode, just update timestamp for debouncing
    if (current_time - last_change_time_ > BUTTON_DEBOUNCE_TIME_MS) {
      last_change_time_ = current_time;
    }
  }
}

/* Private methods -----------------------------------------------------------*/

/**
 * @brief Read GPIO pin state
 */
bool Button::readPin() const {
  bool pin_high = (HAL_GPIO_ReadPin(port_, pin_) == GPIO_PIN_SET);
  return active_low_ ? !pin_high : pin_high;
}

/**
 * @brief Handle button state machine
 */
void Button::stateMachine() {
  uint32_t current_time = HAL_GetTick();
  bool current_pin_state = readPin();

  // Handle debouncing
  if (debounce_active_) {
    if (current_time - last_change_time_ >= BUTTON_DEBOUNCE_TIME_MS) {
      debounce_active_ = false;
    } else {
      return; // Still in debounce period
    }
  }

  ButtonState_t new_state =
      current_pin_state ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;

  // State change detection
  if (new_state != current_state_) {
    last_state_ = current_state_;
    current_state_ = new_state;
    last_change_time_ = current_time;
    debounce_active_ = true;

    if (new_state == BUTTON_STATE_PRESSED) {
      // Button pressed
      press_start_time_ = current_time;
      long_press_triggered_ = false;
      last_long_press_time_ = 0;

      triggerEvent(BUTTON_EVENT_PRESS);

    } else {
      // Button released
      uint32_t press_duration = current_time - press_start_time_;

      triggerEvent(BUTTON_EVENT_RELEASE);

      // Handle click detection
      if (press_duration < long_press_time_ms_) {
        if (multi_click_enabled_) {
          // Multi-click mode
          click_count_++;
          last_click_time_ = current_time;
          multi_click_pending_ = true;
        } else {
          // Single click mode
          triggerEvent(BUTTON_EVENT_CLICK);
        }
      }
    }
  }

  // Check for long press
  if (long_press_enabled_ && current_state_ == BUTTON_STATE_PRESSED) {
    uint32_t press_duration = current_time - press_start_time_;

    if (!long_press_triggered_ && press_duration >= long_press_time_ms_) {
      // First long press trigger
      long_press_triggered_ = true;
      last_long_press_time_ = current_time;
      current_state_ = BUTTON_STATE_LONG_PRESSED;

      triggerEvent(BUTTON_EVENT_LONG_PRESS);
      triggerLongPress(press_duration);

    } else if (long_press_triggered_ && long_press_continuous_) {
      // Continuous long press triggering
      if (current_time - last_long_press_time_ >= long_press_interval_ms_) {
        last_long_press_time_ = current_time;
        triggerLongPress(press_duration);
      }
    }
  }
}

/**
 * @brief Process multi-click detection
 */
void Button::processMultiClick() {
  if (!multi_click_enabled_ || !multi_click_pending_) {
    return;
  }

  uint32_t current_time = HAL_GetTick();
  uint32_t elapsed = current_time - last_click_time_;

  // Check if multi-click timeout expired
  if (elapsed >= multi_click_gap_ms_) {
    // Timeout expired
    if (click_count_ > 1) {
      // Only trigger multi-click event if more than 1 click
      triggerMultiClick(click_count_);
    } else {
      // Single click - trigger normal click event
      triggerEvent(BUTTON_EVENT_CLICK);
    }

    // Reset for next sequence
    click_count_ = 0;
    multi_click_pending_ = false;
  } else if (click_count_ >= max_multi_clicks_) {
    // Maximum clicks reached, trigger immediately
    triggerMultiClick(click_count_);

    // Reset for next sequence
    click_count_ = 0;
    multi_click_pending_ = false;
  }
}

/**
 * @brief Trigger event callback
 */
void Button::triggerEvent(ButtonEvent_t event) {
  if (event_callback_) {
    event_callback_(event, current_state_);
  }
  if (event == BUTTON_EVENT_CLICK && click_callback_) {
    click_callback_();
  }
}

/**
 * @brief Trigger long press callback
 */
void Button::triggerLongPress(uint32_t duration) {
  if (long_press_callback_) {
    long_press_callback_(duration);
  }
}

/**
 * @brief Trigger multi-click callback
 */
void Button::triggerMultiClick(uint8_t click_count) {
  // Only trigger multi-click events for 2 or more clicks
  if (click_count >= 2) {
    if (multi_click_callback_) {
      multi_click_callback_(click_count);
    }

    // Also trigger general event
    triggerEvent(BUTTON_EVENT_MULTI_CLICK);
  }
}

/**
 * @brief Check for long press (for interrupt mode)
 */
void Button::checkLongPress() {
  if (!long_press_enabled_ || current_state_ != BUTTON_STATE_PRESSED) {
    return;
  }

  uint32_t current_time = HAL_GetTick();
  uint32_t press_duration = current_time - press_start_time_;

  if (!long_press_triggered_ && press_duration >= long_press_time_ms_) {
    // First long press trigger
    long_press_triggered_ = true;
    last_long_press_time_ = current_time;
    current_state_ = BUTTON_STATE_LONG_PRESSED;

    triggerEvent(BUTTON_EVENT_LONG_PRESS);
    triggerLongPress(press_duration);

  } else if (long_press_triggered_ && long_press_continuous_) {
    // Continuous long press triggering
    if (current_time - last_long_press_time_ >= long_press_interval_ms_) {
      last_long_press_time_ = current_time;
      triggerLongPress(press_duration);
    }
  }
}
