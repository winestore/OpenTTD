/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file weather.cpp Implementation of dynamic weather system. */

#include "stdafx.h"
#include "weather.h"
#include "core/random_func.hpp"
#include "landscape.h"
#include "settings_type.h"

#include "safeguards.h"

/** Maximum particles to render */
static constexpr uint16_t MAX_WEATHER_PARTICLES = 2000;
static constexpr uint16_t MAX_LIGHTNING_FLASHES = 5;

/** Global weather state */
WeatherState _weather;

/** Particle pool */
std::vector<WeatherParticle> _weather_particles;

/** Lightning flashes */
std::vector<LightningFlash> _lightning_flashes;

/** Initialize weather system */
void InitializeWeather()
{
	_weather.current_type = WeatherType::CLEAR;
	_weather.transitioning_to = WeatherType::CLEAR;
	_weather.intensity = 0;
	_weather.transition_progress = 0;
	_weather.wind_direction = 0;
	_weather.wind_speed = 10;
	_weather.weather_duration = 1000; /* Start with clear weather */
	_weather.last_change_tick = 0;
	_weather.num_particles = 0;
	_weather.lightning_enabled = false;
	_weather.visibility_reduction = 0;
	_weather.temperature_modifier = 0;
	_weather.ambient_darkness = 0;

	_weather_particles.clear();
	_weather_particles.reserve(MAX_WEATHER_PARTICLES);

	_lightning_flashes.clear();
	_lightning_flashes.reserve(MAX_LIGHTNING_FLASHES);
}

/** Weather probability table by season and climate */
static const uint8_t _weather_probability[4][4][static_cast<int>(WeatherType::NUM_WEATHER_TYPES)] = {
	/* TEMPERATE */
	{
		/* Spring */ {30, 25, 20, 5, 10, 5, 0, 3, 2, 0},
		/* Summer */ {45, 20, 15, 10, 5, 0, 0, 2, 3, 5},
		/* Autumn */ {25, 30, 20, 10, 10, 2, 0, 3, 0, 0},
		/* Winter */ {35, 25, 10, 5, 5, 15, 5, 0, 0, 0},
	},
	/* ARCTIC */
	{
		/* Spring */ {20, 20, 10, 5, 5, 25, 10, 5, 0, 0},
		/* Summer */ {40, 25, 15, 5, 5, 5, 0, 3, 2, 0},
		/* Autumn */ {25, 25, 10, 5, 5, 20, 5, 5, 0, 0},
		/* Winter */ {15, 15, 0, 0, 0, 35, 30, 5, 0, 0},
	},
	/* TROPICAL */
	{
		/* Spring */ {35, 20, 25, 15, 5, 0, 0, 0, 0, 0},
		/* Summer */ {20, 15, 25, 25, 5, 0, 0, 0, 5, 5},
		/* Autumn */ {30, 20, 25, 15, 5, 0, 0, 0, 5, 0},
		/* Winter */ {45, 25, 15, 10, 5, 0, 0, 0, 0, 0},
	},
	/* DESERT */
	{
		/* Spring */ {70, 15, 5, 2, 3, 0, 0, 0, 5, 0},
		/* Summer */ {80, 10, 3, 2, 0, 0, 0, 0, 5, 0},
		/* Autumn */ {65, 20, 5, 5, 0, 0, 0, 0, 5, 0},
		/* Winter */ {60, 25, 5, 0, 5, 0, 0, 5, 0, 0},
	},
};

/** Get season from month */
static uint8_t GetSeason(TimerGameCalendar::Month month)
{
	if (month >= 2 && month <= 4) return 0; /* Spring */
	if (month >= 5 && month <= 7) return 1; /* Summer */
	if (month >= 8 && month <= 10) return 2; /* Autumn */
	return 3; /* Winter */
}

/** Determine weather based on season and climate */
WeatherType DetermineWeatherForSeason(ClimateZone zone, TimerGameCalendar::Month month)
{
	uint8_t season = GetSeason(month);
	const uint8_t *probs = _weather_probability[static_cast<uint8_t>(zone)][season];

	uint8_t roll = RandomRange(100);
	uint8_t cumulative = 0;

	for (int i = 0; i < static_cast<int>(WeatherType::NUM_WEATHER_TYPES); i++) {
		cumulative += probs[i];
		if (roll < cumulative) {
			return static_cast<WeatherType>(i);
		}
	}

	return WeatherType::CLEAR;
}

/** Should weather change? */
bool ShouldChangeWeather()
{
	if (_weather.weather_duration > 0) {
		return false;
	}

	/* Base 5% chance per tick when duration expired */
	return RandomRange(100) < 5;
}

/** Update weather each tick */
void UpdateWeather()
{
	/* Decrement duration */
	if (_weather.weather_duration > 0) {
		_weather.weather_duration--;
	}

	/* Check for weather change */
	if (ShouldChangeWeather()) {
		/* Determine new weather */
		ClimateZone zone = ClimateZone::TEMPERATE; /* TODO: Get from landscape */
		WeatherType new_weather = DetermineWeatherForSeason(zone, TimerGameCalendar::month);

		if (new_weather != _weather.current_type) {
			_weather.transitioning_to = new_weather;
			_weather.transition_progress = 0;
		}

		/* Set duration based on weather type */
		switch (new_weather) {
			case WeatherType::STORM:
			case WeatherType::HEAVY_RAIN:
			case WeatherType::HEAVY_SNOW:
				_weather.weather_duration = 200 + RandomRange(300);
				break;
			case WeatherType::FOG:
				_weather.weather_duration = 100 + RandomRange(200);
				break;
			case WeatherType::CLEAR:
				_weather.weather_duration = 500 + RandomRange(1000);
				break;
			default:
				_weather.weather_duration = 300 + RandomRange(500);
				break;
		}
	}

	/* Process transitions */
	ProcessWeatherTransition();

	/* Update wind */
	if (RandomRange(100) < 2) {
		_weather.wind_direction += RandomRange(21) - 10;
		_weather.wind_direction = Clamp(_weather.wind_direction, -100, 100);
	}
	if (RandomRange(100) < 3) {
		int wind_change = RandomRange(11) - 5;
		_weather.wind_speed = Clamp(static_cast<int>(_weather.wind_speed) + wind_change, 0, 100);
	}

	/* Update visibility */
	_weather.visibility_reduction = CalculateVisibilityReduction(_weather.current_type, _weather.intensity);

	/* Update ambient darkness */
	switch (_weather.current_type) {
		case WeatherType::STORM:
			_weather.ambient_darkness = 40 + (_weather.intensity / 5);
			break;
		case WeatherType::HEAVY_RAIN:
		case WeatherType::HEAVY_SNOW:
			_weather.ambient_darkness = 25 + (_weather.intensity / 10);
			break;
		case WeatherType::CLOUDY:
		case WeatherType::LIGHT_RAIN:
		case WeatherType::LIGHT_SNOW:
			_weather.ambient_darkness = 10 + (_weather.intensity / 20);
			break;
		case WeatherType::FOG:
			_weather.ambient_darkness = 15;
			break;
		default:
			_weather.ambient_darkness = 0;
			break;
	}

	/* Lightning during storms */
	if (_weather.current_type == WeatherType::STORM && RandomRange(100) < 3) {
		TriggerLightning();
	}

	/* Update lightning flashes */
	for (auto &flash : _lightning_flashes) {
		if (flash.active) {
			flash.duration--;
			if (flash.duration == 0) {
				flash.active = false;
			}
		}
	}
}

/** Process weather transition */
void ProcessWeatherTransition()
{
	if (_weather.transitioning_to == _weather.current_type) {
		return;
	}

	_weather.transition_progress += 2; /* Gradual transition */

	if (_weather.transition_progress >= 100) {
		_weather.current_type = _weather.transitioning_to;
		_weather.transition_progress = 0;

		/* Set intensity for new weather */
		switch (_weather.current_type) {
			case WeatherType::CLEAR:
			case WeatherType::HAZE:
				_weather.intensity = 0;
				break;
			case WeatherType::CLOUDY:
				_weather.intensity = 20 + RandomRange(30);
				break;
			case WeatherType::LIGHT_RAIN:
			case WeatherType::LIGHT_SNOW:
			case WeatherType::DRIZZLE:
				_weather.intensity = 30 + RandomRange(30);
				break;
			case WeatherType::HEAVY_RAIN:
			case WeatherType::HEAVY_SNOW:
			case WeatherType::FOG:
				_weather.intensity = 60 + RandomRange(40);
				break;
			case WeatherType::STORM:
				_weather.intensity = 80 + RandomRange(20);
				_weather.lightning_enabled = true;
				break;
			default:
				_weather.intensity = 50;
				break;
		}
	}
}

/** Update weather particles */
void UpdateWeatherParticles(int screen_width, int screen_height)
{
	/* Move existing particles */
	for (auto it = _weather_particles.begin(); it != _weather_particles.end(); ) {
		ApplyWindToParticle(*it, _weather.wind_direction, _weather.wind_speed);

		it->x += it->vx;
		it->y += it->vy;

		/* Remove particles that are off screen or expired */
		if (it->y > screen_height + 50 || it->x < -50 || it->x > screen_width + 50 || it->lifetime == 0) {
			it = _weather_particles.erase(it);
		} else {
			if (it->lifetime > 0) it->lifetime--;
			++it;
		}
	}

	/* Spawn new particles */
	SpawnWeatherParticles(screen_width, screen_height);
}

/** Spawn weather particles */
void SpawnWeatherParticles(int screen_width, int screen_height)
{
	if (_weather.current_type == WeatherType::CLEAR ||
	    _weather.current_type == WeatherType::CLOUDY ||
	    _weather.current_type == WeatherType::HAZE) {
		return;
	}

	/* Calculate spawn rate based on intensity */
	uint16_t max_particles = (_weather.intensity * MAX_WEATHER_PARTICLES) / 100;
	uint16_t spawn_count = max_particles / 50; /* Spawn rate per tick */

	if (_weather_particles.size() >= max_particles) {
		return;
	}

	for (uint16_t i = 0; i < spawn_count && _weather_particles.size() < MAX_WEATHER_PARTICLES; i++) {
		WeatherParticle p;
		p.x = RandomRange(screen_width + 100) - 50;
		p.y = -RandomRange(50);
		p.type = static_cast<uint8_t>(_weather.current_type);
		p.lifetime = 255;

		switch (_weather.current_type) {
			case WeatherType::LIGHT_RAIN:
			case WeatherType::DRIZZLE:
				p.vy = 8 + RandomRange(4);
				p.vx = _weather.wind_direction / 20;
				p.alpha = 100 + RandomRange(55);
				p.size = 1;
				break;

			case WeatherType::HEAVY_RAIN:
			case WeatherType::STORM:
				p.vy = 12 + RandomRange(6);
				p.vx = _weather.wind_direction / 10;
				p.alpha = 150 + RandomRange(55);
				p.size = 2;
				break;

			case WeatherType::LIGHT_SNOW:
				p.vy = 2 + RandomRange(2);
				p.vx = _weather.wind_direction / 15 + RandomRange(3) - 1;
				p.alpha = 200 + RandomRange(55);
				p.size = 2 + RandomRange(2);
				break;

			case WeatherType::HEAVY_SNOW:
				p.vy = 3 + RandomRange(3);
				p.vx = _weather.wind_direction / 12 + RandomRange(5) - 2;
				p.alpha = 220 + RandomRange(35);
				p.size = 3 + RandomRange(3);
				break;

			case WeatherType::FOG:
				p.vy = 0;
				p.vx = _weather.wind_direction / 30;
				p.alpha = 30 + RandomRange(30);
				p.size = 20 + RandomRange(40);
				p.lifetime = 100 + RandomRange(100);
				break;

			default:
				p.vy = 5;
				p.vx = 0;
				p.alpha = 128;
				p.size = 1;
				break;
		}

		_weather_particles.push_back(p);
	}
}

/** Trigger lightning */
void TriggerLightning()
{
	/* Find inactive slot or oldest flash */
	LightningFlash *flash = nullptr;
	for (auto &f : _lightning_flashes) {
		if (!f.active) {
			flash = &f;
			break;
		}
	}

	if (flash == nullptr) {
		if (_lightning_flashes.size() < MAX_LIGHTNING_FLASHES) {
			_lightning_flashes.push_back({});
			flash = &_lightning_flashes.back();
		} else {
			flash = &_lightning_flashes[0];
		}
	}

	flash->active = true;
	flash->intensity = 200 + RandomRange(55);
	flash->duration = 3 + RandomRange(5);
	flash->start_tick = 0; /* Will be set by renderer */
	flash->position.x = RandomRange(1920); /* Assume max screen width */
	flash->position.y = RandomRange(200);
}

/** Get weather description */
const char *GetWeatherDescription(WeatherType type)
{
	switch (type) {
		case WeatherType::CLEAR: return "Clear";
		case WeatherType::CLOUDY: return "Cloudy";
		case WeatherType::LIGHT_RAIN: return "Light Rain";
		case WeatherType::HEAVY_RAIN: return "Heavy Rain";
		case WeatherType::DRIZZLE: return "Drizzle";
		case WeatherType::LIGHT_SNOW: return "Light Snow";
		case WeatherType::HEAVY_SNOW: return "Blizzard";
		case WeatherType::FOG: return "Fog";
		case WeatherType::HAZE: return "Haze";
		case WeatherType::STORM: return "Thunderstorm";
		default: return "Unknown";
	}
}

/** Get particle color */
uint32_t GetWeatherParticleColor(WeatherType type, uint8_t variant)
{
	switch (type) {
		case WeatherType::LIGHT_RAIN:
		case WeatherType::HEAVY_RAIN:
		case WeatherType::DRIZZLE:
		case WeatherType::STORM:
			return 0x6688AAFF; /* Blueish rain */

		case WeatherType::LIGHT_SNOW:
		case WeatherType::HEAVY_SNOW:
			return 0xFFFFFFFF; /* White snow */

		case WeatherType::FOG:
			return 0xCCCCCC80; /* Semi-transparent gray */

		default:
			return 0xFFFFFFFF;
	}
}

/** Calculate visibility reduction */
uint8_t CalculateVisibilityReduction(WeatherType type, uint8_t intensity)
{
	switch (type) {
		case WeatherType::FOG:
			return 40 + (intensity / 3);
		case WeatherType::HEAVY_SNOW:
			return 30 + (intensity / 4);
		case WeatherType::STORM:
		case WeatherType::HEAVY_RAIN:
			return 20 + (intensity / 5);
		case WeatherType::LIGHT_SNOW:
		case WeatherType::LIGHT_RAIN:
			return 10 + (intensity / 10);
		case WeatherType::HAZE:
			return 5 + (intensity / 20);
		default:
			return 0;
	}
}

/** Apply wind to particle */
void ApplyWindToParticle(WeatherParticle &particle, int8_t wind_dir, uint8_t wind_speed)
{
	/* Wind affects horizontal movement */
	int wind_effect = (wind_dir * wind_speed) / 500;
	particle.vx += wind_effect / 10;

	/* Clamp horizontal velocity */
	particle.vx = Clamp(particle.vx, static_cast<int16_t>(-20), static_cast<int16_t>(20));
}

/** Get speed modifier from weather */
uint8_t GetWeatherSpeedModifier(WeatherType type, uint8_t intensity)
{
	switch (type) {
		case WeatherType::STORM:
			return 100 - (intensity / 5); /* Up to 20% slower */
		case WeatherType::HEAVY_SNOW:
			return 100 - (intensity / 4); /* Up to 25% slower */
		case WeatherType::HEAVY_RAIN:
			return 100 - (intensity / 10); /* Up to 10% slower */
		case WeatherType::FOG:
			return 100 - (intensity / 8); /* Up to 12% slower */
		case WeatherType::LIGHT_SNOW:
		case WeatherType::LIGHT_RAIN:
			return 100 - (intensity / 20); /* Up to 5% slower */
		default:
			return 100;
	}
}

/** Get station rating modifier from weather */
int8_t GetWeatherStationRatingModifier(WeatherType type)
{
	switch (type) {
		case WeatherType::STORM:
			return -15;
		case WeatherType::HEAVY_RAIN:
		case WeatherType::HEAVY_SNOW:
			return -10;
		case WeatherType::FOG:
			return -5;
		case WeatherType::LIGHT_RAIN:
		case WeatherType::LIGHT_SNOW:
			return -2;
		case WeatherType::CLEAR:
			return 2;
		default:
			return 0;
	}
}

/** Check if weather rendering is enabled */
bool IsWeatherRenderingEnabled()
{
	/* TODO: Add setting check */
	return true;
}
