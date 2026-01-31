/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file achievements.cpp Implementation of the achievement and milestone system. */

#include "stdafx.h"
#include "achievements.h"
#include "company_base.h"
#include "vehicle_base.h"
#include "stock_market.h"
#include "game_events.h"
#include "screen_effects.h"
#include <map>
#include <vector>
#include <cstring>

#include "safeguards.h"

/** All defined achievements */
static const AchievementDef *ALL_ACHIEVEMENTS[] = {
	/* Wealth */
	&Achievements::FIRST_PROFIT,
	&Achievements::MILLIONAIRE,
	&Achievements::HUNDRED_MILLION,
	&Achievements::BILLIONAIRE,
	&Achievements::FROM_RAGS,

	/* Empire */
	&Achievements::FIRST_VEHICLE,
	&Achievements::TEN_VEHICLES,
	&Achievements::FIFTY_VEHICLES,
	&Achievements::HUNDRED_VEHICLES,
	&Achievements::FIVE_HUNDRED_VEHICLES,
	&Achievements::THOUSAND_VEHICLES,
	&Achievements::MULTI_MODAL,

	/* Stock Market */
	&Achievements::FIRST_SHARES,
	&Achievements::CONTROLLING_INTEREST,
	&Achievements::HOSTILE_TAKEOVER,
	&Achievements::MERGER_MASTER,
	&Achievements::GEKKO_AWARD,
	&Achievements::MARKET_MANIPULATOR,
	&Achievements::MONOPOLY,

	/* Ruthless */
	&Achievements::ROUTE_STEALER,
	&Achievements::INDUSTRY_THIEF,
	&Achievements::CORPORATE_RAIDER,
	&Achievements::ROBBER_BARON,

	/* Efficiency */
	&Achievements::PROFITABLE_YEAR,
	&Achievements::TEN_YEAR_STREAK,
	&Achievements::PERFECT_RATING,
	&Achievements::NO_CRASHES,

	/* Longevity */
	&Achievements::SURVIVE_10_YEARS,
	&Achievements::SURVIVE_50_YEARS,
	&Achievements::SURVIVE_100_YEARS,
	&Achievements::SURVIVE_200_YEARS,

	/* Special */
	&Achievements::WEATHERED_STORM,
	&Achievements::NIGHT_OWL,
	&Achievements::REVENGE,
	&Achievements::UNDERDOG,
	&Achievements::SPEED_RUN,
};

static constexpr size_t NUM_ACHIEVEMENTS = sizeof(ALL_ACHIEVEMENTS) / sizeof(ALL_ACHIEVEMENTS[0]);

/** Per-company achievement state */
struct CompanyAchievementState {
	std::map<std::string, AchievementProgress> progress;
	uint32_t unlock_count = 0;

	/* Tracking data for complex achievements */
	int32_t consecutive_profitable_years = 0;
	int32_t years_without_crash = 0;
	Money lowest_value_ever = 0;
	bool was_near_bankruptcy = false;
	uint32_t takeover_count = 0;
};

/** Achievement state for all companies */
static std::map<CompanyID, CompanyAchievementState> _achievement_state;

/** Get or create achievement state for a company */
static CompanyAchievementState &GetAchievementState(CompanyID company)
{
	return _achievement_state[company];
}

/** Find achievement by ID */
static const AchievementDef *FindAchievementById(const char *id)
{
	for (size_t i = 0; i < NUM_ACHIEVEMENTS; i++) {
		if (strcmp(ALL_ACHIEVEMENTS[i]->id, id) == 0) {
			return ALL_ACHIEVEMENTS[i];
		}
	}
	return nullptr;
}

/** Count vehicles of a specific type for a company */
static uint32_t CountVehiclesByType(CompanyID company, VehicleType type)
{
	uint32_t count = 0;
	for (const Vehicle *v : Vehicle::Iterate()) {
		if (v->owner == company && v->type == type) {
			count++;
		}
	}
	return count;
}

/** Count total vehicles for a company */
static uint32_t CountTotalVehicles(CompanyID company)
{
	uint32_t count = 0;
	for (const Vehicle *v : Vehicle::Iterate()) {
		if (v->owner == company) {
			count++;
		}
	}
	return count;
}

/** Count AI companies */
static uint32_t CountAICompanies()
{
	uint32_t count = 0;
	for (const Company *c : Company::Iterate()) {
		if (c->is_ai) count++;
	}
	return count;
}

/** Count total shares owned in AI companies */
static uint32_t CountSharesInAICompanies(CompanyID owner)
{
	uint32_t count = 0;
	for (const Company *c : Company::Iterate()) {
		if (c->is_ai && c->index != owner) {
			count += c->shares.shares_owned[owner];
		}
	}
	return count;
}

/** Count AI companies where player owns any shares */
static uint32_t CountAICompaniesWithShares(CompanyID owner)
{
	uint32_t count = 0;
	for (const Company *c : Company::Iterate()) {
		if (c->is_ai && c->shares.shares_owned[owner] > 0) {
			count++;
		}
	}
	return count;
}

/**
 * Check and potentially unlock achievements for a company.
 * Called after significant game events.
 */
void CheckAchievements(CompanyID company)
{
	Company *c = Company::GetIfValid(company);
	if (c == nullptr) return;

	CompanyAchievementState &state = GetAchievementState(company);
	Money company_value = c->cur_economy.company_value;
	uint32_t vehicle_count = CountTotalVehicles(company);

	/* ===== WEALTH ACHIEVEMENTS ===== */

	/* First Profit */
	if (c->cur_economy.income > 0) {
		if (!IsAchievementUnlocked(company, "first_profit")) {
			UnlockAchievement(company, Achievements::FIRST_PROFIT);
		}
	}

	/* Millionaire */
	if (company_value >= 1000000 && !IsAchievementUnlocked(company, "millionaire")) {
		UnlockAchievement(company, Achievements::MILLIONAIRE);
	}

	/* Centi-Millionaire */
	if (company_value >= 100000000 && !IsAchievementUnlocked(company, "hundred_million")) {
		UnlockAchievement(company, Achievements::HUNDRED_MILLION);
	}

	/* Billionaire */
	if (company_value >= 1000000000 && !IsAchievementUnlocked(company, "billionaire")) {
		UnlockAchievement(company, Achievements::BILLIONAIRE);
	}

	/* From Rags to Riches - track lowest value */
	if (company_value < 50000) {
		state.was_near_bankruptcy = true;
	}
	if (state.was_near_bankruptcy && company_value >= 10000000) {
		if (!IsAchievementUnlocked(company, "from_rags")) {
			UnlockAchievement(company, Achievements::FROM_RAGS);
		}
	}

	/* ===== EMPIRE ACHIEVEMENTS ===== */

	/* First Vehicle */
	if (vehicle_count >= 1 && !IsAchievementUnlocked(company, "first_vehicle")) {
		UnlockAchievement(company, Achievements::FIRST_VEHICLE);
	}

	/* Vehicle milestones */
	if (vehicle_count >= 10 && !IsAchievementUnlocked(company, "ten_vehicles")) {
		UnlockAchievement(company, Achievements::TEN_VEHICLES);
	}
	if (vehicle_count >= 50 && !IsAchievementUnlocked(company, "fifty_vehicles")) {
		UnlockAchievement(company, Achievements::FIFTY_VEHICLES);
	}
	if (vehicle_count >= 100 && !IsAchievementUnlocked(company, "hundred_vehicles")) {
		UnlockAchievement(company, Achievements::HUNDRED_VEHICLES);
	}
	if (vehicle_count >= 500 && !IsAchievementUnlocked(company, "five_hundred_vehicles")) {
		UnlockAchievement(company, Achievements::FIVE_HUNDRED_VEHICLES);
	}
	if (vehicle_count >= 1000 && !IsAchievementUnlocked(company, "thousand_vehicles")) {
		UnlockAchievement(company, Achievements::THOUSAND_VEHICLES);
	}

	/* Multi-Modal Master */
	uint32_t trains = CountVehiclesByType(company, VEH_TRAIN);
	uint32_t road = CountVehiclesByType(company, VEH_ROAD);
	uint32_t ships = CountVehiclesByType(company, VEH_SHIP);
	uint32_t aircraft = CountVehiclesByType(company, VEH_AIRCRAFT);
	if (trains >= 10 && road >= 10 && ships >= 10 && aircraft >= 10) {
		if (!IsAchievementUnlocked(company, "multi_modal")) {
			UnlockAchievement(company, Achievements::MULTI_MODAL);
		}
	}

	/* ===== STOCK MARKET ACHIEVEMENTS ===== */

	/* First Shares */
	uint32_t total_shares_owned = CountSharesInAICompanies(company);
	if (total_shares_owned > 0 && !IsAchievementUnlocked(company, "first_shares")) {
		UnlockAchievement(company, Achievements::FIRST_SHARES);
	}

	/* Controlling Interest - check each AI */
	for (const Company *target : Company::Iterate()) {
		if (target->is_ai && target->shares.shares_owned[company] >= 51) {
			if (!IsAchievementUnlocked(company, "controlling_interest")) {
				UnlockAchievement(company, Achievements::CONTROLLING_INTEREST);
			}
			break;
		}
	}

	/* Gekko Award - own shares in every AI */
	uint32_t ai_count = CountAICompanies();
	uint32_t ai_with_shares = CountAICompaniesWithShares(company);
	if (ai_count > 0 && ai_with_shares == ai_count) {
		if (!IsAchievementUnlocked(company, "gekko_award")) {
			UnlockAchievement(company, Achievements::GEKKO_AWARD);
		}
	}

	/* Merger & Acquisitions */
	if (state.takeover_count >= 5 && !IsAchievementUnlocked(company, "merger_master")) {
		UnlockAchievement(company, Achievements::MERGER_MASTER);
	}

	/* Monopoly - only company left */
	uint32_t total_companies = 0;
	for (const Company *comp : Company::Iterate()) {
		total_companies++;
	}
	if (total_companies == 1 && !IsAchievementUnlocked(company, "monopoly")) {
		UnlockAchievement(company, Achievements::MONOPOLY);
	}
}

/**
 * Unlock a specific achievement for a company.
 */
void UnlockAchievement(CompanyID company, const AchievementDef &achievement)
{
	CompanyAchievementState &state = GetAchievementState(company);

	/* Check if already unlocked */
	auto it = state.progress.find(achievement.id);
	if (it != state.progress.end() && it->second.unlocked) {
		return; /* Already unlocked */
	}

	/* Record the unlock */
	AchievementProgress &prog = state.progress[achievement.id];
	prog.achievement_id = achievement.id;
	prog.unlocked = true;
	prog.unlock_date = 0; /* TODO: Get current game date */
	prog.current_progress = achievement.requirement;

	state.unlock_count++;

	/* Trigger visual effects */
	OnAchievementUnlocked();

	/* Fire game event for news, etc. */
	OnAchievement(company, achievement.id);
}

/**
 * Get progress toward an achievement.
 * Returns the current value, not a percentage.
 */
int32_t GetAchievementProgress(CompanyID company, const char *achievement_id)
{
	Company *c = Company::GetIfValid(company);
	if (c == nullptr) return 0;

	const AchievementDef *def = FindAchievementById(achievement_id);
	if (def == nullptr) return 0;

	CompanyAchievementState &state = GetAchievementState(company);

	/* Check if we have cached progress */
	auto it = state.progress.find(achievement_id);
	if (it != state.progress.end()) {
		return it->second.current_progress;
	}

	/* Calculate progress based on achievement type */
	if (strcmp(achievement_id, "millionaire") == 0 ||
	    strcmp(achievement_id, "hundred_million") == 0 ||
	    strcmp(achievement_id, "billionaire") == 0) {
		return static_cast<int32_t>(c->cur_economy.company_value / 1000); /* In thousands */
	}

	if (strcmp(achievement_id, "first_vehicle") == 0 ||
	    strcmp(achievement_id, "ten_vehicles") == 0 ||
	    strcmp(achievement_id, "fifty_vehicles") == 0 ||
	    strcmp(achievement_id, "hundred_vehicles") == 0 ||
	    strcmp(achievement_id, "five_hundred_vehicles") == 0 ||
	    strcmp(achievement_id, "thousand_vehicles") == 0) {
		return CountTotalVehicles(company);
	}

	if (strcmp(achievement_id, "first_shares") == 0 ||
	    strcmp(achievement_id, "controlling_interest") == 0) {
		return CountSharesInAICompanies(company);
	}

	return 0;
}

/**
 * Check if an achievement is unlocked for a company.
 */
bool IsAchievementUnlocked(CompanyID company, const char *achievement_id)
{
	CompanyAchievementState &state = GetAchievementState(company);

	auto it = state.progress.find(achievement_id);
	if (it != state.progress.end()) {
		return it->second.unlocked;
	}

	return false;
}

/**
 * Get total number of achievements in the game.
 */
uint32_t GetTotalAchievements()
{
	return static_cast<uint32_t>(NUM_ACHIEVEMENTS);
}

/**
 * Get count of unlocked achievements for a company.
 */
uint32_t GetUnlockedAchievementCount(CompanyID company)
{
	CompanyAchievementState &state = GetAchievementState(company);
	return state.unlock_count;
}

/**
 * Get achievement completion percentage for a company (0-100).
 */
uint8_t GetAchievementCompletionPercent(CompanyID company)
{
	uint32_t total = GetTotalAchievements();
	if (total == 0) return 0;

	uint32_t unlocked = GetUnlockedAchievementCount(company);
	return static_cast<uint8_t>((unlocked * 100) / total);
}
