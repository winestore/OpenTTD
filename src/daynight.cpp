/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file daynight.cpp Implementation of Day/Night cycle system. */

#include "stdafx.h"
#include "daynight.h"
#include "settings_type.h"

#include "safeguards.h"

/** Global day/night state */
DayNightState _daynight;

/** How many game ticks represent one minute of in-game time */
static constexpr uint8_t TICKS_PER_MINUTE = 10;

/** Initialize day/night system */
void InitializeDayNight()
{
	_daynight.hour_of_day = 800; /* Start at 8:00 AM */
	_daynight.current_period = TimeOfDay::MORNING;
	_daynight.ambient_light = 100;
	_daynight.transition_progress = 0;

	_daynight.red_tint = 128;
	_daynight.green_tint = 128;
	_daynight.blue_tint = 128;

	_daynight.streetlights_on = false;
	_daynight.headlights_on = false;
	_daynight.star_visibility = 0;

	/* Default temperate sunrise/sunset */
	_daynight.sunrise_time = 600;  /* 6:00 AM */
	_daynight.sunset_time = 2000;  /* 8:00 PM */

	UpdateSunTimes(TimerGameCalendar::month);
}

/** Sunrise times by month (northern hemisphere style) */
static const uint16_t _sunrise_by_month[] = {
	730,  /* Jan - late sunrise */
	700,  /* Feb */
	630,  /* Mar */
	600,  /* Apr */
	530,  /* May */
	500,  /* Jun - earliest */
	515,  /* Jul */
	545,  /* Aug */
	615,  /* Sep */
	645,  /* Oct */
	715,  /* Nov */
	745,  /* Dec - latest */
};

/** Sunset times by month */
static const uint16_t _sunset_by_month[] = {
	1700, /* Jan - early sunset */
	1745, /* Feb */
	1830, /* Mar */
	1915, /* Apr */
	2000, /* May */
	2100, /* Jun - latest */
	2045, /* Jul */
	1945, /* Aug */
	1845, /* Sep */
	1745, /* Oct */
	1645, /* Nov */
	1615, /* Dec - earliest */
};

/** Update sunrise/sunset based on month */
void UpdateSunTimes(TimerGameCalendar::Month month)
{
	_daynight.sunrise_time = _sunrise_by_month[month];
	_daynight.sunset_time = _sunset_by_month[month];
}

/** Update day/night each tick */
void UpdateDayNight()
{
	/* Advance time */
	static uint8_t tick_counter = 0;
	tick_counter++;

	if (tick_counter >= TICKS_PER_MINUTE) {
		tick_counter = 0;
		_daynight.hour_of_day++; /* Advance one minute (0.01 hours) */

		/* Wrap around at midnight */
		if (_daynight.hour_of_day >= 2400) {
			_daynight.hour_of_day = 0;
		}
	}

	/* Determine current period */
	TimeOfDay new_period = GetTimeOfDay(_daynight.hour_of_day);
	if (new_period != _daynight.current_period) {
		_daynight.current_period = new_period;
		_daynight.transition_progress = 0;
	}

	/* Smooth transition between periods */
	if (_daynight.transition_progress < 100) {
		_daynight.transition_progress += 2;
	}

	/* Calculate lighting */
	_daynight.ambient_light = CalculateAmbientLight(
		_daynight.hour_of_day,
		_daynight.sunrise_time,
		_daynight.sunset_time
	);

	/* Calculate color tint */
	CalculateColorTint(
		_daynight.hour_of_day,
		_daynight.red_tint,
		_daynight.green_tint,
		_daynight.blue_tint
	);

	/* Update lighting state */
	_daynight.streetlights_on = ShouldLightsBeOn();
	_daynight.headlights_on = _daynight.streetlights_on;
	_daynight.star_visibility = GetStarVisibility();

	/* Update sun times at month change */
	static TimerGameCalendar::Month last_month = static_cast<TimerGameCalendar::Month>(255);
	if (last_month != TimerGameCalendar::month) {
		last_month = TimerGameCalendar::month;
		UpdateSunTimes(last_month);
	}
}

/** Get time of day period */
TimeOfDay GetTimeOfDay(uint16_t hour)
{
	if (hour < 500) return TimeOfDay::LATE_NIGHT;
	if (hour < 630) return TimeOfDay::DAWN;
	if (hour < 1000) return TimeOfDay::MORNING;
	if (hour < 1400) return TimeOfDay::MIDDAY;
	if (hour < 1700) return TimeOfDay::AFTERNOON;
	if (hour < 1930) return TimeOfDay::EVENING;
	if (hour < 2100) return TimeOfDay::DUSK;
	if (hour < 2300) return TimeOfDay::NIGHT;
	return TimeOfDay::LATE_NIGHT;
}

/** Calculate ambient light (0-100) */
uint8_t CalculateAmbientLight(uint16_t hour, uint16_t sunrise, uint16_t sunset)
{
	/* Pre-dawn darkness */
	if (hour < sunrise - 100) {
		return 15;
	}

	/* Dawn transition */
	if (hour < sunrise) {
		uint16_t progress = hour - (sunrise - 100);
		return 15 + (progress * 35) / 100; /* 15 -> 50 */
	}

	/* Morning brightening */
	if (hour < sunrise + 200) {
		uint16_t progress = hour - sunrise;
		return 50 + (progress * 50) / 200; /* 50 -> 100 */
	}

	/* Full daylight */
	if (hour < sunset - 200) {
		return 100;
	}

	/* Evening dimming */
	if (hour < sunset) {
		uint16_t progress = hour - (sunset - 200);
		return 100 - (progress * 30) / 200; /* 100 -> 70 */
	}

	/* Dusk */
	if (hour < sunset + 100) {
		uint16_t progress = hour - sunset;
		return 70 - (progress * 40) / 100; /* 70 -> 30 */
	}

	/* Night */
	if (hour < 2300) {
		return 20;
	}

	/* Late night */
	return 15;
}

/** Calculate color tint for time of day */
void CalculateColorTint(uint16_t hour, uint8_t &r, uint8_t &g, uint8_t &b)
{
	/* Default neutral */
	r = g = b = 128;

	/* Dawn - orange/pink tint */
	if (hour >= 500 && hour < 700) {
		r = 155; /* More red */
		g = 125; /* Slightly less green */
		b = 110; /* Less blue */
	}
	/* Morning - slight warm */
	else if (hour >= 700 && hour < 1000) {
		r = 135;
		g = 130;
		b = 125;
	}
	/* Midday - neutral/slightly blue (clear sky) */
	else if (hour >= 1000 && hour < 1600) {
		r = 128;
		g = 130;
		b = 135;
	}
	/* Golden hour */
	else if (hour >= 1600 && hour < 1900) {
		uint16_t progress = hour - 1600;
		r = 135 + (progress * 30) / 300; /* Increasing red */
		g = 125;
		b = 115 - (progress * 20) / 300; /* Decreasing blue */
	}
	/* Sunset - orange/red */
	else if (hour >= 1900 && hour < 2030) {
		r = 170;
		g = 115;
		b = 95;
	}
	/* Dusk - purple/blue */
	else if (hour >= 2030 && hour < 2130) {
		r = 130;
		g = 110;
		b = 145;
	}
	/* Night - blue tint */
	else if (hour >= 2130 || hour < 500) {
		r = 90;
		g = 95;
		b = 130;
	}
}

/** Get day/night color modifier */
Colour GetDayNightColorModifier()
{
	Colour c;
	c.r = _daynight.red_tint;
	c.g = _daynight.green_tint;
	c.b = _daynight.blue_tint;
	c.a = _daynight.ambient_light * 255 / 100;
	return c;
}

/** Get time string */
std::string GetTimeString()
{
	uint8_t hours = _daynight.hour_of_day / 100;
	uint8_t minutes = _daynight.hour_of_day % 100;
	minutes = (minutes * 60) / 100; /* Convert from 0-99 to 0-59 */

	char buffer[8];
	snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
	return std::string(buffer);
}

/** Get period name */
const char *GetTimePeriodName(TimeOfDay period)
{
	switch (period) {
		case TimeOfDay::DAWN: return "Dawn";
		case TimeOfDay::MORNING: return "Morning";
		case TimeOfDay::MIDDAY: return "Midday";
		case TimeOfDay::AFTERNOON: return "Afternoon";
		case TimeOfDay::EVENING: return "Evening";
		case TimeOfDay::DUSK: return "Dusk";
		case TimeOfDay::NIGHT: return "Night";
		case TimeOfDay::LATE_NIGHT: return "Late Night";
		default: return "Unknown";
	}
}

/** Should lights be on? */
bool ShouldLightsBeOn()
{
	/* Lights on when ambient is below 50% */
	return _daynight.ambient_light < 50;
}

/** Get star visibility */
uint8_t GetStarVisibility()
{
	if (_daynight.ambient_light > 40) return 0;
	if (_daynight.ambient_light > 25) {
		return (40 - _daynight.ambient_light) * 100 / 15;
	}
	return 100;
}

/** Is day/night cycle enabled? */
bool IsDayNightEnabled()
{
	/* TODO: Add settings check */
	return true;
}

/** Apply day/night transformation to color */
Colour ApplyDayNightToColor(Colour original)
{
	if (!IsDayNightEnabled()) return original;

	/* Apply tint */
	int r = (original.r * _daynight.red_tint) / 128;
	int g = (original.g * _daynight.green_tint) / 128;
	int b = (original.b * _daynight.blue_tint) / 128;

	/* Apply ambient light */
	r = (r * _daynight.ambient_light) / 100;
	g = (g * _daynight.ambient_light) / 100;
	b = (b * _daynight.ambient_light) / 100;

	Colour result;
	result.r = std::min(r, 255);
	result.g = std::min(g, 255);
	result.b = std::min(b, 255);
	result.a = original.a;

	return result;
}

/** Get darkness multiplier */
float GetDarknessMultiplier()
{
	return static_cast<float>(_daynight.ambient_light) / 100.0f;
}
