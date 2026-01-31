/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file particle_effects.h Particle system for visual effects. */

#ifndef PARTICLE_EFFECTS_H
#define PARTICLE_EFFECTS_H

#include "stdafx.h"
#include "gfx_type.h"
#include "tile_type.h"

/** Particle types */
enum class ParticleType : uint8_t {
	NONE,

	/* Environmental */
	SMOKE_SMALL,         ///< Small smoke puff (chimneys)
	SMOKE_LARGE,         ///< Large smoke cloud (factories)
	SMOKE_TRAIN,         ///< Steam locomotive exhaust
	SMOKE_EXPLOSION,     ///< Explosion smoke
	STEAM,               ///< Steam effect (power plants)
	DUST,                ///< Dust cloud (construction, roads)
	LEAVES,              ///< Falling leaves (autumn)
	SNOW_DRIFT,          ///< Blowing snow
	SPARKS,              ///< Electrical sparks

	/* Vehicle effects */
	EXHAUST_DIESEL,      ///< Diesel engine exhaust
	EXHAUST_AIRCRAFT,    ///< Aircraft jet exhaust
	WAKE_WATER,          ///< Ship wake foam
	BRAKE_SPARKS,        ///< Train braking sparks
	WHEEL_DUST,          ///< Road vehicle dust

	/* Financial/UI */
	MONEY_GAIN,          ///< Green money particles floating up
	MONEY_LOSS,          ///< Red money particles falling
	STARS,               ///< Achievement stars
	CONFETTI,            ///< Celebration confetti

	/* Weather */
	RAIN_SPLASH,         ///< Rain hitting ground
	SNOW_ACCUMULATE,     ///< Snow piling up
	LIGHTNING_FLASH,     ///< Lightning bolt
	FOG_WISP,            ///< Fog particles

	/* Disasters */
	FIRE_SMALL,          ///< Small flames
	FIRE_LARGE,          ///< Large fire
	DEBRIS,              ///< Crash debris
	EXPLOSION_SHRAPNEL,  ///< Explosion fragments

	NUM_PARTICLE_TYPES
};

/** Particle blend modes */
enum class ParticleBlend : uint8_t {
	NORMAL,              ///< Standard alpha blending
	ADDITIVE,            ///< Add to background (glow effects)
	MULTIPLY,            ///< Darken (shadows)
};

/** Individual particle */
struct Particle {
	ParticleType type;
	bool active;

	/* Position (in screen pixels or world coords) */
	float x, y, z;

	/* Velocity */
	float vx, vy, vz;

	/* Appearance */
	uint8_t frame;           ///< Animation frame
	uint8_t alpha;           ///< Transparency (0-255)
	float scale;             ///< Size multiplier
	float rotation;          ///< Rotation in radians
	float rotation_speed;    ///< Rotation per tick

	/* Lifetime */
	uint16_t age;            ///< Current age in ticks
	uint16_t max_age;        ///< Maximum lifetime

	/* Color (for tintable particles) */
	Colour color;

	/* Physics */
	float gravity;           ///< Gravity multiplier
	float drag;              ///< Air resistance
	bool world_space;        ///< Is position in world coords?
};

/** Particle emitter - spawns particles over time */
struct ParticleEmitter {
	ParticleType type;
	bool active;

	/* Position */
	TileIndex tile;          ///< World position
	float offset_x, offset_y, offset_z;

	/* Emission settings */
	float emit_rate;         ///< Particles per tick
	float emit_accumulator;  ///< Fractional emission tracking
	uint16_t max_particles;  ///< Maximum concurrent particles
	uint16_t current_count;  ///< Current particle count

	/* Spawn variation */
	float spawn_radius;      ///< Random position offset
	float velocity_min;      ///< Minimum initial speed
	float velocity_max;      ///< Maximum initial speed
	float angle_min;         ///< Minimum emission angle (radians)
	float angle_max;         ///< Maximum emission angle

	/* Lifetime */
	uint16_t particle_life_min;
	uint16_t particle_life_max;
	bool infinite;           ///< Does emitter run forever?
	uint16_t emitter_life;   ///< Emitter lifetime (0 = infinite)

	/* Visual variation */
	float scale_min, scale_max;
	Colour color_start, color_end;  ///< Color lerp over lifetime
};

/** Particle system configuration */
struct ParticleConfig {
	/* Quality settings */
	uint8_t quality_level;       ///< 0=off, 1=low, 2=medium, 3=high
	uint16_t max_total_particles;
	uint16_t max_emitters;
	bool weather_particles;      ///< Enable weather effects
	bool vehicle_particles;      ///< Enable vehicle effects
	bool ui_particles;           ///< Enable UI particles
};

/** Particle system state */
struct ParticleSystemState {
	static constexpr int MAX_PARTICLES = 2000;
	static constexpr int MAX_EMITTERS = 100;

	Particle particles[MAX_PARTICLES];
	ParticleEmitter emitters[MAX_EMITTERS];

	uint16_t active_particle_count;
	uint16_t active_emitter_count;

	ParticleConfig config;
};

/** Global particle system state */
extern ParticleSystemState _particles;

/** Initialize particle system */
void InitializeParticles();

/** Update all particles (call each tick) */
void UpdateParticles();

/** Render all visible particles */
void RenderParticles();

/** Spawn a single particle at position */
void SpawnParticle(ParticleType type, float x, float y, float z,
                   float vx = 0, float vy = 0, float vz = 0);

/** Spawn a particle at a tile */
void SpawnParticleAtTile(ParticleType type, TileIndex tile);

/** Create a particle emitter */
int CreateEmitter(ParticleType type, TileIndex tile, float rate);

/** Destroy an emitter */
void DestroyEmitter(int emitter_id);

/** Spawn a burst of particles (one-shot) */
void SpawnParticleBurst(ParticleType type, float x, float y, float z,
                        int count, float spread = 1.0f);

/** Spawn explosion effect */
void SpawnExplosionEffect(TileIndex tile, uint8_t intensity);

/** Spawn crash effect */
void SpawnCrashEffect(TileIndex tile);

/** Spawn money effect (UI) */
void SpawnMoneyEffect(int screen_x, int screen_y, int64_t amount);

/** Spawn achievement effect */
void SpawnAchievementEffect(int screen_x, int screen_y);

/** Spawn confetti celebration */
void SpawnConfettiEffect(int screen_x, int screen_y, int count);

/** Get particle type properties */
struct ParticleTypeProperties {
	const char *name;
	uint8_t num_frames;          ///< Animation frames
	ParticleBlend blend;
	float default_scale;
	uint16_t default_lifetime;
	float default_gravity;
	bool affected_by_wind;
};

const ParticleTypeProperties &GetParticleProperties(ParticleType type);

/** Set particle quality level */
void SetParticleQuality(uint8_t level);

#endif /* PARTICLE_EFFECTS_H */
