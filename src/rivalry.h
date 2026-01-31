/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file rivalry.h Company rivalry and nemesis system for Transport Empire. */

#ifndef RIVALRY_H
#define RIVALRY_H

#include "stdafx.h"
#include "company_type.h"
#include "economy_type.h"
#include "ai_personality.h"

/**
 * RIVALRY SYSTEM
 *
 * Tracks relationships between companies and creates dramatic narratives.
 * Companies can become rivals, enemies, or even reluctant allies.
 */

/** Relationship status between companies */
enum class RelationshipStatus : uint8_t {
	UNKNOWN,         ///< Haven't interacted much
	NEUTRAL,         ///< Standard competition
	WARY,            ///< Watching each other
	RIVAL,           ///< Active competition
	HOSTILE,         ///< Aggressive conflict
	NEMESIS,         ///< Sworn enemies
	RESPECTFUL,      ///< Mutual respect despite competition
	FRIENDLY,        ///< Cordial relations
};

/** Actions that affect relationships */
enum class RivalryAction : uint8_t {
	BOUGHT_SHARES,           ///< Bought shares in their company
	SOLD_SHARES,             ///< Sold shares in their company
	ATTEMPTED_TAKEOVER,      ///< Tried to take them over
	COMPLETED_TAKEOVER,      ///< Successfully took them over
	UNDERCUT_ROUTE,          ///< Established competing route
	STOLE_INDUSTRY,          ///< Connected to their industry
	OUTBID_CONTRACT,         ///< Won a contract they wanted
	SURVIVED_TAKEOVER,       ///< Resisted their takeover attempt
	BANKRUPT_THEM,           ///< Caused their bankruptcy
	HELPED_DURING_CRISIS,    ///< Didn't attack when they were weak
	POSITIVE_TRADE,          ///< Mutually beneficial deal
};

/** Rivalry event for tracking history */
struct RivalryEvent {
	RivalryAction action;
	CompanyID initiator;
	CompanyID target;
	uint32_t date;
	int32_t relationship_impact;
	std::string description;
};

/** Relationship between two companies */
struct CompanyRelationship {
	CompanyID company_a;
	CompanyID company_b;
	RelationshipStatus status;
	int32_t relationship_score;     ///< -100 (nemesis) to +100 (allied)

	uint32_t rivalry_started_date;
	uint32_t last_interaction_date;

	/* Rivalry statistics */
	uint8_t takeover_attempts_a_to_b;
	uint8_t takeover_attempts_b_to_a;
	uint8_t routes_stolen_a_from_b;
	uint8_t routes_stolen_b_from_a;
	Money shares_bought_a_in_b;
	Money shares_bought_b_in_a;

	/* Dramatic moments */
	bool revenge_arc_active;         ///< One company seeking revenge
	CompanyID revenge_seeker;
	std::string rivalry_nickname;    ///< "The Great Rail War of '52"

	/* History */
	std::vector<RivalryEvent> history;
};

/** Global rivalry state */
struct RivalryState {
	std::vector<CompanyRelationship> relationships;
	CompanyID player_nemesis;        ///< Player's primary rival
	uint32_t total_rivalry_events;
};

extern RivalryState _rivalry;

/** Initialize rivalry system */
void InitializeRivalry();

/** Get or create relationship between companies */
CompanyRelationship *GetRelationship(CompanyID a, CompanyID b);

/** Record a rivalry action */
void RecordRivalryAction(CompanyID initiator, CompanyID target, RivalryAction action);

/** Update relationship status based on score */
void UpdateRelationshipStatus(CompanyRelationship &rel);

/** Get AI response to player action */
std::string GetRivalryResponse(CompanyID ai, RivalryAction player_action);

/** Check for revenge opportunities */
bool ShouldSeekRevenge(CompanyID seeker, CompanyID target);

/** Get player's current nemesis */
CompanyID GetPlayerNemesis();

/** Determine nemesis based on relationships */
void UpdatePlayerNemesis();

/** Get relationship status text */
const char *GetRelationshipStatusText(RelationshipStatus status);

/** Get rivalry narrative summary */
std::string GetRivalrySummary(CompanyID company_a, CompanyID company_b);

/** Calculate relationship impact for action */
int32_t CalculateRelationshipImpact(RivalryAction action);

/** Rivalry-based AI decision modifiers */
float GetRivalryAggressionModifier(CompanyID ai, CompanyID target);

/** Is this company our nemesis? */
bool IsNemesis(CompanyID company);

/** Drama generation - create interesting rivalry moments */
void GenerateRivalryDrama();

#endif /* RIVALRY_H */
