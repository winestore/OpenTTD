/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file game_events.cpp Central event hub implementation. */

#include "stdafx.h"
#include "game_events.h"
#include "empire_news.h"
#include "screen_effects.h"
#include "ceo_dialogue.h"
#include "achievements.h"
#include "weather.h"
#include "daynight.h"
#include "ai_personality.h"
#include "company_base.h"
#include <cstring>
#include <vector>

#include "safeguards.h"

/** Event statistics */
EventStats _event_stats;

/** Event queue for batch processing */
static std::vector<GameEvent> _event_queue;

/** Initialize event system */
void InitializeGameEvents()
{
	_event_stats.total_events_fired = 0;
	_event_stats.events_this_year = 0;
	for (auto &count : _event_stats.events_by_type) {
		count = 0;
	}
	_event_queue.clear();
}

/**
 * FIRE GAME EVENT
 *
 * This is the heart of Transport Empire.
 * Every significant game event flows through here and triggers:
 * 1. News notifications
 * 2. Screen effects
 * 3. Sound cues
 * 4. Achievement checks
 * 5. AI reactions
 * 6. CEO dialogue
 */
void FireGameEvent(const GameEvent &event)
{
	/* Update statistics */
	_event_stats.total_events_fired++;
	_event_stats.events_by_type[static_cast<int>(event.type)]++;
	_event_stats.events_this_year++;

	/* === ROUTE EVENT TO APPROPRIATE HANDLERS === */

	switch (event.type) {
		/* ========== STOCK MARKET EVENTS ========== */
		case GameEventType::EVT_SHARES_BOUGHT:
			GenerateSharePurchaseNews(event.primary_company, event.secondary_company,
				static_cast<uint8_t>(event.int_data & 0xFF),
				static_cast<uint8_t>((event.int_data >> 8) & 0xFF));
			/* TODO: Trigger sound - stock ticker */
			/* Check for controlling interest achievement */
			if (((event.int_data >> 8) & 0xFF) >= 51) {
				/* Check achievement */
			}
			break;

		case GameEventType::EVT_HOSTILE_TAKEOVER_COMPLETE:
			GenerateHostileTakeoverNews(event.primary_company, event.secondary_company);
			OnHostileTakeover(); /* Screen effect */
			/* TODO: Sound - dramatic takeover music sting */
			/* Trigger CEO dialogue from defeated CEO */
			TriggerCEODialogue(event.secondary_company, DialogueTrigger::PLAYER_HOSTILE_TAKEOVER);
			break;

		case GameEventType::EVT_SHARE_PRICE_SURGE:
			/* Flash green, play positive sound */
			TriggerScreenEffect(ScreenEffectType::FLASH_GREEN, 80, 15);
			/* TODO: Sound - cash register / positive chime */
			break;

		case GameEventType::EVT_SHARE_PRICE_CRASH:
			/* Flash red, play negative sound */
			TriggerScreenEffect(ScreenEffectType::FLASH_RED, 100, 15);
			/* TODO: Sound - negative buzzer */
			break;

		/* ========== MARKET-WIDE EVENTS ========== */
		case GameEventType::EVT_MARKET_CRASH:
			GenerateMarketCrashNews(event.string_data);
			OnMarketCrash(); /* Big screen effect */
			/* TODO: Sound - alarm / panic sounds */
			/* Trigger AI reactions - some will panic sell, others buy */
			for (const Company *c : Company::Iterate()) {
				if (c->is_ai) {
					/* AI personality determines reaction */
				}
			}
			break;

		case GameEventType::EVT_MARKET_BOOM:
			GenerateMarketBoomNews();
			OnMarketBoom();
			/* TODO: Sound - triumphant music */
			break;

		case GameEventType::EVT_MARKET_MANIPULATION_DETECTED:
			GenerateInvestigationNews(event.primary_company);
			TriggerScreenEffect(ScreenEffectType::VIGNETTE_RED, 120, 60);
			/* TODO: Sound - police siren / gavel */
			break;

		/* ========== COMPANY EVENTS ========== */
		case GameEventType::EVT_COMPANY_FOUNDED:
			GenerateNewCompanyNews(event.primary_company);
			/* TODO: Sound - fanfare */
			TriggerCEODialogue(event.primary_company, DialogueTrigger::GAME_START);
			break;

		case GameEventType::EVT_COMPANY_BANKRUPTCY_WARNING:
			GenerateBankruptcyNews(event.primary_company, true);
			OnBankruptcyWarning();
			/* TODO: Sound - warning alarm */
			break;

		case GameEventType::EVT_COMPANY_BANKRUPT:
			GenerateBankruptcyNews(event.primary_company, false);
			TriggerScreenEffect(ScreenEffectType::DESATURATE, 150, 90);
			/* TODO: Sound - somber music */
			break;

		case GameEventType::EVT_COMPANY_PROFITABLE_QUARTER:
			if (event.money_amount > 1000000) {
				OnMajorProfit();
				/* TODO: Sound - coin sounds */
			}
			break;

		case GameEventType::EVT_COMPANY_MILESTONE_REACHED:
			GenerateAchievementNews(event.primary_company, event.string_data, "A remarkable achievement!");
			OnAchievementUnlocked();
			/* TODO: Sound - achievement unlock */
			break;

		/* ========== VEHICLE EVENTS ========== */
		case GameEventType::EVT_VEHICLE_CRASHED:
			OnTrainCrash(); /* Screen shake + red flash */
			/* TODO: Sound - crash / explosion */
			/* This could affect company reputation */
			break;

		case GameEventType::EVT_FIRST_VEHICLE_BOUGHT:
			TriggerScreenEffect(ScreenEffectType::FLASH_GREEN, 60, 10);
			/* TODO: Sound - positive chime */
			/* Achievement: First vehicle */
			break;

		/* ========== WEATHER EVENTS ========== */
		case GameEventType::EVT_WEATHER_STORM_STARTED:
			GenerateWeatherNews("Thunderstorm", true);
			/* TODO: Sound - thunder rumble / rain ambient */
			break;

		case GameEventType::EVT_LIGHTNING_STRIKE:
			OnThunderStrike(); /* White flash + shake */
			/* TODO: Sound - thunder crack */
			break;

		case GameEventType::EVT_WEATHER_BLIZZARD:
			GenerateWeatherNews("Blizzard", true);
			TriggerScreenEffect(ScreenEffectType::FLASH_WHITE, 40, 5);
			/* TODO: Sound - wind howling */
			break;

		case GameEventType::EVT_WEATHER_FOG:
			/* Subtle effect */
			TriggerScreenEffect(ScreenEffectType::VIGNETTE_DARK, 60, 120);
			/* TODO: Sound - muffled ambient */
			break;

		/* ========== TIME EVENTS ========== */
		case GameEventType::EVT_DAWN:
			TriggerScreenEffect(ScreenEffectType::FLASH_GOLD, 30, 20);
			/* TODO: Sound - birds chirping / rooster */
			break;

		case GameEventType::EVT_DUSK:
			/* Subtle warm flash */
			TriggerScreenEffect(ScreenEffectType::FLASH_GOLD, 20, 15);
			/* TODO: Sound - evening ambient */
			break;

		case GameEventType::EVT_YEAR_END:
			GenerateYearEndNews(event.int_data);
			OnYearEnd();
			_event_stats.events_this_year = 0;
			/* TODO: Sound - new year celebration */
			break;

		/* ========== ACHIEVEMENT EVENTS ========== */
		case GameEventType::EVT_ACHIEVEMENT_UNLOCKED:
			GenerateAchievementNews(event.primary_company, event.string_data, "");
			OnAchievementUnlocked();
			/* TODO: Sound - achievement unlock fanfare */
			break;

		/* ========== RIVALRY EVENTS ========== */
		case GameEventType::EVT_RIVALRY_STARTED:
			TriggerCEODialogue(event.secondary_company, DialogueTrigger::PLAYER_UNDERCUT_ROUTE);
			TriggerScreenEffect(ScreenEffectType::VIGNETTE_RED, 80, 30);
			/* TODO: Sound - dramatic sting */
			break;

		case GameEventType::EVT_REVENGE_COMPLETE:
			/* Special achievement! */
			OnAchievementUnlocked();
			TriggerScreenEffect(ScreenEffectType::FLASH_GOLD, 150, 20);
			/* TODO: Sound - triumphant fanfare */
			break;

		/* ========== SCANDAL EVENTS ========== */
		case GameEventType::EVT_INVESTIGATION_STARTED:
			GenerateInvestigationNews(event.primary_company);
			TriggerScreenEffect(ScreenEffectType::VIGNETTE_RED, 100, 45);
			/* TODO: Sound - suspenseful music */
			break;

		case GameEventType::EVT_FINE_ISSUED:
			TriggerScreenEffect(ScreenEffectType::FLASH_RED, 80, 10);
			/* TODO: Sound - gavel / cash register (negative) */
			break;

		/* ========== INFRASTRUCTURE EVENTS ========== */
		case GameEventType::EVT_ROUTE_STOLEN:
			TriggerCEODialogue(event.secondary_company, DialogueTrigger::PLAYER_UNDERCUT_ROUTE);
			/* TODO: Sound - negative for victim, positive for thief */
			break;

		case GameEventType::EVT_INDUSTRY_CONNECTED:
			TriggerScreenEffect(ScreenEffectType::FLASH_GREEN, 50, 8);
			/* TODO: Sound - connection sound */
			break;

		default:
			break;
	}

	/* === CHECK ACHIEVEMENTS === */
	CheckAchievements(event.primary_company);
	if (event.secondary_company != CompanyID::Invalid()) {
		CheckAchievements(event.secondary_company);
	}
}

/* ========== CONVENIENCE FUNCTIONS ========== */

void OnSharesBought(CompanyID buyer, CompanyID target, uint8_t amount, uint8_t total)
{
	GameEvent event;
	event.type = GameEventType::EVT_SHARES_BOUGHT;
	event.primary_company = buyer;
	event.secondary_company = target;
	event.int_data = amount | (total << 8);
	FireGameEvent(event);
}

void OnSharesSold(CompanyID seller, CompanyID target, uint8_t amount, uint8_t remaining)
{
	GameEvent event;
	event.type = GameEventType::EVT_SHARES_SOLD;
	event.primary_company = seller;
	event.secondary_company = target;
	event.int_data = amount | (remaining << 8);
	FireGameEvent(event);
}

void OnHostileTakeoverComplete(CompanyID acquirer, CompanyID target)
{
	GameEvent event;
	event.type = GameEventType::EVT_HOSTILE_TAKEOVER_COMPLETE;
	event.primary_company = acquirer;
	event.secondary_company = target;
	FireGameEvent(event);
}

void OnCompanyFounded(CompanyID company)
{
	GameEvent event;
	event.type = GameEventType::EVT_COMPANY_FOUNDED;
	event.primary_company = company;
	FireGameEvent(event);
}

void OnCompanyBankrupt(CompanyID company)
{
	GameEvent event;
	event.type = GameEventType::EVT_COMPANY_BANKRUPT;
	event.primary_company = company;
	FireGameEvent(event);
}

void OnVehicleCrashed(CompanyID owner, VehicleID vehicle)
{
	GameEvent event;
	event.type = GameEventType::EVT_VEHICLE_CRASHED;
	event.primary_company = owner;
	event.vehicle = vehicle;
	FireGameEvent(event);
}

void OnWeatherChange(const char *weather_type, bool severe)
{
	GameEvent event;
	event.type = severe ? GameEventType::EVT_WEATHER_STORM_STARTED : GameEventType::EVT_WEATHER_FOG;
	event.string_data = weather_type;
	FireGameEvent(event);
}

void OnTimeOfDayChange(const char *period)
{
	GameEvent event;
	if (strcmp(period, "Dawn") == 0) {
		event.type = GameEventType::EVT_DAWN;
	} else if (strcmp(period, "Dusk") == 0) {
		event.type = GameEventType::EVT_DUSK;
	} else {
		return; /* Ignore other periods */
	}
	FireGameEvent(event);
}

void OnAchievement(CompanyID company, const char *achievement_id)
{
	GameEvent event;
	event.type = GameEventType::EVT_ACHIEVEMENT_UNLOCKED;
	event.primary_company = company;
	event.string_data = achievement_id;
	FireGameEvent(event);
}

void OnYearEnd(int year)
{
	GameEvent event;
	event.type = GameEventType::EVT_YEAR_END;
	event.int_data = year;
	FireGameEvent(event);
}

void OnQuarterEnd(CompanyID company, Money profit)
{
	GameEvent event;
	event.type = profit > 0 ? GameEventType::EVT_COMPANY_PROFITABLE_QUARTER : GameEventType::EVT_COMPANY_LOSS_QUARTER;
	event.primary_company = company;
	event.money_amount = profit;
	FireGameEvent(event);
}

void OnInvestigationStarted(CompanyID company)
{
	GameEvent event;
	event.type = GameEventType::EVT_INVESTIGATION_STARTED;
	event.primary_company = company;
	FireGameEvent(event);
}

void OnRouteStolen(CompanyID thief, CompanyID victim)
{
	GameEvent event;
	event.type = GameEventType::EVT_ROUTE_STOLEN;
	event.primary_company = thief;
	event.secondary_company = victim;
	FireGameEvent(event);
}

/** Process pending events */
void ProcessGameEvents()
{
	/* Process queued events */
	for (const auto &event : _event_queue) {
		FireGameEvent(event);
	}
	_event_queue.clear();

	/* Process news queue */
	ProcessEmpireNewsQueue();

	/* Update screen effects */
	UpdateScreenEffects();
}
