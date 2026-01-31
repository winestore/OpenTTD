/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file stock_market.cpp Implementation of the stock market system. */

#include "stdafx.h"
#include "stock_market.h"
#include "company_base.h"
#include "company_func.h"
#include "economy_func.h"
#include "news_func.h"
#include "strings_func.h"
#include "core/random_func.hpp"

#include "safeguards.h"

/**
 * Calculate performance modifier based on recent quarterly performance.
 * @param c The company to evaluate.
 * @return Modifier from -20 to +30 percent.
 */
static int CalculatePerformanceModifier(const Company *c)
{
	if (c->num_valid_stat_ent == 0) return 0;

	/* Calculate average income over available quarters */
	Money total_income = 0;
	Money total_expenses = 0;
	uint8_t quarters = std::min<uint8_t>(c->num_valid_stat_ent, 4);

	for (uint8_t i = 0; i < quarters; i++) {
		total_income += c->old_economy[i].income;
		total_expenses += c->old_economy[i].expenses;
	}

	Money avg_profit = (total_income - total_expenses) / quarters;

	/* Scale profit to modifier (-20% to +30%) */
	/* Companies making >$1M/quarter get max bonus, losing >$500k get max penalty */
	if (avg_profit > 1000000) return 30;
	if (avg_profit > 500000) return 20;
	if (avg_profit > 100000) return 10;
	if (avg_profit > 0) return 5;
	if (avg_profit > -100000) return 0;
	if (avg_profit > -500000) return -10;
	return -20;
}

/**
 * Calculate the current share price for a company.
 * Based on company value, performance, sentiment, and volatility.
 */
Money CalculateSharePrice(const Company *c)
{
	if (c == nullptr) return 0;

	/* Base value is company value divided by total shares */
	Money company_value = CalculateCompanyValue(c, false);
	Money base_value = company_value / TOTAL_SHARES;

	/* Minimum base value */
	if (base_value < 100) base_value = 100;

	/* Performance modifier from recent quarters */
	int perf_mod = CalculatePerformanceModifier(c);

	/* Market sentiment modifier */
	int sentiment_mod = c->shares.market_sentiment / 7;  // -14 to +14

	/* Random volatility based on company's volatility rating */
	int volatility_range = c->shares.volatility / 10;  // 0 to 10
	int volatility = RandomRange(volatility_range * 2 + 1) - volatility_range;

	/* Calculate final price */
	int total_modifier = 100 + perf_mod + sentiment_mod + volatility;
	total_modifier = std::max(20, total_modifier);  // Floor at 20% of base

	Money price = base_value * total_modifier / 100;

	/* Ensure minimum price of $1 */
	return std::max(price, Money(1));
}

/**
 * Execute a share purchase.
 */
bool BuyShares(CompanyID buyer, CompanyID target, uint8_t quantity)
{
	Company *buyer_company = Company::GetIfValid(buyer);
	Company *target_company = Company::GetIfValid(target);

	if (buyer_company == nullptr || target_company == nullptr) return false;
	if (buyer == target) return false;  // Can't buy your own shares through market
	if (quantity == 0) return false;

	CompanyShares &shares = target_company->shares;

	/* Check if enough shares are available */
	if (quantity > shares.shares_available) return false;

	/* Calculate cost */
	Money cost = GetShareBuyCost(shares.share_price, quantity);

	/* Check if buyer can afford it */
	if (buyer_company->money < cost) return false;

	/* Execute the transaction */
	buyer_company->money -= cost;
	shares.shares_owned[buyer] += quantity;
	shares.shares_available -= quantity;

	/* The money goes to... the market (simulating various sellers) */
	/* In a more complex system, this could track individual shareholders */

	/* Update sentiment (buying pressure = positive) */
	shares.market_sentiment = std::min<int8_t>(shares.market_sentiment + quantity / 5, 100);

	/* Check for controlling interest */
	if (shares.HasControllingInterest(buyer)) {
		/* TODO: Trigger controlling interest news/event */
	}

	return true;
}

/**
 * Execute a share sale.
 */
bool SellShares(CompanyID seller, CompanyID target, uint8_t quantity)
{
	Company *seller_company = Company::GetIfValid(seller);
	Company *target_company = Company::GetIfValid(target);

	if (seller_company == nullptr || target_company == nullptr) return false;
	if (quantity == 0) return false;

	CompanyShares &shares = target_company->shares;

	/* Check if seller owns enough shares */
	if (quantity > shares.shares_owned[seller]) return false;

	/* Calculate proceeds */
	Money proceeds = GetShareSellProceeds(shares.share_price, quantity);

	/* Execute the transaction */
	seller_company->money += proceeds;
	shares.shares_owned[seller] -= quantity;
	shares.shares_available += quantity;

	/* Update sentiment (selling pressure = negative) */
	shares.market_sentiment = std::max<int8_t>(shares.market_sentiment - quantity / 5, -100);

	return true;
}

/**
 * Issue new shares for a company.
 */
bool IssueShares(CompanyID company, uint8_t quantity)
{
	Company *c = Company::GetIfValid(company);
	if (c == nullptr) return false;
	if (quantity == 0 || quantity > MAX_SHARE_ISSUANCE) return false;

	CompanyShares &shares = c->shares;

	/* Calculate capital raised (90% of market value - 10% issuance cost) */
	Money capital_raised = shares.share_price * quantity * 90 / 100;

	/* Add shares to market */
	shares.shares_available += quantity;

	/* Dilute founder shares proportionally */
	/* This is simplified - in reality would dilute all shareholders */
	if (shares.shares_founder >= quantity) {
		shares.shares_founder -= quantity;
	}

	/* Company receives the capital */
	c->money += capital_raised;

	/* Issuing shares usually causes price drop due to dilution */
	shares.market_sentiment -= quantity / 2;
	shares.market_sentiment = std::max<int8_t>(shares.market_sentiment, -100);

	return true;
}

/**
 * Process a market event for a company.
 */
void ProcessMarketEvent(CompanyID company, MarketEvent event)
{
	Company *c = Company::GetIfValid(company);
	if (c == nullptr) return;

	CompanyShares &shares = c->shares;
	CompanyReputation &rep = c->reputation;

	int price_change = 0;     // Percentage
	int sentiment_change = 0;
	int reputation_change = 0;

	switch (event) {
		case MarketEvent::SCANDAL:
			price_change = -20;
			sentiment_change = -30;
			reputation_change = -25;
			rep.investigation_heat += 20;
			break;

		case MarketEvent::RECORD_PROFITS:
			price_change = 15;
			sentiment_change = 20;
			reputation_change = 10;
			break;

		case MarketEvent::CEO_DEPARTURE:
			price_change = -10;
			sentiment_change = -15;
			shares.volatility += 10;
			break;

		case MarketEvent::EXPANSION_NEWS:
			price_change = 10;
			sentiment_change = 15;
			reputation_change = 5;
			break;

		case MarketEvent::ACCIDENT:
			price_change = -15;
			sentiment_change = -25;
			reputation_change = -20;
			rep.investigation_heat += 10;
			break;

		case MarketEvent::SUBSIDY_AWARDED:
			price_change = 8;
			sentiment_change = 10;
			reputation_change = 5;
			break;

		case MarketEvent::STRIKE:
			price_change = -12;
			sentiment_change = -20;
			reputation_change = -10;
			break;

		case MarketEvent::MERGER_RUMOR:
			price_change = 5;
			sentiment_change = 30;
			shares.volatility += 20;  // High volatility during merger rumors
			break;

		case MarketEvent::INSIDER_TRADING:
			reputation_change = -30;
			rep.investigation_heat += 40;
			rep.months_under_investigation = 6;
			rep.pending_fines = c->money / 10;  // 10% of cash as fine
			break;

		default:
			break;
	}

	/* Apply price change */
	if (price_change != 0) {
		shares.share_price = shares.share_price * (100 + price_change) / 100;
		shares.share_price = std::max(shares.share_price, Money(1));
	}

	/* Apply sentiment change */
	shares.market_sentiment += sentiment_change;
	shares.market_sentiment = std::clamp<int8_t>(shares.market_sentiment, -100, 100);

	/* Apply reputation change */
	rep.public_opinion += reputation_change;
	rep.investor_confidence += sentiment_change / 2;
	rep.ClampValues();

	/* Record the event */
	shares.last_event = event;

	/* TODO: Add news message for significant events */
}

/**
 * Update share prices for all companies.
 * Called monthly.
 */
void UpdateSharePrices()
{
	for (Company *c : Company::Iterate()) {
		CompanyShares &shares = c->shares;

		/* Store current price in history */
		shares.price_history[shares.price_history_index] = shares.share_price;
		shares.price_history_index = (shares.price_history_index + 1) % SHARE_PRICE_HISTORY_SIZE;

		/* Calculate new price */
		shares.share_price = CalculateSharePrice(c);

		/* Decay sentiment toward neutral */
		if (shares.market_sentiment > 0) {
			shares.market_sentiment = std::max<int8_t>(0, shares.market_sentiment - 5);
		} else if (shares.market_sentiment < 0) {
			shares.market_sentiment = std::min<int8_t>(0, shares.market_sentiment + 5);
		}

		/* Decay volatility toward base level */
		if (shares.volatility > 10) {
			shares.volatility = std::max<uint8_t>(10, shares.volatility - 2);
		}
	}
}

/**
 * Check for random market events.
 * Called monthly.
 */
void CheckMarketEvents()
{
	/* Small chance of market-wide events */
	if (RandomRange(100) < 3) {
		/* Market crash - affects all companies */
		for (Company *c : Company::Iterate()) {
			c->shares.share_price = c->shares.share_price * 85 / 100;
			c->shares.market_sentiment -= 20;
		}
		/* TODO: Add news about market crash */
	} else if (RandomRange(100) < 5) {
		/* Bull market - benefits all companies */
		for (Company *c : Company::Iterate()) {
			c->shares.share_price = c->shares.share_price * 110 / 100;
			c->shares.market_sentiment += 10;
		}
		/* TODO: Add news about bull market */
	}

	/* Company-specific events */
	for (Company *c : Company::Iterate()) {
		uint chance = RandomRange(100);

		if (chance < 2) {
			ProcessMarketEvent(c->index, MarketEvent::SCANDAL);
		} else if (chance < 5) {
			ProcessMarketEvent(c->index, MarketEvent::EXPANSION_NEWS);
		} else if (chance < 8) {
			/* Check if company had record profits */
			if (c->cur_economy.income > c->old_economy[0].income * 120 / 100) {
				ProcessMarketEvent(c->index, MarketEvent::RECORD_PROFITS);
			}
		}

		/* Check investigation heat */
		if (c->reputation.investigation_heat > 0 && RandomRange(100) < c->reputation.investigation_heat) {
			ProcessMarketEvent(c->index, MarketEvent::INSIDER_TRADING);
		}

		/* Decay investigation heat */
		if (c->reputation.investigation_heat > 0) {
			c->reputation.investigation_heat = std::max<uint8_t>(0, c->reputation.investigation_heat - 2);
		}

		/* Process ongoing investigations */
		if (c->reputation.months_under_investigation > 0) {
			c->reputation.months_under_investigation--;
			if (c->reputation.months_under_investigation == 0 && c->reputation.pending_fines > 0) {
				/* Apply fines */
				c->money -= c->reputation.pending_fines;
				c->reputation.pending_fines = 0;
				/* TODO: News about fine */
			}
		}
	}
}

/**
 * Initialize stock market for a new company.
 */
void InitializeCompanyShares(CompanyID company)
{
	Company *c = Company::GetIfValid(company);
	if (c == nullptr) return;

	CompanyShares &shares = c->shares;

	/* Reset all ownership */
	shares.shares_owned.fill(0);
	shares.shares_available = 0;
	shares.shares_founder = TOTAL_SHARES;

	/* Calculate initial share price */
	shares.share_price = CalculateSharePrice(c);
	if (shares.share_price == 0) shares.share_price = 1000;  // Default for new companies

	/* Initialize history with current price */
	shares.price_history.fill(shares.share_price);
	shares.price_history_index = 0;

	/* Neutral sentiment and base volatility */
	shares.market_sentiment = 0;
	shares.volatility = 10;
	shares.last_event = MarketEvent::NONE;

	/* Initialize reputation */
	c->reputation = CompanyReputation{};
}

/**
 * Handle company bankruptcy in stock market.
 */
void HandleStockMarketBankruptcy(CompanyID company)
{
	Company *c = Company::GetIfValid(company);
	if (c == nullptr) return;

	/* Shares become worthless */
	c->shares.share_price = 0;
	c->shares.market_sentiment = -100;

	/* Compensate shareholders with whatever liquidation value exists */
	Money liquidation_per_share = c->bankrupt_value / TOTAL_SHARES;

	for (CompanyID owner = CompanyID::Begin(); owner < MAX_COMPANIES; ++owner) {
		uint8_t owned = c->shares.shares_owned[owner];
		if (owned > 0) {
			Company *shareholder = Company::GetIfValid(owner);
			if (shareholder != nullptr) {
				shareholder->money += liquidation_per_share * owned;
			}
			c->shares.shares_owned[owner] = 0;
		}
	}
}
