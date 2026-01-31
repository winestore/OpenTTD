/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file empire_audio.cpp Implementation of Transport Empire audio system. */

#include "stdafx.h"
#include "empire_audio.h"
#include "core/random_func.hpp"
#include <cstring>
#include <algorithm>

#include "safeguards.h"

/** Global audio state */
EmpireAudioState _empire_audio;

/** Sound effect file mappings */
static const char *_sound_effect_files[] = {
	/* UI Sounds */
	"ui_click.wav",
	"ui_open.wav",
	"ui_close.wav",
	"ui_error.wav",
	"ui_success.wav",

	/* Stock Market */
	"stock_buy.wav",
	"stock_sell.wav",
	"stock_ticker.wav",
	"market_crash.wav",
	"market_boom.wav",
	"takeover_complete.wav",
	"shares_acquired.wav",

	/* Money */
	"cash_register.wav",
	"coins.wav",
	"money_loss.wav",
	"profit_large.wav",
	"bankruptcy_warning.wav",

	/* Achievements */
	"achievement_unlock.wav",
	"milestone.wav",
	"level_up.wav",

	/* Vehicles */
	"train_crash.wav",
	"breakdown.wav",
	"horn_train.wav",
	"horn_ship.wav",

	/* Weather */
	"thunder_near.wav",
	"thunder_distant.wav",
	"lightning.wav",

	/* Notifications */
	"news_breaking.wav",
	"news_normal.wav",
	"ceo_speaks.wav",

	/* Dramatic */
	"dramatic_sting.wav",
	"tension_rise.wav",
	"victory_fanfare.wav",
	"defeat_somber.wav",
};

/** Ambient sound file mappings */
static const char *_ambient_sound_files[] = {
	/* Weather */
	"amb_rain_light.ogg",
	"amb_rain_heavy.ogg",
	"amb_wind.ogg",
	"amb_wind_storm.ogg",
	"amb_thunder_rumble.ogg",
	"amb_snow_wind.ogg",

	/* Time of Day */
	"amb_morning_birds.ogg",
	"amb_daytime.ogg",
	"amb_evening_crickets.ogg",
	"amb_night_quiet.ogg",

	/* Location */
	"amb_city.ogg",
	"amb_countryside.ogg",
	"amb_industrial.ogg",
	"amb_coastal.ogg",

	/* Mood */
	"amb_tension.ogg",
	"amb_prosperity.ogg",
	"amb_desperation.ogg",
};

/** Initialize audio system */
void InitializeEmpireAudio()
{
	_empire_audio.master_volume = 80;
	_empire_audio.music_volume = 70;
	_empire_audio.effects_volume = 100;
	_empire_audio.ambient_volume = 60;

	_empire_audio.current_music_mood = MusicMood::MUSIC_NEUTRAL;
	_empire_audio.current_ambient = AmbientSound::AMB_DAYTIME_BUSY;
	_empire_audio.music_playing = false;
	_empire_audio.ambient_playing = false;

	_empire_audio.music_fade = 1.0f;
	_empire_audio.ambient_fade = 1.0f;

	_empire_audio.tension_high = false;
	_empire_audio.prosperity_high = false;
	_empire_audio.crisis_mode = false;

	_empire_audio.playlist_index = 0;
	_empire_audio.shuffle_enabled = true;
}

/** Update audio each tick */
void UpdateEmpireAudio()
{
	/* Handle crossfades */
	if (_empire_audio.music_fade < 1.0f) {
		_empire_audio.music_fade += 0.01f;
		if (_empire_audio.music_fade > 1.0f) _empire_audio.music_fade = 1.0f;
	}

	if (_empire_audio.ambient_fade < 1.0f) {
		_empire_audio.ambient_fade += 0.005f;
		if (_empire_audio.ambient_fade > 1.0f) _empire_audio.ambient_fade = 1.0f;
	}

	/* Dynamic music mood changes based on game state */
	if (_empire_audio.crisis_mode) {
		if (_empire_audio.current_music_mood != MusicMood::MUSIC_INTENSE) {
			SetMusicMood(MusicMood::MUSIC_INTENSE);
		}
	} else if (_empire_audio.tension_high) {
		if (_empire_audio.current_music_mood != MusicMood::MUSIC_TENSE) {
			SetMusicMood(MusicMood::MUSIC_TENSE);
		}
	} else if (_empire_audio.prosperity_high) {
		if (_empire_audio.current_music_mood != MusicMood::MUSIC_UPBEAT) {
			SetMusicMood(MusicMood::MUSIC_UPBEAT);
		}
	}
}

/** Play sound effect */
void PlaySoundEffect(SoundEffect effect)
{
	PlaySoundEffectVolume(effect, 100);
}

/** Play sound effect with volume */
void PlaySoundEffectVolume(SoundEffect effect, uint8_t volume)
{
	if (effect >= SoundEffect::NUM_SOUND_EFFECTS) return;

	/* Calculate final volume */
	uint8_t final_volume = (_empire_audio.master_volume * _empire_audio.effects_volume * volume) / 10000;

	/* TODO: Actually play the sound using OpenTTD's sound system */
	/* For now, this is the interface - actual playback would use:
	 * SndPlayFx() or similar OpenTTD sound functions */

	(void)final_volume; /* Suppress unused warning until implemented */
}

/** Start ambient sound */
void StartAmbientSound(AmbientSound ambient)
{
	_empire_audio.current_ambient = ambient;
	_empire_audio.ambient_playing = true;
	_empire_audio.ambient_fade = 0.0f; /* Start fade in */

	/* TODO: Start looping ambient sound */
}

/** Stop ambient sound */
void StopAmbientSound()
{
	_empire_audio.ambient_playing = false;
	/* TODO: Stop ambient playback */
}

/** Crossfade to new ambient */
void CrossfadeAmbient(AmbientSound new_ambient, float duration_seconds)
{
	(void)duration_seconds; /* TODO: Use for actual crossfade timing */

	_empire_audio.current_ambient = new_ambient;
	_empire_audio.ambient_fade = 0.0f;
}

/** Set music mood */
void SetMusicMood(MusicMood mood)
{
	if (mood == _empire_audio.current_music_mood) return;

	_empire_audio.current_music_mood = mood;
	_empire_audio.music_fade = 0.0f; /* Start crossfade */

	/* TODO: Transition to appropriate music track */
}

/** Pause music */
void PauseMusic()
{
	_empire_audio.music_playing = false;
	/* TODO: Actually pause */
}

/** Resume music */
void ResumeMusic()
{
	_empire_audio.music_playing = true;
	/* TODO: Actually resume */
}

/** Volume setters */
void SetMasterVolume(uint8_t volume) { _empire_audio.master_volume = std::min(volume, (uint8_t)100); }
void SetMusicVolume(uint8_t volume) { _empire_audio.music_volume = std::min(volume, (uint8_t)100); }
void SetEffectsVolume(uint8_t volume) { _empire_audio.effects_volume = std::min(volume, (uint8_t)100); }
void SetAmbientVolume(uint8_t volume) { _empire_audio.ambient_volume = std::min(volume, (uint8_t)100); }

/** Load custom playlist */
bool LoadCustomPlaylist(const std::string &directory)
{
	_empire_audio.custom_playlist.clear();
	_empire_audio.playlist_index = 0;

	/* TODO: Scan directory for music files (.mp3, .ogg, .wav, .flac) */
	/* This would allow users to add their own music */

	return !_empire_audio.custom_playlist.empty();
}

/** Get sound effect path */
const char *GetSoundEffectPath(SoundEffect effect)
{
	if (effect >= SoundEffect::NUM_SOUND_EFFECTS) return nullptr;
	return _sound_effect_files[static_cast<int>(effect)];
}

/** Get ambient sound path */
const char *GetAmbientSoundPath(AmbientSound ambient)
{
	if (ambient >= AmbientSound::NUM_AMBIENT_SOUNDS) return nullptr;
	return _ambient_sound_files[static_cast<int>(ambient)];
}

/** Update audio for weather */
void UpdateAudioForWeather(const char *weather_type)
{
	if (strcmp(weather_type, "Storm") == 0 || strcmp(weather_type, "Thunderstorm") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_RAIN_HEAVY, 2.0f);
	} else if (strcmp(weather_type, "Heavy Rain") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_RAIN_HEAVY, 1.5f);
	} else if (strcmp(weather_type, "Light Rain") == 0 || strcmp(weather_type, "Drizzle") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_RAIN_LIGHT, 1.5f);
	} else if (strcmp(weather_type, "Blizzard") == 0 || strcmp(weather_type, "Heavy Snow") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_SNOW_WIND, 2.0f);
	} else if (strcmp(weather_type, "Fog") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_NIGHT_QUIET, 2.0f);
	} else {
		/* Clear weather - use time-based ambient */
	}
}

/** Update audio for time of day */
void UpdateAudioForTimeOfDay(const char *time_period)
{
	if (strcmp(time_period, "Dawn") == 0 || strcmp(time_period, "Morning") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_MORNING_BIRDS, 3.0f);
	} else if (strcmp(time_period, "Midday") == 0 || strcmp(time_period, "Afternoon") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_DAYTIME_BUSY, 2.0f);
	} else if (strcmp(time_period, "Evening") == 0 || strcmp(time_period, "Dusk") == 0) {
		CrossfadeAmbient(AmbientSound::AMB_EVENING_CRICKETS, 3.0f);
	} else {
		CrossfadeAmbient(AmbientSound::AMB_NIGHT_QUIET, 3.0f);
	}
}

/** Update audio for game state */
void UpdateAudioForGameState(Money company_value, bool under_attack, bool crisis)
{
	_empire_audio.crisis_mode = crisis;
	_empire_audio.tension_high = under_attack;
	_empire_audio.prosperity_high = (company_value > 100000000); /* $100M+ */

	if (crisis) {
		CrossfadeAmbient(AmbientSound::AMB_TENSION, 1.0f);
	} else if (under_attack) {
		CrossfadeAmbient(AmbientSound::AMB_TENSION, 2.0f);
	} else if (_empire_audio.prosperity_high) {
		CrossfadeAmbient(AmbientSound::AMB_PROSPERITY, 3.0f);
	}
}

/* ========== EVENT-TRIGGERED SOUNDS ========== */

void OnAudioEvent_StockBuy()
{
	PlaySoundEffect(SoundEffect::SFX_STOCK_BUY);
	PlaySoundEffectVolume(SoundEffect::SFX_COINS, 60);
}

void OnAudioEvent_StockSell()
{
	PlaySoundEffect(SoundEffect::SFX_STOCK_SELL);
	PlaySoundEffect(SoundEffect::SFX_CASH_REGISTER);
}

void OnAudioEvent_TakeoverComplete()
{
	PlaySoundEffect(SoundEffect::SFX_TAKEOVER_COMPLETE);
	PlaySoundEffect(SoundEffect::SFX_DRAMATIC_STING);
	SetMusicMood(MusicMood::MUSIC_TRIUMPHANT);
}

void OnAudioEvent_MarketCrash()
{
	PlaySoundEffect(SoundEffect::SFX_MARKET_CRASH);
	PlaySoundEffect(SoundEffect::SFX_TENSION_RISE);
	SetMusicMood(MusicMood::MUSIC_INTENSE);
	CrossfadeAmbient(AmbientSound::AMB_DESPERATION, 0.5f);
}

void OnAudioEvent_Achievement()
{
	PlaySoundEffect(SoundEffect::SFX_ACHIEVEMENT_UNLOCK);
}

void OnAudioEvent_TrainCrash()
{
	PlaySoundEffect(SoundEffect::SFX_TRAIN_CRASH);
}

void OnAudioEvent_Thunder()
{
	/* Randomly choose near or distant thunder */
	if (RandomRange(3) == 0) {
		PlaySoundEffect(SoundEffect::SFX_THUNDER_NEAR);
		PlaySoundEffect(SoundEffect::SFX_LIGHTNING_STRIKE);
	} else {
		PlaySoundEffect(SoundEffect::SFX_THUNDER_DISTANT);
	}
}

void OnAudioEvent_BreakingNews()
{
	PlaySoundEffect(SoundEffect::SFX_NEWS_BREAKING);
}
