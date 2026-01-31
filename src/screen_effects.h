/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file screen_effects.h Screen effects system for visual feedback and game juice. */

#ifndef SCREEN_EFFECTS_H
#define SCREEN_EFFECTS_H

#include "stdafx.h"
#include "gfx_type.h"

/** Types of screen effects */
enum class ScreenEffectType : uint8_t {
	NONE,
	SCREEN_SHAKE,        ///< Camera shake (explosions, thunder, crashes)
	FLASH_WHITE,         ///< White flash (lightning, huge profit)
	FLASH_RED,           ///< Red flash (crash, loss, hostile takeover)
	FLASH_GREEN,         ///< Green flash (profit, success)
	FLASH_GOLD,          ///< Gold flash (achievement, milestone)
	VIGNETTE_DARK,       ///< Dark edges (danger, bankruptcy)
	VIGNETTE_RED,        ///< Red edges (under attack, hostile)
	CHROMATIC_ABERRATION, ///< Screen distortion (chaos, scandal)
	DESATURATE,          ///< Color drain (recession, depression)
	ZOOM_PULSE,          ///< Subtle zoom in/out (heartbeat, tension)
	RADIAL_BLUR,         ///< Speed blur from center (fast forward, time skip)
};

/** Screen effect instance */
struct ScreenEffect {
	ScreenEffectType type;
	uint8_t intensity;       ///< 0-255 strength
	uint8_t duration;        ///< Frames remaining
	uint8_t max_duration;    ///< Original duration (for fade calculation)
	int16_t offset_x;        ///< Shake offset X
	int16_t offset_y;        ///< Shake offset Y
	bool active;
};

/** Screen effects manager */
struct ScreenEffectsState {
	static constexpr int MAX_CONCURRENT_EFFECTS = 8;

	ScreenEffect effects[MAX_CONCURRENT_EFFECTS];
	uint8_t active_count;

	/* Combined effect outputs */
	int16_t total_shake_x;
	int16_t total_shake_y;
	uint8_t flash_alpha;
	Colour flash_color;
	uint8_t vignette_intensity;
	Colour vignette_color;
	uint8_t desaturation;
	float zoom_modifier;
};

/** Global screen effects state */
extern ScreenEffectsState _screen_effects;

/** Initialize screen effects system */
void InitializeScreenEffects();

/** Update all active effects (call each frame) */
void UpdateScreenEffects();

/** Trigger a new screen effect */
void TriggerScreenEffect(ScreenEffectType type, uint8_t intensity, uint8_t duration);

/** Convenience functions for common effects */
void TriggerScreenShake(uint8_t intensity, uint8_t duration);
void TriggerFlash(Colour color, uint8_t intensity, uint8_t duration);
void TriggerVignette(Colour color, uint8_t intensity, uint8_t duration);

/** Event-based effect triggers */
void OnTrainCrash();           ///< Major screen shake + red flash
void OnThunderStrike();        ///< White flash + medium shake
void OnHostileTakeover();      ///< Red vignette + shake
void OnMajorProfit();          ///< Green/gold flash
void OnBankruptcyWarning();    ///< Dark vignette
void OnMarketCrash();          ///< Red flash + chromatic aberration
void OnMarketBoom();           ///< Gold flash
void OnAchievementUnlocked();  ///< Gold flash + zoom pulse
void OnYearEnd();              ///< Subtle flash

/** Get current shake offset for rendering */
void GetShakeOffset(int16_t &x, int16_t &y);

/** Get current flash overlay color and alpha */
Colour GetFlashOverlay(uint8_t &alpha);

/** Get vignette parameters */
void GetVignetteParams(Colour &color, uint8_t &intensity);

/** Should rendering apply desaturation? */
uint8_t GetDesaturationLevel();

/** Get zoom modifier (1.0 = normal) */
float GetZoomModifier();

#endif /* SCREEN_EFFECTS_H */
