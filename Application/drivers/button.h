/**
 * @file button.h
 * @brief Object-Oriented Button Driver Header
 * @author User
 * @date 2025-09-06
 */

#ifndef __BUTTON_H__
#define __BUTTON_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#include <functional>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Button states enumeration
 */
typedef enum {
    BUTTON_STATE_RELEASED = 0,
    BUTTON_STATE_PRESSED = 1,
    BUTTON_STATE_LONG_PRESSED = 2
} ButtonState_t;

/**
 * @brief Button event types
 */
typedef enum {
    BUTTON_EVENT_PRESS = 0,
    BUTTON_EVENT_RELEASE,
    BUTTON_EVENT_CLICK,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_MULTI_CLICK
} ButtonEvent_t;

/* Exported constants --------------------------------------------------------*/
#define BUTTON_DEBOUNCE_TIME_MS     50              // Button debounce time
#define BUTTON_DEFAULT_LONG_PRESS_TIME_MS   800     // Default long press threshold
#define BUTTON_DEFAULT_MULTI_CLICK_GAP_MS   250     // Default multi-click gap time
#define BUTTON_MAX_MULTI_CLICKS     5               // Maximum multi-click count

/* C++ Class Definition ------------------------------------------------------*/

/**
 * @brief Object-oriented Button class
 */
class Button {
public:
    // Callback function types
    using EventCallback = std::function<void(ButtonEvent_t event, ButtonState_t state)>;
    using ClickCallback = std::function<void()>;
    using LongPressCallback = std::function<void(uint32_t duration_ms)>;
    using MultiClickCallback = std::function<void(uint8_t click_count)>;

private:
    // Hardware configuration
    GPIO_TypeDef* port_;
    uint16_t pin_;
    bool active_low_;
    
    // State variables
    ButtonState_t current_state_;
    ButtonState_t last_state_;
    uint32_t last_change_time_;
    uint32_t press_start_time_;
    bool debounce_active_;
    bool long_press_triggered_;
    
    // Multi-click detection
    uint8_t click_count_;
    uint32_t last_click_time_;
    uint32_t multi_click_gap_ms_;
    bool multi_click_pending_;
    
    // Long press configuration
    uint32_t long_press_time_ms_;
    bool long_press_enabled_;
    
    // Callbacks
    EventCallback event_callback_;
    ClickCallback click_callback_;
    LongPressCallback long_press_callback_;
    MultiClickCallback multi_click_callback_;
    
    // Multi-click configuration
    uint8_t max_multi_clicks_;
    bool multi_click_enabled_;
    
    // Interrupt mode
    bool interrupt_mode_enabled_;

public:
    /**
     * @brief Constructor
     * @param port GPIO port
     * @param pin GPIO pin
     * @param active_low true if button is active low, false if active high
     */
    Button(GPIO_TypeDef* port, uint16_t pin, bool active_low = true);
    
    /**
     * @brief Destructor
     */
    ~Button() = default;
    
    /**
     * @brief Initialize button
     */
    void init();
    
    /**
     * @brief Process button events (call this regularly, or use interrupt mode)
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
     * @brief Register click callback
     * @param callback Click callback function
     */
    void handleClick(ClickCallback callback);

    /**
     * @brief Register long press callback
     * @param callback Long press callback function
     * @param time_ms Long press threshold in milliseconds
     */
    void handleLongPress(LongPressCallback callback, uint32_t time_ms = BUTTON_DEFAULT_LONG_PRESS_TIME_MS);
    
    /**
     * @brief Register multi-click callback
     * @param max_clicks Maximum number of clicks to detect
     * @param callback Multi-click callback function
     * @param gap_ms Gap time between clicks in milliseconds
     */
    void handleMultiClick(uint8_t max_clicks, MultiClickCallback callback, uint32_t gap_ms = BUTTON_DEFAULT_MULTI_CLICK_GAP_MS);
    
    /**
     * @brief Get current button state
     * @return Current button state
     */
    ButtonState_t getState() const;
    
    /**
     * @brief Check if button is currently pressed
     * @return true if pressed, false if released
     */
    bool isPressed() const;
    
    /**
     * @brief Get press duration for current press
     * @return Press duration in milliseconds, 0 if not pressed
     */
    uint32_t getPressDuration() const;
    
    /**
     * @brief Disable long press detection
     */
    void disableLongPress();
    
    /**
     * @brief Disable multi-click detection
     */
    void disableMultiClick();
    
    /**
     * @brief GPIO EXTI callback (call this from interrupt handler)
     */
    void onGpioInterrupt();

private:
    /**
     * @brief Read GPIO pin state
     * @return true if button is pressed, false if released
     */
    bool readPin() const;
    
    /**
     * @brief Handle button state machine
     */
    void stateMachine();
    
    /**
     * @brief Process multi-click detection
     */
    void processMultiClick();
    
    /**
     * @brief Check for long press (for interrupt mode)
     */
    void checkLongPress();
    
    /**
     * @brief Trigger event callback if registered
     * @param event Event type
     */
    void triggerEvent(ButtonEvent_t event);
    
    /**
     * @brief Trigger long press callback if registered
     * @param duration Press duration in milliseconds
     */
    void triggerLongPress(uint32_t duration);
    
    /**
     * @brief Trigger multi-click callback if registered
     * @param click_count Number of clicks detected
     */
    void triggerMultiClick(uint8_t click_count);
};

#endif /* __BUTTON_H__ */
