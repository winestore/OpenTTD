/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file game_events.h Central event hub - connects all Transport Empire systems. */

#ifndef GAME_EVENTS_H
#define GAME_EVENTS_H

#include "stdafx.h"
#include "company_type.h"
#include "vehicle_type.h"
#include "stock_market.h"

/**
 * GAME EVENTS HUB
 *
 * This is the central nervous system of Transport Empire.
 * When ANYTHING happens, it goes through here and triggers:
 * - News notifications
 * - Screen effects
 * - Sound effects
 * - Achievement checks
 * - AI reactions
 * - CEO dialogue
 */

/** Event types that flow through the hub */
enum class GameEventType : uint8_t {
	/* Stock Market Events */
	EVT_SHARES_BOUGHT,
	EVT_SHARES_SOLD,
	EVT_CONTROLLING_INTEREST,
	EVT_HOSTILE_TAKEOVER_COMPLETE,
	EVT_SHARE_PRICE_SURGE,
	EVT_SHARE_PRICE_CRASH,

	/* Market-wide Events */
	EVT_MARKET_CRASH,
	EVT_MARKET_BOOM,
	EVT_MARKET_MANIPULATION_DETECTED,

	/* Company Events */
	EVT_COMPANY_FOUNDED,
	EVT_COMPANY_BANKRUPTCY_WARNING,
	EVT_COMPANY_BANKRUPT,
	EVT_COMPANY_PROFITABLE_QUARTER,
	EVT_COMPANY_LOSS_QUARTER,
	EVT_COMPANY_MILESTONE_REACHED,

	/* Vehicle Events */
	EVT_VEHICLE_CRASHED,
	EVT_VEHICLE_BREAKDOWN,
	EVT_FIRST_VEHICLE_BOUGHT,
	EVT_FLEET_MILESTONE,

	/* Weather Events */
	EVT_WEATHER_STORM_STARTED,
	EVT_WEATHER_STORM_ENDED,
	EVT_WEATHER_BLIZZARD,
	EVT_WEATHER_FOG,
	EVT_LIGHTNING_STRIKE,

	/* Time Events */
	EVT_DAWN,
	EVT_DUSK,
	EVT_MIDNIGHT,
	EVT_YEAR_END,
	EVT_DECADE_END,

	/* Achievement Events */
	EVT_ACHIEVEMENT_UNLOCKED,

	/* Rivalry Events */
	EVT_RIVALRY_STARTED,
	EVT_RIVALRY_INTENSIFIED,
	EVT_REVENGE_COMPLETE,

	/* Scandal Events */
	EVT_INVESTIGATION_STARTED,
	EVT_INVESTIGATION_COMPLETE,
	EVT_FINE_ISSUED,

	/* Infrastructure Events */
	EVT_ROUTE_STOLEN,
	EVT_INDUSTRY_CONNECTED,
	EVT_STATION_BUILT,

	NUM_EVENT_TYPES
};

/** Event data passed to handlers */
struct GameEvent {
	GameEventType type;
	CompanyID primary_company;
	CompanyID secondary_company;
	VehicleID vehicle;
	Money money_amount;
	int32_t int_data;
	const char *string_data;
};

/** Initialize the event system */
void InitializeGameEvents();

/** Fire an event - this triggers ALL connected systems */
void FireGameEvent(const GameEvent &event);

/** Convenience functions for common events */
void OnSharesBought(CompanyID buyer, CompanyID target, uint8_t amount, uint8_t total);
void OnSharesSold(CompanyID seller, CompanyID target, uint8_t amount, uint8_t remaining);
void OnHostileTakeoverComplete(CompanyID acquirer, CompanyID target);
void OnCompanyFounded(CompanyID company);
void OnCompanyBankrupt(CompanyID company);
void OnVehicleCrashed(CompanyID owner, VehicleID vehicle);
void OnWeatherChange(const char *weather_type, bool severe);
void OnTimeOfDayChange(const char *period);
void OnAchievement(CompanyID company, const char *achievement_id);
void OnYearEnd(int year);
void OnQuarterEnd(CompanyID company, Money profit);
void OnInvestigationStarted(CompanyID company);
void OnRouteStolen(CompanyID thief, CompanyID victim);

/** Process pending events (call each tick) */
void ProcessGameEvents();

/** Event statistics */
struct EventStats {
	uint32_t total_events_fired;
	uint32_t events_by_type[static_cast<int>(GameEventType::NUM_EVENT_TYPES)];
	uint32_t events_this_year;
};

extern EventStats _event_stats;

#endif /* GAME_EVENTS_H */
