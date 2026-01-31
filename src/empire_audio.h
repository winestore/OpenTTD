/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file empire_audio.h Transport Empire audio system - ambient sounds, music, effects. */

#ifndef EMPIRE_AUDIO_H
#define EMPIRE_AUDIO_H

#include "stdafx.h"
#include "economy_type.h"
#include <vector>
#include <string>

/**
 * TRANSPORT EMPIRE AUDIO SYSTEM
 *
 * Three layers of audio:
 * 1. AMBIENT - Background environmental sounds (weather, time of day)
 * 2. MUSIC - Dynamic soundtrack that responds to game state
 * 3. EFFECTS - One-shot sounds for events (crashes, achievements, etc.)
 */

/** Sound effect identifiers */
enum class SoundEffect : uint8_t {
	/* UI Sounds */
	SFX_UI_CLICK,
	SFX_UI_OPEN_WINDOW,
	SFX_UI_CLOSE_WINDOW,
	SFX_UI_ERROR,
	SFX_UI_SUCCESS,

	/* Stock Market Sounds */
	SFX_STOCK_BUY,
	SFX_STOCK_SELL,
	SFX_STOCK_TICKER,
	SFX_MARKET_CRASH,
	SFX_MARKET_BOOM,
	SFX_TAKEOVER_COMPLETE,
	SFX_SHARES_ACQUIRED,

	/* Money Sounds */
	SFX_CASH_REGISTER,
	SFX_COINS,
	SFX_MONEY_LOSS,
	SFX_PROFIT_LARGE,
	SFX_BANKRUPTCY_WARNING,

	/* Achievement Sounds */
	SFX_ACHIEVEMENT_UNLOCK,
	SFX_MILESTONE_REACHED,
	SFX_LEVEL_UP,

	/* Vehicle Sounds */
	SFX_TRAIN_CRASH,
	SFX_VEHICLE_BREAKDOWN,
	SFX_HORN_TRAIN,
	SFX_HORN_SHIP,

	/* Weather Sounds */
	SFX_THUNDER_NEAR,
	SFX_THUNDER_DISTANT,
	SFX_LIGHTNING_STRIKE,

	/* Notification Sounds */
	SFX_NEWS_BREAKING,
	SFX_NEWS_NORMAL,
	SFX_CEO_SPEAKS,

	/* Dramatic Sounds */
	SFX_DRAMATIC_STING,
	SFX_TENSION_RISE,
	SFX_VICTORY_FANFARE,
	SFX_DEFEAT_SOMBER,

	NUM_SOUND_EFFECTS
};

/** Ambient sound types */
enum class AmbientSound : uint8_t {
	/* Weather Ambient */
	AMB_RAIN_LIGHT,
	AMB_RAIN_HEAVY,
	AMB_WIND,
	AMB_WIND_STORM,
	AMB_THUNDER_RUMBLE,
	AMB_SNOW_WIND,

	/* Time of Day Ambient */
	AMB_MORNING_BIRDS,
	AMB_DAYTIME_BUSY,
	AMB_EVENING_CRICKETS,
	AMB_NIGHT_QUIET,

	/* Location Ambient */
	AMB_CITY,
	AMB_COUNTRYSIDE,
	AMB_INDUSTRIAL,
	AMB_COASTAL,

	/* Mood Ambient */
	AMB_TENSION,
	AMB_PROSPERITY,
	AMB_DESPERATION,

	NUM_AMBIENT_SOUNDS
};

/** Music mood categories */
enum class MusicMood : uint8_t {
	MUSIC_NEUTRAL,       ///< Standard gameplay
	MUSIC_UPBEAT,        ///< Things are going well
	MUSIC_TENSE,         ///< Competition/rivalry
	MUSIC_DRAMATIC,      ///< Takeover in progress
	MUSIC_TRIUMPHANT,    ///< Major victory
	MUSIC_SOMBER,        ///< Defeat/bankruptcy
	MUSIC_PEACEFUL,      ///< Quiet moment
	MUSIC_INTENSE,       ///< Market crash/crisis
};

/** Audio state */
struct EmpireAudioState {
	/* Volume levels (0-100) */
	uint8_t master_volume;
	uint8_t music_volume;
	uint8_t effects_volume;
	uint8_t ambient_volume;

	/* Current playback state */
	MusicMood current_music_mood;
	AmbientSound current_ambient;
	bool music_playing;
	bool ambient_playing;

	/* Crossfade state */
	float music_fade;
	float ambient_fade;

	/* Dynamic music triggers */
	bool tension_high;
	bool prosperity_high;
	bool crisis_mode;

	/* Custom playlist support */
	std::vector<std::string> custom_playlist;
	size_t playlist_index;
	bool shuffle_enabled;
};

/** Global audio state */
extern EmpireAudioState _empire_audio;

/** Initialize audio system */
void InitializeEmpireAudio();

/** Update audio (call each tick) */
void UpdateEmpireAudio();

/** Play a sound effect */
void PlaySoundEffect(SoundEffect effect);

/** Play sound effect with volume modifier */
void PlaySoundEffectVolume(SoundEffect effect, uint8_t volume);

/** Start ambient sound loop */
void StartAmbientSound(AmbientSound ambient);

/** Stop ambient sound */
void StopAmbientSound();

/** Crossfade to new ambient */
void CrossfadeAmbient(AmbientSound new_ambient, float duration_seconds);

/** Set music mood (will crossfade) */
void SetMusicMood(MusicMood mood);

/** Pause/resume music */
void PauseMusic();
void ResumeMusic();

/** Set volume levels */
void SetMasterVolume(uint8_t volume);
void SetMusicVolume(uint8_t volume);
void SetEffectsVolume(uint8_t volume);
void SetAmbientVolume(uint8_t volume);

/** Load custom music playlist from directory */
bool LoadCustomPlaylist(const std::string &directory);

/** Get sound file path for effect */
const char *GetSoundEffectPath(SoundEffect effect);

/** Get ambient sound file path */
const char *GetAmbientSoundPath(AmbientSound ambient);

/** Audio based on game state */
void UpdateAudioForWeather(const char *weather_type);
void UpdateAudioForTimeOfDay(const char *time_period);
void UpdateAudioForGameState(Money company_value, bool under_attack, bool crisis);

/** Event-triggered sounds */
void OnAudioEvent_StockBuy();
void OnAudioEvent_StockSell();
void OnAudioEvent_TakeoverComplete();
void OnAudioEvent_MarketCrash();
void OnAudioEvent_Achievement();
void OnAudioEvent_TrainCrash();
void OnAudioEvent_Thunder();
void OnAudioEvent_BreakingNews();

#endif /* EMPIRE_AUDIO_H */
