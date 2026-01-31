/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file daynight.h Day/Night cycle system for Transport Empire. */

#ifndef DAYNIGHT_H
#define DAYNIGHT_H

#include "stdafx.h"
#include "date_type.h"
#include "gfx_type.h"
#include <string>

/** Time of day phases */
enum class TimeOfDay : uint8_t {
	DAWN,           ///< Early morning, sun rising
	MORNING,        ///< Morning light
	MIDDAY,         ///< Brightest, sun overhead
	AFTERNOON,      ///< Afternoon light
	EVENING,        ///< Golden hour
	DUSK,           ///< Sun setting
	NIGHT,          ///< Dark, lights on
	LATE_NIGHT,     ///< Darkest period
	NUM_TIME_PERIODS
};

/** Day/night cycle state */
struct DayNightState {
	TimeOfDay current_period;
	uint16_t hour_of_day;           ///< 0-2400 (represents 00:00 to 24:00)
	uint8_t ambient_light;          ///< 0-100 brightness level
	uint8_t transition_progress;    ///< 0-100 smooth transition

	/* Color tinting */
	uint8_t red_tint;               ///< Red channel modifier (0-255, 128 = neutral)
	uint8_t green_tint;             ///< Green channel modifier
	uint8_t blue_tint;              ///< Blue channel modifier

	/* Lighting features */
	bool streetlights_on;           ///< Are streetlights/building lights active
	bool headlights_on;             ///< Vehicle headlights
	uint8_t star_visibility;        ///< 0-100 how visible stars are

	/* Season affects day length */
	uint16_t sunrise_time;          ///< Hour*100 of sunrise (e.g., 600 = 6:00 AM)
	uint16_t sunset_time;           ///< Hour*100 of sunset (e.g., 2000 = 8:00 PM)
};

/** Global day/night state */
extern DayNightState _daynight;

/** Initialize day/night system */
void InitializeDayNight();

/** Update day/night cycle each tick */
void UpdateDayNight();

/** Get time of day from hour */
TimeOfDay GetTimeOfDay(uint16_t hour);

/** Update sunrise/sunset times based on season */
void UpdateSunTimes(Month month);

/** Calculate ambient light level for current time */
uint8_t CalculateAmbientLight(uint16_t hour, uint16_t sunrise, uint16_t sunset);

/** Get color tint for current time */
void CalculateColorTint(uint16_t hour, uint8_t &r, uint8_t &g, uint8_t &b);

/** Get palette modification for day/night */
Colour GetDayNightColorModifier();

/** Get time string (e.g., "14:32") */
std::string GetTimeString();

/** Get period description (e.g., "Evening") */
const char *GetTimePeriodName(TimeOfDay period);

/** Should lights be on at this time? */
bool ShouldLightsBeOn();

/** Get star visibility (0-100) */
uint8_t GetStarVisibility();

/** Is day/night cycle enabled? */
bool IsDayNightEnabled();

/** Apply day/night color transformation to a color */
Colour ApplyDayNightToColor(Colour original);

/** Get the darkness multiplier (1.0 = full brightness, 0.0 = complete darkness) */
float GetDarknessMultiplier();

#endif /* DAYNIGHT_H */
