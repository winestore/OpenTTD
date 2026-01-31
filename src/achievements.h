/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file achievements.h Achievement and milestone system for Transport Empire. */

#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include "stdafx.h"
#include "company_type.h"
#include "timer/timer_game_calendar.h"
#include <string>

/** Achievement categories */
enum class AchievementCategory : uint8_t {
	WEALTH,          ///< Money-related achievements
	EMPIRE,          ///< Company size/vehicles
	STOCK_MARKET,    ///< Trading and takeovers
	RUTHLESS,        ///< Aggressive/competitive actions
	EFFICIENCY,      ///< Operational excellence
	LONGEVITY,       ///< Time-based achievements
	SPECIAL,         ///< Unique/hidden achievements
};

/** Achievement rarity */
enum class AchievementRarity : uint8_t {
	COMMON,          ///< Most players unlock
	UNCOMMON,        ///< 50% of players
	RARE,            ///< 25% of players
	EPIC,            ///< 10% of players
	LEGENDARY,       ///< 1% of players
};

/** Individual achievement definition */
struct AchievementDef {
	const char *id;
	const char *name;
	const char *description;
	const char *secret_hint;     ///< Hint for hidden achievements (nullptr if visible)
	AchievementCategory category;
	AchievementRarity rarity;
	int32_t requirement;         ///< Numeric threshold (context-dependent)
	bool hidden;                 ///< Hidden until unlocked?
};

/** All achievements in the game */
namespace Achievements {

	/* ===== WEALTH ACHIEVEMENTS ===== */
	constexpr AchievementDef FIRST_PROFIT = {
		"first_profit", "First Dollar",
		"Earn your first profit",
		nullptr, AchievementCategory::WEALTH, AchievementRarity::COMMON, 1, false
	};

	constexpr AchievementDef MILLIONAIRE = {
		"millionaire", "Millionaire",
		"Accumulate $1,000,000 in company value",
		nullptr, AchievementCategory::WEALTH, AchievementRarity::COMMON, 1000000, false
	};

	constexpr AchievementDef HUNDRED_MILLION = {
		"hundred_million", "Centi-Millionaire",
		"Accumulate $100,000,000 in company value",
		nullptr, AchievementCategory::WEALTH, AchievementRarity::UNCOMMON, 100000000, false
	};

	constexpr AchievementDef BILLIONAIRE = {
		"billionaire", "Billionaire",
		"Accumulate $1,000,000,000 in company value",
		nullptr, AchievementCategory::WEALTH, AchievementRarity::RARE, 1000000000, false
	};

	constexpr AchievementDef FROM_RAGS = {
		"from_rags", "From Rags to Riches",
		"Recover from near-bankruptcy to $10M+ value",
		nullptr, AchievementCategory::WEALTH, AchievementRarity::RARE, 10000000, false
	};

	/* ===== EMPIRE ACHIEVEMENTS ===== */
	constexpr AchievementDef FIRST_VEHICLE = {
		"first_vehicle", "Wheels in Motion",
		"Purchase your first vehicle",
		nullptr, AchievementCategory::EMPIRE, AchievementRarity::COMMON, 1, false
	};

	constexpr AchievementDef TEN_VEHICLES = {
		"ten_vehicles", "Small Fleet",
		"Own 10 vehicles",
		nullptr, AchievementCategory::EMPIRE, AchievementRarity::COMMON, 10, false
	};

	constexpr AchievementDef FIFTY_VEHICLES = {
		"fifty_vehicles", "Growing Fleet",
		"Own 50 vehicles",
		nullptr, AchievementCategory::EMPIRE, AchievementRarity::UNCOMMON, 50, false
	};

	constexpr AchievementDef HUNDRED_VEHICLES = {
		"hundred_vehicles", "Transport Mogul",
		"Own 100 vehicles",
		nullptr, AchievementCategory::EMPIRE, AchievementRarity::UNCOMMON, 100, false
	};

	constexpr AchievementDef FIVE_HUNDRED_VEHICLES = {
		"five_hundred_vehicles", "Transport Empire",
		"Own 500 vehicles",
		nullptr, AchievementCategory::EMPIRE, AchievementRarity::RARE, 500, false
	};

	constexpr AchievementDef THOUSAND_VEHICLES = {
		"thousand_vehicles", "Logistical Titan",
		"Own 1000 vehicles",
		nullptr, AchievementCategory::EMPIRE, AchievementRarity::EPIC, 1000, false
	};

	constexpr AchievementDef MULTI_MODAL = {
		"multi_modal", "Multi-Modal Master",
		"Own at least 10 of each vehicle type (train, road, ship, aircraft)",
		nullptr, AchievementCategory::EMPIRE, AchievementRarity::UNCOMMON, 40, false
	};

	/* ===== STOCK MARKET ACHIEVEMENTS ===== */
	constexpr AchievementDef FIRST_SHARES = {
		"first_shares", "Playing the Market",
		"Buy shares in another company",
		nullptr, AchievementCategory::STOCK_MARKET, AchievementRarity::COMMON, 1, false
	};

	constexpr AchievementDef CONTROLLING_INTEREST = {
		"controlling_interest", "Controlling Interest",
		"Own 51% of another company's shares",
		nullptr, AchievementCategory::STOCK_MARKET, AchievementRarity::UNCOMMON, 51, false
	};

	constexpr AchievementDef HOSTILE_TAKEOVER = {
		"hostile_takeover", "Hostile Takeover",
		"Complete a hostile takeover of another company",
		nullptr, AchievementCategory::STOCK_MARKET, AchievementRarity::UNCOMMON, 1, false
	};

	constexpr AchievementDef MERGER_MASTER = {
		"merger_master", "Merger & Acquisitions",
		"Complete 5 hostile takeovers",
		nullptr, AchievementCategory::STOCK_MARKET, AchievementRarity::RARE, 5, false
	};

	constexpr AchievementDef GEKKO_AWARD = {
		"gekko_award", "Gordon Gekko Award",
		"Own shares in every AI company simultaneously",
		nullptr, AchievementCategory::STOCK_MARKET, AchievementRarity::RARE, 1, false
	};

	constexpr AchievementDef MARKET_MANIPULATOR = {
		"market_manipulator", "Market Manipulator",
		"Cause a 50% price swing through trading",
		nullptr, AchievementCategory::STOCK_MARKET, AchievementRarity::EPIC, 50, true
	};

	constexpr AchievementDef MONOPOLY = {
		"monopoly", "Monopoly",
		"Be the only remaining company",
		nullptr, AchievementCategory::STOCK_MARKET, AchievementRarity::LEGENDARY, 1, false
	};

	/* ===== RUTHLESS ACHIEVEMENTS ===== */
	constexpr AchievementDef ROUTE_STEALER = {
		"route_stealer", "Route Stealer",
		"Establish a competing route that bankrupts another service",
		nullptr, AchievementCategory::RUTHLESS, AchievementRarity::UNCOMMON, 1, false
	};

	constexpr AchievementDef INDUSTRY_THIEF = {
		"industry_thief", "Industry Thief",
		"Take over an industry connection from a competitor",
		nullptr, AchievementCategory::RUTHLESS, AchievementRarity::COMMON, 1, false
	};

	constexpr AchievementDef CORPORATE_RAIDER = {
		"corporate_raider", "Corporate Raider",
		"Force 3 companies into bankruptcy",
		nullptr, AchievementCategory::RUTHLESS, AchievementRarity::RARE, 3, false
	};

	constexpr AchievementDef ROBBER_BARON = {
		"robber_baron", "Robber Baron",
		"Own 75% of all transport infrastructure on the map",
		nullptr, AchievementCategory::RUTHLESS, AchievementRarity::LEGENDARY, 75, true
	};

	/* ===== EFFICIENCY ACHIEVEMENTS ===== */
	constexpr AchievementDef PROFITABLE_YEAR = {
		"profitable_year", "In the Black",
		"End a year with positive profit",
		nullptr, AchievementCategory::EFFICIENCY, AchievementRarity::COMMON, 1, false
	};

	constexpr AchievementDef TEN_YEAR_STREAK = {
		"ten_year_streak", "Decade of Profit",
		"10 consecutive profitable years",
		nullptr, AchievementCategory::EFFICIENCY, AchievementRarity::UNCOMMON, 10, false
	};

	constexpr AchievementDef PERFECT_RATING = {
		"perfect_rating", "Perfect Rating",
		"Achieve maximum station rating at any station",
		nullptr, AchievementCategory::EFFICIENCY, AchievementRarity::UNCOMMON, 100, false
	};

	constexpr AchievementDef NO_CRASHES = {
		"no_crashes", "Safety First",
		"Play for 50 years without a vehicle crash",
		nullptr, AchievementCategory::EFFICIENCY, AchievementRarity::EPIC, 50, false
	};

	/* ===== LONGEVITY ACHIEVEMENTS ===== */
	constexpr AchievementDef SURVIVE_10_YEARS = {
		"survive_10", "Survivor",
		"Keep your company alive for 10 years",
		nullptr, AchievementCategory::LONGEVITY, AchievementRarity::COMMON, 10, false
	};

	constexpr AchievementDef SURVIVE_50_YEARS = {
		"survive_50", "Half Century",
		"Keep your company alive for 50 years",
		nullptr, AchievementCategory::LONGEVITY, AchievementRarity::UNCOMMON, 50, false
	};

	constexpr AchievementDef SURVIVE_100_YEARS = {
		"survive_100", "Centenarian",
		"Keep your company alive for 100 years",
		nullptr, AchievementCategory::LONGEVITY, AchievementRarity::RARE, 100, false
	};

	constexpr AchievementDef SURVIVE_200_YEARS = {
		"survive_200", "Dynasty",
		"Keep your company alive for 200 years",
		nullptr, AchievementCategory::LONGEVITY, AchievementRarity::EPIC, 200, false
	};

	/* ===== SPECIAL/HIDDEN ACHIEVEMENTS ===== */
	constexpr AchievementDef WEATHERED_STORM = {
		"weathered_storm", "Weathered the Storm",
		"Survive a major market crash without going bankrupt",
		"Sometimes the market falls...", AchievementCategory::SPECIAL, AchievementRarity::RARE, 1, true
	};

	constexpr AchievementDef NIGHT_OWL = {
		"night_owl", "Night Owl",
		"Make 90% of your profit during night hours",
		"Some work best after dark...", AchievementCategory::SPECIAL, AchievementRarity::EPIC, 90, true
	};

	constexpr AchievementDef REVENGE = {
		"revenge", "Revenge is Sweet",
		"Take over a company that previously tried to take you over",
		"What goes around...", AchievementCategory::SPECIAL, AchievementRarity::LEGENDARY, 1, true
	};

	constexpr AchievementDef UNDERDOG = {
		"underdog", "Underdog",
		"Take over the largest AI company while being the smallest",
		"David vs Goliath...", AchievementCategory::SPECIAL, AchievementRarity::LEGENDARY, 1, true
	};

	constexpr AchievementDef SPEED_RUN = {
		"speed_run", "Speed Runner",
		"Reach $100M in under 5 years",
		"Time is money...", AchievementCategory::SPECIAL, AchievementRarity::LEGENDARY, 5, true
	};
}

/** Player's achievement progress */
struct AchievementProgress {
	std::string achievement_id;
	int32_t current_progress;
	bool unlocked;
	uint32_t unlock_date;        ///< Game date when unlocked
};

/** Check and potentially unlock achievements for a company */
void CheckAchievements(CompanyID company);

/** Unlock a specific achievement */
void UnlockAchievement(CompanyID company, const AchievementDef &achievement);

/** Get progress toward an achievement */
int32_t GetAchievementProgress(CompanyID company, const char *achievement_id);

/** Is achievement unlocked? */
bool IsAchievementUnlocked(CompanyID company, const char *achievement_id);

/** Get total achievement count */
uint32_t GetTotalAchievements();

/** Get unlocked achievement count for company */
uint32_t GetUnlockedAchievementCount(CompanyID company);

/** Get achievement completion percentage */
uint8_t GetAchievementCompletionPercent(CompanyID company);

#endif /* ACHIEVEMENTS_H */
