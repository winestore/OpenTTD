/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file screen_effects.cpp Implementation of screen effects system. */

#include "stdafx.h"
#include "screen_effects.h"
#include "core/random_func.hpp"
#include <algorithm>
#include <cmath>

#include "safeguards.h"

/** Global screen effects state */
ScreenEffectsState _screen_effects;

/** Initialize screen effects */
void InitializeScreenEffects()
{
	_screen_effects.active_count = 0;
	_screen_effects.total_shake_x = 0;
	_screen_effects.total_shake_y = 0;
	_screen_effects.flash_alpha = 0;
	_screen_effects.flash_color = {255, 255, 255, 255};
	_screen_effects.vignette_intensity = 0;
	_screen_effects.vignette_color = {0, 0, 0, 255};
	_screen_effects.desaturation = 0;
	_screen_effects.zoom_modifier = 1.0f;

	for (auto &effect : _screen_effects.effects) {
		effect.active = false;
	}
}

/** Find a free effect slot */
static ScreenEffect *FindFreeEffectSlot()
{
	for (auto &effect : _screen_effects.effects) {
		if (!effect.active) return &effect;
	}
	/* No free slot - find lowest priority/oldest */
	return &_screen_effects.effects[0];
}

/** Trigger screen effect */
void TriggerScreenEffect(ScreenEffectType type, uint8_t intensity, uint8_t duration)
{
	ScreenEffect *effect = FindFreeEffectSlot();

	effect->type = type;
	effect->intensity = intensity;
	effect->duration = duration;
	effect->max_duration = duration;
	effect->offset_x = 0;
	effect->offset_y = 0;
	effect->active = true;

	if (!effect->active) _screen_effects.active_count++;
}

/** Update all effects */
void UpdateScreenEffects()
{
	/* Reset combined values */
	_screen_effects.total_shake_x = 0;
	_screen_effects.total_shake_y = 0;
	_screen_effects.flash_alpha = 0;
	_screen_effects.vignette_intensity = 0;
	_screen_effects.desaturation = 0;
	_screen_effects.zoom_modifier = 1.0f;

	for (auto &effect : _screen_effects.effects) {
		if (!effect.active) continue;

		/* Calculate fade based on remaining duration */
		float fade = static_cast<float>(effect.duration) / static_cast<float>(effect.max_duration);
		uint8_t effective_intensity = static_cast<uint8_t>(effect.intensity * fade);

		switch (effect.type) {
			case ScreenEffectType::SCREEN_SHAKE: {
				/* Random shake within intensity bounds */
				int shake_range = effective_intensity / 8;
				effect.offset_x = RandomRange(shake_range * 2 + 1) - shake_range;
				effect.offset_y = RandomRange(shake_range * 2 + 1) - shake_range;
				_screen_effects.total_shake_x += effect.offset_x;
				_screen_effects.total_shake_y += effect.offset_y;
				break;
			}

			case ScreenEffectType::FLASH_WHITE:
				_screen_effects.flash_color = {255, 255, 255, 255};
				_screen_effects.flash_alpha = std::max(_screen_effects.flash_alpha, effective_intensity);
				break;

			case ScreenEffectType::FLASH_RED:
				_screen_effects.flash_color = {255, 50, 50, 255};
				_screen_effects.flash_alpha = std::max(_screen_effects.flash_alpha, effective_intensity);
				break;

			case ScreenEffectType::FLASH_GREEN:
				_screen_effects.flash_color = {50, 255, 100, 255};
				_screen_effects.flash_alpha = std::max(_screen_effects.flash_alpha, effective_intensity);
				break;

			case ScreenEffectType::FLASH_GOLD:
				_screen_effects.flash_color = {255, 215, 0, 255};
				_screen_effects.flash_alpha = std::max(_screen_effects.flash_alpha, effective_intensity);
				break;

			case ScreenEffectType::VIGNETTE_DARK:
				_screen_effects.vignette_color = {0, 0, 0, 255};
				_screen_effects.vignette_intensity = std::max(_screen_effects.vignette_intensity, effective_intensity);
				break;

			case ScreenEffectType::VIGNETTE_RED:
				_screen_effects.vignette_color = {180, 0, 0, 255};
				_screen_effects.vignette_intensity = std::max(_screen_effects.vignette_intensity, effective_intensity);
				break;

			case ScreenEffectType::DESATURATE:
				_screen_effects.desaturation = std::max(_screen_effects.desaturation, effective_intensity);
				break;

			case ScreenEffectType::ZOOM_PULSE: {
				/* Subtle zoom oscillation */
				float pulse = std::sin(static_cast<float>(effect.max_duration - effect.duration) * 0.5f);
				float zoom_amount = (effective_intensity / 255.0f) * 0.02f * pulse;
				_screen_effects.zoom_modifier += zoom_amount;
				break;
			}

			default:
				break;
		}

		/* Decrement duration */
		effect.duration--;
		if (effect.duration == 0) {
			effect.active = false;
			_screen_effects.active_count--;
		}
	}
}

/** Convenience: Screen shake */
void TriggerScreenShake(uint8_t intensity, uint8_t duration)
{
	TriggerScreenEffect(ScreenEffectType::SCREEN_SHAKE, intensity, duration);
}

/** Convenience: Flash */
void TriggerFlash(Colour color, uint8_t intensity, uint8_t duration)
{
	ScreenEffectType type = ScreenEffectType::FLASH_WHITE;

	if (color.r > 200 && color.g < 100 && color.b < 100) {
		type = ScreenEffectType::FLASH_RED;
	} else if (color.g > 200 && color.r < 150) {
		type = ScreenEffectType::FLASH_GREEN;
	} else if (color.r > 200 && color.g > 180 && color.b < 100) {
		type = ScreenEffectType::FLASH_GOLD;
	}

	TriggerScreenEffect(type, intensity, duration);
}

/** Convenience: Vignette */
void TriggerVignette(Colour color, uint8_t intensity, uint8_t duration)
{
	ScreenEffectType type = (color.r > 100) ? ScreenEffectType::VIGNETTE_RED : ScreenEffectType::VIGNETTE_DARK;
	TriggerScreenEffect(type, intensity, duration);
}

/* ============ Event Triggers ============ */

void OnTrainCrash()
{
	TriggerScreenShake(200, 30);
	TriggerScreenEffect(ScreenEffectType::FLASH_RED, 150, 10);
}

void OnThunderStrike()
{
	TriggerScreenEffect(ScreenEffectType::FLASH_WHITE, 255, 3);
	TriggerScreenShake(100, 15);
}

void OnHostileTakeover()
{
	TriggerScreenEffect(ScreenEffectType::VIGNETTE_RED, 180, 60);
	TriggerScreenShake(80, 20);
	TriggerScreenEffect(ScreenEffectType::FLASH_RED, 100, 8);
}

void OnMajorProfit()
{
	TriggerScreenEffect(ScreenEffectType::FLASH_GREEN, 120, 15);
	TriggerScreenEffect(ScreenEffectType::ZOOM_PULSE, 100, 30);
}

void OnBankruptcyWarning()
{
	TriggerScreenEffect(ScreenEffectType::VIGNETTE_DARK, 150, 120);
	TriggerScreenEffect(ScreenEffectType::DESATURATE, 100, 120);
}

void OnMarketCrash()
{
	TriggerScreenEffect(ScreenEffectType::FLASH_RED, 180, 20);
	TriggerScreenShake(150, 40);
	TriggerScreenEffect(ScreenEffectType::DESATURATE, 80, 90);
	TriggerScreenEffect(ScreenEffectType::CHROMATIC_ABERRATION, 120, 60);
}

void OnMarketBoom()
{
	TriggerScreenEffect(ScreenEffectType::FLASH_GOLD, 150, 20);
	TriggerScreenEffect(ScreenEffectType::ZOOM_PULSE, 80, 40);
}

void OnAchievementUnlocked()
{
	TriggerScreenEffect(ScreenEffectType::FLASH_GOLD, 200, 15);
	TriggerScreenEffect(ScreenEffectType::ZOOM_PULSE, 120, 30);
}

void OnYearEnd()
{
	TriggerScreenEffect(ScreenEffectType::FLASH_WHITE, 60, 10);
}

/* ============ Getters for Rendering ============ */

void GetShakeOffset(int16_t &x, int16_t &y)
{
	x = _screen_effects.total_shake_x;
	y = _screen_effects.total_shake_y;
}

Colour GetFlashOverlay(uint8_t &alpha)
{
	alpha = _screen_effects.flash_alpha;
	return _screen_effects.flash_color;
}

void GetVignetteParams(Colour &color, uint8_t &intensity)
{
	color = _screen_effects.vignette_color;
	intensity = _screen_effects.vignette_intensity;
}

uint8_t GetDesaturationLevel()
{
	return _screen_effects.desaturation;
}

float GetZoomModifier()
{
	return _screen_effects.zoom_modifier;
}
