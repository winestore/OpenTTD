/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tutorial.h Interactive tutorial system for Transport Empire. */

#ifndef TUTORIAL_H
#define TUTORIAL_H

#include "stdafx.h"
#include <vector>
#include <string>

/**
 * TRANSPORT EMPIRE TUTORIAL SYSTEM
 *
 * Step-by-step guidance for new players.
 * Shows contextual hints, highlights UI elements, and tracks progress.
 */

/** Tutorial types */
enum class TutorialType : uint8_t {
	NONE,
	BASIC,           ///< Basic gameplay tutorial
	STOCK_MARKET,    ///< Stock market mechanics
	ADVANCED,        ///< Advanced strategies
};

/** Tutorial step types */
enum class TutorialStepType : uint8_t {
	MESSAGE,         ///< Just show a message
	BUILD_STATION,   ///< Build a station
	BUILD_TRACK,     ///< Build track
	BUILD_DEPOT,     ///< Build a depot
	BUY_VEHICLE,     ///< Purchase a vehicle
	CREATE_ORDERS,   ///< Set up vehicle orders
	START_VEHICLE,   ///< Start a vehicle
	OPEN_WINDOW,     ///< Open a specific window
	BUY_SHARES,      ///< Buy shares in a company
	WAIT_FOR_PROFIT, ///< Wait until profitable
};

/** UI elements to highlight */
enum class TutorialHighlight : uint8_t {
	NONE,
	TOOLBAR_RAIL,
	TOOLBAR_ROAD,
	TOOLBAR_WATER,
	TOOLBAR_AIR,
	TOOLBAR_COMPANY,
	DEPOT,
	STATION,
	VEHICLE_ORDERS,
	VEHICLE_START,
	STOCK_WINDOW,
};

/** Conditions to check for step completion */
enum class TutorialCondition : uint8_t {
	NONE,
	STATION_BUILT,
	STATION_COUNT_2,
	TRACK_BUILT,
	DEPOT_BUILT,
	VEHICLE_BOUGHT,
	ORDERS_CREATED,
	VEHICLE_STARTED,
	PROFIT_MADE,
	SHARES_BOUGHT,
	WINDOW_OPENED,
};

/** Contextual hint triggers */
enum class TutorialContext : uint8_t {
	NONE,
	FIRST_STATION,
	FIRST_VEHICLE,
	LOW_FUNDS,
	COMPETITOR_SHARES,
	FIRST_PROFIT,
	VEHICLE_STUCK,
	INDUSTRY_CLOSING,
};

/** A single tutorial step */
struct TutorialStep {
	TutorialStepType type;
	const char *title;
	const char *description;
	TutorialHighlight highlight;
	std::vector<TutorialCondition> conditions;
	int target_value;
};

/** Tutorial state */
struct TutorialState {
	bool active;
	TutorialType current_tutorial;
	size_t current_step;
	uint32_t completed_tutorials;    ///< Bitmask of completed tutorial types
	bool hints_enabled;

	/* UI positioning */
	int16_t arrow_position_x;
	int16_t arrow_position_y;
	bool show_arrow;
	TutorialHighlight highlight;
};

/** Global tutorial state */
extern TutorialState _tutorial;

/** Initialize tutorial system */
void InitializeTutorial();

/** Start a tutorial */
bool StartTutorial(TutorialType type);

/** Get current tutorial step */
const TutorialStep *GetCurrentTutorialStep();

/** Show the current tutorial step */
void ShowTutorialStep();

/** Check if current step conditions are met */
bool CheckTutorialConditions();

/** Check a specific condition */
bool CheckTutorialCondition(TutorialCondition condition);

/** Advance to next step */
void AdvanceTutorialStep();

/** Complete current tutorial */
void CompleteTutorial();

/** Skip current tutorial */
void SkipTutorial();

/** Is tutorial active? */
bool IsTutorialActive();

/** Is specific tutorial completed? */
bool IsTutorialCompleted(TutorialType type);

/** Get contextual hint for a situation */
std::string GetContextualHint(TutorialContext context);

/** Enable/disable hints */
void SetHintsEnabled(bool enabled);

/** Update tutorial (call each tick) */
void UpdateTutorial();

/** Handle tutorial UI clicks */
void HandleTutorialClick(bool skip);

/** Should show tutorial prompt for new game? */
bool ShouldShowTutorialPrompt();

/** Hint messages for common situations */
namespace Hints {
	constexpr const char *FIRST_VEHICLE = "Tip: Start with buses - they're cheap and profitable in growing towns!";
	constexpr const char *LOW_CASH = "Tip: You can take out a loan to fund expansion. Just watch the interest!";
	constexpr const char *COMPETITOR_GROWING = "Tip: Keep an eye on your competitors. Buy their shares before they buy yours!";
	constexpr const char *STOCK_AVAILABLE = "Tip: This company has shares available. Consider investing!";
	constexpr const char *TAKEOVER_POSSIBLE = "Tip: You own enough shares for a controlling interest!";
	constexpr const char *WEATHER_WARNING = "Tip: Bad weather slows vehicles. Plan for delays!";
	constexpr const char *PROFITABLE_ROUTE = "Tip: This route is very profitable. Consider adding more vehicles!";
	constexpr const char *LOSING_MONEY = "Tip: You're losing money. Check your vehicle routes for inefficiencies.";
}

#endif /* TUTORIAL_H */
