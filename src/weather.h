/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file weather.h Dynamic weather system for Transport Empire. */

#ifndef WEATHER_H
#define WEATHER_H

#include "stdafx.h"
#include "tile_type.h"
#include "timer/timer_game_calendar.h"
#include "core/geometry_type.hpp"
#include <vector>

/** Weather types */
enum class WeatherType : uint8_t {
	CLEAR,           ///< Clear skies
	CLOUDY,          ///< Overcast but no precipitation
	LIGHT_RAIN,      ///< Light rain
	HEAVY_RAIN,      ///< Heavy rain/thunderstorm
	DRIZZLE,         ///< Misty drizzle
	LIGHT_SNOW,      ///< Light snowfall
	HEAVY_SNOW,      ///< Blizzard conditions
	FOG,             ///< Thick fog
	HAZE,            ///< Light haze
	STORM,           ///< Severe storm with lightning
	NUM_WEATHER_TYPES
};

/** Climate zones affect weather patterns */
enum class ClimateZone : uint8_t {
	TEMPERATE,       ///< Standard temperate weather
	ARCTIC,          ///< Cold, snowy
	TROPICAL,        ///< Hot, rainy seasons
	DESERT,          ///< Mostly clear, occasional sandstorms
};

/** Individual weather particle for rendering */
struct WeatherParticle {
	int32_t x;           ///< Screen X position
	int32_t y;           ///< Screen Y position
	int16_t vx;          ///< Velocity X (for wind)
	int16_t vy;          ///< Velocity Y (fall speed)
	uint8_t type;        ///< Particle type (rain drop, snowflake, etc.)
	uint8_t alpha;       ///< Transparency
	uint8_t size;        ///< Size variant
	uint8_t lifetime;    ///< Frames remaining (for effects)
};

/** Lightning flash effect */
struct LightningFlash {
	uint32_t start_tick;
	uint8_t intensity;   ///< 0-255 brightness
	uint8_t duration;    ///< Frames remaining
	Point position;      ///< Screen position for bolt
	bool active;
};

/** Current weather state */
struct WeatherState {
	WeatherType current_type;
	WeatherType transitioning_to;

	uint8_t intensity;              ///< 0-100 intensity of current weather
	uint8_t transition_progress;    ///< 0-100 progress to next weather
	int8_t wind_direction;          ///< -128 to 127 (negative = left, positive = right)
	uint8_t wind_speed;             ///< 0-100

	uint32_t weather_duration;      ///< Ticks remaining for current weather
	uint32_t last_change_tick;

	/* Rendering state */
	uint16_t num_particles;
	bool lightning_enabled;
	uint8_t visibility_reduction;   ///< 0-100 fog/precipitation visibility reduction

	/* Ambient effects */
	int8_t temperature_modifier;    ///< Affects game speed perception
	uint8_t ambient_darkness;       ///< Additional darkness from clouds (0-50)
};

/** Global weather state */
extern WeatherState _weather;

/** Weather particle pool */
extern std::vector<WeatherParticle> _weather_particles;

/** Active lightning flashes */
extern std::vector<LightningFlash> _lightning_flashes;

/* Weather system functions */

/** Initialize weather system */
void InitializeWeather();

/** Update weather each game tick */
void UpdateWeather();

/** Process weather transitions */
void ProcessWeatherTransition();

/** Update all weather particles for rendering */
void UpdateWeatherParticles(int screen_width, int screen_height);

/** Spawn new weather particles based on current conditions */
void SpawnWeatherParticles(int screen_width, int screen_height);

/** Trigger a lightning flash */
void TriggerLightning();

/** Get appropriate weather for current date/climate */
WeatherType DetermineWeatherForSeason(ClimateZone zone, TimerGameCalendar::Month month);

/** Random weather event chance */
bool ShouldChangeWeather();

/** Get weather description string */
const char *GetWeatherDescription(WeatherType type);

/** Get particle color for weather type */
uint32_t GetWeatherParticleColor(WeatherType type, uint8_t variant);

/** Calculate visibility modifier for fog/precipitation */
uint8_t CalculateVisibilityReduction(WeatherType type, uint8_t intensity);

/** Should we render weather effects? (check settings) */
bool IsWeatherRenderingEnabled();

/** Get wind effect on particles */
void ApplyWindToParticle(WeatherParticle &particle, int8_t wind_dir, uint8_t wind_speed);

/** Weather affects vehicle speeds */
uint8_t GetWeatherSpeedModifier(WeatherType type, uint8_t intensity);

/** Weather affects station ratings */
int8_t GetWeatherStationRatingModifier(WeatherType type);

#endif /* WEATHER_H */
