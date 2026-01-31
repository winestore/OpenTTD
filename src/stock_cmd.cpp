/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file stock_cmd.cpp Commands related to stock market operations. */

#include "stdafx.h"
#include "stock_market.h"
#include "stock_cmd.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "news_func.h"

#include "safeguards.h"

/**
 * Buy shares of another company.
 * @param flags Command flags.
 * @param target_company Company to buy shares of.
 * @param quantity Number of shares to buy.
 * @return Cost of the operation or error.
 */
CommandCost CmdBuyShares(DoCommandFlags flags, CompanyID target_company, uint8_t quantity)
{
	Company *c = Company::GetIfValid(_current_company);
	Company *target = Company::GetIfValid(target_company);

	if (c == nullptr) return CMD_ERROR;
	if (target == nullptr) return CMD_ERROR;
	if (_current_company == target_company) return CMD_ERROR; /* Can't buy your own shares */
	if (quantity == 0 || quantity > 100) return CMD_ERROR;

	CompanyShares &shares = target->shares;

	/* Check if enough shares are available */
	if (quantity > shares.shares_available) {
		return CommandCost(STR_ERROR_NOT_ENOUGH_SHARES_AVAILABLE);
	}

	/* Calculate cost with broker fee */
	Money cost = GetShareBuyCost(shares.share_price, quantity);

	if (flags.Test(DoCommandFlag::Execute)) {
		/* Execute the purchase */
		shares.shares_owned[_current_company.base()] += quantity;
		shares.shares_available -= quantity;

		/* Update sentiment */
		shares.market_sentiment = std::min<int8_t>(shares.market_sentiment + quantity / 5, 100);

		/* Check for controlling interest */
		if (shares.shares_owned[_current_company.base()] >= CONTROLLING_INTEREST) {
			/* TODO: Add news about takeover */
			/* AddNewsItem(...) */
		}

		/* Invalidate windows */
		InvalidateWindowData(WC_FINANCES, target_company);
		InvalidateWindowClassesData(WC_FINANCES);
	}

	return CommandCost(EXPENSES_OTHER, cost);
}

/**
 * Sell shares of a company.
 * @param flags Command flags.
 * @param target_company Company whose shares to sell.
 * @param quantity Number of shares to sell.
 * @return Proceeds from the sale.
 */
CommandCost CmdSellShares(DoCommandFlags flags, CompanyID target_company, uint8_t quantity)
{
	Company *c = Company::GetIfValid(_current_company);
	Company *target = Company::GetIfValid(target_company);

	if (c == nullptr) return CMD_ERROR;
	if (target == nullptr) return CMD_ERROR;
	if (quantity == 0 || quantity > 100) return CMD_ERROR;

	CompanyShares &shares = target->shares;

	/* Check if we own enough shares */
	if (quantity > shares.shares_owned[_current_company.base()]) {
		return CommandCost(STR_ERROR_NOT_ENOUGH_SHARES_OWNED);
	}

	/* Calculate proceeds with broker fee */
	Money proceeds = GetShareSellProceeds(shares.share_price, quantity);

	if (flags.Test(DoCommandFlag::Execute)) {
		/* Execute the sale */
		shares.shares_owned[_current_company.base()] -= quantity;
		shares.shares_available += quantity;

		/* Update sentiment */
		shares.market_sentiment = std::max<int8_t>(shares.market_sentiment - quantity / 5, -100);

		/* Give money to seller */
		c->money += proceeds;

		/* Invalidate windows */
		InvalidateWindowData(WC_FINANCES, target_company);
		InvalidateWindowClassesData(WC_FINANCES);
	}

	return CommandCost(EXPENSES_OTHER, -proceeds); /* Negative cost = income */
}

/**
 * Issue new shares to raise capital.
 * @param flags Command flags.
 * @param quantity Number of shares to issue.
 * @return Success or error.
 */
CommandCost CmdIssueShares(DoCommandFlags flags, uint8_t quantity)
{
	Company *c = Company::GetIfValid(_current_company);

	if (c == nullptr) return CMD_ERROR;
	if (quantity == 0 || quantity > MAX_SHARE_ISSUANCE) return CMD_ERROR;

	CompanyShares &shares = c->shares;

	/* Check if we have room to issue more shares */
	uint8_t total_outstanding = shares.GetSharesOwnedByOthers() + shares.shares_available;
	if (total_outstanding + quantity > TOTAL_SHARES) {
		return CMD_ERROR;
	}

	if (flags.Test(DoCommandFlag::Execute)) {
		/* Calculate capital raised (90% of market value) */
		Money capital_raised = shares.share_price * quantity * 90 / 100;

		/* Add shares to market */
		shares.shares_available += quantity;

		/* Company receives capital */
		c->money += capital_raised;

		/* Dilution causes sentiment drop */
		shares.market_sentiment = std::max<int8_t>(shares.market_sentiment - quantity / 2, -100);

		/* Invalidate windows */
		InvalidateWindowClassesData(WC_FINANCES);
	}

	return CommandCost();
}
