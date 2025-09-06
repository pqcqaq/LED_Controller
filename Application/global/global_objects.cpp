/**
 * @file global_objects.cpp
 * @brief Global Objects Implementation
 * @author User
 * @date 2025-09-06
 */

/* Includes ------------------------------------------------------------------*/
#include "global_objects.h"

/* Global Objects ------------------------------------------------------------*/

// Global rotary encoder instance
// Pin configuration: PA (PB12), PB (PB13), Button (PB14)
RotaryEncoder rotary_encoder(GPIOB, GPIO_PIN_12,  // Pin A
                           GPIOB, GPIO_PIN_13,   // Pin B
                           GPIOB, GPIO_PIN_14,   // Button
                           true);                // Button active low

// Global button instance (encoder button)
Button encoder_button(GPIOB, GPIO_PIN_14, true);

// PA5
Button btn_1(GPIOA, GPIO_PIN_5, true);

// PA6
Button btn_2(GPIOA, GPIO_PIN_6, true);


/* Function implementations --------------------------------------------------*/

/**
 * @brief Initialize all global objects
 */
void GlobalObjects_Init(void) {
    // Initialize encoder
    rotary_encoder.init();
    
    // Initialize button
    encoder_button.init();

    // Initialize PA5 button
    btn_1.init();

    // Initialize PA6 button
    btn_2.init();
    
    // Temporarily disable interrupt mode and use polling mode
    // Enable interrupt mode for all buttons and encoder
    encoder_button.setInterruptMode(true);  // Use polling mode for now
    btn_1.setInterruptMode(true);           // Use polling mode for now  
    btn_2.setInterruptMode(true);           // Use polling mode for now
    rotary_encoder.setInterruptMode(true);  // Use polling mode for now
}

/**
 * @brief Process all global objects
 */
void GlobalObjects_Process(void) {
    // Process encoder
    rotary_encoder.process();
    
    // Process button
    encoder_button.process();

    // Process PA5 button
    btn_1.process();

    // Process PA6 button
    btn_2.process();
}