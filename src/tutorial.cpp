/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tutorial.cpp Implementation of the interactive tutorial system. */

#include "stdafx.h"
#include "tutorial.h"
#include "company_base.h"
#include "vehicle_base.h"
#include "station_base.h"

#include "safeguards.h"

/** Global tutorial state */
TutorialState _tutorial;

/** Tutorial step definitions */
static const TutorialStep BASIC_TUTORIAL_STEPS[] = {
	/* Step 0: Welcome */
	{
		TutorialStepType::MESSAGE,
		"Welcome to Transport Empire!",
		"You're about to build a transportation empire from the ground up. "
		"This tutorial will teach you the basics of running a successful transport company.",
		TutorialHighlight::NONE,
		{},
		0
	},
	/* Step 1: Camera controls */
	{
		TutorialStepType::MESSAGE,
		"Camera Controls",
		"Use the mouse wheel to zoom in and out. Right-click and drag to move the camera. "
		"Arrow keys also work for navigation.",
		TutorialHighlight::NONE,
		{},
		0
	},
	/* Step 2: Build first station */
	{
		TutorialStepType::BUILD_STATION,
		"Build Your First Station",
		"Let's start by building a train station. Click the railway construction button "
		"in the toolbar, then select 'Build station'. Place it near a coal mine.",
		TutorialHighlight::TOOLBAR_RAIL,
		{TutorialCondition::STATION_BUILT},
		0
	},
	/* Step 3: Build second station */
	{
		TutorialStepType::BUILD_STATION,
		"Connect to a Power Station",
		"Now build another station near a power station. Coal from the mine "
		"will be transported here for profit!",
		TutorialHighlight::TOOLBAR_RAIL,
		{TutorialCondition::STATION_COUNT_2},
		0
	},
	/* Step 4: Build track */
	{
		TutorialStepType::BUILD_TRACK,
		"Lay the Rails",
		"Connect your two stations with railway track. Click and drag to build "
		"tracks between them. Use the autorail tool for easier construction.",
		TutorialHighlight::TOOLBAR_RAIL,
		{TutorialCondition::TRACK_BUILT},
		0
	},
	/* Step 5: Build depot */
	{
		TutorialStepType::BUILD_DEPOT,
		"Build a Train Depot",
		"Trains need a depot to be built. Place a depot connected to your track. "
		"This is where you'll buy and maintain your trains.",
		TutorialHighlight::TOOLBAR_RAIL,
		{TutorialCondition::DEPOT_BUILT},
		0
	},
	/* Step 6: Buy train */
	{
		TutorialStepType::BUY_VEHICLE,
		"Purchase Your First Train",
		"Click on your depot to open it, then click 'New Vehicles'. "
		"Buy a locomotive and add coal wagons to it.",
		TutorialHighlight::DEPOT,
		{TutorialCondition::VEHICLE_BOUGHT},
		0
	},
	/* Step 7: Create orders */
	{
		TutorialStepType::CREATE_ORDERS,
		"Set Up the Route",
		"Click on your train, then click the 'Orders' button. Add both stations "
		"to the train's orders so it knows where to go.",
		TutorialHighlight::VEHICLE_ORDERS,
		{TutorialCondition::ORDERS_CREATED},
		0
	},
	/* Step 8: Start train */
	{
		TutorialStepType::START_VEHICLE,
		"Start Your Train!",
		"Click the red 'Stopped' button to start your train. Watch it travel "
		"between stations and start earning money!",
		TutorialHighlight::VEHICLE_START,
		{TutorialCondition::VEHICLE_STARTED},
		0
	},
	/* Step 9: Complete */
	{
		TutorialStepType::MESSAGE,
		"Congratulations!",
		"You've completed the basic tutorial! Your train is now running and earning profit. "
		"Expand your network, explore other transport types, and build your empire!",
		TutorialHighlight::NONE,
		{},
		0
	},
};

static const TutorialStep STOCK_MARKET_TUTORIAL_STEPS[] = {
	/* Step 0: Introduction */
	{
		TutorialStepType::MESSAGE,
		"The Stock Market",
		"Welcome to Transport Empire's stock market system! Learn how to "
		"invest in competitors, defend against hostile takeovers, and become a true tycoon.",
		TutorialHighlight::NONE,
		{},
		0
	},
	/* Step 1: Open stock market */
	{
		TutorialStepType::OPEN_WINDOW,
		"Open the Stock Market",
		"Click on any company's name in the company list, or use the Stock Market "
		"button in the company toolbar to view shares.",
		TutorialHighlight::TOOLBAR_COMPANY,
		{TutorialCondition::WINDOW_OPENED},
		0
	},
	/* Step 2: Understand shares */
	{
		TutorialStepType::MESSAGE,
		"Understanding Shares",
		"Each company has 100 shares. Owning shares gives you a percentage of their profits. "
		"At 51%% ownership, you gain control. At 75%%, you can force a complete takeover!",
		TutorialHighlight::STOCK_WINDOW,
		{},
		0
	},
	/* Step 3: Buy shares */
	{
		TutorialStepType::BUY_SHARES,
		"Buy Your First Shares",
		"Select a competitor and click 'Buy Shares'. Start small - each purchase "
		"buys 10%% of the company. Watch how it affects the relationship!",
		TutorialHighlight::STOCK_WINDOW,
		{TutorialCondition::SHARES_BOUGHT},
		0
	},
	/* Step 4: Check company value */
	{
		TutorialStepType::MESSAGE,
		"Share Prices",
		"Share prices are based on company value divided by 100. As companies grow, "
		"their shares become more valuable. Buy low, sell high!",
		TutorialHighlight::STOCK_WINDOW,
		{},
		0
	},
	/* Step 5: Hostile takeover warning */
	{
		TutorialStepType::MESSAGE,
		"Hostile Takeovers",
		"When you own 75%% of a company, you can execute a hostile takeover! "
		"But beware - AI CEOs will fight back, and it creates enemies.",
		TutorialHighlight::STOCK_WINDOW,
		{},
		0
	},
	/* Step 6: Protection */
	{
		TutorialStepType::MESSAGE,
		"Protecting Your Company",
		"Other companies can buy YOUR shares too! Keep your company value high "
		"and watch for competitors accumulating your stock.",
		TutorialHighlight::NONE,
		{},
		0
	},
	/* Step 7: Complete */
	{
		TutorialStepType::MESSAGE,
		"Stock Market Master",
		"You now understand the stock market! Use it to earn passive income, "
		"gain control of competitors, or build a diversified empire. Good luck!",
		TutorialHighlight::NONE,
		{},
		0
	},
};

static const TutorialStep ADVANCED_TUTORIAL_STEPS[] = {
	/* Advanced topics: signals, road vehicles, ships, aircraft, etc. */
	{
		TutorialStepType::MESSAGE,
		"Advanced Transport Empire",
		"Ready for advanced strategies? Learn about signal blocks, "
		"multi-modal transport, and competing with aggressive AI CEOs.",
		TutorialHighlight::NONE,
		{},
		0
	},
	/* More steps would go here... */
};

/** Initialize tutorial system */
void InitializeTutorial()
{
	_tutorial.active = false;
	_tutorial.current_tutorial = TutorialType::NONE;
	_tutorial.current_step = 0;
	_tutorial.completed_tutorials = 0;
	_tutorial.hints_enabled = true;
	_tutorial.arrow_position_x = 0;
	_tutorial.arrow_position_y = 0;
	_tutorial.show_arrow = false;
	_tutorial.highlight = TutorialHighlight::NONE;
}

/** Start a tutorial */
bool StartTutorial(TutorialType type)
{
	if (_tutorial.active) {
		/* Already in a tutorial */
		return false;
	}

	_tutorial.current_tutorial = type;
	_tutorial.current_step = 0;
	_tutorial.active = true;

	/* Show first step */
	ShowTutorialStep();

	return true;
}

/** Get current tutorial steps */
static const TutorialStep *GetTutorialSteps(size_t &count)
{
	switch (_tutorial.current_tutorial) {
		case TutorialType::BASIC:
			count = lengthof(BASIC_TUTORIAL_STEPS);
			return BASIC_TUTORIAL_STEPS;
		case TutorialType::STOCK_MARKET:
			count = lengthof(STOCK_MARKET_TUTORIAL_STEPS);
			return STOCK_MARKET_TUTORIAL_STEPS;
		case TutorialType::ADVANCED:
			count = lengthof(ADVANCED_TUTORIAL_STEPS);
			return ADVANCED_TUTORIAL_STEPS;
		default:
			count = 0;
			return nullptr;
	}
}

/** Get current step */
const TutorialStep *GetCurrentTutorialStep()
{
	if (!_tutorial.active) return nullptr;

	size_t count;
	const TutorialStep *steps = GetTutorialSteps(count);
	if (steps == nullptr || _tutorial.current_step >= count) return nullptr;

	return &steps[_tutorial.current_step];
}

/** Show the current tutorial step */
void ShowTutorialStep()
{
	const TutorialStep *step = GetCurrentTutorialStep();
	if (step == nullptr) return;

	/* Update highlight */
	_tutorial.highlight = step->highlight;

	/* Position arrow if needed */
	switch (step->highlight) {
		case TutorialHighlight::TOOLBAR_RAIL:
			_tutorial.show_arrow = true;
			_tutorial.arrow_position_x = 100; /* TODO: Get actual toolbar position */
			_tutorial.arrow_position_y = 20;
			break;
		case TutorialHighlight::TOOLBAR_COMPANY:
			_tutorial.show_arrow = true;
			_tutorial.arrow_position_x = 400;
			_tutorial.arrow_position_y = 20;
			break;
		default:
			_tutorial.show_arrow = false;
			break;
	}

	/* TODO: Show tutorial dialog window with step->title and step->description */
}

/** Check if tutorial conditions are met */
bool CheckTutorialConditions()
{
	const TutorialStep *step = GetCurrentTutorialStep();
	if (step == nullptr) return false;

	/* If no conditions, it's a message step - always completable */
	if (step->conditions.empty()) return true;

	/* Check all conditions */
	for (TutorialCondition cond : step->conditions) {
		if (!CheckTutorialCondition(cond)) {
			return false;
		}
	}

	return true;
}

/** Check a single condition */
bool CheckTutorialCondition(TutorialCondition condition)
{
	Company *c = Company::GetIfValid(CompanyID::Begin()); /* Player company */
	if (c == nullptr) return false;

	switch (condition) {
		case TutorialCondition::STATION_BUILT:
			/* Check if player has at least one station */
			for (const Station *st : Station::Iterate()) {
				if (st->owner == CompanyID::Begin()) return true;
			}
			return false;

		case TutorialCondition::STATION_COUNT_2:
			{
				int count = 0;
				for (const Station *st : Station::Iterate()) {
					if (st->owner == CompanyID::Begin()) count++;
				}
				return count >= 2;
			}

		case TutorialCondition::TRACK_BUILT:
			/* Simplified check - assume track is built if we have 2 stations */
			return true; /* TODO: Actually check track */

		case TutorialCondition::DEPOT_BUILT:
			/* Check for depot */
			return true; /* TODO: Check depot count */

		case TutorialCondition::VEHICLE_BOUGHT:
			for (const Vehicle *v : Vehicle::Iterate()) {
				if (v->owner == CompanyID::Begin()) return true;
			}
			return false;

		case TutorialCondition::ORDERS_CREATED:
			for (const Vehicle *v : Vehicle::Iterate()) {
				if (v->owner == CompanyID::Begin() && v->GetNumOrders() >= 2) return true;
			}
			return false;

		case TutorialCondition::VEHICLE_STARTED:
			for (const Vehicle *v : Vehicle::Iterate()) {
				if (v->owner == CompanyID::Begin() && !v->IsStoppedInDepot()) return true;
			}
			return false;

		case TutorialCondition::PROFIT_MADE:
			return c->cur_economy.income > 0;

		case TutorialCondition::SHARES_BOUGHT:
			/* Check if player owns shares in any other company */
			return true; /* TODO: Check share ownership */

		default:
			return true;
	}
}

/** Advance to next tutorial step */
void AdvanceTutorialStep()
{
	if (!_tutorial.active) return;

	size_t count;
	GetTutorialSteps(count);

	_tutorial.current_step++;

	if (_tutorial.current_step >= count) {
		/* Tutorial complete */
		CompleteTutorial();
	} else {
		ShowTutorialStep();
	}
}

/** Complete current tutorial */
void CompleteTutorial()
{
	if (!_tutorial.active) return;

	/* Mark as completed */
	_tutorial.completed_tutorials |= (1 << static_cast<uint8_t>(_tutorial.current_tutorial));

	_tutorial.active = false;
	_tutorial.current_tutorial = TutorialType::NONE;
	_tutorial.show_arrow = false;
	_tutorial.highlight = TutorialHighlight::NONE;

	/* TODO: Show completion message and reward */
}

/** Skip current tutorial */
void SkipTutorial()
{
	_tutorial.active = false;
	_tutorial.current_tutorial = TutorialType::NONE;
	_tutorial.show_arrow = false;
	_tutorial.highlight = TutorialHighlight::NONE;
}

/** Is tutorial active? */
bool IsTutorialActive()
{
	return _tutorial.active;
}

/** Is tutorial completed? */
bool IsTutorialCompleted(TutorialType type)
{
	return (_tutorial.completed_tutorials & (1 << static_cast<uint8_t>(type))) != 0;
}

/** Get contextual hint */
std::string GetContextualHint(TutorialContext context)
{
	if (!_tutorial.hints_enabled) return "";

	switch (context) {
		case TutorialContext::FIRST_STATION:
			return "Tip: Place stations close to industries to maximize cargo pickup.";
		case TutorialContext::FIRST_VEHICLE:
			return "Tip: Make sure your vehicle has enough capacity for the cargo type!";
		case TutorialContext::LOW_FUNDS:
			return "Tip: Take out a loan if you need more cash, or sell some assets.";
		case TutorialContext::COMPETITOR_SHARES:
			return "Tip: Someone is buying your shares! Consider buying them back.";
		case TutorialContext::FIRST_PROFIT:
			return "Congratulations on your first profit! Keep expanding to grow faster.";
		case TutorialContext::VEHICLE_STUCK:
			return "Tip: Your vehicle can't find a path. Check for missing track or signals.";
		case TutorialContext::INDUSTRY_CLOSING:
			return "Tip: Industries close if not serviced. Connect them to keep them alive!";
		default:
			return "";
	}
}

/** Toggle hints */
void SetHintsEnabled(bool enabled)
{
	_tutorial.hints_enabled = enabled;
}

/** Update tutorial (called each tick) */
void UpdateTutorial()
{
	if (!_tutorial.active) return;

	/* Check if current step conditions are met */
	if (CheckTutorialConditions()) {
		const TutorialStep *step = GetCurrentTutorialStep();
		if (step != nullptr && step->type != TutorialStepType::MESSAGE) {
			/* Auto-advance for action steps when conditions are met */
			/* For message steps, player must click to continue */
		}
	}
}

/** Handle tutorial click (player clicked continue/skip) */
void HandleTutorialClick(bool skip)
{
	if (skip) {
		SkipTutorial();
	} else {
		/* Check if we can advance */
		if (CheckTutorialConditions()) {
			AdvanceTutorialStep();
		}
	}
}

/** Should show tutorial prompt for new game? */
bool ShouldShowTutorialPrompt()
{
	return !IsTutorialCompleted(TutorialType::BASIC);
}
