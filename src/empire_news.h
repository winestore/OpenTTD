/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file empire_news.h Transport Empire news and notification system. */

#ifndef EMPIRE_NEWS_H
#define EMPIRE_NEWS_H

#include "stdafx.h"
#include "company_type.h"
#include "date_type.h"
#include "ceo_dialogue.h"

/** News priority levels */
enum class NewsPriority : uint8_t {
	TICKER,          ///< Scrolling ticker only
	MINOR,           ///< Small notification
	NORMAL,          ///< Standard news popup
	MAJOR,           ///< Large news with sound
	CRITICAL,        ///< Full-screen breaking news
};

/** News categories for filtering */
enum class NewsCategory : uint8_t {
	STOCK_MARKET,    ///< Share trading, prices
	TAKEOVER,        ///< Hostile takeovers, mergers
	COMPANY,         ///< Company events (bankruptcy, founding)
	ECONOMY,         ///< Market crashes, booms
	WEATHER,         ///< Severe weather warnings
	ACHIEVEMENT,     ///< Player achievements
	RIVALRY,         ///< CEO interactions
	SCANDAL,         ///< Investigations, fines
	MILESTONE,       ///< Game milestones
};

/** A news item */
struct EmpireNewsItem {
	std::string headline;
	std::string body;
	std::string ceo_quote;           ///< Optional CEO quote
	CEOPersonality quote_personality; ///< Who said the quote
	NewsCategory category;
	NewsPriority priority;
	CompanyID related_company;
	CompanyID secondary_company;     ///< For takeovers (acquirer/target)
	uint32_t date;
	bool read;
	bool has_ceo_portrait;
};

/** News templates for common events */
namespace NewsTemplates {

	/* Stock Market */
	constexpr const char *SHARES_PURCHASED_HEADLINE = "{COMPANY} Acquires Stake in {TARGET}";
	constexpr const char *SHARES_PURCHASED_BODY = "{COMPANY} has purchased {AMOUNT}% of {TARGET}'s outstanding shares, "
		"bringing their total ownership to {TOTAL}%. Market analysts are watching closely.";

	constexpr const char *CONTROLLING_INTEREST_HEADLINE = "BREAKING: {COMPANY} Takes Control of {TARGET}";
	constexpr const char *CONTROLLING_INTEREST_BODY = "In a dramatic market move, {COMPANY} has acquired a controlling "
		"51% stake in {TARGET}. CEO {CEO_NAME} now has voting control over all major decisions.";

	constexpr const char *HOSTILE_TAKEOVER_HEADLINE = "HOSTILE TAKEOVER: {TARGET} Falls to {COMPANY}";
	constexpr const char *HOSTILE_TAKEOVER_BODY = "After a fierce battle for control, {COMPANY} has completed a hostile "
		"takeover of {TARGET}. The {TARGET} brand will be absorbed into {COMPANY}'s growing empire. "
		"Former CEO {DEFEATED_CEO} has been ousted from the board.";

	/* Market Events */
	constexpr const char *MARKET_CRASH_HEADLINE = "MARKET CRASH: Transport Stocks Plummet";
	constexpr const char *MARKET_CRASH_BODY = "Panic selling has gripped the transport sector as share prices collapse "
		"across the board. Analysts cite {REASON} as the primary cause. Investors are advised to remain calm.";

	constexpr const char *MARKET_BOOM_HEADLINE = "BULL MARKET: Transport Stocks Surge";
	constexpr const char *MARKET_BOOM_BODY = "Optimism is running high as transport company valuations reach record highs. "
		"Strong cargo volumes and passenger numbers are driving investor confidence.";

	constexpr const char *PRICE_SURGE_HEADLINE = "{COMPANY} Stock Surges {PERCENT}%";
	constexpr const char *PRICE_SURGE_BODY = "Shares of {COMPANY} jumped {PERCENT}% today following {REASON}. "
		"The company's market capitalization now stands at ${VALUE}.";

	constexpr const char *PRICE_CRASH_HEADLINE = "{COMPANY} Stock in Freefall";
	constexpr const char *PRICE_CRASH_BODY = "Investors are fleeing {COMPANY} as shares plunged {PERCENT}% in heavy trading. "
		"{REASON} has shaken confidence in the firm's leadership.";

	/* Company Events */
	constexpr const char *NEW_COMPANY_HEADLINE = "New Competitor Enters Market: {COMPANY}";
	constexpr const char *NEW_COMPANY_BODY = "{CEO_NAME} has founded {COMPANY}, promising to {MOTTO}. "
		"Industry veterans are skeptical, but the new CEO appears confident.";

	constexpr const char *BANKRUPTCY_WARNING_HEADLINE = "WARNING: {COMPANY} Faces Bankruptcy";
	constexpr const char *BANKRUPTCY_WARNING_BODY = "{COMPANY} is struggling to meet its financial obligations. "
		"Unless {CEO_NAME} can turn things around, the company may be forced into liquidation.";

	constexpr const char *BANKRUPTCY_HEADLINE = "{COMPANY} Declares Bankruptcy";
	constexpr const char *BANKRUPTCY_BODY = "After months of financial struggles, {COMPANY} has officially declared bankruptcy. "
		"All assets will be liquidated. {CEO_NAME} could not be reached for comment.";

	/* Scandals */
	constexpr const char *INVESTIGATION_HEADLINE = "INVESTIGATION: {COMPANY} Under Regulatory Scrutiny";
	constexpr const char *INVESTIGATION_BODY = "Regulators have launched an investigation into {COMPANY}'s trading practices. "
		"Allegations of market manipulation have surfaced. {CEO_NAME} denies all wrongdoing.";

	constexpr const char *FINE_HEADLINE = "{COMPANY} Fined ${AMOUNT} for Market Violations";
	constexpr const char *FINE_BODY = "Regulators have levied a ${AMOUNT} fine against {COMPANY} for {VIOLATION}. "
		"This marks the {COUNT} regulatory action against the firm this year.";

	constexpr const char *SCANDAL_HEADLINE = "SCANDAL: {COMPANY} CEO Accused of {ACCUSATION}";
	constexpr const char *SCANDAL_BODY = "Shocking allegations have emerged against {CEO_NAME} of {COMPANY}. "
		"Sources claim {DETAILS}. The company's stock has dropped sharply on the news.";

	/* Weather */
	constexpr const char *STORM_WARNING_HEADLINE = "SEVERE WEATHER: Major Storm Approaching";
	constexpr const char *STORM_WARNING_BODY = "Meteorologists are warning of severe weather conditions. "
		"Transport services may be disrupted. Operators are advised to take precautions.";

	constexpr const char *BLIZZARD_HEADLINE = "BLIZZARD CONDITIONS: Transport Paralyzed";
	constexpr const char *BLIZZARD_BODY = "Heavy snowfall has brought much of the transport network to a standstill. "
		"Road and rail services are experiencing significant delays.";

	/* Achievements */
	constexpr const char *ACHIEVEMENT_HEADLINE = "MILESTONE: {PLAYER} Achieves {ACHIEVEMENT}";
	constexpr const char *ACHIEVEMENT_BODY = "Congratulations are in order as {PLAYER} has achieved the prestigious "
		"\"{ACHIEVEMENT}\" milestone. {DESCRIPTION}";

	/* Rivalry */
	constexpr const char *RIVALRY_DECLARED_HEADLINE = "{CEO1} Declares War on {CEO2}";
	constexpr const char *RIVALRY_DECLARED_BODY = "Tensions are boiling over between {COMPANY1} and {COMPANY2}. "
		"{CEO1} has publicly challenged {CEO2}, stating: \"{QUOTE}\"";

	/* Milestones */
	constexpr const char *YEAR_END_HEADLINE = "Year in Review: {YEAR}";
	constexpr const char *YEAR_END_BODY = "As {YEAR} comes to a close, {TOP_COMPANY} leads the industry with "
		"${TOP_VALUE} in company value. {SUMMARY}";
}

/** Create and queue a news item */
void QueueEmpireNews(const EmpireNewsItem &news);

/** Generate news for share purchase */
void GenerateSharePurchaseNews(CompanyID buyer, CompanyID target, uint8_t shares_bought, uint8_t total_shares);

/** Generate news for hostile takeover */
void GenerateHostileTakeoverNews(CompanyID acquirer, CompanyID target);

/** Generate news for market crash */
void GenerateMarketCrashNews(const char *reason);

/** Generate news for market boom */
void GenerateMarketBoomNews();

/** Generate news for new company */
void GenerateNewCompanyNews(CompanyID company);

/** Generate news for bankruptcy */
void GenerateBankruptcyNews(CompanyID company, bool warning_only);

/** Generate news for investigation */
void GenerateInvestigationNews(CompanyID company);

/** Generate news for weather */
void GenerateWeatherNews(const char *weather_type, bool severe);

/** Generate news for achievement */
void GenerateAchievementNews(CompanyID company, const char *achievement_name, const char *description);

/** Generate year-end summary */
void GenerateYearEndNews(int year);

/** Process pending news queue */
void ProcessEmpireNewsQueue();

/** Get unread news count */
uint32_t GetUnreadNewsCount();

/** Mark all news as read */
void MarkAllNewsRead();

#endif /* EMPIRE_NEWS_H */
