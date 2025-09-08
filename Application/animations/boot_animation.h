/**
 * @file boot_animation.h
 * @brief Modern Boot Animation with Ripple Effects and Particles for STM32
 * (Fixed-point arithmetic)
 * @author AI Assistant
 * @date 2025-09-07
 * @version 2.0
 *
 * Features:
 * - Ripple wave expansion effects
 * - Particle system around logo
 * - Progressive text reveal
 * - Circular progress indicator
 * - Smooth easing animations
 */

#ifndef BOOT_ANIMATION_H
#define BOOT_ANIMATION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32_u8g2.h"
#include <stdbool.h>
#include <stdint.h>


/* Exported types ------------------------------------------------------------*/

/**
 * @brief Boot animation state enumeration
 */
typedef enum {
  BOOT_ANIM_STATE_IDLE = 0,    ///< Animation not started
  BOOT_ANIM_STATE_INIT_LINE,   ///< Ripple expansion phase
  BOOT_ANIM_STATE_SPLIT_LINES, ///< Particle activation and text reveal
  BOOT_ANIM_STATE_SHOW_TEXT,   ///< Full animation with progress ring
  BOOT_ANIM_STATE_COMPLETE,    ///< Animation completed
  BOOT_ANIM_STATE_FINISHED     ///< Ready to proceed to main app
} BootAnimState_t;

/**
 * @brief Animation parameters structure (all values in fixed-point * 1000)
 */
typedef struct {
  int32_t progress;      ///< Current animation progress (0 - 1000)
  uint32_t start_time;   ///< Animation start timestamp
  uint32_t current_time; ///< Current timestamp
  BootAnimState_t state; ///< Current animation state

  // Line animation parameters
  int32_t line_angle_mil; ///< Initial line tilt angle (milliradians * 1000)
  int16_t line_center_x;  ///< Line center X position
  int16_t line_center_y;  ///< Line center Y position
  int16_t line_length;    ///< Line length

  // Split animation parameters
  int32_t split_progress;     ///< Line splitting progress (0 - 1000)
  int16_t left_curve_offset;  ///< Left curve horizontal offset
  int16_t right_curve_offset; ///< Right curve horizontal offset

  // Text animation parameters
  int32_t text_alpha; ///< Text opacity (0 - 1000)
  int16_t text_x;     ///< Text X position
  int16_t text_y;     ///< Text Y position

  // Timing parameters
  uint16_t init_line_duration; ///< Initial line display duration (ms)
  uint16_t split_duration;     ///< Line splitting duration (ms)
  uint16_t text_fade_duration; ///< Text fade-in duration (ms)
  uint16_t hold_duration;      ///< Final display hold duration (ms)
} BootAnimParams_t;

/* Exported constants --------------------------------------------------------*/

// Fixed-point arithmetic constants
#define FIXED_POINT_SCALE 1000 ///< Fixed-point scale factor
#define FIXED_POINT_ONE 1000   ///< Fixed-point representation of 1.0
#define FIXED_POINT_HALF 500   ///< Fixed-point representation of 0.5

// Animation timing constants (milliseconds)
#define BOOT_ANIM_INIT_LINE_TIME 800 ///< Initial line display time
#define BOOT_ANIM_SPLIT_TIME 1       ///< Line splitting animation time
#define BOOT_ANIM_TEXT_FADE_TIME 1   ///< Text fade-in time
#define BOOT_ANIM_HOLD_TIME 1        ///< Final hold time
#define BOOT_ANIM_TOTAL_TIME                                                   \
  (BOOT_ANIM_INIT_LINE_TIME + BOOT_ANIM_SPLIT_TIME +                           \
   BOOT_ANIM_TEXT_FADE_TIME + BOOT_ANIM_HOLD_TIME)

// Animation visual constants
#define BOOT_ANIM_LINE_ANGLE_DEG 15   ///< Initial line tilt angle in degrees
#define BOOT_ANIM_LINE_LENGTH 50      ///< Initial line length
#define BOOT_ANIM_MAX_SPLIT_OFFSET 35 ///< Maximum horizontal split offset
#define BOOT_ANIM_BEZIER_SEGMENTS 15  ///< Number of bezier curve segments

// Trigonometric constants (scaled by 1000)
#define PI_SCALED 3142              ///< PI * 1000
#define PI_DIV_180_SCALED 17        ///< (PI/180) * 1000

/* Exported variables --------------------------------------------------------*/

extern BootAnimParams_t g_boot_anim_params;

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Initialize boot animation system
 * @param u8g2_display Pointer to U8G2 display object
 * @retval true if initialization successful
 * @retval false if initialization failed
 */
bool BootAnimation_Init(STM32_U8G2_Display *u8g2_display);

/**
 * @brief Start boot animation
 * @note Call this function once to begin the animation sequence
 * @retval true if animation started successfully
 * @retval false if animation already running or failed to start
 */
bool BootAnimation_Start(void);

/**
 * @brief Update boot animation (call this in main loop)
 * @retval true if animation is still running
 * @retval false if animation is complete
 */
bool BootAnimation_Update(void);

/**
 * @brief Render current animation frame
 * @note This function handles the actual drawing to the display
 * @retval true if frame rendered successfully
 * @retval false if rendering failed
 */
bool BootAnimation_Render(void);

/**
 * @brief Check if boot animation is complete
 * @retval true if animation finished and ready to proceed
 * @retval false if animation still running or not started
 */
bool BootAnimation_IsComplete(void);

/**
 * @brief Stop and reset boot animation
 * @note Use this to abort animation or reset for restart
 */
void BootAnimation_Stop(void);

/**
 * @brief Get current animation progress
 * @retval int32_t Current overall progress (0 - 1000)
 */
int32_t BootAnimation_GetProgress(void);

/**
 * @brief Get current animation state
 * @retval BootAnimState_t Current state of the animation
 */
BootAnimState_t BootAnimation_GetState(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_ANIMATION_H */