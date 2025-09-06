/**
 * @file encoder.h
 * @brief Object-Oriented Rotary Encoder Driver Header
 * @author User
 * @date 2025-09-06
 */

#ifndef __ENCODER_H__
#define __ENCODER_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#include <functional>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Encoder direction enumeration
 */
typedef enum {
    ENCODER_DIR_NONE = 0,
    ENCODER_DIR_CW = 1,    // Clockwise
    ENCODER_DIR_CCW = -1   // Counter-clockwise
} EncoderDirection_t;

/**
 * @brief Encoder event types
 */
typedef enum {
    ENCODER_EVENT_ROTATE_CW = 0,
    ENCODER_EVENT_ROTATE_CCW,
    ENCODER_EVENT_BUTTON_PRESS,
    ENCODER_EVENT_BUTTON_RELEASE,
    ENCODER_EVENT_BUTTON_CLICK,
    ENCODER_EVENT_BUTTON_LONG_PRESS
} EncoderEvent_t;

/**
 * @brief Encoder rotation speed enumeration
 */
typedef enum {
    ENCODER_SPEED_SLOW = 1,
    ENCODER_SPEED_MEDIUM = 2,
    ENCODER_SPEED_FAST = 4
} EncoderSpeed_t;

/* Exported constants --------------------------------------------------------*/
#define ENCODER_DEFAULT_DEBOUNCE_TIME_MS    5       // Encoder debounce time
#define ENCODER_BUTTON_DEBOUNCE_TIME_MS     50      // Button debounce time
#define ENCODER_BUTTON_LONG_PRESS_TIME_MS   1000    // Button long press threshold
#define ENCODER_SPEED_THRESHOLD_FAST_MS     50      // Fast rotation threshold
#define ENCODER_SPEED_THRESHOLD_MEDIUM_MS   200     // Medium rotation threshold

/* C++ Class Definition ------------------------------------------------------*/

/**
 * @brief Object-oriented Rotary Encoder class
 */
class RotaryEncoder {
public:
    // Callback function types
    using EventCallback = std::function<void(EncoderEvent_t event, EncoderDirection_t direction, int32_t steps)>;
    using RotationCallback = std::function<void(EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed)>;
    using ButtonCallback = std::function<void(EncoderEvent_t event, uint32_t duration_ms)>;
    using PositionCallback = std::function<void(int32_t position, int32_t delta)>;

private:
    // Hardware configuration
    GPIO_TypeDef* pin_a_port_;
    uint16_t pin_a_;
    GPIO_TypeDef* pin_b_port_;
    uint16_t pin_b_;
    GPIO_TypeDef* button_port_;
    uint16_t button_pin_;
    bool button_active_low_;
    bool has_button_;
    
    // Encoder state variables (using table decode method)
    uint8_t prev_next_code_;
    uint16_t state_store_;
    int32_t position_;
    int32_t delta_;
    uint32_t last_rotation_time_;
    bool rotation_active_;
    
    // Button state variables
    bool button_current_state_;
    bool button_last_state_;
    uint32_t button_last_change_time_;
    uint32_t button_press_start_time_;
    bool button_debounce_active_;
    bool button_long_press_triggered_;
    
    // Speed detection
    uint32_t rotation_intervals_[4];
    uint8_t interval_index_;
    EncoderSpeed_t current_speed_;
    
    // Acceleration feature
    bool acceleration_enabled_;
    uint32_t acceleration_threshold_ms_;
    uint8_t acceleration_factor_;
    
    // Callbacks
    EventCallback event_callback_;
    RotationCallback rotation_callback_;
    ButtonCallback button_callback_;
    PositionCallback position_callback_;
    
    // Configuration
    uint32_t debounce_time_ms_;
    bool reversed_;
    bool interrupt_mode_enabled_;

public:
    /**
     * @brief Constructor
     * @param pin_a_port GPIO port for encoder pin A
     * @param pin_a GPIO pin for encoder pin A
     * @param pin_b_port GPIO port for encoder pin B
     * @param pin_b GPIO pin for encoder pin B
     * @param button_port GPIO port for encoder button (optional)
     * @param button_pin GPIO pin for encoder button (optional)
     * @param button_active_low true if button is active low
     */
    RotaryEncoder(GPIO_TypeDef* pin_a_port, uint16_t pin_a,
                  GPIO_TypeDef* pin_b_port, uint16_t pin_b,
                  GPIO_TypeDef* button_port = nullptr, uint16_t button_pin = 0,
                  bool button_active_low = true);
    
    /**
     * @brief Destructor
     */
    ~RotaryEncoder() = default;
    
    /**
     * @brief Initialize encoder
     */
    void init();
    
    /**
     * @brief Process encoder events (call this regularly, or use interrupt mode)
     */
    void process();
    
    /**
     * @brief Enable interrupt-driven mode
     * @param enabled true to enable interrupt mode, false for polling mode
     */
    void setInterruptMode(bool enabled);
    
    /**
     * @brief Set general event callback
     * @param callback Event callback function
     */
    void setEventCallback(EventCallback callback);
    
    /**
     * @brief Set rotation-specific callback
     * @param callback Rotation callback function
     */
    void setRotationCallback(RotationCallback callback);
    
    /**
     * @brief Set button callback (if encoder has button)
     * @param callback Button callback function
     */
    void setButtonCallback(ButtonCallback callback);
    
    /**
     * @brief Set position change callback
     * @param callback Position callback function
     */
    void setPositionCallback(PositionCallback callback);
    
    /**
     * @brief Get current encoder position
     * @return Current encoder position
     */
    int32_t getPosition() const;
    
    /**
     * @brief Get encoder delta since last read
     * @return Encoder delta value
     */
    int32_t getDelta();
    
    /**
     * @brief Reset encoder position to zero
     */
    void resetPosition();
    
    /**
     * @brief Set encoder position to specific value
     * @param position New position value
     */
    void setPosition(int32_t position);
    
    /**
     * @brief Get current rotation speed
     * @return Current rotation speed
     */
    EncoderSpeed_t getSpeed() const;
    
    /**
     * @brief Check if button is currently pressed (if available)
     * @return true if pressed, false if released
     */
    bool isButtonPressed() const;
    
    /**
     * @brief Enable/disable acceleration feature
     * @param enabled true to enable acceleration
     * @param threshold_ms time threshold for acceleration in milliseconds
     * @param factor acceleration factor (2-10)
     */
    void setAcceleration(bool enabled, uint32_t threshold_ms = 100, uint8_t factor = 2);
    
    /**
     * @brief Reverse encoder direction
     * @param reversed true to reverse direction
     */
    void setReversed(bool reversed);
    
    /**
     * @brief Set debounce time
     * @param time_ms debounce time in milliseconds
     */
    void setDebounceTime(uint32_t time_ms);
    
    /**
     * @brief GPIO EXTI callback (call this from interrupt handler)
     * @param gpio_pin Pin that triggered the interrupt
     */
    void onGpioInterrupt(uint16_t gpio_pin);

private:
    /**
     * @brief Read encoder pin state
     * @param port GPIO port
     * @param pin GPIO pin
     * @return true if pin is high, false if low
     */
    bool readPin(GPIO_TypeDef* port, uint16_t pin) const;
    
    /**
     * @brief Process encoder rotation using robust table decode method
     * @return rotation direction (-1, 0, 1)
     */
    int8_t processRotation();
    
    /**
     * @brief Handle rotation result (common logic for both modes)
     * @param rotation_result rotation direction (-1, 0, 1)
     */
    void handleRotation(int8_t rotation_result);
    
    /**
     * @brief Process button state machine
     */
    void processButton();
    
    /**
     * @brief Update rotation speed based on timing
     */
    void updateSpeed();
    
    /**
     * @brief Calculate acceleration factor based on speed
     * @return acceleration multiplier
     */
    uint8_t calculateAcceleration() const;
    
    /**
     * @brief Trigger event callback if registered
     * @param event Event type
     * @param direction Rotation direction
     * @param steps Number of steps
     */
    void triggerEvent(EncoderEvent_t event, EncoderDirection_t direction = ENCODER_DIR_NONE, int32_t steps = 0);
    
    /**
     * @brief Trigger rotation callback if registered
     * @param direction Rotation direction
     * @param steps Number of steps
     * @param speed Rotation speed
     */
    void triggerRotation(EncoderDirection_t direction, int32_t steps, EncoderSpeed_t speed);
    
    /**
     * @brief Trigger button callback if registered
     * @param event Button event
     * @param duration Press duration in milliseconds
     */
    void triggerButton(EncoderEvent_t event, uint32_t duration = 0);
    
    /**
     * @brief Trigger position callback if registered
     */
    void triggerPosition();
};

#endif /* __ENCODER_H__ */
