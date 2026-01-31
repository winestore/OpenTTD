/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file scenarios.h Transport Empire scenario and campaign system. */

#ifndef SCENARIOS_H
#define SCENARIOS_H

#include "stdafx.h"
#include "company_type.h"
#include "economy_type.h"
#include "ai_personality.h"
#include <vector>
#include <string>
#include <map>

/**
 * TRANSPORT EMPIRE CAMPAIGNS
 *
 * Guided gameplay experiences that tell a story while teaching mechanics.
 * Each campaign has multiple scenarios that must be completed in order.
 */

/** Scenario difficulty */
enum class ScenarioDifficulty : uint8_t {
	TUTORIAL,    ///< Hand-holding, teaches basics
	EASY,        ///< Relaxed goals
	MEDIUM,      ///< Standard challenge
	HARD,        ///< Tough competition
	NIGHTMARE,   ///< Brutal difficulty
};

/** Victory condition types */
enum class VictoryCondition : uint8_t {
	COMPANY_VALUE,          ///< Reach X company value
	PROFIT_YEARLY,          ///< Achieve X yearly profit
	VEHICLE_COUNT,          ///< Own X vehicles
	ROUTE_COUNT,            ///< Establish X routes
	TAKEOVER_COMPANY,       ///< Take over specific company
	TAKEOVER_COUNT,         ///< Complete X takeovers
	MONOPOLY,               ///< Be the only company
	SHARE_CONTROL,          ///< Control X% of all shares
	SURVIVE_YEARS,          ///< Survive for X years
	INDUSTRY_MONOPOLY,      ///< Control X% of an industry
	DEFEAT_RIVAL,           ///< Bankrupt specific AI
	CUSTOM,                 ///< Script-defined condition
};

/** Failure condition types */
enum class FailureCondition : uint8_t {
	BANKRUPTCY,             ///< Go bankrupt
	TIME_LIMIT,             ///< Run out of time
	RIVAL_WINS,             ///< Rival achieves their goal first
	LOSE_CONTROL,           ///< Lose controlling interest
	CUSTOM,                 ///< Script-defined failure
};

/** Scenario objective */
struct ScenarioObjective {
	VictoryCondition type;
	int64_t target_value;
	std::string description;
	bool completed;
	bool optional;           ///< Bonus objective
	int32_t reward_money;    ///< Bonus money for completing
};

/** Scenario rival configuration */
struct ScenarioRival {
	std::string name;
	CEOPersonality personality;
	Money starting_money;
	uint8_t aggressiveness;  ///< 0-100
	std::string intro_dialogue;
	std::string taunt_dialogue;
	std::string defeat_dialogue;
};

/** A single scenario definition */
struct ScenarioDefinition {
	std::string id;
	std::string name;
	std::string description;
	std::string briefing;            ///< Story intro
	std::string debriefing_win;      ///< Story on victory
	std::string debriefing_lose;     ///< Story on defeat

	ScenarioDifficulty difficulty;
	uint16_t time_limit_years;       ///< 0 = no limit

	Money starting_money;
	Money starting_loan;

	std::vector<ScenarioObjective> objectives;
	std::vector<FailureCondition> fail_conditions;
	std::vector<ScenarioRival> rivals;

	/* Map settings */
	std::string map_name;            ///< Specific map to use
	uint16_t map_size_x;
	uint16_t map_size_y;

	/* Unlocks */
	std::string required_scenario;   ///< Must complete this first
	std::string unlocks_scenario;    ///< Completing this unlocks...
};

/** Campaign definition (series of scenarios) */
struct CampaignDefinition {
	std::string id;
	std::string name;
	std::string description;
	std::vector<std::string> scenario_ids;  ///< Ordered list of scenarios
};

/** Current scenario state during gameplay */
struct ScenarioState {
	const ScenarioDefinition *definition;
	bool active;
	uint32_t start_date;
	uint32_t current_date;
	uint16_t years_elapsed;

	std::vector<bool> objective_status;
	bool victory_achieved;
	bool failure_triggered;
	std::string failure_reason;

	/* Rival tracking */
	std::map<std::string, CompanyID> rival_company_map;
};

/** Global scenario state */
extern ScenarioState _current_scenario;

/** Built-in campaigns */
namespace BuiltinCampaigns {

	/* ========== CAMPAIGN 1: RISE OF AN EMPIRE ========== */
	extern const CampaignDefinition RISE_OF_EMPIRE;

	/* Chapter 1: Humble Beginnings */
	extern const ScenarioDefinition RISE_CH1_FIRST_ROUTE;

	/* Chapter 2: Competition Arrives */
	extern const ScenarioDefinition RISE_CH2_FIRST_RIVAL;

	/* Chapter 3: The Stock Market */
	extern const ScenarioDefinition RISE_CH3_WALL_STREET;

	/* Chapter 4: Hostile Waters */
	extern const ScenarioDefinition RISE_CH4_FIRST_TAKEOVER;

	/* Chapter 5: Empire Building */
	extern const ScenarioDefinition RISE_CH5_MONOPOLY;

	/* ========== CAMPAIGN 2: GORDON GEKKO ========== */
	extern const CampaignDefinition GEKKO_CAMPAIGN;

	/* Chapter 1: Greed is Good */
	extern const ScenarioDefinition GEKKO_CH1_GREED;

	/* Chapter 2: Blue Horseshoe */
	extern const ScenarioDefinition GEKKO_CH2_INSIDER;

	/* Chapter 3: The Anacott Steel Play */
	extern const ScenarioDefinition GEKKO_CH3_TAKEOVER;

	/* ========== CAMPAIGN 3: RAILWAY TYCOON ========== */
	extern const CampaignDefinition RAILWAY_CAMPAIGN;

	/* ========== CAMPAIGN 4: SURVIVAL ========== */
	extern const CampaignDefinition SURVIVAL_CAMPAIGN;
}

/** Initialize scenario system */
void InitializeScenarios();

/** Start a scenario */
bool StartScenario(const std::string &scenario_id);

/** Start a campaign */
bool StartCampaign(const std::string &campaign_id);

/** Update scenario state (call each tick) */
void UpdateScenario();

/** Check victory/failure conditions */
void CheckScenarioConditions();

/** Get current objective progress */
float GetObjectiveProgress(size_t objective_index);

/** Is scenario active? */
bool IsScenarioActive();

/** Get scenario remaining time (years) */
int16_t GetScenarioTimeRemaining();

/** Abort current scenario */
void AbortScenario();

/** Get all available scenarios */
std::vector<const ScenarioDefinition*> GetAvailableScenarios();

/** Get all campaigns */
std::vector<const CampaignDefinition*> GetAllCampaigns();

/** Is scenario unlocked? */
bool IsScenarioUnlocked(const std::string &scenario_id);

/** Mark scenario as completed */
void MarkScenarioCompleted(const std::string &scenario_id);

/** Get scenario by ID */
const ScenarioDefinition *GetScenarioById(const std::string &id);

#endif /* SCENARIOS_H */
