/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file economy_dashboard.cpp Implementation of economic indicators dashboard. */

#include "stdafx.h"
#include "economy_dashboard.h"
#include "company_base.h"
#include "industry.h"
#include "station_base.h"
#include "vehicle_base.h"
#include "core/random_func.hpp"

#include "safeguards.h"

/** Global dashboard state */
EconomyDashboardState _economy_dashboard;

/** Economic event subscribers */
static std::vector<EconomicEventCallback> _economic_event_callbacks;

/** Initialize dashboard */
void InitializeEconomyDashboard()
{
	_economy_dashboard.market_health = EconomicHealth::STABLE;
	_economy_dashboard.gdp_index = 100.0f;
	_economy_dashboard.inflation_rate = 2.0f;
	_economy_dashboard.interest_rate = 5.0f;

	/* Initialize history */
	for (int i = 0; i < EconomyDashboardState::HISTORY_SIZE; i++) {
		_economy_dashboard.gdp_history[i] = 100.0f;
		_economy_dashboard.inflation_history[i] = 2.0f;
	}

	_economy_dashboard.sectors.clear();
	_economy_dashboard.company_health.clear();
	_economy_dashboard.indicators.clear();
	_economy_dashboard.opportunities.clear();
	_economy_dashboard.last_update_date = 0;

	/* Initialize sectors */
	_economy_dashboard.sectors.push_back({"Coal & Power", 0, 0, Trend::STABLE, false, false});
	_economy_dashboard.sectors.push_back({"Iron & Steel", 0, 0, Trend::STABLE, false, false});
	_economy_dashboard.sectors.push_back({"Agriculture", 0, 0, Trend::STABLE, false, false});
	_economy_dashboard.sectors.push_back({"Passenger Transport", 0, 0, Trend::STABLE, true, false});
	_economy_dashboard.sectors.push_back({"Mail & Goods", 0, 0, Trend::STABLE, false, false});
	_economy_dashboard.sectors.push_back({"Oil & Chemicals", 0, 0, Trend::STABLE, false, true});
}

/** Calculate trend from change percentage */
static Trend CalculateTrend(int32_t change_pct)
{
	if (change_pct <= -10) return Trend::FALLING_FAST;
	if (change_pct < -2) return Trend::FALLING;
	if (change_pct <= 2) return Trend::STABLE;
	if (change_pct < 10) return Trend::RISING;
	return Trend::RISING_FAST;
}

/** Update sector performance */
static void UpdateSectorPerformance()
{
	for (auto &sector : _economy_dashboard.sectors) {
		/* Simulate sector changes - in real implementation would track actual production */
		int32_t random_change = RandomRange(21) - 10; /* -10 to +10 */

		/* Add market health influence */
		int health_modifier = static_cast<int>(_economy_dashboard.market_health) - 3;
		random_change += health_modifier * 2;

		sector.production_change_pct = random_change;
		sector.price_change_pct = random_change / 2;
		sector.trend = CalculateTrend(random_change);
		sector.is_hot = random_change > 5;
		sector.is_risky = std::abs(random_change) > 8;
	}
}

/** Calculate company health snapshot */
static CompanyHealthSnapshot CalculateCompanySnapshot(Company *c)
{
	CompanyHealthSnapshot snapshot;

	snapshot.company = c->index;
	/* snapshot.name = c->name; -- would need proper string handling */
	snapshot.name = "Company";

	/* Financials */
	snapshot.revenue = c->cur_economy.income;
	snapshot.expenses = c->cur_economy.expenses;
	snapshot.profit = snapshot.revenue - snapshot.expenses;

	if (snapshot.revenue > 0) {
		snapshot.profit_margin_pct = static_cast<int32_t>((snapshot.profit * 100) / snapshot.revenue);
	} else {
		snapshot.profit_margin_pct = 0;
	}

	/* Growth - compare to previous year */
	if (c->old_economy[0].income > 0) {
		snapshot.revenue_growth_pct = static_cast<int32_t>(
			((snapshot.revenue - c->old_economy[0].income) * 100) / c->old_economy[0].income
		);
	} else {
		snapshot.revenue_growth_pct = 0;
	}

	/* Fleet size - count vehicles */
	int vehicle_count = 0;
	for (const Vehicle *v : Vehicle::Iterate()) {
		if (v->owner == c->index && v->IsPrimaryVehicle()) vehicle_count++;
	}
	snapshot.fleet_growth_pct = 0; /* Would need historical tracking */

	/* Route count - count stations with cargo */
	int route_count = 0;
	for (const Station *st : Station::Iterate()) {
		if (st->owner == c->index) route_count++;
	}
	snapshot.route_growth_pct = 0;

	/* Financial stability */
	snapshot.cash_on_hand = c->money;
	snapshot.debt = c->current_loan;

	Money equity = c->money - c->current_loan;
	if (equity > 0) {
		snapshot.debt_to_equity = static_cast<float>(c->current_loan) / static_cast<float>(equity);
	} else {
		snapshot.debt_to_equity = 99.9f; /* Effectively infinite */
	}

	/* Runway calculation */
	if (snapshot.profit < 0) {
		snapshot.months_of_runway = static_cast<int32_t>(snapshot.cash_on_hand / (-snapshot.profit / 12));
	} else {
		snapshot.months_of_runway = 999; /* Profitable = infinite runway */
	}

	/* Stock performance */
	Money company_value = CalculateCompanyValue(c);
	snapshot.share_price = company_value / 100;
	snapshot.share_price_change_pct = 0; /* Would need historical tracking */

	/* Count available shares */
	uint8_t shares_owned_by_others = 0;
	for (const Company *other : Company::Iterate()) {
		if (other->index != c->index) {
			shares_owned_by_others += c->share_owners[other->index.IsValid() ? other->index.base() : 0];
		}
	}
	snapshot.shares_available = 100 - shares_owned_by_others;
	snapshot.takeover_vulnerable = snapshot.shares_available > 50;

	/* Overall health score */
	int score = 50; /* Start at middle */

	/* Profitability boost */
	if (snapshot.profit_margin_pct > 20) score += 20;
	else if (snapshot.profit_margin_pct > 10) score += 10;
	else if (snapshot.profit_margin_pct < 0) score -= 20;

	/* Cash position */
	if (snapshot.months_of_runway > 24) score += 10;
	else if (snapshot.months_of_runway < 6) score -= 15;

	/* Debt burden */
	if (snapshot.debt_to_equity < 0.5f) score += 10;
	else if (snapshot.debt_to_equity > 2.0f) score -= 15;

	/* Growth */
	if (snapshot.revenue_growth_pct > 10) score += 10;
	else if (snapshot.revenue_growth_pct < -10) score -= 10;

	snapshot.health_score = static_cast<uint8_t>(Clamp(score, 0, 100));

	/* Health label */
	if (snapshot.health_score >= 80) snapshot.health_label = "Excellent";
	else if (snapshot.health_score >= 60) snapshot.health_label = "Strong";
	else if (snapshot.health_score >= 40) snapshot.health_label = "Stable";
	else if (snapshot.health_score >= 20) snapshot.health_label = "At Risk";
	else snapshot.health_label = "Critical";

	return snapshot;
}

/** Update company health snapshots */
static void UpdateCompanyHealth()
{
	_economy_dashboard.company_health.clear();

	for (Company *c : Company::Iterate()) {
		_economy_dashboard.company_health.push_back(CalculateCompanySnapshot(c));
	}
}

/** Update market indicators */
static void UpdateMarketIndicators()
{
	_economy_dashboard.indicators.clear();

	/* GDP Growth */
	MarketIndicator gdp;
	gdp.name = "GDP Index";
	gdp.description = "Overall economic output relative to baseline";
	gdp.previous_value = _economy_dashboard.gdp_index;

	/* Simulate GDP changes */
	float gdp_change = static_cast<float>(RandomRange(10) - 5) / 10.0f;
	int health_mod = static_cast<int>(_economy_dashboard.market_health) - 3;
	gdp_change += health_mod * 0.5f;

	_economy_dashboard.gdp_index = Clamp(_economy_dashboard.gdp_index + gdp_change, 50.0f, 200.0f);
	gdp.value = _economy_dashboard.gdp_index;
	gdp.trend = CalculateTrend(static_cast<int32_t>((gdp.value - gdp.previous_value) * 10));

	if (gdp.value > 110) gdp.interpretation = "Economy expanding";
	else if (gdp.value < 90) gdp.interpretation = "Economy contracting";
	else gdp.interpretation = "Stable growth";

	_economy_dashboard.indicators.push_back(gdp);

	/* Inflation Rate */
	MarketIndicator inflation;
	inflation.name = "Inflation Rate";
	inflation.description = "Annual price increase percentage";
	inflation.previous_value = _economy_dashboard.inflation_rate;

	float inflation_change = static_cast<float>(RandomRange(10) - 5) / 10.0f;
	_economy_dashboard.inflation_rate = Clamp(_economy_dashboard.inflation_rate + inflation_change, -2.0f, 15.0f);
	inflation.value = _economy_dashboard.inflation_rate;
	inflation.trend = CalculateTrend(static_cast<int32_t>((inflation.value - inflation.previous_value) * 20));

	if (inflation.value > 5) inflation.interpretation = "High inflation - costs rising";
	else if (inflation.value < 0) inflation.interpretation = "Deflation - prices falling";
	else inflation.interpretation = "Healthy inflation";

	_economy_dashboard.indicators.push_back(inflation);

	/* Interest Rate */
	MarketIndicator interest;
	interest.name = "Interest Rate";
	interest.description = "Cost of borrowing money";
	interest.previous_value = _economy_dashboard.interest_rate;

	/* Interest rates respond to inflation */
	if (_economy_dashboard.inflation_rate > 5) {
		_economy_dashboard.interest_rate += 0.25f;
	} else if (_economy_dashboard.inflation_rate < 1) {
		_economy_dashboard.interest_rate -= 0.25f;
	}
	_economy_dashboard.interest_rate = Clamp(_economy_dashboard.interest_rate, 1.0f, 20.0f);

	interest.value = _economy_dashboard.interest_rate;
	interest.trend = CalculateTrend(static_cast<int32_t>((interest.value - interest.previous_value) * 20));

	if (interest.value > 10) interest.interpretation = "Expensive to borrow";
	else if (interest.value < 3) interest.interpretation = "Cheap credit available";
	else interest.interpretation = "Normal borrowing costs";

	_economy_dashboard.indicators.push_back(interest);
}

/** Find investment opportunities */
static void FindInvestmentOpportunities()
{
	_economy_dashboard.opportunities.clear();

	/* Check for takeover opportunities */
	for (const auto &snapshot : _economy_dashboard.company_health) {
		if (snapshot.takeover_vulnerable && snapshot.health_score < 40) {
			InvestmentOpportunity opp;
			opp.type = InvestmentOpportunity::Type::BUY_SHARES;
			opp.title = "Takeover Opportunity: " + snapshot.name;
			opp.description = "Company is vulnerable with low health score";
			opp.expected_return_pct = 50;
			opp.risk_level = 3;
			opp.required_investment = snapshot.share_price * 51;
			opp.target_company = snapshot.company;
			_economy_dashboard.opportunities.push_back(opp);
		}
	}

	/* Check for hot sectors */
	for (const auto &sector : _economy_dashboard.sectors) {
		if (sector.is_hot && !sector.is_risky) {
			InvestmentOpportunity opp;
			opp.type = InvestmentOpportunity::Type::NEW_INDUSTRY;
			opp.title = "Expand into " + sector.name;
			opp.description = "Sector showing strong growth with low risk";
			opp.expected_return_pct = sector.production_change_pct + 10;
			opp.risk_level = 2;
			opp.required_investment = 100000;
			opp.target_company = CompanyID::Invalid();
			_economy_dashboard.opportunities.push_back(opp);
		}
	}

	/* Limit to top 5 */
	if (_economy_dashboard.opportunities.size() > 5) {
		_economy_dashboard.opportunities.resize(5);
	}
}

/** Update economic health assessment */
static void UpdateEconomicHealth()
{
	/* Calculate health based on multiple factors */
	float health_score = 0;

	/* GDP contribution */
	if (_economy_dashboard.gdp_index > 120) health_score += 2;
	else if (_economy_dashboard.gdp_index > 105) health_score += 1;
	else if (_economy_dashboard.gdp_index < 80) health_score -= 2;
	else if (_economy_dashboard.gdp_index < 95) health_score -= 1;

	/* Inflation contribution */
	if (_economy_dashboard.inflation_rate > 8) health_score -= 1;
	else if (_economy_dashboard.inflation_rate < 0) health_score -= 1;

	/* Company health contribution */
	int struggling_companies = 0;
	int thriving_companies = 0;
	for (const auto &snapshot : _economy_dashboard.company_health) {
		if (snapshot.health_score < 30) struggling_companies++;
		if (snapshot.health_score > 70) thriving_companies++;
	}

	if (thriving_companies > struggling_companies * 2) health_score += 1;
	if (struggling_companies > thriving_companies * 2) health_score -= 1;

	/* Convert to enum */
	if (health_score <= -2) _economy_dashboard.market_health = EconomicHealth::DEPRESSION;
	else if (health_score == -1) _economy_dashboard.market_health = EconomicHealth::RECESSION;
	else if (health_score == 0) _economy_dashboard.market_health = EconomicHealth::STABLE;
	else if (health_score == 1) _economy_dashboard.market_health = EconomicHealth::GROWING;
	else if (health_score == 2) _economy_dashboard.market_health = EconomicHealth::BOOMING;
	else _economy_dashboard.market_health = EconomicHealth::BUBBLE;
}

/** Update dashboard */
void UpdateEconomyDashboard()
{
	/* Shift history */
	for (int i = EconomyDashboardState::HISTORY_SIZE - 1; i > 0; i--) {
		_economy_dashboard.gdp_history[i] = _economy_dashboard.gdp_history[i-1];
		_economy_dashboard.inflation_history[i] = _economy_dashboard.inflation_history[i-1];
	}
	_economy_dashboard.gdp_history[0] = _economy_dashboard.gdp_index;
	_economy_dashboard.inflation_history[0] = _economy_dashboard.inflation_rate;

	/* Update all components */
	UpdateSectorPerformance();
	UpdateCompanyHealth();
	UpdateMarketIndicators();
	UpdateEconomicHealth();
	FindInvestmentOpportunities();

	/* Generate forecast */
	_economy_dashboard.forecast = GenerateEconomicForecast();

	/* Check for alerts */
	CheckEconomicAlerts();
}

/** Get current economic health */
EconomicHealth GetEconomicHealth()
{
	return _economy_dashboard.market_health;
}

/** Get health label text */
const char *GetEconomicHealthText(EconomicHealth health)
{
	switch (health) {
		case EconomicHealth::DEPRESSION: return "Depression";
		case EconomicHealth::RECESSION: return "Recession";
		case EconomicHealth::STAGNANT: return "Stagnant";
		case EconomicHealth::STABLE: return "Stable";
		case EconomicHealth::GROWING: return "Growing";
		case EconomicHealth::BOOMING: return "Booming";
		case EconomicHealth::BUBBLE: return "Bubble";
		default: return "Unknown";
	}
}

/** Get trend text */
const char *GetTrendText(Trend trend)
{
	switch (trend) {
		case Trend::FALLING_FAST: return "Falling Fast";
		case Trend::FALLING: return "Falling";
		case Trend::STABLE: return "Stable";
		case Trend::RISING: return "Rising";
		case Trend::RISING_FAST: return "Rising Fast";
		default: return "Unknown";
	}
}

/** Calculate company health score */
uint8_t CalculateCompanyHealthScore(CompanyID company)
{
	Company *c = Company::GetIfValid(company);
	if (c == nullptr) return 0;

	CompanyHealthSnapshot snapshot = CalculateCompanySnapshot(c);
	return snapshot.health_score;
}

/** Get top investment opportunities */
std::vector<InvestmentOpportunity> GetTopOpportunities(int count)
{
	std::vector<InvestmentOpportunity> result;
	int n = std::min(count, static_cast<int>(_economy_dashboard.opportunities.size()));

	for (int i = 0; i < n; i++) {
		result.push_back(_economy_dashboard.opportunities[i]);
	}

	return result;
}

/** Get sector performance */
const SectorPerformance *GetSectorPerformance(const std::string &sector_name)
{
	for (const auto &sector : _economy_dashboard.sectors) {
		if (sector.name == sector_name) return &sector;
	}
	return nullptr;
}

/** Get company health snapshot */
const CompanyHealthSnapshot *GetCompanyHealth(CompanyID company)
{
	for (const auto &snapshot : _economy_dashboard.company_health) {
		if (snapshot.company == company) return &snapshot;
	}
	return nullptr;
}

/** Generate economic forecast */
EconomicForecast GenerateEconomicForecast()
{
	EconomicForecast forecast;
	forecast.current_health = _economy_dashboard.market_health;

	/* Simple trend-based prediction */
	float gdp_trend = _economy_dashboard.gdp_history[0] - _economy_dashboard.gdp_history[2];

	if (gdp_trend > 5) {
		if (forecast.current_health == EconomicHealth::BOOMING) {
			forecast.predicted_health = EconomicHealth::BUBBLE;
			forecast.summary = "Economy may be overheating - watch for correction";
		} else {
			forecast.predicted_health = EconomicHealth::BOOMING;
			forecast.summary = "Strong growth expected to continue";
		}
		forecast.confidence_pct = 70;
	} else if (gdp_trend < -5) {
		if (forecast.current_health == EconomicHealth::RECESSION) {
			forecast.predicted_health = EconomicHealth::DEPRESSION;
			forecast.summary = "Downturn deepening - consider defensive positions";
		} else {
			forecast.predicted_health = EconomicHealth::RECESSION;
			forecast.summary = "Economic slowdown expected";
		}
		forecast.confidence_pct = 65;
	} else {
		forecast.predicted_health = EconomicHealth::STABLE;
		forecast.summary = "Economy expected to remain stable";
		forecast.confidence_pct = 50;
	}

	/* Add factors */
	if (_economy_dashboard.inflation_rate > 5) {
		forecast.factors.push_back("High inflation pressuring margins");
	}
	if (_economy_dashboard.interest_rate > 10) {
		forecast.factors.push_back("High interest rates limiting expansion");
	}

	int hot_sectors = 0;
	for (const auto &sector : _economy_dashboard.sectors) {
		if (sector.is_hot) hot_sectors++;
	}
	if (hot_sectors >= 3) {
		forecast.factors.push_back("Multiple sectors showing strong growth");
	}

	return forecast;
}

/** Get dashboard widget data */
DashboardWidgetData GetDashboardWidgetData()
{
	DashboardWidgetData data;

	/* Market status */
	switch (_economy_dashboard.market_health) {
		case EconomicHealth::DEPRESSION:
		case EconomicHealth::RECESSION:
			data.market_status = "Bear Market";
			data.status_color = {255, 100, 100, 255};
			break;
		case EconomicHealth::BOOMING:
		case EconomicHealth::BUBBLE:
			data.market_status = "Bull Market";
			data.status_color = {100, 255, 100, 255};
			break;
		default:
			data.market_status = "Stable Market";
			data.status_color = {200, 200, 200, 255};
			break;
	}

	/* Top opportunity */
	if (!_economy_dashboard.opportunities.empty()) {
		data.top_opportunity = _economy_dashboard.opportunities[0].title;
	} else {
		data.top_opportunity = "No major opportunities detected";
	}

	/* Biggest risk */
	int at_risk_companies = 0;
	for (const auto &snapshot : _economy_dashboard.company_health) {
		if (snapshot.health_score < 30) at_risk_companies++;
	}
	if (at_risk_companies > 0) {
		data.biggest_risk = std::to_string(at_risk_companies) + " companies at risk of bankruptcy";
	} else if (_economy_dashboard.inflation_rate > 8) {
		data.biggest_risk = "High inflation eroding profits";
	} else {
		data.biggest_risk = "No major risks detected";
	}

	/* Key metrics */
	data.market_change_pct = static_cast<int32_t>(
		(_economy_dashboard.gdp_history[0] - _economy_dashboard.gdp_history[1]) * 10
	);

	/* Player metrics would need actual player company tracking */
	data.player_profit_change_pct = 0;
	data.industry_output_change_pct = 0;

	/* Alerts */
	if (_economy_dashboard.market_health == EconomicHealth::RECESSION ||
	    _economy_dashboard.market_health == EconomicHealth::DEPRESSION) {
		data.alerts.push_back("Economic downturn in progress");
	}
	if (_economy_dashboard.inflation_rate > 8) {
		data.alerts.push_back("High inflation warning");
	}
	for (const auto &snapshot : _economy_dashboard.company_health) {
		if (snapshot.takeover_vulnerable) {
			data.alerts.push_back(snapshot.name + " is vulnerable to takeover");
		}
	}

	return data;
}

/** Check for economic alerts */
void CheckEconomicAlerts()
{
	/* Check for market crash */
	if (_economy_dashboard.gdp_history[0] - _economy_dashboard.gdp_history[1] < -10) {
		for (auto callback : _economic_event_callbacks) {
			callback(EconomicEventType::MARKET_CRASH, nullptr);
		}
	}

	/* Check for market recovery */
	if (_economy_dashboard.market_health == EconomicHealth::GROWING &&
	    _economy_dashboard.gdp_history[1] < 90) {
		for (auto callback : _economic_event_callbacks) {
			callback(EconomicEventType::MARKET_RECOVERY, nullptr);
		}
	}
}

/** Subscribe to economic events */
void SubscribeEconomicEvents(EconomicEventCallback callback)
{
	_economic_event_callbacks.push_back(callback);
}
