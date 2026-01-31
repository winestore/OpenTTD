/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file ai_personality.h AI Personality system for Transport Empire - creates diverse, interesting competitors. */

#ifndef AI_PERSONALITY_H
#define AI_PERSONALITY_H

#include "stdafx.h"
#include "company_type.h"
#include "core/random_func.hpp"
#include <string>
#include <vector>

/** CEO personality archetypes - each has unique strategies and behaviors */
enum class CEOPersonality : uint8_t {
	GORDON_GEKKO,      ///< Aggressive stock manipulator, hostile takeovers, "Greed is good"
	WARREN_BUFFETT,    ///< Long-term value investor, diversified, patient growth
	ELON_MUSK,         ///< High risk, innovation-focused, erratic but visionary
	JOHN_ROCKEFELLER,  ///< Monopoly builder, dominates single industries
	RICHARD_BRANSON,   ///< Brand-focused, passenger services, PR master
	CORNELIUS_VANDERBILT, ///< Rail baron, aggressive expansion, cuts competitors
	STEADY_EDDIE,      ///< Conservative, safe growth, avoids conflict
	PENNY_PINCHER,     ///< Extreme cost-cutting, low service, high margins
	TECH_TITAN,        ///< Early adopter, newest vehicles, efficiency focused
	RANDOM_CHAOS,      ///< Unpredictable, chaotic decisions
	NUM_PERSONALITIES
};

/** Personality trait values (0-100 scale) */
struct PersonalityTraits {
	uint8_t aggression;          ///< How aggressively they compete (hostile takeovers, undercutting)
	uint8_t risk_tolerance;      ///< Willingness to take financial risks
	uint8_t expansion_drive;     ///< How quickly they want to expand
	uint8_t stock_market_focus;  ///< Interest in stock manipulation and trading
	uint8_t passenger_preference; ///< Preference for passengers vs cargo
	uint8_t innovation_drive;    ///< Interest in newest vehicles and tech
	uint8_t cost_focus;          ///< How much they care about cutting costs
	uint8_t reputation_care;     ///< How much they care about public image
	uint8_t monopoly_tendency;   ///< Desire to dominate specific routes/industries
	uint8_t patience;            ///< Willingness to wait for long-term payoffs
};

/** Current mood/state that affects short-term decisions */
enum class CEOMood : uint8_t {
	CONFIDENT,    ///< Recent success, more willing to take risks
	CAUTIOUS,     ///< Recent setback, playing it safe
	AGGRESSIVE,   ///< Competitor encroaching, fighting back
	DESPERATE,    ///< Company in trouble, erratic decisions
	EUPHORIC,     ///< Major windfall, may overextend
	PARANOID,     ///< Suspects manipulation, defensive
	VENGEFUL,     ///< Target of takeover attempt, seeking revenge
	NEUTRAL,      ///< Baseline state
};

/** Strategic focus for medium-term planning */
enum class StrategicFocus : uint8_t {
	RAIL_NETWORK,      ///< Building rail infrastructure
	ROAD_NETWORK,      ///< Building road/truck routes
	AIR_DOMINANCE,     ///< Airport and aircraft focus
	SHIPPING_EMPIRE,   ///< Ports and ships
	MIXED_TRANSPORT,   ///< Balanced approach
	HOSTILE_TAKEOVER,  ///< Acquiring other companies
	STOCK_MANIPULATION, ///< Market games
	INDUSTRY_CHAINS,   ///< Controlling cargo chains
	PASSENGER_KING,    ///< Passenger transport focus
	CARGO_HAULER,      ///< Freight focus
};

/** Full AI personality profile */
struct AIPersonalityProfile {
	CEOPersonality archetype;
	PersonalityTraits traits;
	CEOMood current_mood;
	StrategicFocus primary_focus;
	StrategicFocus secondary_focus;

	CompanyID rival_company;        ///< Primary competitor to watch/fight
	CompanyID takeover_target;      ///< Company they want to acquire

	uint8_t mood_intensity;         ///< How strong current mood is (0-100)
	uint16_t mood_duration;         ///< Ticks remaining in current mood

	int32_t player_opinion;         ///< Opinion of human player (-100 to +100)
	int32_t recent_profit_trend;    ///< Tracking recent performance

	uint32_t last_major_decision;   ///< Tick of last big strategic choice
	uint8_t decision_cooldown;      ///< Months before next major strategy change

	std::string ceo_name;           ///< Generated CEO name
	std::string company_motto;      ///< Company slogan based on personality
};

/** Get default traits for a personality archetype */
PersonalityTraits GetDefaultTraits(CEOPersonality personality);

/** Generate a random personality with some variation */
AIPersonalityProfile GenerateRandomPersonality();

/** Generate a personality that complements existing AIs (adds variety) */
AIPersonalityProfile GenerateComplementaryPersonality(const std::vector<CEOPersonality> &existing);

/** Update AI mood based on recent events */
void UpdateAIMood(AIPersonalityProfile &profile, Money recent_profit, bool was_attacked, bool lost_route);

/** Get strategic recommendation based on personality and game state */
StrategicFocus RecommendStrategy(const AIPersonalityProfile &profile, Money cash, uint16_t vehicles);

/** Should this AI attempt a hostile takeover right now? */
bool ShouldAttemptTakeover(const AIPersonalityProfile &profile, CompanyID target, uint8_t current_shares);

/** Should this AI buy/sell stocks? */
int8_t GetStockTradeRecommendation(const AIPersonalityProfile &profile, CompanyID target, Money share_price, int8_t price_trend);

/** Generate appropriate CEO name for personality */
std::string GenerateCEOName(CEOPersonality personality);

/** Generate company motto for personality */
std::string GenerateCompanyMotto(CEOPersonality personality);

/** Get personality description for UI */
std::string GetPersonalityDescription(CEOPersonality personality);

/** Personality-based response to player actions */
void ReactToPlayerAction(AIPersonalityProfile &profile, CompanyID player, const char *action_type);

#endif /* AI_PERSONALITY_H */
