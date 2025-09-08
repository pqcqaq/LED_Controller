/**
 * @file boot_animation.cpp
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

/* Includes ------------------------------------------------------------------*/
#include "boot_animation.h"
#include "main.h"
#include <string.h>
#include "global/global_objects.h"

/* Private variables ---------------------------------------------------------*/
static STM32_U8G2_Display *g_display = nullptr;
static bool g_animation_initialized = false;

/* Global variables ----------------------------------------------------------*/
BootAnimParams_t g_boot_anim_params = {0};

/* Particle system constants */
#define MAX_PARTICLES 8
#define PARTICLE_SPEED_SCALE 50

typedef struct {
  int16_t x, y;     // Position
  int16_t vx, vy;   // Velocity (fixed point)
  int16_t life;     // Life time remaining
  int16_t max_life; // Maximum life time
  uint8_t size;     // Particle size
} Particle_t;

static Particle_t particles[MAX_PARTICLES];
static bool particles_initialized = false;

/* Private lookup tables for trigonometric functions ------------------------*/

// Sine lookup table for 0 to PI/2 (0 to 1.57 radians) with 0.01 step
// Values are scaled by 1000
static const int32_t sine_table_90[] = {
    0,   10,  20,  30,  40,  50,  60,  70,  80,  90,  // 0.00 - 0.09
    100, 110, 120, 130, 140, 150, 160, 170, 180, 190, // 0.10 - 0.19
    200, 210, 220, 230, 240, 250, 260, 270, 280, 290, // 0.20 - 0.29
    300, 310, 320, 330, 340, 350, 360, 370, 380, 390, // 0.30 - 0.39
    400, 410, 420, 430, 440, 450, 460, 470, 480, 490, // 0.40 - 0.49
    500, 510, 520, 530, 540, 549, 559, 569, 579, 588, // 0.50 - 0.59
    598, 608, 617, 627, 637, 646, 656, 665, 675, 684, // 0.60 - 0.69
    694, 703, 713, 722, 731, 741, 750, 759, 768, 777, // 0.70 - 0.79
    786, 795, 804, 813, 822, 831, 839, 848, 857, 866, // 0.80 - 0.89
    874, 883, 891, 900, 908, 917, 925, 933, 942, 950, // 0.90 - 0.99
    958, 966, 974, 982, 990, 998, 1000                // 1.00 - 1.06
};

#define SINE_TABLE_SIZE (sizeof(sine_table_90) / sizeof(sine_table_90[0]))

/* Private function implementations ------------------------------------------*/

/**
 * @brief Fixed-point sine function (input in milliradians)
 */
static int32_t sin_fixed(int32_t angle_mr) {
  // Normalize angle to 0 - 2PI range
  while (angle_mr < 0)
    angle_mr += 2 * PI_SCALED;
  while (angle_mr >= 2 * PI_SCALED)
    angle_mr -= 2 * PI_SCALED;

  bool negative = false;

  // Handle negative values for sine in different quadrants
  if (angle_mr >= PI_SCALED) {
    angle_mr -= PI_SCALED;
    negative = true;
  }

  // Handle second quadrant (PI/2 to PI)
  if (angle_mr > PI_SCALED / 2) {
    angle_mr = PI_SCALED - angle_mr;
  }

  // Convert to table index (angle_mr is in range 0 to PI/2)
  int32_t index = (angle_mr * ((int32_t)SINE_TABLE_SIZE - 1)) / (PI_SCALED / 2);
  if (index >= (int32_t)SINE_TABLE_SIZE)
    index = (int32_t)SINE_TABLE_SIZE - 1;

  int32_t result = sine_table_90[index];
  return negative ? -result : result;
}

/**
 * @brief Fixed-point cosine function (input in milliradians)
 */
static int32_t cos_fixed(int32_t angle_mr) {
  // cos(x) = sin(x + PI/2)
  return sin_fixed(angle_mr + PI_SCALED / 2);
}

/**
 * @brief Ease-in-out cubic function for smooth transitions
 */
static int32_t ease_in_out_cubic_fixed(int32_t t) {
  if (t < FIXED_POINT_HALF) {
    // 4 * t^3, where t is in range [0, 500]
    int32_t t_norm = (t * 2); // Normalize to [0, 1000]
    int32_t t2 = (t_norm * t_norm) / FIXED_POINT_SCALE;
    int32_t t3 = (t2 * t_norm) / FIXED_POINT_SCALE;
    return (4 * t3) / FIXED_POINT_SCALE;
  } else {
    // 1 + p^3/2, where p = 2*t - 2
    int32_t p = 2 * t - 2 * FIXED_POINT_SCALE;
    int32_t p2 = (p * p) / FIXED_POINT_SCALE;
    int32_t p3 = (p2 * p) / FIXED_POINT_SCALE;
    return FIXED_POINT_SCALE + p3 / 2;
  }
}

/**
 * @brief draw author text
 */
static void draw_author_text(void) {
  if (!g_display)
    return;

  g_display->setFont(u8g2_font_5x7_mr);
  const char *author = AUTHOR_TEXT;
  int16_t text_width = g_display->getStrWidth(author);
  g_display->drawStr(64 - text_width / 2, 60, author);
}

/**
 * @brief Initialize particle system
 */
static void init_particles(void) {
  if (particles_initialized)
    return;

  for (int i = 0; i < MAX_PARTICLES; i++) {
    // Random positions around center
    int32_t angle = (i * 2 * PI_SCALED) / MAX_PARTICLES;
    int32_t radius = 15 + (i % 3) * 5; // Varying radius

    particles[i].x = 64 + (radius * cos_fixed(angle)) / FIXED_POINT_SCALE;
    particles[i].y = 32 + (radius * sin_fixed(angle)) / FIXED_POINT_SCALE;

    // Random velocities
    particles[i].vx = cos_fixed(angle + PI_SCALED / 4) / 20;
    particles[i].vy = sin_fixed(angle + PI_SCALED / 4) / 20;

    particles[i].life = 1000 + (i * 100);
    particles[i].max_life = particles[i].life;
    particles[i].size = 1 + (i % 2);
  }

  particles_initialized = true;
}

/**
 * @brief Update particle system
 */
static void update_particles(uint32_t elapsed_time) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    // Update position
    particles[i].x += particles[i].vx;
    particles[i].y += particles[i].vy;

    // Update life
    particles[i].life -= 10;
    if (particles[i].life <= 0) {
      // Respawn particle
      int32_t angle = (i * 2 * PI_SCALED) / MAX_PARTICLES + (elapsed_time / 10);
      int32_t radius = 15 + (i % 3) * 5;

      particles[i].x = 64 + (radius * cos_fixed(angle)) / FIXED_POINT_SCALE;
      particles[i].y = 32 + (radius * sin_fixed(angle)) / FIXED_POINT_SCALE;
      particles[i].life = particles[i].max_life;
    }

    // Apply orbital motion with easing
    int32_t dx = particles[i].x - 64;
    int32_t dy = particles[i].y - 32;
    int32_t ease_factor = ease_in_out_cubic_fixed(500); // Use moderate easing
    particles[i].vx = (-dy * ease_factor) / (30 * FIXED_POINT_SCALE);
    particles[i].vy = (dx * ease_factor) / (30 * FIXED_POINT_SCALE);
  }
}

/**
 * @brief Draw particle system
 */
static void draw_particles(int32_t alpha) {
  if (!g_display || alpha <= 0)
    return;

  for (int i = 0; i < MAX_PARTICLES; i++) {
    // Calculate particle alpha based on life and global alpha
    int32_t particle_alpha = (particles[i].life * alpha) /
                             (particles[i].max_life * FIXED_POINT_SCALE);

    if (particle_alpha > 300) { // Only draw if visible enough
      // Draw particle with varying brightness
      g_display->setDrawColor(1);

      if (particle_alpha > 800) {
        // Full brightness - draw solid
        g_display->drawDisc(particles[i].x, particles[i].y, particles[i].size);
      } else if (particle_alpha > 500) {
        // Medium brightness - draw circle
        g_display->drawCircle(particles[i].x, particles[i].y,
                              particles[i].size);
      } else {
        // Low brightness - draw pixel
        g_display->drawPixel(particles[i].x, particles[i].y);
      }
    }
  }
}

/**
 * @brief Draw expanding ripple waves with scaling text
 */
static void draw_ripples(int32_t progress) {
  if (!g_display)
    return;

  // Draw scaling TITLE_TEXT text in center
  if (progress > 0) {
    g_display->setDrawColor(1);
    draw_author_text();

    // Calculate text scaling progress with easing
    int32_t text_scale_progress = ease_in_out_cubic_fixed(progress);

    // Different font sizes based on scale progress
    if (text_scale_progress < FIXED_POINT_SCALE / 4) {
      // Very small - use small font
      g_display->setFont(u8g2_font_5x7_mr);
      int16_t text_width = g_display->getStrWidth(TITLE_TEXT);
      g_display->drawStr(64 - text_width / 2, 34, TITLE_TEXT);
    } else if (text_scale_progress < FIXED_POINT_SCALE / 2) {
      // Small - use medium font
      g_display->setFont(u8g2_font_6x10_mr);
      int16_t text_width = g_display->getStrWidth(TITLE_TEXT);
      g_display->drawStr(64 - text_width / 2, 35, TITLE_TEXT);
    } else if (text_scale_progress < FIXED_POINT_SCALE * 3 / 4) {
      // Medium - use normal font
      g_display->setFont(u8g2_font_7x13_mr);
      int16_t text_width = g_display->getStrWidth(TITLE_TEXT);
      g_display->drawStr(64 - text_width / 2, 36, TITLE_TEXT);
    } else {
      // Large - use big font
      g_display->setFont(u8g2_font_9x15_mr);
      int16_t text_width = g_display->getStrWidth(TITLE_TEXT);
      g_display->drawStr(64 - text_width / 2, 37, TITLE_TEXT);
    }
  }

  // Multiple ripples with different phases
  for (int wave = 0; wave < 3; wave++) {
    int32_t wave_progress = progress - (wave * FIXED_POINT_SCALE / 4);
    if (wave_progress <= 0)
      continue;

    // For the last wave (wave 2), allow it to continue expanding beyond normal
    // limit
    int32_t max_radius;
    int32_t alpha;

    if (wave == 2) {
      // Last wave - continue expanding until it goes off screen
      max_radius =
          160; // Increased max radius to go off screen (screen is 128x64)
      int32_t radius = (max_radius * wave_progress) / FIXED_POINT_SCALE;

      // Keep some alpha even when expanding beyond normal range
      if (wave_progress <= FIXED_POINT_SCALE) {
        alpha = FIXED_POINT_SCALE - wave_progress;
      } else {
        // Continue with minimal alpha for off-screen expansion
        alpha = 100; // Minimal visibility
      }

      if (alpha > 50 && radius > 0) { // Draw with lower threshold for last wave
        g_display->setDrawColor(1);

        // Always draw thin ripple for the expanding final wave
        if (radius < 60) {
          g_display->drawCircle(64, 32, radius);
        } else {
          // For very large radius, use dithered circle to save performance
          for (int i = 0; i < 360; i += 20) {
            int32_t angle = (i * PI_SCALED) / 180;
            int16_t x = 64 + (radius * cos_fixed(angle)) / FIXED_POINT_SCALE;
            int16_t y = 32 + (radius * sin_fixed(angle)) / FIXED_POINT_SCALE;
            // Only draw if within reasonable screen bounds
            if (x >= -10 && x <= 138 && y >= -10 && y <= 74) {
              g_display->drawPixel(x, y);
            }
          }
        }
      }
    } else {
      // Normal waves with original behavior
      if (wave_progress > FIXED_POINT_SCALE)
        wave_progress = FIXED_POINT_SCALE;

      // Calculate ripple radius
      max_radius = 40;
      int32_t radius = (max_radius * wave_progress) / FIXED_POINT_SCALE;

      // Calculate alpha (fade out as it expands)
      alpha = FIXED_POINT_SCALE - wave_progress;

      if (alpha > 200 && radius > 0) { // Only draw if visible
        g_display->setDrawColor(1);

        // Draw ripple with thickness based on alpha
        if (alpha > 700) {
          // Draw thick ripple
          g_display->drawCircle(64, 32, radius);
          g_display->drawCircle(64, 32, radius + 1);
        } else if (alpha > 400) {
          // Draw normal ripple
          g_display->drawCircle(64, 32, radius);
        } else {
          // Draw thin ripple with dithering
          for (int i = 0; i < 360; i += 30) {
            int32_t angle = (i * PI_SCALED) / 180;
            int16_t x = 64 + (radius * cos_fixed(angle)) / FIXED_POINT_SCALE;
            int16_t y = 32 + (radius * sin_fixed(angle)) / FIXED_POINT_SCALE;
            g_display->drawPixel(x, y);
          }
        }
      }
    }
  }
}

/**
 * @brief Draw circular progress indicator
 */
static void draw_progress_ring(int32_t progress) {
  if (!g_display)
    return;

  int16_t center_x = 64;
  int16_t center_y = 50;
  int16_t radius = 12;

  // Draw background ring (dashed)
  g_display->setDrawColor(1);
  for (int angle = 0; angle < 360; angle += 15) {
    int32_t rad = (angle * PI_SCALED) / 180;
    int16_t x = center_x + (radius * cos_fixed(rad)) / FIXED_POINT_SCALE;
    int16_t y = center_y + (radius * sin_fixed(rad)) / FIXED_POINT_SCALE;
    g_display->drawPixel(x, y);
  }

  // Draw progress arc
  int32_t arc_length = (360 * progress) / FIXED_POINT_SCALE;
  for (int angle = 0; angle < arc_length; angle += 5) {
    int32_t rad = ((angle - 90) * PI_SCALED) / 180; // Start from top
    int16_t x = center_x + (radius * cos_fixed(rad)) / FIXED_POINT_SCALE;
    int16_t y = center_y + (radius * sin_fixed(rad)) / FIXED_POINT_SCALE;

    // Draw thick arc
    g_display->drawDisc(x, y, 1);
  }
}

/**
 * @brief Draw STM32 text with progressive reveal
 */
static void draw_progressive_text(int32_t text_progress) {
  if (!g_display)
    return;

  g_display->setFont(u8g2_font_inb21_mr); // Large bold font

  const char chars[] = {'S', 'T', 'M', '3', '2'};
  int16_t char_spacing = 12;
  int16_t start_x = 25; // Adjusted for better centering
  int16_t text_y = 35;

  // Calculate how many characters to show
  int32_t chars_to_show = (5 * text_progress) / FIXED_POINT_SCALE;
  int32_t current_char_progress = (5 * text_progress) % FIXED_POINT_SCALE;

  g_display->setDrawColor(1);

  for (int i = 0; i < 5; i++) {
    int16_t char_x = start_x + i * char_spacing;

    if (i < chars_to_show) {
      // Fully visible character
      char temp_str[2] = {chars[i], '\0'};
      g_display->drawStr(char_x, text_y, temp_str);
    } else if (i == chars_to_show && current_char_progress > 0) {
      // Partially visible character (fade in effect)
      char temp_str[2] = {chars[i], '\0'};

      if (current_char_progress > 700) {
        // Nearly full - draw solid
        g_display->drawStr(char_x, text_y, temp_str);
      } else if (current_char_progress > 400) {
        // Half visible - draw with some transparency effect
        g_display->drawStr(char_x, text_y, temp_str);
        // Add transparency by drawing background pixels
        for (int py = text_y - 15; py < text_y + 3; py += 3) {
          for (int px = char_x; px < char_x + 10; px += 4) {
            if ((px + py) % 6 == 0) {
              g_display->setDrawColor(0);
              g_display->drawPixel(px, py);
              g_display->setDrawColor(1);
            }
          }
        }
      } else {
        // Low visibility - outline only
        for (int dx = -1; dx <= 1; dx++) {
          for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0)
              continue;
            if ((dx + dy) % 2 == 0) {
              g_display->drawStr(char_x + dx, text_y + dy, temp_str);
            }
          }
        }
        g_display->setDrawColor(0);
        g_display->drawStr(char_x, text_y, temp_str);
        g_display->setDrawColor(1);
      }
    }
  }
}

/**
 * @brief Update animation parameters based on current time
 */
static void update_animation_params(BootAnimParams_t *params) {
  params->current_time = HAL_GetTick();
  uint32_t elapsed_time = params->current_time - params->start_time;

  // Overall progress (fixed-point)
  params->progress = (elapsed_time * FIXED_POINT_SCALE) / BOOT_ANIM_TOTAL_TIME;
  if (params->progress > FIXED_POINT_SCALE)
    params->progress = FIXED_POINT_SCALE;

  // New state machine for modern animation
  if (elapsed_time < BOOT_ANIM_INIT_LINE_TIME) {
    // Phase 1: Ripple expansion
    params->state = BOOT_ANIM_STATE_INIT_LINE;
    params->split_progress =
        (elapsed_time * FIXED_POINT_SCALE) / BOOT_ANIM_INIT_LINE_TIME;
    params->text_alpha = 0;
  } else if (elapsed_time < (BOOT_ANIM_INIT_LINE_TIME + BOOT_ANIM_SPLIT_TIME)) {
    // Phase 2: Particle activation and text reveal
    params->state = BOOT_ANIM_STATE_SPLIT_LINES;
    uint32_t phase_elapsed = elapsed_time - BOOT_ANIM_INIT_LINE_TIME;
    params->split_progress = FIXED_POINT_SCALE; // Ripples complete
    params->text_alpha =
        (phase_elapsed * FIXED_POINT_SCALE) / BOOT_ANIM_SPLIT_TIME;
    if (params->text_alpha > FIXED_POINT_SCALE)
      params->text_alpha = FIXED_POINT_SCALE;
  } else if (elapsed_time < (BOOT_ANIM_INIT_LINE_TIME + BOOT_ANIM_SPLIT_TIME +
                             BOOT_ANIM_TEXT_FADE_TIME)) {
    // Phase 3: Full animation display with particles
    params->state = BOOT_ANIM_STATE_SHOW_TEXT;
    params->split_progress = FIXED_POINT_SCALE;
    params->text_alpha = FIXED_POINT_SCALE;
  } else if (elapsed_time < BOOT_ANIM_TOTAL_TIME) {
    // Phase 4: Animation completion
    params->state = BOOT_ANIM_STATE_COMPLETE;
    params->split_progress = FIXED_POINT_SCALE;
    params->text_alpha = FIXED_POINT_SCALE;
  } else {
    // Animation finished
    params->state = BOOT_ANIM_STATE_FINISHED;
    params->split_progress = FIXED_POINT_SCALE;
    params->text_alpha = FIXED_POINT_SCALE;
  }

  // Update particle system
  if (params->state >= BOOT_ANIM_STATE_SPLIT_LINES) {
    update_particles(elapsed_time);
  }
}

/* Public function implementations -------------------------------------------*/

/**
 * @brief Initialize boot animation system
 */
bool BootAnimation_Init(STM32_U8G2_Display *u8g2_display) {
  if (!u8g2_display) {
    return false;
  }

  g_display = u8g2_display;
  g_animation_initialized = true;

  // Initialize animation parameters
  memset(&g_boot_anim_params, 0, sizeof(BootAnimParams_t));

  // Set up default parameters for modern animation
  g_boot_anim_params.line_angle_mil =
      (BOOT_ANIM_LINE_ANGLE_DEG * PI_DIV_180_SCALED);
  g_boot_anim_params.line_center_x = 64; // Center of 128px wide display
  g_boot_anim_params.line_center_y = 32; // Center of 64px high display
  g_boot_anim_params.line_length = BOOT_ANIM_LINE_LENGTH;

  g_boot_anim_params.left_curve_offset = -BOOT_ANIM_MAX_SPLIT_OFFSET;
  g_boot_anim_params.right_curve_offset = BOOT_ANIM_MAX_SPLIT_OFFSET;

  g_boot_anim_params.text_x = 64; // Will be centered in draw function
  g_boot_anim_params.text_y = 35; // Center of display

  g_boot_anim_params.init_line_duration = BOOT_ANIM_INIT_LINE_TIME;
  g_boot_anim_params.split_duration = BOOT_ANIM_SPLIT_TIME;
  g_boot_anim_params.text_fade_duration = BOOT_ANIM_TEXT_FADE_TIME;
  g_boot_anim_params.hold_duration = BOOT_ANIM_HOLD_TIME;

  g_boot_anim_params.state = BOOT_ANIM_STATE_IDLE;

  // Initialize particle system
  init_particles();

  return true;
}

/**
 * @brief Start boot animation
 */
bool BootAnimation_Start(void) {
  if (!g_animation_initialized ||
      g_boot_anim_params.state != BOOT_ANIM_STATE_IDLE) {
    return false;
  }

  g_boot_anim_params.start_time = HAL_GetTick();
  g_boot_anim_params.current_time = g_boot_anim_params.start_time;
  g_boot_anim_params.progress = 0;
  g_boot_anim_params.split_progress = 0;
  g_boot_anim_params.text_alpha = 0;
  g_boot_anim_params.state = BOOT_ANIM_STATE_INIT_LINE;

  // Reset particle system
  particles_initialized = false;
  init_particles();

  return true;
}

/**
 * @brief Update boot animation
 */
bool BootAnimation_Update(void) {
  if (!g_animation_initialized ||
      g_boot_anim_params.state == BOOT_ANIM_STATE_IDLE) {
    return false;
  }

  if (g_boot_anim_params.state == BOOT_ANIM_STATE_FINISHED) {
    return false; // Animation complete
  }

  update_animation_params(&g_boot_anim_params);

  return true; // Animation still running
}

/**
 * @brief Render current animation frame
 */
bool BootAnimation_Render(void) {
  if (!g_display || !g_animation_initialized) {
    return false;
  }

  g_display->firstPage();
  do {
    g_display->clearBuffer();

    switch (g_boot_anim_params.state) {
    case BOOT_ANIM_STATE_INIT_LINE:
      // Phase 1: Ripple expansion from center with scaling text
      draw_ripples(g_boot_anim_params.split_progress);
      break;

      // case BOOT_ANIM_STATE_SPLIT_LINES:
      //     // Phase 2: Ripples complete, particles activate, text starts
      //     revealing draw_ripples(FIXED_POINT_SCALE); // Keep final ripples
      //     visible draw_particles(g_boot_anim_params.text_alpha);
      //     draw_progressive_text(g_boot_anim_params.text_alpha);
      //     break;

      // case BOOT_ANIM_STATE_SHOW_TEXT:
      //     // Phase 3: Full animation - particles, complete text, progress
      //     ring draw_particles(FIXED_POINT_SCALE);
      //     draw_progressive_text(FIXED_POINT_SCALE);
      //     draw_progress_ring(g_boot_anim_params.progress);
      //     break;

      // case BOOT_ANIM_STATE_COMPLETE:
      // case BOOT_ANIM_STATE_FINISHED:
      //     // Final state - show complete animation
      //     draw_particles(FIXED_POINT_SCALE);
      //     draw_progressive_text(FIXED_POINT_SCALE);
      //     draw_progress_ring(FIXED_POINT_SCALE);
      //     break;

    case BOOT_ANIM_STATE_IDLE:
    default:
      // Do nothing
      break;
    }

  } while (g_display->nextPage());

  return true;
}

/**
 * @brief Check if boot animation is complete
 */
bool BootAnimation_IsComplete(void) {
  return (g_boot_anim_params.state == BOOT_ANIM_STATE_FINISHED);
}

/**
 * @brief Stop and reset boot animation
 */
void BootAnimation_Stop(void) {
  g_boot_anim_params.state = BOOT_ANIM_STATE_IDLE;
  g_boot_anim_params.progress = 0;
  g_boot_anim_params.split_progress = 0;
  g_boot_anim_params.text_alpha = 0;
}

/**
 * @brief Get current animation progress
 */
int32_t BootAnimation_GetProgress(void) { return g_boot_anim_params.progress; }

/**
 * @brief Get current animation state
 */
BootAnimState_t BootAnimation_GetState(void) {
  return g_boot_anim_params.state;
}