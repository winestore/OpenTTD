/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file stock_gui.cpp GUI for the stock market system. */

#include "stdafx.h"
#include "stock_market.h"
#include "company_base.h"
#include "company_func.h"
#include "company_gui.h"
#include "window_gui.h"
#include "window_func.h"
#include "strings_func.h"
#include "string_func.h"
#include "gfx_func.h"
#include "currency.h"
#include "core/geometry_func.hpp"
#include "sortlist_type.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "3rdparty/fmt/format.h"

#include "widgets/stock_market_widget.h"

#include "table/strings.h"
#include "table/sprites.h"

#include "safeguards.h"

/* Forward declarations */
void ShowStockTradeWindow(CompanyID company, bool buying);

/** Colours for different price changes */
static const uint8_t STOCK_COLOUR_UP = TC_GREEN;
static const uint8_t STOCK_COLOUR_DOWN = TC_RED;
static const uint8_t STOCK_COLOUR_NEUTRAL = TC_BLACK;

/**
 * Get the text colour for a price change.
 * @param change The price change amount.
 * @return Text colour to use.
 */
static uint8_t GetPriceChangeColour(Money change)
{
	if (change > 0) return STOCK_COLOUR_UP;
	if (change < 0) return STOCK_COLOUR_DOWN;
	return STOCK_COLOUR_NEUTRAL;
}

/**
 * Format a price change as a percentage string.
 * @param old_price Previous price.
 * @param new_price Current price.
 * @return Formatted string like "+5.2%" or "-3.1%".
 */
static std::string FormatPriceChange(Money old_price, Money new_price)
{
	if (old_price == 0) return "N/A";

	int64_t change_percent = ((new_price - old_price) * 1000) / old_price;  // x10 for one decimal

	if (change_percent >= 0) {
		return fmt::format("+{}.{}%", change_percent / 10, change_percent % 10);
	} else {
		change_percent = -change_percent;
		return fmt::format("-{}.{}%", change_percent / 10, change_percent % 10);
	}
}

/** Stock market list entry */
struct StockListEntry {
	CompanyID company;
	Money share_price;
	Money price_change;
	uint8_t shares_owned;
	uint8_t shares_available;
};

/** Main stock market window showing all companies */
struct StockMarketWindow : Window {
	Scrollbar *vscroll = nullptr;
	std::vector<StockListEntry> companies;
	int selected_index = -1;

	StockMarketWindow(WindowDesc &desc, WindowNumber window_number) : Window(desc)
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_STM_SCROLLBAR);
		this->FinishInitNested(window_number);
		this->BuildCompanyList();
	}

	void BuildCompanyList()
	{
		this->companies.clear();

		CompanyID my_company = _local_company;

		for (const Company *c : Company::Iterate()) {
			StockListEntry entry;
			entry.company = c->index;
			entry.share_price = c->shares.share_price;

			/* Calculate price change from last month */
			uint8_t prev_idx = (c->shares.price_history_index + SHARE_PRICE_HISTORY_SIZE - 1) % SHARE_PRICE_HISTORY_SIZE;
			Money prev_price = c->shares.price_history[prev_idx];
			entry.price_change = entry.share_price - prev_price;

			/* Get shares owned by current player */
			if (Company::IsValidID(my_company)) {
				entry.shares_owned = c->shares.shares_owned[my_company.base()];
			} else {
				entry.shares_owned = 0;
			}
			entry.shares_available = c->shares.shares_available;

			this->companies.push_back(entry);
		}

		this->vscroll->SetCount((int)this->companies.size());
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		switch (widget) {
			case WID_STM_PANEL:
				resize.height = GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.matrix.Vertical();
				size.height = 10 * resize.height;  /* Show 10 companies by default */
				break;
		}
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		if (widget != WID_STM_PANEL) return;

		Rect ir = r.Shrink(WidgetDimensions::scaled.framerect);
		int line_height = GetCharacterHeight(FS_NORMAL) + WidgetDimensions::scaled.matrix.Vertical();

		/* Draw header */
		int y = ir.top;
		DrawString(ir.left, ir.left + 150, y, "Company", TC_WHITE, SA_LEFT);
		DrawString(ir.left + 155, ir.left + 230, y, "Price", TC_WHITE, SA_RIGHT);
		DrawString(ir.left + 235, ir.left + 300, y, "Change", TC_WHITE, SA_RIGHT);
		DrawString(ir.left + 305, ir.left + 360, y, "Owned", TC_WHITE, SA_RIGHT);
		DrawString(ir.left + 365, ir.left + 420, y, "Avail", TC_WHITE, SA_RIGHT);
		y += line_height;

		/* Draw separator line */
		GfxFillRect(ir.left, y - 2, ir.right, y - 1, PC_BLACK);

		int pos = -this->vscroll->GetPosition();
		int cap = this->vscroll->GetCapacity();

		for (size_t i = 0; i < this->companies.size(); i++) {
			if (pos >= 0 && pos < cap) {
				const StockListEntry &entry = this->companies[i];
				const Company *c = Company::GetIfValid(entry.company);
				if (c == nullptr) continue;

				bool selected = ((int)i == this->selected_index);
				if (selected) {
					GfxFillRect(ir.left, y, ir.right, y + line_height - 1, PC_DARK_BLUE);
				}

				/* Company name with colour */
				DrawCompanyIcon(entry.company, ir.left + 2, y + 2);
				SetDParam(0, entry.company);
				DrawString(ir.left + 20, ir.left + 150, y, STR_COMPANY_NAME, TC_BLACK, SA_LEFT);

				/* Share price */
				SetDParam(0, entry.share_price);
				DrawString(ir.left + 155, ir.left + 230, y, STR_JUST_CURRENCY_LONG, TC_BLACK, SA_RIGHT);

				/* Price change */
				std::string change_str = FormatPriceChange(entry.share_price - entry.price_change, entry.share_price);
				DrawString(ir.left + 235, ir.left + 300, y, change_str, static_cast<TextColour>(GetPriceChangeColour(entry.price_change)), SA_RIGHT);

				/* Shares owned */
				std::string owned_str = fmt::format("{}%", entry.shares_owned);
				DrawString(ir.left + 305, ir.left + 360, y, owned_str,
					entry.shares_owned >= CONTROLLING_INTEREST ? TC_GREEN : TC_BLACK, SA_RIGHT);

				/* Shares available */
				std::string avail_str = fmt::format("{}%", entry.shares_available);
				DrawString(ir.left + 365, ir.left + 420, y, avail_str, TC_BLACK, SA_RIGHT);

				y += line_height;
			}
			pos++;
		}
	}

	void OnClick(Point pt, WidgetID widget, int click_count) override
	{
		switch (widget) {
			case WID_STM_PANEL: {
				int row = this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_STM_PANEL);
				row--;  /* Account for header */
				if (row >= 0 && row < (int)this->companies.size()) {
					this->selected_index = row;
					this->SetDirty();
				}
				break;
			}

			case WID_STM_BUY_BUTTON:
				if (this->selected_index >= 0 && this->selected_index < (int)this->companies.size()) {
					/* TODO: Open buy shares dialog */
					ShowStockTradeWindow(this->companies[this->selected_index].company, true);
				}
				break;

			case WID_STM_SELL_BUTTON:
				if (this->selected_index >= 0 && this->selected_index < (int)this->companies.size()) {
					/* TODO: Open sell shares dialog */
					ShowStockTradeWindow(this->companies[this->selected_index].company, false);
				}
				break;

			case WID_STM_COMPANY_INFO:
				if (this->selected_index >= 0 && this->selected_index < (int)this->companies.size()) {
					ShowCompany(this->companies[this->selected_index].company);
				}
				break;
		}
	}

	void OnResize() override
	{
		this->vscroll->SetCapacityFromWidget(this, WID_STM_PANEL);
	}

	void OnInvalidateData([[maybe_unused]] int data = 0, [[maybe_unused]] bool gui_scope = true) override
	{
		if (!gui_scope) return;
		this->BuildCompanyList();
	}

	/** Timer to refresh data periodically */
	IntervalTimer<TimerWindow> refresh_interval = {std::chrono::seconds(3), [this](auto) {
		this->BuildCompanyList();
		this->SetDirty();
	}};
};

/** Widget definitions for stock market window */
static constexpr std::initializer_list<NWidgetPart> _nested_stock_market_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_BROWN),
		NWidget(WWT_CAPTION, COLOUR_BROWN, WID_STM_CAPTION), SetDataTip(STR_JUST_STRING1, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS), SetTextStyle(TC_WHITE),
		NWidget(WWT_SHADEBOX, COLOUR_BROWN),
		NWidget(WWT_DEFSIZEBOX, COLOUR_BROWN),
		NWidget(WWT_STICKYBOX, COLOUR_BROWN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_BROWN, WID_STM_PANEL), SetResize(1, 1), SetScrollbar(WID_STM_SCROLLBAR), EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_BROWN, WID_STM_SCROLLBAR),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_STM_BUY_BUTTON), SetMinimalSize(80, 12), SetDataTip(STR_JUST_STRING1, STR_NULL), SetTextStyle(TC_WHITE),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_STM_SELL_BUTTON), SetMinimalSize(80, 12), SetDataTip(STR_JUST_STRING1, STR_NULL), SetTextStyle(TC_WHITE),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_STM_COMPANY_INFO), SetMinimalSize(80, 12), SetDataTip(STR_JUST_STRING1, STR_NULL), SetTextStyle(TC_WHITE),
		NWidget(WWT_PANEL, COLOUR_BROWN), SetFill(1, 0), SetResize(1, 0), EndContainer(),
		NWidget(WWT_RESIZEBOX, COLOUR_BROWN),
	EndContainer(),
};

static WindowDesc _stock_market_desc(
	WDP_AUTO, "stock_market", 450, 300,
	WC_FINANCES, WC_NONE,  /* TODO: Add proper window class */
	{},
	_nested_stock_market_widgets
);

/**
 * Show the stock market window.
 */
void ShowStockMarketWindow()
{
	AllocateWindowDescFront<StockMarketWindow>(_stock_market_desc, 0);
}

/** Stock trading dialog (buy/sell shares) */
struct StockTradeWindow : Window {
	CompanyID target_company;
	bool is_buying;
	uint8_t quantity = 1;

	StockTradeWindow(WindowDesc &desc, CompanyID company, bool buying) : Window(desc)
	{
		this->target_company = company;
		this->is_buying = buying;

		this->InitNested(company);
	}

	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		switch (widget) {
			case WID_ST_COMPANY_NAME:
			case WID_ST_SHARE_PRICE:
			case WID_ST_TOTAL_COST:
				size.width = 200;
				break;
		}
	}

	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		const Company *c = Company::GetIfValid(this->target_company);
		if (c == nullptr) return;

		switch (widget) {
			case WID_ST_COMPANY_NAME: {
				SetDParam(0, this->target_company);
				DrawString(r.left, r.right, r.top, STR_COMPANY_NAME, TC_BLACK, SA_CENTER);
				break;
			}

			case WID_ST_SHARE_PRICE: {
				std::string text = "Share Price: ";
				SetDParam(0, c->shares.share_price);
				text += GetString(STR_JUST_CURRENCY_LONG);
				DrawString(r.left, r.right, r.top, text, TC_BLACK, SA_CENTER);
				break;
			}

			case WID_ST_SHARES_AVAILABLE: {
				std::string buffer = fmt::format("Available: {}%", c->shares.shares_available);
				DrawString(r.left, r.right, r.top, buffer, TC_BLACK, SA_CENTER);
				break;
			}

			case WID_ST_YOUR_SHARES: {
				const Company *my = Company::GetIfValid(_local_company);
				uint8_t owned = (my != nullptr) ? c->shares.shares_owned[_local_company.base()] : 0;
				std::string buffer = fmt::format("You Own: {}%", owned);
				DrawString(r.left, r.right, r.top, buffer, TC_BLACK, SA_CENTER);
				break;
			}

			case WID_ST_QUANTITY_TEXT: {
				std::string buffer = fmt::format("{}%", this->quantity);
				DrawString(r.left, r.right, r.top, buffer, TC_BLACK, SA_CENTER);
				break;
			}

			case WID_ST_TOTAL_COST: {
				Money total;
				if (this->is_buying) {
					total = GetShareBuyCost(c->shares.share_price, this->quantity);
				} else {
					total = GetShareSellProceeds(c->shares.share_price, this->quantity);
				}
				std::string text = this->is_buying ? "Total Cost: " : "Proceeds: ";
				SetDParam(0, total);
				text += GetString(STR_JUST_CURRENCY_LONG);
				DrawString(r.left, r.right, r.top, text, TC_BLACK, SA_CENTER);
				break;
			}
		}
	}

	void OnClick(Point pt, WidgetID widget, int click_count) override
	{
		const Company *c = Company::GetIfValid(this->target_company);
		if (c == nullptr) return;

		switch (widget) {
			case WID_ST_QUANTITY_DOWN:
				if (this->quantity > 1) {
					this->quantity--;
					this->SetDirty();
				}
				break;

			case WID_ST_QUANTITY_UP: {
				uint8_t max_qty;
				if (this->is_buying) {
					max_qty = c->shares.shares_available;
				} else {
					const Company *my = Company::GetIfValid(_local_company);
					max_qty = (my != nullptr) ? c->shares.shares_owned[_local_company.base()] : 0;
				}
				if (this->quantity < max_qty) {
					this->quantity++;
					this->SetDirty();
				}
				break;
			}

			case WID_ST_EXECUTE:
				if (this->is_buying) {
					BuyShares(_local_company, this->target_company, this->quantity);
				} else {
					SellShares(_local_company, this->target_company, this->quantity);
				}
				this->Close();
				break;

			case WID_ST_CANCEL:
				this->Close();
				break;
		}
	}
};

static constexpr std::initializer_list<NWidgetPart> _nested_stock_trade_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_ST_CAPTION), SetDataTip(STR_JUST_STRING1, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY),
		NWidget(NWID_VERTICAL), SetPadding(10),
			NWidget(WWT_TEXT, INVALID_COLOUR, WID_ST_COMPANY_NAME), SetMinimalSize(200, 12), SetFill(1, 0),
			NWidget(NWID_SPACER), SetMinimalSize(0, 5),
			NWidget(WWT_TEXT, INVALID_COLOUR, WID_ST_SHARE_PRICE), SetMinimalSize(200, 12), SetFill(1, 0),
			NWidget(WWT_TEXT, INVALID_COLOUR, WID_ST_SHARES_AVAILABLE), SetMinimalSize(200, 12), SetFill(1, 0),
			NWidget(WWT_TEXT, INVALID_COLOUR, WID_ST_YOUR_SHARES), SetMinimalSize(200, 12), SetFill(1, 0),
			NWidget(NWID_SPACER), SetMinimalSize(0, 10),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_TEXT, INVALID_COLOUR, WID_ST_QUANTITY_LABEL), SetDataTip(STR_JUST_STRING1, STR_NULL), SetMinimalSize(60, 12),
				NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_ST_QUANTITY_DOWN), SetDataTip(AWV_DECREASE, STR_NULL),
				NWidget(WWT_TEXT, INVALID_COLOUR, WID_ST_QUANTITY_TEXT), SetMinimalSize(50, 12),
				NWidget(WWT_PUSHARROWBTN, COLOUR_GREY, WID_ST_QUANTITY_UP), SetDataTip(AWV_INCREASE, STR_NULL),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(0, 5),
			NWidget(WWT_TEXT, INVALID_COLOUR, WID_ST_TOTAL_COST), SetMinimalSize(200, 12), SetFill(1, 0),
			NWidget(NWID_SPACER), SetMinimalSize(0, 10),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_ST_EXECUTE), SetMinimalSize(80, 12), SetDataTip(STR_JUST_STRING1, STR_NULL), SetFill(1, 0),
				NWidget(NWID_SPACER), SetMinimalSize(10, 0),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_ST_CANCEL), SetMinimalSize(80, 12), SetDataTip(STR_JUST_STRING1, STR_NULL), SetFill(1, 0),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _stock_trade_desc(
	WDP_CENTER, "stock_trade", 250, 200,
	WC_FINANCES, WC_NONE,
	WDF_MODAL,
	_nested_stock_trade_widgets
);

/**
 * Show the stock trading window for buying or selling shares.
 * @param company Target company whose shares to trade.
 * @param buying True for buying, false for selling.
 */
void ShowStockTradeWindow(CompanyID company, bool buying)
{
	new StockTradeWindow(_stock_trade_desc, company, buying);
}
