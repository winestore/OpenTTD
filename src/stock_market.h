/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file stock_market.h Stock market system for Transport Empire. */

#ifndef STOCK_MARKET_H
#define STOCK_MARKET_H

#include "company_type.h"
#include "economy_type.h"
#include "core/bitmath_func.hpp"

/** Total shares per company (100 = 100% ownership) */
static constexpr uint8_t TOTAL_SHARES = 100;

/** Minimum shares to gain controlling interest */
static constexpr uint8_t CONTROLLING_INTEREST = 51;

/** Minimum shares to force buyout of remaining shareholders */
static constexpr uint8_t FORCED_BUYOUT_THRESHOLD = 75;

/** Broker fee percentage (5%) */
static constexpr uint8_t BROKER_FEE_PERCENT = 5;

/** Maximum shares that can be issued at once */
static constexpr uint8_t MAX_SHARE_ISSUANCE = 25;

/** Number of months to track share price history */
static constexpr uint8_t SHARE_PRICE_HISTORY_SIZE = 12;

/** Market events that can affect share prices */
enum class MarketEvent : uint8_t {
	NONE,
	SCANDAL,           ///< -20% price, -30 sentiment
	RECORD_PROFITS,    ///< +15% price, +20 sentiment
	CEO_DEPARTURE,     ///< -10% price, -15 sentiment
	EXPANSION_NEWS,    ///< +10% price, +15 sentiment
	ACCIDENT,          ///< -15% price, -25 sentiment
	SUBSIDY_AWARDED,   ///< +8% price, +10 sentiment
	STRIKE,            ///< -12% price, -20 sentiment
	MERGER_RUMOR,      ///< +5% price, +30 sentiment (high volatility)
	INSIDER_TRADING,   ///< Investigation triggered
	MARKET_CRASH,      ///< All stocks -15%
	BULL_MARKET,       ///< All stocks +10%
};

/** Structure tracking share ownership for a company */
struct CompanyShares {
	std::array<uint8_t, MAX_COMPANIES> shares_owned{}; ///< Shares owned by each company (index = owner company ID)
	uint8_t shares_available = 0;                       ///< Shares available on the market
	uint8_t shares_founder = TOTAL_SHARES;              ///< Shares held by founding owner (NPC for AI companies)

	Money share_price = 0;                              ///< Current market price per share
	std::array<Money, SHARE_PRICE_HISTORY_SIZE> price_history{}; ///< Monthly price history
	uint8_t price_history_index = 0;                    ///< Current index in price history

	int8_t market_sentiment = 0;                        ///< -100 to +100, affects price volatility
	uint8_t volatility = 10;                            ///< Base volatility (0-100)

	MarketEvent last_event = MarketEvent::NONE;         ///< Most recent market event

	/** Get total shares owned by other companies */
	uint8_t GetSharesOwnedByOthers() const {
		uint8_t total = 0;
		for (auto shares : this->shares_owned) {
			total += shares;
		}
		return total;
	}

	/** Check if a company has controlling interest */
	bool HasControllingInterest(CompanyID owner) const {
		return this->shares_owned[owner] >= CONTROLLING_INTEREST;
	}

	/** Check if a company can force buyout */
	bool CanForceBuyout(CompanyID owner) const {
		return this->shares_owned[owner] >= FORCED_BUYOUT_THRESHOLD;
	}

	/** Get ownership percentage for a company */
	uint8_t GetOwnershipPercent(CompanyID owner) const {
		return this->shares_owned[owner];
	}
};

/** Company reputation affecting stock prices and public relations */
struct CompanyReputation {
	int8_t public_opinion = 0;       ///< -100 to +100, affects customers
	int8_t investor_confidence = 50; ///< -100 to +100, affects share price
	int8_t regulatory_standing = 50; ///< -100 to +100, affects investigation chance
	uint8_t investigation_heat = 0;  ///< 0-100, chance of investigation per month

	uint8_t months_under_investigation = 0; ///< Months currently under investigation
	Money pending_fines = 0;                ///< Fines that will be applied

	/** Clamp all values to valid ranges */
	void ClampValues() {
		this->public_opinion = std::clamp<int8_t>(this->public_opinion, -100, 100);
		this->investor_confidence = std::clamp<int8_t>(this->investor_confidence, -100, 100);
		this->regulatory_standing = std::clamp<int8_t>(this->regulatory_standing, -100, 100);
		this->investigation_heat = std::min<uint8_t>(this->investigation_heat, 100);
	}
};

/* Function declarations */

/**
 * Calculate the current share price for a company.
 * @param c The company to calculate share price for.
 * @return The calculated share price.
 */
Money CalculateSharePrice(const struct Company *c);

/**
 * Execute a share purchase.
 * @param buyer The company buying shares.
 * @param target The company whose shares are being bought.
 * @param quantity Number of shares to buy.
 * @return True if purchase was successful.
 */
bool BuyShares(CompanyID buyer, CompanyID target, uint8_t quantity);

/**
 * Execute a share sale.
 * @param seller The company selling shares.
 * @param target The company whose shares are being sold.
 * @param quantity Number of shares to sell.
 * @return True if sale was successful.
 */
bool SellShares(CompanyID seller, CompanyID target, uint8_t quantity);

/**
 * Issue new shares for a company (dilutes ownership, raises capital).
 * @param company The company issuing shares.
 * @param quantity Number of new shares to issue.
 * @return True if issuance was successful.
 */
bool IssueShares(CompanyID company, uint8_t quantity);

/**
 * Process a market event for a company.
 * @param company The affected company.
 * @param event The market event.
 */
void ProcessMarketEvent(CompanyID company, MarketEvent event);

/**
 * Update share prices for all companies (called monthly).
 */
void UpdateSharePrices();

/**
 * Check for random market events (called monthly).
 */
void CheckMarketEvents();

/**
 * Initialize stock market for a new company.
 * @param company The new company.
 */
void InitializeCompanyShares(CompanyID company);

/**
 * Handle company bankruptcy in stock market.
 * @param company The bankrupt company.
 */
void HandleStockMarketBankruptcy(CompanyID company);

/**
 * Get the cost to buy shares including broker fee.
 * @param share_price Price per share.
 * @param quantity Number of shares.
 * @return Total cost including fees.
 */
inline Money GetShareBuyCost(Money share_price, uint8_t quantity) {
	Money base_cost = share_price * quantity;
	return base_cost + (base_cost * BROKER_FEE_PERCENT / 100);
}

/**
 * Get the proceeds from selling shares after broker fee.
 * @param share_price Price per share.
 * @param quantity Number of shares.
 * @return Net proceeds after fees.
 */
inline Money GetShareSellProceeds(Money share_price, uint8_t quantity) {
	Money base_value = share_price * quantity;
	return base_value - (base_value * BROKER_FEE_PERCENT / 100);
}

#endif /* STOCK_MARKET_H */
