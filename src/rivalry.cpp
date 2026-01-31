/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file rivalry.cpp Implementation of company rivalry system. */

#include "stdafx.h"
#include "rivalry.h"
#include "company_base.h"
#include "ceo_dialogue.h"
#include "empire_news.h"
#include "game_events.h"
#include <utility>

#include "safeguards.h"

/** Global rivalry state */
RivalryState _rivalry;

/** Initialize rivalry system */
void InitializeRivalry()
{
	_rivalry.relationships.clear();
	_rivalry.player_nemesis = CompanyID::Invalid();
	_rivalry.total_rivalry_events = 0;
}

/** Get or create relationship */
CompanyRelationship *GetRelationship(CompanyID a, CompanyID b)
{
	/* Normalize order */
	if (a > b) std::swap(a, b);

	/* Find existing */
	for (auto &rel : _rivalry.relationships) {
		if (rel.company_a == a && rel.company_b == b) {
			return &rel;
		}
	}

	/* Create new */
	CompanyRelationship rel;
	rel.company_a = a;
	rel.company_b = b;
	rel.status = RelationshipStatus::UNKNOWN;
	rel.relationship_score = 0;
	rel.rivalry_started_date = 0;
	rel.last_interaction_date = 0;
	rel.takeover_attempts_a_to_b = 0;
	rel.takeover_attempts_b_to_a = 0;
	rel.routes_stolen_a_from_b = 0;
	rel.routes_stolen_b_from_a = 0;
	rel.shares_bought_a_in_b = 0;
	rel.shares_bought_b_in_a = 0;
	rel.revenge_arc_active = false;
	rel.revenge_seeker = CompanyID::Invalid();

	_rivalry.relationships.push_back(rel);
	return &_rivalry.relationships.back();
}

/** Calculate relationship impact */
int32_t CalculateRelationshipImpact(RivalryAction action)
{
	switch (action) {
		case RivalryAction::BOUGHT_SHARES:
			return -5; /* Slight negative - they're watching */
		case RivalryAction::SOLD_SHARES:
			return 2; /* Slight positive - less threat */
		case RivalryAction::ATTEMPTED_TAKEOVER:
			return -30; /* Major negative */
		case RivalryAction::COMPLETED_TAKEOVER:
			return -50; /* Devastating to relationship */
		case RivalryAction::UNDERCUT_ROUTE:
			return -15; /* Significant negative */
		case RivalryAction::STOLE_INDUSTRY:
			return -20; /* Major negative */
		case RivalryAction::OUTBID_CONTRACT:
			return -10; /* Moderate negative */
		case RivalryAction::SURVIVED_TAKEOVER:
			return -5; /* They're annoyed they failed */
		case RivalryAction::BANKRUPT_THEM:
			return -100; /* Ultimate betrayal */
		case RivalryAction::HELPED_DURING_CRISIS:
			return 25; /* Major positive */
		case RivalryAction::POSITIVE_TRADE:
			return 10; /* Positive */
		default:
			return 0;
	}
}

/** Record a rivalry action */
void RecordRivalryAction(CompanyID initiator, CompanyID target, RivalryAction action)
{
	CompanyRelationship *rel = GetRelationship(initiator, target);

	int32_t impact = CalculateRelationshipImpact(action);
	rel->relationship_score = Clamp(rel->relationship_score + impact, -100, 100);
	rel->last_interaction_date = 0; /* TODO: Current date */

	/* Track specific stats */
	bool a_is_initiator = (initiator == rel->company_a);

	switch (action) {
		case RivalryAction::ATTEMPTED_TAKEOVER:
			if (a_is_initiator) rel->takeover_attempts_a_to_b++;
			else rel->takeover_attempts_b_to_a++;
			break;
		case RivalryAction::UNDERCUT_ROUTE:
		case RivalryAction::STOLE_INDUSTRY:
			if (a_is_initiator) rel->routes_stolen_a_from_b++;
			else rel->routes_stolen_b_from_a++;
			break;
		default:
			break;
	}

	/* Record event */
	RivalryEvent event;
	event.action = action;
	event.initiator = initiator;
	event.target = target;
	event.date = 0; /* TODO: Current date */
	event.relationship_impact = impact;
	rel->history.push_back(event);

	/* Update status */
	UpdateRelationshipStatus(*rel);

	/* Check for revenge arc */
	if (impact <= -25 && !rel->revenge_arc_active) {
		rel->revenge_arc_active = true;
		rel->revenge_seeker = target;
	}

	_rivalry.total_rivalry_events++;
}

/** Update relationship status */
void UpdateRelationshipStatus(CompanyRelationship &rel)
{
	int32_t score = rel.relationship_score;

	if (score <= -80) {
		rel.status = RelationshipStatus::NEMESIS;
	} else if (score <= -50) {
		rel.status = RelationshipStatus::HOSTILE;
	} else if (score <= -25) {
		rel.status = RelationshipStatus::RIVAL;
	} else if (score <= -10) {
		rel.status = RelationshipStatus::WARY;
	} else if (score <= 10) {
		rel.status = RelationshipStatus::NEUTRAL;
	} else if (score <= 30) {
		rel.status = RelationshipStatus::RESPECTFUL;
	} else {
		rel.status = RelationshipStatus::FRIENDLY;
	}

	/* Generate rivalry name for significant conflicts */
	if (rel.status == RelationshipStatus::NEMESIS && rel.rivalry_nickname.empty()) {
		rel.rivalry_nickname = "The Great Corporate War";
	}
}

/** Get status text */
const char *GetRelationshipStatusText(RelationshipStatus status)
{
	switch (status) {
		case RelationshipStatus::UNKNOWN: return "Unknown";
		case RelationshipStatus::NEUTRAL: return "Neutral";
		case RelationshipStatus::WARY: return "Wary";
		case RelationshipStatus::RIVAL: return "Rival";
		case RelationshipStatus::HOSTILE: return "Hostile";
		case RelationshipStatus::NEMESIS: return "Nemesis";
		case RelationshipStatus::RESPECTFUL: return "Respectful";
		case RelationshipStatus::FRIENDLY: return "Friendly";
		default: return "Unknown";
	}
}

/** Get AI response to player action */
std::string GetRivalryResponse(CompanyID ai, RivalryAction player_action)
{
	/* Get AI personality */
	CEOPersonality personality = static_cast<CEOPersonality>(
		ai.base() % static_cast<uint8_t>(CEOPersonality::NUM_PERSONALITIES)
	);

	switch (player_action) {
		case RivalryAction::BOUGHT_SHARES:
			return GetCEODialogue(personality, DialogueTrigger::PLAYER_BOUGHT_SHARES);
		case RivalryAction::ATTEMPTED_TAKEOVER:
		case RivalryAction::COMPLETED_TAKEOVER:
			return GetCEODialogue(personality, DialogueTrigger::PLAYER_HOSTILE_TAKEOVER);
		case RivalryAction::UNDERCUT_ROUTE:
			return GetCEODialogue(personality, DialogueTrigger::PLAYER_UNDERCUT_ROUTE);
		default:
			return "";
	}
}

/** Should AI seek revenge? */
bool ShouldSeekRevenge(CompanyID seeker, CompanyID target)
{
	CompanyRelationship *rel = GetRelationship(seeker, target);
	if (rel == nullptr) return false;

	return rel->revenge_arc_active && rel->revenge_seeker == seeker;
}

/** Get player nemesis */
CompanyID GetPlayerNemesis()
{
	return _rivalry.player_nemesis;
}

/** Update player nemesis */
void UpdatePlayerNemesis()
{
	CompanyID player = CompanyID::Begin(); /* TODO: Get actual player company */
	CompanyID worst_enemy = CompanyID::Invalid();
	int32_t worst_score = 0;

	for (const auto &rel : _rivalry.relationships) {
		if (rel.company_a == player || rel.company_b == player) {
			if (rel.relationship_score < worst_score) {
				worst_score = rel.relationship_score;
				worst_enemy = (rel.company_a == player) ? rel.company_b : rel.company_a;
			}
		}
	}

	_rivalry.player_nemesis = worst_enemy;
}

/** Is this our nemesis? */
bool IsNemesis(CompanyID company)
{
	return company == _rivalry.player_nemesis;
}

/** Get aggression modifier for AI */
float GetRivalryAggressionModifier(CompanyID ai, CompanyID target)
{
	CompanyRelationship *rel = GetRelationship(ai, target);
	if (rel == nullptr) return 1.0f;

	/* Nemesis = 2x aggression, Friendly = 0.5x */
	switch (rel->status) {
		case RelationshipStatus::NEMESIS: return 2.0f;
		case RelationshipStatus::HOSTILE: return 1.5f;
		case RelationshipStatus::RIVAL: return 1.2f;
		case RelationshipStatus::WARY: return 1.1f;
		case RelationshipStatus::NEUTRAL: return 1.0f;
		case RelationshipStatus::RESPECTFUL: return 0.8f;
		case RelationshipStatus::FRIENDLY: return 0.5f;
		default: return 1.0f;
	}
}

/** Get rivalry summary */
std::string GetRivalrySummary(CompanyID company_a, CompanyID company_b)
{
	CompanyRelationship *rel = GetRelationship(company_a, company_b);
	if (rel == nullptr) return "No significant history.";

	std::string summary;
	summary += "Relationship: ";
	summary += GetRelationshipStatusText(rel->status);
	summary += "\n";

	if (rel->takeover_attempts_a_to_b > 0 || rel->takeover_attempts_b_to_a > 0) {
		summary += "Takeover attempts: " + std::to_string(rel->takeover_attempts_a_to_b + rel->takeover_attempts_b_to_a) + "\n";
	}

	if (!rel->rivalry_nickname.empty()) {
		summary += "Known as: " + rel->rivalry_nickname + "\n";
	}

	if (rel->revenge_arc_active) {
		summary += "REVENGE ARC ACTIVE\n";
	}

	return summary;
}

/** Generate dramatic rivalry moments */
void GenerateRivalryDrama()
{
	/* Check for interesting rivalry situations */
	for (auto &rel : _rivalry.relationships) {
		if (rel.status == RelationshipStatus::NEMESIS) {
			/* High drama potential - maybe generate news? */
		}

		/* Check for revenge completion */
		if (rel.revenge_arc_active) {
			/* TODO: Check if revenge seeker has achieved something against target */
		}
	}
}
