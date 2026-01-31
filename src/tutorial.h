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

/**
 * TRANSPORT EMPIRE TUTORIAL SYSTEM
 *
 * Step-by-step guidance for new players.
 * Shows contextual hints, highlights UI elements, and tracks progress.
 */

/** Tutorial categories */
enum class TutorialCategory : uint8_t {
	BASICS,          ///< Camera, UI, basics
	VEHICLES,        ///< Buying and managing vehicles
	ROUTES,          ///< Creating profitable routes
	STATIONS,        ///< Building stations
	FINANCES,        ///< Loans, profits, budgeting
	STOCK_MARKET,    ///< Trading shares
	TAKEOVERS,       ///< Hostile takeovers
	AI_RIVALS,       ///< Understanding competitors
	ADVANCED,        ///< Advanced strategies
};

/** Tutorial step definition */
struct TutorialStep {
	std::string id;
	std::string title;
	std::string description;
	std::string hint;
	std::string completion_trigger;  ///< What action completes this step
	std::string highlight_element;   ///< UI element to highlight
	bool requires_action;            ///< Must player do something?
	bool can_skip;
};

/** Tutorial sequence */
struct TutorialSequence {
	std::string id;
	std::string name;
	TutorialCategory category;
	std::vector<TutorialStep> steps;
};

/** Tutorial progress state */
struct TutorialState {
	bool tutorial_active;
	std::string current_sequence_id;
	size_t current_step_index;
	std::set<std::string> completed_sequences;
	std::set<std::string> completed_steps;
	bool hints_enabled;
	bool auto_advance;
};

/** Global tutorial state */
extern TutorialState _tutorial;

/** Built-in tutorials */
namespace Tutorials {

	/* ========== BASICS ========== */
	extern const TutorialSequence BASICS_CAMERA;
	extern const TutorialSequence BASICS_UI;

	/* ========== VEHICLES ========== */
	extern const TutorialSequence VEHICLES_FIRST_BUS;
	extern const TutorialSequence VEHICLES_TRAINS;

	/* ========== STOCK MARKET ========== */
	extern const TutorialSequence STOCK_MARKET_INTRO;
	extern const TutorialSequence STOCK_MARKET_TRADING;
	extern const TutorialSequence STOCK_MARKET_TAKEOVER;
}

/** Initialize tutorial system */
void InitializeTutorial();

/** Start a tutorial sequence */
bool StartTutorial(const std::string &sequence_id);

/** Advance to next step */
void AdvanceTutorialStep();

/** Skip current step */
void SkipTutorialStep();

/** Complete current tutorial */
void CompleteTutorial();

/** Cancel current tutorial */
void CancelTutorial();

/** Check if action completes current step */
void CheckTutorialTrigger(const std::string &action);

/** Is tutorial active? */
bool IsTutorialActive();

/** Get current step */
const TutorialStep *GetCurrentTutorialStep();

/** Get all available tutorials */
std::vector<const TutorialSequence*> GetAvailableTutorials();

/** Is tutorial completed? */
bool IsTutorialCompleted(const std::string &sequence_id);

/** Enable/disable hints */
void SetHintsEnabled(bool enabled);

/** Show contextual hint based on player action */
void ShowContextualHint(const std::string &context);

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
