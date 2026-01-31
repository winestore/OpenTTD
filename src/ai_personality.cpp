/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file ai_personality.cpp Implementation of AI Personality system for Transport Empire. */

#include "stdafx.h"
#include "ai_personality.h"
#include "company_base.h"
#include "stock_market.h"
#include <array>
#include <algorithm>

#include "safeguards.h"

/** Default personality trait profiles for each archetype */
PersonalityTraits GetDefaultTraits(CEOPersonality personality)
{
	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
			return {
				.aggression = 95,
				.risk_tolerance = 85,
				.expansion_drive = 70,
				.stock_market_focus = 100,
				.passenger_preference = 50,
				.innovation_drive = 60,
				.cost_focus = 40,
				.reputation_care = 10,
				.monopoly_tendency = 80,
				.patience = 30
			};

		case CEOPersonality::WARREN_BUFFETT:
			return {
				.aggression = 20,
				.risk_tolerance = 30,
				.expansion_drive = 40,
				.stock_market_focus = 70,
				.passenger_preference = 50,
				.innovation_drive = 30,
				.cost_focus = 60,
				.reputation_care = 80,
				.monopoly_tendency = 40,
				.patience = 95
			};

		case CEOPersonality::ELON_MUSK:
			return {
				.aggression = 70,
				.risk_tolerance = 95,
				.expansion_drive = 90,
				.stock_market_focus = 50,
				.passenger_preference = 70,
				.innovation_drive = 100,
				.cost_focus = 20,
				.reputation_care = 60,
				.monopoly_tendency = 60,
				.patience = 20
			};

		case CEOPersonality::JOHN_ROCKEFELLER:
			return {
				.aggression = 80,
				.risk_tolerance = 50,
				.expansion_drive = 60,
				.stock_market_focus = 60,
				.passenger_preference = 20,
				.innovation_drive = 40,
				.cost_focus = 70,
				.reputation_care = 30,
				.monopoly_tendency = 100,
				.patience = 70
			};

		case CEOPersonality::RICHARD_BRANSON:
			return {
				.aggression = 50,
				.risk_tolerance = 70,
				.expansion_drive = 80,
				.stock_market_focus = 40,
				.passenger_preference = 90,
				.innovation_drive = 75,
				.cost_focus = 30,
				.reputation_care = 95,
				.monopoly_tendency = 30,
				.patience = 50
			};

		case CEOPersonality::CORNELIUS_VANDERBILT:
			return {
				.aggression = 90,
				.risk_tolerance = 60,
				.expansion_drive = 85,
				.stock_market_focus = 50,
				.passenger_preference = 40,
				.innovation_drive = 50,
				.cost_focus = 60,
				.reputation_care = 20,
				.monopoly_tendency = 90,
				.patience = 40
			};

		case CEOPersonality::STEADY_EDDIE:
			return {
				.aggression = 15,
				.risk_tolerance = 20,
				.expansion_drive = 40,
				.stock_market_focus = 20,
				.passenger_preference = 50,
				.innovation_drive = 40,
				.cost_focus = 50,
				.reputation_care = 70,
				.monopoly_tendency = 20,
				.patience = 80
			};

		case CEOPersonality::PENNY_PINCHER:
			return {
				.aggression = 40,
				.risk_tolerance = 25,
				.expansion_drive = 30,
				.stock_market_focus = 30,
				.passenger_preference = 30,
				.innovation_drive = 20,
				.cost_focus = 100,
				.reputation_care = 15,
				.monopoly_tendency = 50,
				.patience = 60
			};

		case CEOPersonality::TECH_TITAN:
			return {
				.aggression = 55,
				.risk_tolerance = 65,
				.expansion_drive = 70,
				.stock_market_focus = 45,
				.passenger_preference = 60,
				.innovation_drive = 95,
				.cost_focus = 40,
				.reputation_care = 65,
				.monopoly_tendency = 40,
				.patience = 45
			};

		case CEOPersonality::RANDOM_CHAOS:
		default:
			return {
				.aggression = static_cast<uint8_t>(RandomRange(100)),
				.risk_tolerance = static_cast<uint8_t>(RandomRange(100)),
				.expansion_drive = static_cast<uint8_t>(RandomRange(100)),
				.stock_market_focus = static_cast<uint8_t>(RandomRange(100)),
				.passenger_preference = static_cast<uint8_t>(RandomRange(100)),
				.innovation_drive = static_cast<uint8_t>(RandomRange(100)),
				.cost_focus = static_cast<uint8_t>(RandomRange(100)),
				.reputation_care = static_cast<uint8_t>(RandomRange(100)),
				.monopoly_tendency = static_cast<uint8_t>(RandomRange(100)),
				.patience = static_cast<uint8_t>(RandomRange(100))
			};
	}
}

/** Add random variation to traits */
static void AddTraitVariation(PersonalityTraits &traits, int variation_range)
{
	auto vary = [variation_range](uint8_t &val) {
		int change = RandomRange(variation_range * 2 + 1) - variation_range;
		val = static_cast<uint8_t>(Clamp(static_cast<int>(val) + change, 0, 100));
	};

	vary(traits.aggression);
	vary(traits.risk_tolerance);
	vary(traits.expansion_drive);
	vary(traits.stock_market_focus);
	vary(traits.passenger_preference);
	vary(traits.innovation_drive);
	vary(traits.cost_focus);
	vary(traits.reputation_care);
	vary(traits.monopoly_tendency);
	vary(traits.patience);
}

/** Generate random personality */
AIPersonalityProfile GenerateRandomPersonality()
{
	AIPersonalityProfile profile;

	/* Pick random archetype */
	profile.archetype = static_cast<CEOPersonality>(RandomRange(static_cast<uint>(CEOPersonality::NUM_PERSONALITIES)));
	profile.traits = GetDefaultTraits(profile.archetype);

	/* Add some random variation (±15 points) */
	AddTraitVariation(profile.traits, 15);

	/* Set initial mood */
	profile.current_mood = CEOMood::NEUTRAL;
	profile.mood_intensity = 50;
	profile.mood_duration = 0;

	/* Determine strategic focus based on traits */
	if (profile.traits.stock_market_focus > 80) {
		profile.primary_focus = StrategicFocus::STOCK_MANIPULATION;
	} else if (profile.traits.monopoly_tendency > 80) {
		profile.primary_focus = StrategicFocus::INDUSTRY_CHAINS;
	} else if (profile.traits.passenger_preference > 70) {
		profile.primary_focus = StrategicFocus::PASSENGER_KING;
	} else if (profile.traits.innovation_drive > 80) {
		profile.primary_focus = StrategicFocus::AIR_DOMINANCE;
	} else {
		profile.primary_focus = StrategicFocus::MIXED_TRANSPORT;
	}

	profile.secondary_focus = StrategicFocus::MIXED_TRANSPORT;
	profile.rival_company = CompanyID::Invalid();
	profile.takeover_target = CompanyID::Invalid();
	profile.player_opinion = 0;
	profile.recent_profit_trend = 0;
	profile.last_major_decision = 0;
	profile.decision_cooldown = 6; /* 6 months */

	profile.ceo_name = GenerateCEOName(profile.archetype);
	profile.company_motto = GenerateCompanyMotto(profile.archetype);

	return profile;
}

/** Generate personality that adds variety to existing AIs */
AIPersonalityProfile GenerateComplementaryPersonality(const std::vector<CEOPersonality> &existing)
{
	/* Count existing personalities */
	std::array<int, static_cast<size_t>(CEOPersonality::NUM_PERSONALITIES)> counts{};
	for (CEOPersonality p : existing) {
		counts[static_cast<size_t>(p)]++;
	}

	/* Find least common personalities */
	int min_count = INT_MAX;
	for (int c : counts) {
		if (c < min_count) min_count = c;
	}

	/* Collect candidates with minimum count */
	std::vector<CEOPersonality> candidates;
	for (size_t i = 0; i < counts.size(); i++) {
		if (counts[i] == min_count) {
			candidates.push_back(static_cast<CEOPersonality>(i));
		}
	}

	/* Generate profile with random underrepresented personality */
	AIPersonalityProfile profile;
	profile.archetype = candidates[RandomRange(static_cast<uint>(candidates.size()))];
	profile.traits = GetDefaultTraits(profile.archetype);
	AddTraitVariation(profile.traits, 15);

	/* Initialize rest */
	profile.current_mood = CEOMood::NEUTRAL;
	profile.mood_intensity = 50;
	profile.mood_duration = 0;
	profile.primary_focus = StrategicFocus::MIXED_TRANSPORT;
	profile.secondary_focus = StrategicFocus::MIXED_TRANSPORT;
	profile.rival_company = CompanyID::Invalid();
	profile.takeover_target = CompanyID::Invalid();
	profile.player_opinion = 0;
	profile.recent_profit_trend = 0;
	profile.last_major_decision = 0;
	profile.decision_cooldown = 6;

	profile.ceo_name = GenerateCEOName(profile.archetype);
	profile.company_motto = GenerateCompanyMotto(profile.archetype);

	return profile;
}

/** Update AI mood based on events */
void UpdateAIMood(AIPersonalityProfile &profile, Money recent_profit, bool was_attacked, bool lost_route)
{
	/* Decay current mood */
	if (profile.mood_duration > 0) {
		profile.mood_duration--;
		if (profile.mood_duration == 0) {
			profile.current_mood = CEOMood::NEUTRAL;
			profile.mood_intensity = 50;
		}
	}

	/* Check for mood-changing events */
	if (was_attacked) {
		if (profile.traits.aggression > 60) {
			profile.current_mood = CEOMood::VENGEFUL;
		} else {
			profile.current_mood = CEOMood::PARANOID;
		}
		profile.mood_intensity = 80;
		profile.mood_duration = 24; /* 2 years */
		return;
	}

	if (lost_route) {
		if (profile.traits.aggression > 50) {
			profile.current_mood = CEOMood::AGGRESSIVE;
		} else {
			profile.current_mood = CEOMood::CAUTIOUS;
		}
		profile.mood_intensity = 70;
		profile.mood_duration = 12;
		return;
	}

	/* Profit-based mood */
	if (recent_profit > profile.recent_profit_trend * 2 && recent_profit > 100000) {
		profile.current_mood = CEOMood::EUPHORIC;
		profile.mood_intensity = 75;
		profile.mood_duration = 6;
	} else if (recent_profit < 0) {
		if (recent_profit < -500000) {
			profile.current_mood = CEOMood::DESPERATE;
			profile.mood_intensity = 90;
			profile.mood_duration = 12;
		} else {
			profile.current_mood = CEOMood::CAUTIOUS;
			profile.mood_intensity = 60;
			profile.mood_duration = 6;
		}
	} else if (recent_profit > 0 && profile.current_mood == CEOMood::NEUTRAL) {
		profile.current_mood = CEOMood::CONFIDENT;
		profile.mood_intensity = 55;
		profile.mood_duration = 6;
	}

	profile.recent_profit_trend = recent_profit;
}

/** Get strategic focus recommendation */
StrategicFocus RecommendStrategy(const AIPersonalityProfile &profile, Money cash, uint16_t vehicles)
{
	/* Early game - build base network */
	if (vehicles < 10) {
		if (profile.traits.passenger_preference > 60) {
			return StrategicFocus::PASSENGER_KING;
		}
		return StrategicFocus::MIXED_TRANSPORT;
	}

	/* Cash-rich with aggressive tendencies - consider takeovers */
	if (cash > 1000000 && profile.traits.aggression > 70 && profile.traits.stock_market_focus > 50) {
		if (profile.current_mood == CEOMood::VENGEFUL || profile.current_mood == CEOMood::CONFIDENT) {
			return StrategicFocus::HOSTILE_TAKEOVER;
		}
	}

	/* Desperate - focus on core profitable routes */
	if (profile.current_mood == CEOMood::DESPERATE) {
		return StrategicFocus::CARGO_HAULER; /* Usually more reliable income */
	}

	/* Innovation focused with cash */
	if (profile.traits.innovation_drive > 80 && cash > 500000) {
		return StrategicFocus::AIR_DOMINANCE;
	}

	/* Monopoly builder */
	if (profile.traits.monopoly_tendency > 75) {
		return StrategicFocus::INDUSTRY_CHAINS;
	}

	/* Default to personality's natural focus */
	return profile.primary_focus;
}

/** Should attempt hostile takeover? */
bool ShouldAttemptTakeover(const AIPersonalityProfile &profile, CompanyID target, uint8_t current_shares)
{
	/* Must have stock market interest */
	if (profile.traits.stock_market_focus < 40) return false;

	/* Check personality thresholds */
	if (profile.archetype == CEOPersonality::GORDON_GEKKO) {
		/* Gekko types are always looking for opportunities */
		return current_shares >= 25 || (profile.current_mood == CEOMood::CONFIDENT && current_shares >= 10);
	}

	/* Vengeful mood makes takeovers more likely */
	if (profile.current_mood == CEOMood::VENGEFUL && profile.rival_company == target) {
		return current_shares >= 30;
	}

	/* Standard threshold - need significant stake to push for takeover */
	if (profile.traits.aggression > 70 && current_shares >= 40) return true;
	if (profile.traits.monopoly_tendency > 80 && current_shares >= 35) return true;

	/* Conservative AIs rarely attempt takeovers */
	if (profile.traits.aggression < 40) return false;

	return current_shares >= 45; /* Only when very close */
}

/** Stock trading recommendation: -1 sell, 0 hold, 1 buy */
int8_t GetStockTradeRecommendation(const AIPersonalityProfile &profile, CompanyID target, Money share_price, int8_t price_trend)
{
	/* Ignore stock market if not interested */
	if (profile.traits.stock_market_focus < 30) return 0;

	/* Warren Buffett type - buy undervalued, long term hold */
	if (profile.archetype == CEOPersonality::WARREN_BUFFETT) {
		if (price_trend < -5) return 1;  /* Buy the dip */
		return 0; /* Hold */
	}

	/* Gekko type - active trader */
	if (profile.archetype == CEOPersonality::GORDON_GEKKO) {
		if (profile.current_mood == CEOMood::CONFIDENT && price_trend > 0) return 1;
		if (price_trend > 10) return -1; /* Take profits */
		if (price_trend < -10) return 1; /* Buy low */
		return 0;
	}

	/* Desperate mood - might sell for cash */
	if (profile.current_mood == CEOMood::DESPERATE) return -1;

	/* Euphoric mood - might overbuy */
	if (profile.current_mood == CEOMood::EUPHORIC && profile.traits.risk_tolerance > 60) return 1;

	/* Default: follow trend with personality modifier */
	if (price_trend > 5 && profile.traits.risk_tolerance > 50) return -1; /* Sell high */
	if (price_trend < -5 && profile.traits.patience > 50) return 1; /* Buy low */

	return 0;
}

/** Generate CEO name */
std::string GenerateCEOName(CEOPersonality personality)
{
	static const char *first_names[] = {
		"James", "William", "Charles", "Robert", "Michael",
		"Elizabeth", "Margaret", "Victoria", "Catherine", "Alexandra",
		"Alexander", "Theodore", "Franklin", "Winston", "George"
	};

	static const char *last_names_aggressive[] = {
		"Steele", "Powers", "Strong", "Wolfe", "Hawk",
		"Sharpe", "Thorne", "Blackwell", "Ironwood", "Striker"
	};

	static const char *last_names_conservative[] = {
		"Whitmore", "Goodwin", "Sterling", "Fairfax", "Ashworth",
		"Pemberton", "Hartley", "Crawford", "Brighton", "Wellington"
	};

	static const char *last_names_innovative[] = {
		"Tesla", "Edison", "Fulton", "Bell", "Wright",
		"Ford", "Carnegie", "Morgan", "Astor", "Rothschild"
	};

	std::string name = first_names[RandomRange(15)];
	name += " ";

	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
		case CEOPersonality::CORNELIUS_VANDERBILT:
		case CEOPersonality::JOHN_ROCKEFELLER:
			name += last_names_aggressive[RandomRange(10)];
			break;

		case CEOPersonality::ELON_MUSK:
		case CEOPersonality::TECH_TITAN:
			name += last_names_innovative[RandomRange(10)];
			break;

		default:
			name += last_names_conservative[RandomRange(10)];
			break;
	}

	return name;
}

/** Generate company motto */
std::string GenerateCompanyMotto(CEOPersonality personality)
{
	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
			return "Greed is Good";
		case CEOPersonality::WARREN_BUFFETT:
			return "Value Through Patience";
		case CEOPersonality::ELON_MUSK:
			return "The Future is Now";
		case CEOPersonality::JOHN_ROCKEFELLER:
			return "Excellence Through Dominance";
		case CEOPersonality::RICHARD_BRANSON:
			return "Adventure Awaits";
		case CEOPersonality::CORNELIUS_VANDERBILT:
			return "Connecting Nations";
		case CEOPersonality::STEADY_EDDIE:
			return "Reliable Service, Every Time";
		case CEOPersonality::PENNY_PINCHER:
			return "Maximum Value, Minimum Cost";
		case CEOPersonality::TECH_TITAN:
			return "Innovation in Motion";
		case CEOPersonality::RANDOM_CHAOS:
		default:
			return "Expect the Unexpected";
	}
}

/** Get personality description */
std::string GetPersonalityDescription(CEOPersonality personality)
{
	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
			return "A ruthless corporate raider who views companies as commodities to be bought, stripped, and sold. "
			       "Will aggressively pursue hostile takeovers and market manipulation.";
		case CEOPersonality::WARREN_BUFFETT:
			return "A patient, value-focused investor who builds wealth slowly through sound fundamentals. "
			       "Rarely sells, prefers quality over quantity.";
		case CEOPersonality::ELON_MUSK:
			return "A visionary risk-taker who pushes technological boundaries. Makes bold bets that either "
			       "revolutionize the industry or spectacularly fail.";
		case CEOPersonality::JOHN_ROCKEFELLER:
			return "A methodical monopolist who systematically dominates one market at a time. "
			       "Will undercut competitors until they surrender or sell.";
		case CEOPersonality::RICHARD_BRANSON:
			return "A charismatic brand-builder focused on passenger experience and public relations. "
			       "Values reputation above short-term profits.";
		case CEOPersonality::CORNELIUS_VANDERBILT:
			return "An aggressive rail baron who builds empires through relentless expansion. "
			       "Views competitors as obstacles to be crushed.";
		case CEOPersonality::STEADY_EDDIE:
			return "A conservative manager who prioritizes stability over growth. "
			       "Avoids conflict and prefers safe, proven strategies.";
		case CEOPersonality::PENNY_PINCHER:
			return "An extreme cost-cutter who sacrifices service quality for margins. "
			       "Runs the cheapest operation possible, for better or worse.";
		case CEOPersonality::TECH_TITAN:
			return "An efficiency-obsessed innovator who always wants the newest equipment. "
			       "May overspend on technology but runs a modern fleet.";
		case CEOPersonality::RANDOM_CHAOS:
		default:
			return "An unpredictable wildcard whose decisions seem to follow no pattern. "
			       "Could be a genius or a madman - possibly both.";
	}
}

/** React to player actions */
void ReactToPlayerAction(AIPersonalityProfile &profile, CompanyID player, const char *action_type)
{
	std::string action(action_type);

	if (action == "bought_shares") {
		/* Player bought our shares - react based on personality */
		if (profile.traits.aggression > 60) {
			profile.player_opinion -= 20;
			if (profile.player_opinion < -50) {
				profile.current_mood = CEOMood::PARANOID;
				profile.rival_company = player;
			}
		}
	} else if (action == "undercut_route") {
		/* Player established competing route */
		profile.player_opinion -= 15;
		if (profile.traits.aggression > 70) {
			profile.current_mood = CEOMood::AGGRESSIVE;
			profile.rival_company = player;
		}
	} else if (action == "sold_shares") {
		/* Player sold our shares - slight positive if suspicious */
		if (profile.current_mood == CEOMood::PARANOID) {
			profile.player_opinion += 5;
		}
	} else if (action == "cooperation") {
		/* Some form of positive interaction */
		profile.player_opinion += 10;
		profile.player_opinion = std::min(profile.player_opinion, 100);
	}

	profile.player_opinion = Clamp(profile.player_opinion, -100, 100);
}
