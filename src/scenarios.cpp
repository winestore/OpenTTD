/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file scenarios.cpp Implementation of Transport Empire scenario system. */

#include "stdafx.h"
#include "scenarios.h"
#include "company_base.h"
#include "ai_personality.h"
#include "game_events.h"
#include "empire_news.h"
#include "screen_effects.h"
#include <set>

#include "safeguards.h"

/** Global scenario state */
ScenarioState _current_scenario;

/** Completed scenarios (persisted) */
static std::set<std::string> _completed_scenarios;

/* ========== BUILT-IN SCENARIO DEFINITIONS ========== */

namespace BuiltinCampaigns {

/* ---------- RISE OF AN EMPIRE CAMPAIGN ---------- */

const ScenarioDefinition RISE_CH1_FIRST_ROUTE = {
	.id = "rise_ch1",
	.name = "Chapter 1: Humble Beginnings",
	.description = "Every empire starts somewhere. Build your first profitable route.",
	.briefing = "Welcome to the transport industry! You've just founded your company with a modest loan. "
	            "Your first task is simple: connect two towns with a bus route and turn a profit. "
	            "The road to becoming a transport tycoon begins with a single route.",
	.debriefing_win = "Excellent work! You've established your first profitable route. "
	                   "But don't get comfortable - competition is coming.",
	.debriefing_lose = "Your company has failed. The transport industry is unforgiving. "
	                    "Review your strategy and try again.",
	.difficulty = ScenarioDifficulty::TUTORIAL,
	.time_limit_years = 5,
	.starting_money = 100000,
	.starting_loan = 100000,
	.objectives = {
		{VictoryCondition::VEHICLE_COUNT, 3, "Own at least 3 vehicles", false, false, 0},
		{VictoryCondition::PROFIT_YEARLY, 10000, "Achieve $10,000 yearly profit", false, false, 0},
		{VictoryCondition::VEHICLE_COUNT, 10, "Own 10 vehicles (bonus)", false, true, 50000},
	},
	.fail_conditions = {FailureCondition::BANKRUPTCY, FailureCondition::TIME_LIMIT},
	.rivals = {},
	.map_name = "",
	.map_size_x = 256,
	.map_size_y = 256,
	.required_scenario = "",
	.unlocks_scenario = "rise_ch2",
};

const ScenarioDefinition RISE_CH2_FIRST_RIVAL = {
	.id = "rise_ch2",
	.name = "Chapter 2: Competition Arrives",
	.description = "A rival company has entered your territory. Defend your market share!",
	.briefing = "Your success has attracted attention. 'Steady Transport Co.' has set up shop nearby. "
	            "Their CEO, Edward Pemberton, is a conservative operator who believes in slow, steady growth. "
	            "You'll need to outmaneuver him to maintain your dominance.",
	.debriefing_win = "You've proven you can handle competition! Pemberton is retreating. "
	                   "But bigger challenges await...",
	.debriefing_lose = "Pemberton's steady approach won the day. Sometimes slow and steady does win the race.",
	.difficulty = ScenarioDifficulty::EASY,
	.time_limit_years = 10,
	.starting_money = 200000,
	.starting_loan = 100000,
	.objectives = {
		{VictoryCondition::COMPANY_VALUE, 500000, "Reach $500,000 company value", false, false, 0},
		{VictoryCondition::VEHICLE_COUNT, 15, "Own at least 15 vehicles", false, false, 0},
		{VictoryCondition::DEFEAT_RIVAL, 1, "Make Steady Transport go bankrupt (bonus)", false, true, 100000},
	},
	.fail_conditions = {FailureCondition::BANKRUPTCY, FailureCondition::TIME_LIMIT},
	.rivals = {
		{
			.name = "Steady Transport Co.",
			.personality = CEOPersonality::STEADY_EDDIE,
			.starting_money = 150000,
			.aggressiveness = 30,
			.intro_dialogue = "Welcome to the industry. I hope we can compete fairly.",
			.taunt_dialogue = "Slow and steady wins the race, friend.",
			.defeat_dialogue = "Well played. Perhaps I was too cautious.",
		}
	},
	.map_name = "",
	.map_size_x = 512,
	.map_size_y = 512,
	.required_scenario = "rise_ch1",
	.unlocks_scenario = "rise_ch3",
};

const ScenarioDefinition RISE_CH3_WALL_STREET = {
	.id = "rise_ch3",
	.name = "Chapter 3: Wall Street Calling",
	.description = "Enter the stock market. Buy shares, manipulate prices, grow your empire.",
	.briefing = "Your company has caught the eye of investors. It's time to go public. "
	            "The stock market offers new opportunities - and new dangers. "
	            "Learn to buy and sell shares, and watch out for hostile takeover attempts.",
	.debriefing_win = "You've mastered the basics of the stock market. "
	                   "But there are sharks in these waters...",
	.debriefing_lose = "The market can be cruel. Study the trends and try again.",
	.difficulty = ScenarioDifficulty::MEDIUM,
	.time_limit_years = 15,
	.starting_money = 500000,
	.starting_loan = 200000,
	.objectives = {
		{VictoryCondition::COMPANY_VALUE, 2000000, "Reach $2,000,000 company value", false, false, 0},
		{VictoryCondition::SHARE_CONTROL, 25, "Own 25% shares in any competitor", false, false, 0},
		{VictoryCondition::TAKEOVER_COUNT, 1, "Complete a hostile takeover (bonus)", false, true, 250000},
	},
	.fail_conditions = {FailureCondition::BANKRUPTCY, FailureCondition::LOSE_CONTROL},
	.rivals = {
		{
			.name = "Buffett Rail Inc.",
			.personality = CEOPersonality::WARREN_BUFFETT,
			.starting_money = 400000,
			.aggressiveness = 20,
			.intro_dialogue = "Price is what you pay. Value is what you get.",
			.taunt_dialogue = "Be fearful when others are greedy.",
			.defeat_dialogue = "I see you understand value investing.",
		}
	},
	.map_name = "",
	.map_size_x = 512,
	.map_size_y = 512,
	.required_scenario = "rise_ch2",
	.unlocks_scenario = "rise_ch4",
};

const ScenarioDefinition RISE_CH4_FIRST_TAKEOVER = {
	.id = "rise_ch4",
	.name = "Chapter 4: Hostile Waters",
	.description = "Gordon Gekko is circling. Take him down before he takes you.",
	.briefing = "Word on the street: Gordon Gekko of 'Predator Capital' has you in his sights. "
	            "He's already buying up shares. You have two choices: take him over first, "
	            "or be consumed by his empire. This is corporate warfare.",
	.debriefing_win = "Gekko is defeated! You've proven you can play with the big boys. "
	                   "Now it's time to become one.",
	.debriefing_lose = "Gekko was right: greed is good. At least, his greed was.",
	.difficulty = ScenarioDifficulty::HARD,
	.time_limit_years = 10,
	.starting_money = 1000000,
	.starting_loan = 500000,
	.objectives = {
		{VictoryCondition::TAKEOVER_COMPANY, 1, "Take over Predator Capital", false, false, 0},
		{VictoryCondition::SURVIVE_YEARS, 10, "OR survive for 10 years without being taken over", false, false, 0},
	},
	.fail_conditions = {FailureCondition::BANKRUPTCY, FailureCondition::LOSE_CONTROL},
	.rivals = {
		{
			.name = "Predator Capital",
			.personality = CEOPersonality::GORDON_GEKKO,
			.starting_money = 1500000,
			.aggressiveness = 95,
			.intro_dialogue = "Greed, for lack of a better word, is good.",
			.taunt_dialogue = "The most valuable commodity I know of is information.",
			.defeat_dialogue = "I underestimated you. That won't happen again.",
		}
	},
	.map_name = "",
	.map_size_x = 1024,
	.map_size_y = 1024,
	.required_scenario = "rise_ch3",
	.unlocks_scenario = "rise_ch5",
};

const ScenarioDefinition RISE_CH5_MONOPOLY = {
	.id = "rise_ch5",
	.name = "Chapter 5: Empire",
	.description = "The final chapter. Become the only company standing.",
	.briefing = "You've come so far. Now, there's only one goal left: total domination. "
	            "Take over or bankrupt every competitor. Build a transport monopoly. "
	            "This is your empire. Make it absolute.",
	.debriefing_win = "VICTORY! You are the undisputed ruler of the transport industry. "
	                   "Your name will be remembered alongside the great tycoons of history.",
	.debriefing_lose = "So close, yet so far. The empire crumbles.",
	.difficulty = ScenarioDifficulty::NIGHTMARE,
	.time_limit_years = 0,  /* No time limit */
	.starting_money = 5000000,
	.starting_loan = 1000000,
	.objectives = {
		{VictoryCondition::MONOPOLY, 1, "Be the only remaining company", false, false, 0},
	},
	.fail_conditions = {FailureCondition::BANKRUPTCY},
	.rivals = {
		{
			.name = "Titan Railways",
			.personality = CEOPersonality::CORNELIUS_VANDERBILT,
			.starting_money = 3000000,
			.aggressiveness = 90,
			.intro_dialogue = "I have been insane on the subject of moneymaking all my life.",
			.taunt_dialogue = "You have undertaken to cheat me. I won't sue you, I'll ruin you.",
			.defeat_dialogue = "You've stolen my railroad. But not my legacy.",
		},
		{
			.name = "Rockefeller Logistics",
			.personality = CEOPersonality::JOHN_ROCKEFELLER,
			.starting_money = 4000000,
			.aggressiveness = 80,
			.intro_dialogue = "Competition is a sin.",
			.taunt_dialogue = "I have ways of making money that you know nothing of.",
			.defeat_dialogue = "The growth of a large business... merely survival of the fittest.",
		},
		{
			.name = "Future Transport",
			.personality = CEOPersonality::ELON_MUSK,
			.starting_money = 2000000,
			.aggressiveness = 70,
			.intro_dialogue = "When something is important enough, you do it even if the odds are against you.",
			.taunt_dialogue = "I'd rather be optimistic and wrong than pessimistic and right.",
			.defeat_dialogue = "Whatever. This just frees me up for my next venture.",
		},
	},
	.map_name = "",
	.map_size_x = 2048,
	.map_size_y = 2048,
	.required_scenario = "rise_ch4",
	.unlocks_scenario = "",
};

/* Campaign definition */
const CampaignDefinition RISE_OF_EMPIRE = {
	.id = "rise_of_empire",
	.name = "Rise of an Empire",
	.description = "From humble beginnings to transport monopoly. The classic tycoon journey.",
	.scenario_ids = {"rise_ch1", "rise_ch2", "rise_ch3", "rise_ch4", "rise_ch5"},
};

/* ---------- GORDON GEKKO CAMPAIGN ---------- */

const ScenarioDefinition GEKKO_CH1_GREED = {
	.id = "gekko_ch1",
	.name = "Greed is Good",
	.description = "Play as a corporate raider. Make money through takeovers, not transport.",
	.briefing = "Forget building routes. Real money is made on Wall Street. "
	            "Your goal: acquire controlling interests in other companies and strip their assets. "
	            "Remember: greed is good.",
	.debriefing_win = "Your first kill. The taste of corporate blood is sweet.",
	.debriefing_lose = "Even sharks get eaten sometimes.",
	.difficulty = ScenarioDifficulty::MEDIUM,
	.time_limit_years = 10,
	.starting_money = 2000000,
	.starting_loan = 1000000,
	.objectives = {
		{VictoryCondition::TAKEOVER_COUNT, 2, "Complete 2 hostile takeovers", false, false, 0},
		{VictoryCondition::COMPANY_VALUE, 5000000, "Reach $5,000,000 company value", false, false, 0},
	},
	.fail_conditions = {FailureCondition::BANKRUPTCY, FailureCondition::TIME_LIMIT},
	.rivals = {
		{
			.name = "Old Money Rail",
			.personality = CEOPersonality::STEADY_EDDIE,
			.starting_money = 800000,
			.aggressiveness = 20,
			.intro_dialogue = "We've been in this business for generations.",
			.taunt_dialogue = "Money can't buy class.",
			.defeat_dialogue = "Father always said to watch out for your type.",
		},
		{
			.name = "Mom & Pop Transport",
			.personality = CEOPersonality::PENNY_PINCHER,
			.starting_money = 500000,
			.aggressiveness = 10,
			.intro_dialogue = "We built this company with our own two hands.",
			.taunt_dialogue = "Some things are more important than money.",
			.defeat_dialogue = "How could you do this to a family business?",
		},
	},
	.map_name = "",
	.map_size_x = 512,
	.map_size_y = 512,
	.required_scenario = "",
	.unlocks_scenario = "gekko_ch2",
};

const CampaignDefinition GEKKO_CAMPAIGN = {
	.id = "gekko_campaign",
	.name = "Gordon Gekko: Corporate Raider",
	.description = "Play the villain. Make your fortune through hostile takeovers and market manipulation.",
	.scenario_ids = {"gekko_ch1", "gekko_ch2", "gekko_ch3"},
};

} /* namespace BuiltinCampaigns */

/** All scenarios */
static std::vector<const ScenarioDefinition*> _all_scenarios;

/** All campaigns */
static std::vector<const CampaignDefinition*> _all_campaigns;

/** Initialize scenarios */
void InitializeScenarios()
{
	_current_scenario.active = false;
	_current_scenario.definition = nullptr;

	/* Register built-in scenarios */
	_all_scenarios.clear();
	_all_scenarios.push_back(&BuiltinCampaigns::RISE_CH1_FIRST_ROUTE);
	_all_scenarios.push_back(&BuiltinCampaigns::RISE_CH2_FIRST_RIVAL);
	_all_scenarios.push_back(&BuiltinCampaigns::RISE_CH3_WALL_STREET);
	_all_scenarios.push_back(&BuiltinCampaigns::RISE_CH4_FIRST_TAKEOVER);
	_all_scenarios.push_back(&BuiltinCampaigns::RISE_CH5_MONOPOLY);
	_all_scenarios.push_back(&BuiltinCampaigns::GEKKO_CH1_GREED);

	/* Register campaigns */
	_all_campaigns.clear();
	_all_campaigns.push_back(&BuiltinCampaigns::RISE_OF_EMPIRE);
	_all_campaigns.push_back(&BuiltinCampaigns::GEKKO_CAMPAIGN);
}

/** Start a scenario */
bool StartScenario(const std::string &scenario_id)
{
	const ScenarioDefinition *def = GetScenarioById(scenario_id);
	if (def == nullptr) return false;

	if (!IsScenarioUnlocked(scenario_id)) return false;

	_current_scenario.definition = def;
	_current_scenario.active = true;
	_current_scenario.start_date = 0; /* TODO: Get current date */
	_current_scenario.years_elapsed = 0;
	_current_scenario.victory_achieved = false;
	_current_scenario.failure_triggered = false;

	/* Initialize objective tracking */
	_current_scenario.objective_status.clear();
	for (size_t i = 0; i < def->objectives.size(); i++) {
		_current_scenario.objective_status.push_back(false);
	}

	/* TODO: Apply starting money/loan */
	/* TODO: Spawn rival companies */

	/* Show briefing */
	/* TODO: Display briefing in UI */

	return true;
}

/** Update scenario */
void UpdateScenario()
{
	if (!_current_scenario.active) return;

	CheckScenarioConditions();
}

/** Check victory/failure */
void CheckScenarioConditions()
{
	if (!_current_scenario.active || _current_scenario.definition == nullptr) return;

	const ScenarioDefinition *def = _current_scenario.definition;

	/* Check failure conditions */
	for (const auto &fail : def->fail_conditions) {
		switch (fail) {
			case FailureCondition::BANKRUPTCY:
				/* TODO: Check if player is bankrupt */
				break;
			case FailureCondition::TIME_LIMIT:
				if (def->time_limit_years > 0 && _current_scenario.years_elapsed >= def->time_limit_years) {
					_current_scenario.failure_triggered = true;
					_current_scenario.failure_reason = "Time limit exceeded";
				}
				break;
			default:
				break;
		}
	}

	if (_current_scenario.failure_triggered) {
		/* TODO: Show failure screen */
		OnMarketCrash(); /* Dramatic effect */
		return;
	}

	/* Check victory conditions */
	bool all_required_complete = true;
	for (size_t i = 0; i < def->objectives.size(); i++) {
		const auto &obj = def->objectives[i];
		float progress = GetObjectiveProgress(i);

		if (progress >= 1.0f) {
			_current_scenario.objective_status[i] = true;
		}

		if (!obj.optional && !_current_scenario.objective_status[i]) {
			all_required_complete = false;
		}
	}

	if (all_required_complete) {
		_current_scenario.victory_achieved = true;
		MarkScenarioCompleted(def->id);
		OnAchievementUnlocked(); /* Victory effect */
		/* TODO: Show victory screen */
	}
}

/** Get objective progress (0.0 to 1.0+) */
float GetObjectiveProgress(size_t objective_index)
{
	if (!_current_scenario.active || _current_scenario.definition == nullptr) return 0.0f;
	if (objective_index >= _current_scenario.definition->objectives.size()) return 0.0f;

	const auto &obj = _current_scenario.definition->objectives[objective_index];

	/* TODO: Actually calculate progress based on game state */
	/* For now, return placeholder */

	switch (obj.type) {
		case VictoryCondition::COMPANY_VALUE:
			/* return player_company_value / obj.target_value */
			break;
		case VictoryCondition::VEHICLE_COUNT:
			/* return player_vehicle_count / obj.target_value */
			break;
		default:
			break;
	}

	return 0.0f;
}

/** Is scenario active? */
bool IsScenarioActive()
{
	return _current_scenario.active;
}

/** Get remaining time */
int16_t GetScenarioTimeRemaining()
{
	if (!_current_scenario.active || _current_scenario.definition == nullptr) return -1;
	if (_current_scenario.definition->time_limit_years == 0) return -1;

	return _current_scenario.definition->time_limit_years - _current_scenario.years_elapsed;
}

/** Get scenario by ID */
const ScenarioDefinition *GetScenarioById(const std::string &id)
{
	for (const auto *scenario : _all_scenarios) {
		if (scenario->id == id) return scenario;
	}
	return nullptr;
}

/** Is scenario unlocked? */
bool IsScenarioUnlocked(const std::string &scenario_id)
{
	const ScenarioDefinition *def = GetScenarioById(scenario_id);
	if (def == nullptr) return false;

	/* First scenario of campaign is always unlocked */
	if (def->required_scenario.empty()) return true;

	/* Check if required scenario is completed */
	return _completed_scenarios.count(def->required_scenario) > 0;
}

/** Mark completed */
void MarkScenarioCompleted(const std::string &scenario_id)
{
	_completed_scenarios.insert(scenario_id);
}

/** Get available scenarios */
std::vector<const ScenarioDefinition*> GetAvailableScenarios()
{
	return _all_scenarios;
}

/** Get all campaigns */
std::vector<const CampaignDefinition*> GetAllCampaigns()
{
	return _all_campaigns;
}

/** Abort scenario */
void AbortScenario()
{
	_current_scenario.active = false;
	_current_scenario.definition = nullptr;
}

/** Start campaign */
bool StartCampaign(const std::string &campaign_id)
{
	for (const auto *campaign : _all_campaigns) {
		if (campaign->id == campaign_id) {
			if (!campaign->scenario_ids.empty()) {
				return StartScenario(campaign->scenario_ids[0]);
			}
		}
	}
	return false;
}
