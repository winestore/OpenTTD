/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file economy_dashboard.h Economic indicators dashboard for Transport Empire. */

#ifndef ECONOMY_DASHBOARD_H
#define ECONOMY_DASHBOARD_H

#include "stdafx.h"
#include "company_type.h"
#include "timer/timer_game_calendar.h"
#include "economy_type.h"
#include "gfx_type.h"
#include <string>
#include <vector>

/**
 * TRANSPORT EMPIRE ECONOMY DASHBOARD
 *
 * Provides at-a-glance economic indicators for informed decision-making:
 * - Market health indicators
 * - Industry trends
 * - Company performance metrics
 * - Investment recommendations
 * - Risk assessment
 */

/** Economic health rating */
enum class EconomicHealth : uint8_t {
	DEPRESSION,      ///< Severe downturn (-3)
	RECESSION,       ///< Moderate decline (-2)
	STAGNANT,        ///< Flat growth (-1)
	STABLE,          ///< Normal conditions (0)
	GROWING,         ///< Moderate growth (+1)
	BOOMING,         ///< Strong growth (+2)
	BUBBLE,          ///< Unsustainable growth (+3)
};

/** Trend direction */
enum class Trend : uint8_t {
	FALLING_FAST,
	FALLING,
	STABLE,
	RISING,
	RISING_FAST,
};

/** Industry sector performance */
struct SectorPerformance {
	std::string name;
	int32_t production_change_pct;   ///< % change from last period
	int32_t price_change_pct;        ///< Cargo price change
	Trend trend;
	bool is_hot;                      ///< Currently profitable
	bool is_risky;                    ///< High volatility
};

/** Company financial health snapshot */
struct CompanyHealthSnapshot {
	CompanyID company;
	std::string name;

	/* Profitability */
	Money revenue;
	Money expenses;
	Money profit;
	int32_t profit_margin_pct;

	/* Growth */
	int32_t revenue_growth_pct;
	int32_t fleet_growth_pct;
	int32_t route_growth_pct;

	/* Financial stability */
	Money cash_on_hand;
	Money debt;
	float debt_to_equity;
	int32_t months_of_runway;        ///< How long can survive at current burn

	/* Stock performance */
	Money share_price;
	int32_t share_price_change_pct;
	uint8_t shares_available;
	bool takeover_vulnerable;        ///< >50% shares available

	/* Overall rating */
	uint8_t health_score;            ///< 0-100
	std::string health_label;        ///< "Strong", "Stable", "At Risk", etc.
};

/** Market indicator */
struct MarketIndicator {
	std::string name;
	std::string description;
	float value;
	float previous_value;
	Trend trend;
	std::string interpretation;
};

/** Investment opportunity */
struct InvestmentOpportunity {
	enum class Type : uint8_t {
		BUY_SHARES,
		SELL_SHARES,
		EXPAND_ROUTE,
		NEW_INDUSTRY,
		VEHICLE_UPGRADE,
	};

	Type type;
	std::string title;
	std::string description;
	int32_t expected_return_pct;
	uint8_t risk_level;              ///< 1-5
	Money required_investment;
	CompanyID target_company;        ///< For share transactions
};

/** Economic forecast */
struct EconomicForecast {
	EconomicHealth current_health;
	EconomicHealth predicted_health;
	uint8_t confidence_pct;
	std::string summary;
	std::vector<std::string> factors;
};

/** Dashboard state */
struct EconomyDashboardState {
	/* Overall economy */
	EconomicHealth market_health;
	float gdp_index;                 ///< 100 = baseline
	float inflation_rate;
	float interest_rate;

	/* Historical data (last 12 periods) */
	static constexpr int HISTORY_SIZE = 12;
	float gdp_history[HISTORY_SIZE];
	float inflation_history[HISTORY_SIZE];

	/* Sector performance */
	std::vector<SectorPerformance> sectors;

	/* Company snapshots */
	std::vector<CompanyHealthSnapshot> company_health;

	/* Market indicators */
	std::vector<MarketIndicator> indicators;

	/* Investment opportunities */
	std::vector<InvestmentOpportunity> opportunities;

	/* Forecast */
	EconomicForecast forecast;

	/* Update tracking */
	uint32_t last_update_date;
};

/** Global dashboard state */
extern EconomyDashboardState _economy_dashboard;

/** Initialize dashboard */
void InitializeEconomyDashboard();

/** Update dashboard (call monthly or on demand) */
void UpdateEconomyDashboard();

/** Get current economic health */
EconomicHealth GetEconomicHealth();

/** Get health label text */
const char *GetEconomicHealthText(EconomicHealth health);

/** Get trend text */
const char *GetTrendText(Trend trend);

/** Calculate company health score */
uint8_t CalculateCompanyHealthScore(CompanyID company);

/** Get top investment opportunities */
std::vector<InvestmentOpportunity> GetTopOpportunities(int count);

/** Get sector performance */
const SectorPerformance *GetSectorPerformance(const std::string &sector_name);

/** Get company health snapshot */
const CompanyHealthSnapshot *GetCompanyHealth(CompanyID company);

/** Generate economic forecast */
EconomicForecast GenerateEconomicForecast();

/** Dashboard widget data for GUI */
struct DashboardWidgetData {
	/* Quick summary */
	std::string market_status;       ///< "Bull Market", "Bear Market", etc.
	Colour status_color;
	std::string top_opportunity;
	std::string biggest_risk;

	/* Key metrics */
	int32_t market_change_pct;
	int32_t player_profit_change_pct;
	int32_t industry_output_change_pct;

	/* Alerts */
	std::vector<std::string> alerts;
};

/** Get dashboard widget data */
DashboardWidgetData GetDashboardWidgetData();

/** Check for economic alerts */
void CheckEconomicAlerts();

/** Economic event types */
enum class EconomicEventType : uint8_t {
	MARKET_CRASH,
	MARKET_RECOVERY,
	INDUSTRY_BOOM,
	INDUSTRY_BUST,
	INFLATION_SPIKE,
	DEFLATION,
	COMPETITOR_STRUGGLING,
	COMPETITOR_THRIVING,
	TAKEOVER_OPPORTUNITY,
	INVESTMENT_MATURED,
};

/** Subscribe to economic events */
using EconomicEventCallback = void (*)(EconomicEventType, const void *data);
void SubscribeEconomicEvents(EconomicEventCallback callback);

#endif /* ECONOMY_DASHBOARD_H */
