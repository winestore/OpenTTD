/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file stock_cmd.h Command definitions for stock market operations. */

#ifndef STOCK_CMD_H
#define STOCK_CMD_H

#include "command_type.h"
#include "company_type.h"

CommandCost CmdBuyShares(DoCommandFlags flags, CompanyID target_company, uint8_t quantity);
CommandCost CmdSellShares(DoCommandFlags flags, CompanyID target_company, uint8_t quantity);
CommandCost CmdIssueShares(DoCommandFlags flags, uint8_t quantity);

DEF_CMD_TRAIT(Commands::BuyShares, CmdBuyShares, {}, CommandType::MoneyManagement)
DEF_CMD_TRAIT(Commands::SellShares, CmdSellShares, {}, CommandType::MoneyManagement)
DEF_CMD_TRAIT(Commands::IssueShares, CmdIssueShares, {}, CommandType::MoneyManagement)

#endif /* STOCK_CMD_H */
