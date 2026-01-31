/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file particle_effects.cpp Implementation of particle effects system. */

#include "stdafx.h"
#include "particle_effects.h"
#include "core/random_func.hpp"
#include "landscape.h"

#include "safeguards.h"

/** Global particle system state */
ParticleSystemState _particles;

/** Particle type properties database */
static const ParticleTypeProperties PARTICLE_PROPERTIES[] = {
	/* NONE */          {"None", 1, ParticleBlend::NORMAL, 1.0f, 0, 0.0f, false},
	/* SMOKE_SMALL */   {"Small Smoke", 8, ParticleBlend::NORMAL, 0.5f, 60, -0.02f, true},
	/* SMOKE_LARGE */   {"Large Smoke", 12, ParticleBlend::NORMAL, 1.5f, 120, -0.01f, true},
	/* SMOKE_TRAIN */   {"Train Steam", 10, ParticleBlend::NORMAL, 0.8f, 80, -0.03f, true},
	/* SMOKE_EXPLOSION */{"Explosion Smoke", 8, ParticleBlend::NORMAL, 2.0f, 90, -0.01f, false},
	/* STEAM */         {"Steam", 6, ParticleBlend::ADDITIVE, 1.0f, 45, -0.04f, true},
	/* DUST */          {"Dust", 4, ParticleBlend::NORMAL, 0.6f, 40, 0.01f, true},
	/* LEAVES */        {"Leaves", 4, ParticleBlend::NORMAL, 0.3f, 180, 0.02f, true},
	/* SNOW_DRIFT */    {"Snow Drift", 1, ParticleBlend::NORMAL, 0.2f, 60, 0.005f, true},
	/* SPARKS */        {"Sparks", 4, ParticleBlend::ADDITIVE, 0.2f, 20, 0.05f, false},
	/* EXHAUST_DIESEL */{"Diesel Exhaust", 6, ParticleBlend::NORMAL, 0.4f, 50, -0.02f, true},
	/* EXHAUST_AIRCRAFT */{"Jet Exhaust", 8, ParticleBlend::ADDITIVE, 0.6f, 30, 0.0f, false},
	/* WAKE_WATER */    {"Water Wake", 6, ParticleBlend::NORMAL, 1.0f, 90, 0.0f, false},
	/* BRAKE_SPARKS */  {"Brake Sparks", 3, ParticleBlend::ADDITIVE, 0.15f, 15, 0.03f, false},
	/* WHEEL_DUST */    {"Wheel Dust", 4, ParticleBlend::NORMAL, 0.4f, 35, 0.01f, true},
	/* MONEY_GAIN */    {"Money Gain", 1, ParticleBlend::NORMAL, 0.5f, 60, -0.03f, false},
	/* MONEY_LOSS */    {"Money Loss", 1, ParticleBlend::NORMAL, 0.5f, 60, 0.02f, false},
	/* STARS */         {"Stars", 4, ParticleBlend::ADDITIVE, 0.4f, 90, -0.01f, false},
	/* CONFETTI */      {"Confetti", 1, ParticleBlend::NORMAL, 0.3f, 180, 0.015f, true},
	/* RAIN_SPLASH */   {"Rain Splash", 4, ParticleBlend::NORMAL, 0.2f, 10, 0.0f, false},
	/* SNOW_ACCUMULATE */{"Snow Settle", 1, ParticleBlend::NORMAL, 0.1f, 5, 0.0f, false},
	/* LIGHTNING_FLASH */{"Lightning", 3, ParticleBlend::ADDITIVE, 5.0f, 8, 0.0f, false},
	/* FOG_WISP */      {"Fog", 1, ParticleBlend::NORMAL, 3.0f, 300, 0.0f, true},
	/* FIRE_SMALL */    {"Small Fire", 8, ParticleBlend::ADDITIVE, 0.5f, 30, -0.02f, false},
	/* FIRE_LARGE */    {"Large Fire", 12, ParticleBlend::ADDITIVE, 1.5f, 45, -0.02f, false},
	/* DEBRIS */        {"Debris", 1, ParticleBlend::NORMAL, 0.4f, 60, 0.08f, false},
	/* EXPLOSION_SHRAPNEL */{"Shrapnel", 1, ParticleBlend::NORMAL, 0.3f, 45, 0.06f, false},
};

/** Initialize particle system */
void InitializeParticles()
{
	/* Clear all particles */
	for (int i = 0; i < ParticleSystemState::MAX_PARTICLES; i++) {
		_particles.particles[i].active = false;
	}

	/* Clear all emitters */
	for (int i = 0; i < ParticleSystemState::MAX_EMITTERS; i++) {
		_particles.emitters[i].active = false;
	}

	_particles.active_particle_count = 0;
	_particles.active_emitter_count = 0;

	/* Default config */
	_particles.config.quality_level = 2; /* Medium */
	_particles.config.max_total_particles = 1000;
	_particles.config.max_emitters = 50;
	_particles.config.weather_particles = true;
	_particles.config.vehicle_particles = true;
	_particles.config.ui_particles = true;
}

/** Find a free particle slot */
static Particle *FindFreeParticle()
{
	if (_particles.active_particle_count >= _particles.config.max_total_particles) {
		return nullptr;
	}

	for (int i = 0; i < ParticleSystemState::MAX_PARTICLES; i++) {
		if (!_particles.particles[i].active) {
			return &_particles.particles[i];
		}
	}
	return nullptr;
}

/** Find a free emitter slot */
static ParticleEmitter *FindFreeEmitter()
{
	if (_particles.active_emitter_count >= _particles.config.max_emitters) {
		return nullptr;
	}

	for (int i = 0; i < ParticleSystemState::MAX_EMITTERS; i++) {
		if (!_particles.emitters[i].active) {
			return &_particles.emitters[i];
		}
	}
	return nullptr;
}

/** Update a single particle */
static void UpdateParticle(Particle &p)
{
	if (!p.active) return;

	/* Age */
	p.age++;
	if (p.age >= p.max_age) {
		p.active = false;
		_particles.active_particle_count--;
		return;
	}

	/* Physics */
	p.vy += p.gravity;
	p.vx *= (1.0f - p.drag);
	p.vy *= (1.0f - p.drag);
	p.vz *= (1.0f - p.drag);

	p.x += p.vx;
	p.y += p.vy;
	p.z += p.vz;

	/* Rotation */
	p.rotation += p.rotation_speed;

	/* Fade out near end of life */
	float life_ratio = static_cast<float>(p.age) / static_cast<float>(p.max_age);
	if (life_ratio > 0.7f) {
		p.alpha = static_cast<uint8_t>(255 * (1.0f - (life_ratio - 0.7f) / 0.3f));
	}

	/* Animation */
	const ParticleTypeProperties &props = GetParticleProperties(p.type);
	if (props.num_frames > 1) {
		p.frame = static_cast<uint8_t>((p.age / 4) % props.num_frames);
	}
}

/** Update a single emitter */
static void UpdateEmitter(ParticleEmitter &e)
{
	if (!e.active) return;

	/* Lifetime check */
	if (!e.infinite && e.emitter_life > 0) {
		e.emitter_life--;
		if (e.emitter_life == 0) {
			e.active = false;
			_particles.active_emitter_count--;
			return;
		}
	}

	/* Emit particles */
	e.emit_accumulator += e.emit_rate;
	while (e.emit_accumulator >= 1.0f && e.current_count < e.max_particles) {
		e.emit_accumulator -= 1.0f;

		Particle *p = FindFreeParticle();
		if (p == nullptr) break;

		/* Initialize particle */
		p->type = e.type;
		p->active = true;

		/* Position with random offset */
		float angle = static_cast<float>(RandomRange(628)) / 100.0f; /* 0 to 2*PI */
		float radius = static_cast<float>(RandomRange(100)) / 100.0f * e.spawn_radius;
		p->x = e.offset_x + radius * cos(angle);
		p->y = e.offset_y;
		p->z = e.offset_z + radius * sin(angle);
		p->world_space = true;

		/* Velocity */
		float speed = e.velocity_min + static_cast<float>(RandomRange(100)) / 100.0f * (e.velocity_max - e.velocity_min);
		float emit_angle = e.angle_min + static_cast<float>(RandomRange(100)) / 100.0f * (e.angle_max - e.angle_min);
		p->vx = speed * cos(emit_angle) * (RandomRange(2) ? 1.0f : -1.0f);
		p->vy = -speed; /* Default upward */
		p->vz = speed * sin(emit_angle) * (RandomRange(2) ? 1.0f : -1.0f);

		/* Lifetime */
		p->age = 0;
		p->max_age = e.particle_life_min + RandomRange(e.particle_life_max - e.particle_life_min + 1);

		/* Appearance */
		p->scale = e.scale_min + static_cast<float>(RandomRange(100)) / 100.0f * (e.scale_max - e.scale_min);
		p->alpha = 255;
		p->frame = 0;
		p->rotation = static_cast<float>(RandomRange(628)) / 100.0f;
		p->rotation_speed = static_cast<float>(RandomRange(20) - 10) / 100.0f;

		/* Physics */
		const ParticleTypeProperties &props = GetParticleProperties(e.type);
		p->gravity = props.default_gravity;
		p->drag = 0.02f;

		/* Color */
		p->color = e.color_start;

		_particles.active_particle_count++;
		e.current_count++;
	}
}

/** Update all particles */
void UpdateParticles()
{
	if (_particles.config.quality_level == 0) return;

	/* Update emitters first */
	for (int i = 0; i < ParticleSystemState::MAX_EMITTERS; i++) {
		UpdateEmitter(_particles.emitters[i]);
	}

	/* Update all particles */
	for (int i = 0; i < ParticleSystemState::MAX_PARTICLES; i++) {
		UpdateParticle(_particles.particles[i]);
	}
}

/** Render all visible particles */
void RenderParticles()
{
	if (_particles.config.quality_level == 0) return;

	/* TODO: Actual rendering integration with OpenTTD's blitter */
	/* This would iterate through active particles and draw their sprites */
}

/** Spawn a single particle at position */
void SpawnParticle(ParticleType type, float x, float y, float z, float vx, float vy, float vz)
{
	if (_particles.config.quality_level == 0) return;

	Particle *p = FindFreeParticle();
	if (p == nullptr) return;

	const ParticleTypeProperties &props = GetParticleProperties(type);

	p->type = type;
	p->active = true;
	p->x = x;
	p->y = y;
	p->z = z;
	p->vx = vx;
	p->vy = vy;
	p->vz = vz;
	p->frame = 0;
	p->alpha = 255;
	p->scale = props.default_scale;
	p->rotation = 0;
	p->rotation_speed = 0;
	p->age = 0;
	p->max_age = props.default_lifetime;
	p->gravity = props.default_gravity;
	p->drag = 0.02f;
	p->world_space = true;
	p->color = {255, 255, 255, 255};

	_particles.active_particle_count++;
}

/** Spawn a particle at a tile */
void SpawnParticleAtTile(ParticleType type, TileIndex tile)
{
	/* Convert tile to screen position */
	/* TODO: Proper coordinate conversion */
	float x = static_cast<float>(TileX(tile) * 16);
	float z = static_cast<float>(TileY(tile) * 16);
	float y = 0; /* TODO: Get tile height */

	SpawnParticle(type, x, y, z);
}

/** Create a particle emitter */
int CreateEmitter(ParticleType type, TileIndex tile, float rate)
{
	ParticleEmitter *e = FindFreeEmitter();
	if (e == nullptr) return -1;

	int id = static_cast<int>(e - _particles.emitters);

	e->type = type;
	e->active = true;
	e->tile = tile;
	e->offset_x = static_cast<float>(TileX(tile) * 16 + 8);
	e->offset_y = 0;
	e->offset_z = static_cast<float>(TileY(tile) * 16 + 8);
	e->emit_rate = rate;
	e->emit_accumulator = 0;
	e->max_particles = 50;
	e->current_count = 0;
	e->spawn_radius = 4.0f;
	e->velocity_min = 0.5f;
	e->velocity_max = 1.5f;
	e->angle_min = 0;
	e->angle_max = 6.28f;

	const ParticleTypeProperties &props = GetParticleProperties(type);
	e->particle_life_min = static_cast<uint16_t>(props.default_lifetime * 0.8f);
	e->particle_life_max = static_cast<uint16_t>(props.default_lifetime * 1.2f);
	e->infinite = true;
	e->emitter_life = 0;
	e->scale_min = props.default_scale * 0.8f;
	e->scale_max = props.default_scale * 1.2f;
	e->color_start = {255, 255, 255, 255};
	e->color_end = {255, 255, 255, 0};

	_particles.active_emitter_count++;

	return id;
}

/** Destroy an emitter */
void DestroyEmitter(int emitter_id)
{
	if (emitter_id < 0 || emitter_id >= ParticleSystemState::MAX_EMITTERS) return;

	if (_particles.emitters[emitter_id].active) {
		_particles.emitters[emitter_id].active = false;
		_particles.active_emitter_count--;
	}
}

/** Spawn a burst of particles */
void SpawnParticleBurst(ParticleType type, float x, float y, float z, int count, float spread)
{
	for (int i = 0; i < count; i++) {
		float angle = static_cast<float>(RandomRange(628)) / 100.0f;
		float speed = spread * static_cast<float>(RandomRange(100) + 50) / 100.0f;

		float vx = speed * cos(angle);
		float vy = -speed * 0.5f + static_cast<float>(RandomRange(100) - 50) / 100.0f;
		float vz = speed * sin(angle);

		SpawnParticle(type, x, y, z, vx, vy, vz);
	}
}

/** Spawn explosion effect */
void SpawnExplosionEffect(TileIndex tile, uint8_t intensity)
{
	float x = static_cast<float>(TileX(tile) * 16 + 8);
	float z = static_cast<float>(TileY(tile) * 16 + 8);
	float y = 0;

	/* Fire core */
	SpawnParticleBurst(ParticleType::FIRE_LARGE, x, y, z, intensity, 2.0f);

	/* Smoke cloud */
	SpawnParticleBurst(ParticleType::SMOKE_EXPLOSION, x, y - 5, z, intensity * 2, 3.0f);

	/* Debris */
	SpawnParticleBurst(ParticleType::DEBRIS, x, y, z, intensity / 2, 4.0f);

	/* Shrapnel */
	SpawnParticleBurst(ParticleType::EXPLOSION_SHRAPNEL, x, y, z, intensity / 2, 5.0f);

	/* Sparks */
	SpawnParticleBurst(ParticleType::SPARKS, x, y, z, intensity, 3.0f);
}

/** Spawn crash effect */
void SpawnCrashEffect(TileIndex tile)
{
	SpawnExplosionEffect(tile, 20);
}

/** Spawn money effect (UI) */
void SpawnMoneyEffect(int screen_x, int screen_y, int64_t amount)
{
	if (!_particles.config.ui_particles) return;

	ParticleType type = (amount >= 0) ? ParticleType::MONEY_GAIN : ParticleType::MONEY_LOSS;
	int count = std::min(10, static_cast<int>(std::abs(amount) / 10000) + 1);

	for (int i = 0; i < count; i++) {
		float x = static_cast<float>(screen_x + RandomRange(40) - 20);
		float y = static_cast<float>(screen_y);
		float vy = (amount >= 0) ? -1.5f : 1.0f;

		Particle *p = FindFreeParticle();
		if (p == nullptr) break;

		p->type = type;
		p->active = true;
		p->x = x;
		p->y = y;
		p->z = 0;
		p->vx = static_cast<float>(RandomRange(20) - 10) / 20.0f;
		p->vy = vy;
		p->vz = 0;
		p->frame = 0;
		p->alpha = 255;
		p->scale = 0.5f;
		p->rotation = 0;
		p->rotation_speed = 0;
		p->age = 0;
		p->max_age = 60;
		p->gravity = (amount >= 0) ? 0.0f : 0.02f;
		p->drag = 0.01f;
		p->world_space = false;
		p->color = (amount >= 0) ? Colour{100, 255, 100, 255} : Colour{255, 100, 100, 255};

		_particles.active_particle_count++;
	}
}

/** Spawn achievement effect */
void SpawnAchievementEffect(int screen_x, int screen_y)
{
	if (!_particles.config.ui_particles) return;

	/* Starburst effect */
	for (int i = 0; i < 12; i++) {
		float angle = static_cast<float>(i) / 12.0f * 6.28f;

		Particle *p = FindFreeParticle();
		if (p == nullptr) break;

		p->type = ParticleType::STARS;
		p->active = true;
		p->x = static_cast<float>(screen_x);
		p->y = static_cast<float>(screen_y);
		p->z = 0;
		p->vx = cos(angle) * 2.0f;
		p->vy = sin(angle) * 2.0f;
		p->vz = 0;
		p->frame = 0;
		p->alpha = 255;
		p->scale = 0.4f + static_cast<float>(RandomRange(20)) / 100.0f;
		p->rotation = angle;
		p->rotation_speed = 0.1f;
		p->age = 0;
		p->max_age = 90;
		p->gravity = 0;
		p->drag = 0.02f;
		p->world_space = false;
		p->color = {255, 215, 0, 255}; /* Gold */

		_particles.active_particle_count++;
	}
}

/** Spawn confetti celebration */
void SpawnConfettiEffect(int screen_x, int screen_y, int count)
{
	if (!_particles.config.ui_particles) return;

	static const Colour CONFETTI_COLORS[] = {
		{255, 0, 0, 255},     /* Red */
		{0, 255, 0, 255},     /* Green */
		{0, 0, 255, 255},     /* Blue */
		{255, 255, 0, 255},   /* Yellow */
		{255, 0, 255, 255},   /* Magenta */
		{0, 255, 255, 255},   /* Cyan */
		{255, 128, 0, 255},   /* Orange */
		{255, 192, 203, 255}, /* Pink */
	};

	for (int i = 0; i < count; i++) {
		Particle *p = FindFreeParticle();
		if (p == nullptr) break;

		p->type = ParticleType::CONFETTI;
		p->active = true;
		p->x = static_cast<float>(screen_x + RandomRange(100) - 50);
		p->y = static_cast<float>(screen_y - RandomRange(20));
		p->z = 0;
		p->vx = static_cast<float>(RandomRange(40) - 20) / 10.0f;
		p->vy = -2.0f - static_cast<float>(RandomRange(20)) / 10.0f;
		p->vz = 0;
		p->frame = 0;
		p->alpha = 255;
		p->scale = 0.2f + static_cast<float>(RandomRange(20)) / 100.0f;
		p->rotation = static_cast<float>(RandomRange(628)) / 100.0f;
		p->rotation_speed = static_cast<float>(RandomRange(20) - 10) / 50.0f;
		p->age = 0;
		p->max_age = 180;
		p->gravity = 0.015f;
		p->drag = 0.005f;
		p->world_space = false;
		p->color = CONFETTI_COLORS[RandomRange(lengthof(CONFETTI_COLORS))];

		_particles.active_particle_count++;
	}
}

/** Get particle type properties */
const ParticleTypeProperties &GetParticleProperties(ParticleType type)
{
	uint8_t idx = static_cast<uint8_t>(type);
	if (idx >= lengthof(PARTICLE_PROPERTIES)) idx = 0;
	return PARTICLE_PROPERTIES[idx];
}

/** Set particle quality level */
void SetParticleQuality(uint8_t level)
{
	_particles.config.quality_level = std::min(level, static_cast<uint8_t>(3));

	switch (level) {
		case 0:
			_particles.config.max_total_particles = 0;
			_particles.config.max_emitters = 0;
			break;
		case 1:
			_particles.config.max_total_particles = 300;
			_particles.config.max_emitters = 20;
			break;
		case 2:
			_particles.config.max_total_particles = 1000;
			_particles.config.max_emitters = 50;
			break;
		case 3:
			_particles.config.max_total_particles = 2000;
			_particles.config.max_emitters = 100;
			break;
	}
}
