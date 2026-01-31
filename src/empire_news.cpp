/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file empire_news.cpp Implementation of Transport Empire news system. */

#include "stdafx.h"
#include "empire_news.h"
#include "company_base.h"
#include "ceo_dialogue.h"
#include "screen_effects.h"
#include "ai_personality.h"
#include "core/random_func.hpp"
#include <map>

#include "safeguards.h"

/** News queue */
static std::vector<EmpireNewsItem> _news_queue;

/** News history */
static std::vector<EmpireNewsItem> _news_history;

/** Maximum news history size */
static constexpr size_t MAX_NEWS_HISTORY = 100;

/** Replace placeholders in template */
static std::string FormatNewsTemplate(const char *tmpl,
	const std::map<std::string, std::string> &replacements)
{
	std::string result(tmpl);

	for (const auto &[key, value] : replacements) {
		std::string placeholder = "{" + key + "}";
		size_t pos;
		while ((pos = result.find(placeholder)) != std::string::npos) {
			result.replace(pos, placeholder.length(), value);
		}
	}

	return result;
}

/** Get company name safely */
static std::string GetCompanyNameSafe(CompanyID id)
{
	Company *c = Company::GetIfValid(id);
	if (c == nullptr) return "Unknown Company";
	if (!c->name.empty()) return c->name;
	return "Transport Co. #" + std::to_string(static_cast<int>(id) + 1);
}

/** Queue a news item */
void QueueEmpireNews(const EmpireNewsItem &news)
{
	_news_queue.push_back(news);

	/* Trigger screen effects based on priority */
	switch (news.priority) {
		case NewsPriority::CRITICAL:
			OnMarketCrash(); /* Big dramatic effect */
			break;
		case NewsPriority::MAJOR:
			TriggerScreenEffect(ScreenEffectType::FLASH_WHITE, 100, 10);
			break;
		default:
			break;
	}
}

/** Generate share purchase news */
void GenerateSharePurchaseNews(CompanyID buyer, CompanyID target, uint8_t shares_bought, uint8_t total_shares)
{
	EmpireNewsItem news;
	news.category = NewsCategory::STOCK_MARKET;
	news.related_company = buyer;
	news.secondary_company = target;
	news.read = false;
	news.date = 0; /* TODO: Get current date */

	std::map<std::string, std::string> replacements = {
		{"COMPANY", GetCompanyNameSafe(buyer)},
		{"TARGET", GetCompanyNameSafe(target)},
		{"AMOUNT", std::to_string(shares_bought)},
		{"TOTAL", std::to_string(total_shares)},
	};

	/* Determine news importance based on stake size */
	if (total_shares >= 51) {
		/* Controlling interest! */
		news.priority = NewsPriority::MAJOR;
		news.headline = FormatNewsTemplate(NewsTemplates::CONTROLLING_INTEREST_HEADLINE, replacements);
		news.body = FormatNewsTemplate(NewsTemplates::CONTROLLING_INTEREST_BODY, replacements);

		/* Add CEO quote */
		CEOPersonality personality = static_cast<CEOPersonality>(
			static_cast<uint8_t>(buyer) % static_cast<uint8_t>(CEOPersonality::NUM_PERSONALITIES)
		);
		news.ceo_quote = GetCEODialogue(personality, DialogueTrigger::AI_TOOK_CONTROL);
		news.quote_personality = personality;
		news.has_ceo_portrait = true;

		/* Trigger dramatic effect */
		OnHostileTakeover();
	} else if (total_shares >= 25) {
		news.priority = NewsPriority::NORMAL;
		news.headline = FormatNewsTemplate(NewsTemplates::SHARES_PURCHASED_HEADLINE, replacements);
		news.body = FormatNewsTemplate(NewsTemplates::SHARES_PURCHASED_BODY, replacements);
		news.has_ceo_portrait = false;
	} else {
		news.priority = NewsPriority::MINOR;
		news.headline = FormatNewsTemplate(NewsTemplates::SHARES_PURCHASED_HEADLINE, replacements);
		news.body = FormatNewsTemplate(NewsTemplates::SHARES_PURCHASED_BODY, replacements);
		news.has_ceo_portrait = false;
	}

	QueueEmpireNews(news);
}

/** Generate hostile takeover news */
void GenerateHostileTakeoverNews(CompanyID acquirer, CompanyID target)
{
	EmpireNewsItem news;
	news.category = NewsCategory::TAKEOVER;
	news.priority = NewsPriority::CRITICAL;
	news.related_company = acquirer;
	news.secondary_company = target;
	news.read = false;
	news.date = 0;

	std::map<std::string, std::string> replacements = {
		{"COMPANY", GetCompanyNameSafe(acquirer)},
		{"TARGET", GetCompanyNameSafe(target)},
		{"DEFEATED_CEO", "the former management"},
	};

	news.headline = FormatNewsTemplate(NewsTemplates::HOSTILE_TAKEOVER_HEADLINE, replacements);
	news.body = FormatNewsTemplate(NewsTemplates::HOSTILE_TAKEOVER_BODY, replacements);

	/* Defeated CEO's final words */
	CEOPersonality defeated_personality = static_cast<CEOPersonality>(
		static_cast<uint8_t>(target) % static_cast<uint8_t>(CEOPersonality::NUM_PERSONALITIES)
	);
	news.ceo_quote = GetCEODefeatDialogue(defeated_personality);
	news.quote_personality = defeated_personality;
	news.has_ceo_portrait = true;

	QueueEmpireNews(news);

	/* Big screen effect */
	OnHostileTakeover();
}

/** Generate market crash news */
void GenerateMarketCrashNews(const char *reason)
{
	EmpireNewsItem news;
	news.category = NewsCategory::ECONOMY;
	news.priority = NewsPriority::CRITICAL;
	news.related_company = CompanyID::Invalid();
	news.read = false;
	news.date = 0;

	std::map<std::string, std::string> replacements = {
		{"REASON", reason ? reason : "economic uncertainty"},
	};

	news.headline = NewsTemplates::MARKET_CRASH_HEADLINE;
	news.body = FormatNewsTemplate(NewsTemplates::MARKET_CRASH_BODY, replacements);
	news.has_ceo_portrait = false;

	QueueEmpireNews(news);
	OnMarketCrash();
}

/** Generate market boom news */
void GenerateMarketBoomNews()
{
	EmpireNewsItem news;
	news.category = NewsCategory::ECONOMY;
	news.priority = NewsPriority::MAJOR;
	news.related_company = CompanyID::Invalid();
	news.read = false;
	news.date = 0;

	news.headline = NewsTemplates::MARKET_BOOM_HEADLINE;
	news.body = NewsTemplates::MARKET_BOOM_BODY;
	news.has_ceo_portrait = false;

	QueueEmpireNews(news);
	OnMarketBoom();
}

/** Generate new company news */
void GenerateNewCompanyNews(CompanyID company)
{
	EmpireNewsItem news;
	news.category = NewsCategory::COMPANY;
	news.priority = NewsPriority::NORMAL;
	news.related_company = company;
	news.read = false;
	news.date = 0;

	CEOPersonality personality = static_cast<CEOPersonality>(
		company.base() % static_cast<uint8_t>(CEOPersonality::NUM_PERSONALITIES)
	);

	std::string ceo_name = GenerateCEOName(personality);
	std::string motto = GenerateCompanyMotto(personality);

	std::map<std::string, std::string> replacements = {
		{"COMPANY", GetCompanyNameSafe(company)},
		{"CEO_NAME", ceo_name},
		{"MOTTO", motto},
	};

	news.headline = FormatNewsTemplate(NewsTemplates::NEW_COMPANY_HEADLINE, replacements);
	news.body = FormatNewsTemplate(NewsTemplates::NEW_COMPANY_BODY, replacements);
	news.ceo_quote = GetCEODialogue(personality, DialogueTrigger::GAME_START);
	news.quote_personality = personality;
	news.has_ceo_portrait = true;

	QueueEmpireNews(news);
}

/** Generate bankruptcy news */
void GenerateBankruptcyNews(CompanyID company, bool warning_only)
{
	EmpireNewsItem news;
	news.category = NewsCategory::COMPANY;
	news.priority = warning_only ? NewsPriority::MAJOR : NewsPriority::CRITICAL;
	news.related_company = company;
	news.read = false;
	news.date = 0;

	std::map<std::string, std::string> replacements = {
		{"COMPANY", GetCompanyNameSafe(company)},
		{"CEO_NAME", "the CEO"},
	};

	if (warning_only) {
		news.headline = FormatNewsTemplate(NewsTemplates::BANKRUPTCY_WARNING_HEADLINE, replacements);
		news.body = FormatNewsTemplate(NewsTemplates::BANKRUPTCY_WARNING_BODY, replacements);
		OnBankruptcyWarning();
	} else {
		news.headline = FormatNewsTemplate(NewsTemplates::BANKRUPTCY_HEADLINE, replacements);
		news.body = FormatNewsTemplate(NewsTemplates::BANKRUPTCY_BODY, replacements);

		CEOPersonality personality = static_cast<CEOPersonality>(
			company.base() % static_cast<uint8_t>(CEOPersonality::NUM_PERSONALITIES)
		);
		news.ceo_quote = GetCEODefeatDialogue(personality);
		news.quote_personality = personality;
		news.has_ceo_portrait = true;
	}

	QueueEmpireNews(news);
}

/** Generate investigation news */
void GenerateInvestigationNews(CompanyID company)
{
	EmpireNewsItem news;
	news.category = NewsCategory::SCANDAL;
	news.priority = NewsPriority::MAJOR;
	news.related_company = company;
	news.read = false;
	news.date = 0;

	std::map<std::string, std::string> replacements = {
		{"COMPANY", GetCompanyNameSafe(company)},
		{"CEO_NAME", "company leadership"},
	};

	news.headline = FormatNewsTemplate(NewsTemplates::INVESTIGATION_HEADLINE, replacements);
	news.body = FormatNewsTemplate(NewsTemplates::INVESTIGATION_BODY, replacements);
	news.has_ceo_portrait = false;

	QueueEmpireNews(news);
	TriggerScreenEffect(ScreenEffectType::VIGNETTE_RED, 100, 30);
}

/** Generate weather news */
void GenerateWeatherNews(const char *weather_type, bool severe)
{
	EmpireNewsItem news;
	news.category = NewsCategory::WEATHER;
	news.priority = severe ? NewsPriority::MAJOR : NewsPriority::NORMAL;
	news.related_company = CompanyID::Invalid();
	news.read = false;
	news.date = 0;

	if (severe) {
		news.headline = NewsTemplates::STORM_WARNING_HEADLINE;
		news.body = NewsTemplates::STORM_WARNING_BODY;
	} else {
		news.headline = "Weather Update";
		news.body = std::string("Current conditions: ") + weather_type;
	}
	news.has_ceo_portrait = false;

	QueueEmpireNews(news);
}

/** Generate achievement news */
void GenerateAchievementNews(CompanyID company, const char *achievement_name, const char *description)
{
	EmpireNewsItem news;
	news.category = NewsCategory::ACHIEVEMENT;
	news.priority = NewsPriority::NORMAL;
	news.related_company = company;
	news.read = false;
	news.date = 0;

	std::map<std::string, std::string> replacements = {
		{"PLAYER", GetCompanyNameSafe(company)},
		{"ACHIEVEMENT", achievement_name},
		{"DESCRIPTION", description},
	};

	news.headline = FormatNewsTemplate(NewsTemplates::ACHIEVEMENT_HEADLINE, replacements);
	news.body = FormatNewsTemplate(NewsTemplates::ACHIEVEMENT_BODY, replacements);
	news.has_ceo_portrait = false;

	QueueEmpireNews(news);
	OnAchievementUnlocked();
}

/** Generate year-end news */
void GenerateYearEndNews(int year)
{
	EmpireNewsItem news;
	news.category = NewsCategory::MILESTONE;
	news.priority = NewsPriority::NORMAL;
	news.related_company = CompanyID::Invalid();
	news.read = false;
	news.date = 0;

	/* Find top company */
	CompanyID top_company = CompanyID::Invalid();
	Money top_value = 0;

	for (const Company *c : Company::Iterate()) {
		Money value = c->cur_economy.company_value;
		if (value > top_value) {
			top_value = value;
			top_company = c->index;
		}
	}

	std::map<std::string, std::string> replacements = {
		{"YEAR", std::to_string(year)},
		{"TOP_COMPANY", GetCompanyNameSafe(top_company)},
		{"TOP_VALUE", std::to_string(top_value / 1000000) + "M"},
		{"SUMMARY", "It was a year of growth and competition."},
	};

	news.headline = FormatNewsTemplate(NewsTemplates::YEAR_END_HEADLINE, replacements);
	news.body = FormatNewsTemplate(NewsTemplates::YEAR_END_BODY, replacements);
	news.has_ceo_portrait = false;

	QueueEmpireNews(news);
	OnYearEnd();
}

/** Process news queue */
void ProcessEmpireNewsQueue()
{
	/* Move queued items to history */
	for (auto &news : _news_queue) {
		_news_history.push_back(std::move(news));

		/* Trim history if too large */
		if (_news_history.size() > MAX_NEWS_HISTORY) {
			_news_history.erase(_news_history.begin());
		}
	}

	_news_queue.clear();

	/* TODO: Actually display news in-game UI */
}

/** Get unread count */
uint32_t GetUnreadNewsCount()
{
	uint32_t count = 0;
	for (const auto &news : _news_history) {
		if (!news.read) count++;
	}
	return count;
}

/** Mark all read */
void MarkAllNewsRead()
{
	for (auto &news : _news_history) {
		news.read = true;
	}
}
