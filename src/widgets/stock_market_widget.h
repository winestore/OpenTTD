/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file stock_market_widget.h Types related to the stock market widgets. */

#ifndef WIDGETS_STOCK_MARKET_WIDGET_H
#define WIDGETS_STOCK_MARKET_WIDGET_H

/** Widgets of the #StockMarketWindow class. */
enum StockMarketWidgets : WidgetID {
	WID_STM_CAPTION,          ///< Caption of the window.
	WID_STM_PANEL,            ///< Main panel listing all companies.
	WID_STM_SCROLLBAR,        ///< Scrollbar for the panel.
	WID_STM_BUY_BUTTON,       ///< Buy shares button.
	WID_STM_SELL_BUTTON,      ///< Sell shares button.
	WID_STM_COMPANY_INFO,     ///< View company details button.
	WID_STM_HISTORY_BUTTON,   ///< View price history button.
};

/** Widgets of the #StockTradeWindow class (buy/sell dialog). */
enum StockTradeWidgets : WidgetID {
	WID_ST_CAPTION,          ///< Caption of the window.
	WID_ST_COMPANY_NAME,     ///< Name of the company.
	WID_ST_SHARE_PRICE,      ///< Current share price.
	WID_ST_SHARES_AVAILABLE, ///< Shares available for purchase.
	WID_ST_YOUR_SHARES,      ///< Shares you currently own.
	WID_ST_QUANTITY_LABEL,   ///< Label for quantity selector.
	WID_ST_QUANTITY_DOWN,    ///< Decrease quantity.
	WID_ST_QUANTITY_TEXT,    ///< Quantity display.
	WID_ST_QUANTITY_UP,      ///< Increase quantity.
	WID_ST_TOTAL_COST,       ///< Total cost/proceeds.
	WID_ST_EXECUTE,          ///< Execute trade button.
	WID_ST_CANCEL,           ///< Cancel button.
};

/** Widgets of the #StockPortfolioWindow class. */
enum StockPortfolioWidgets : WidgetID {
	WID_SP_CAPTION,          ///< Caption of the window.
	WID_SP_TOTAL_VALUE,      ///< Total portfolio value.
	WID_SP_CHANGE,           ///< Today's change.
	WID_SP_PANEL,            ///< Holdings panel.
	WID_SP_SCROLLBAR,        ///< Scrollbar.
};

/** Widgets of the #StockHistoryWindow class. */
enum StockHistoryWidgets : WidgetID {
	WID_SH_CAPTION,          ///< Caption of the window.
	WID_SH_GRAPH,            ///< Price history graph.
	WID_SH_KEY,              ///< Graph key/legend.
};

#endif /* WIDGETS_STOCK_MARKET_WIDGET_H */
