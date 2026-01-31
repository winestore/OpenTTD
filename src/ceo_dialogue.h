/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file ceo_dialogue.h CEO Dialogue and Taunts system - brings AI personalities to life. */

#ifndef CEO_DIALOGUE_H
#define CEO_DIALOGUE_H

#include "stdafx.h"
#include "company_type.h"
#include "ai_personality.h"

/** Events that trigger CEO dialogue */
enum class DialogueTrigger : uint8_t {
	/* Competitive events */
	PLAYER_BOUGHT_SHARES,        ///< Player bought shares in AI company
	PLAYER_SOLD_SHARES,          ///< Player sold shares in AI company
	PLAYER_TOOK_CONTROL,         ///< Player gained controlling interest
	PLAYER_HOSTILE_TAKEOVER,     ///< Player completed hostile takeover
	AI_BOUGHT_PLAYER_SHARES,     ///< AI bought shares in player company
	AI_TOOK_CONTROL,             ///< AI gained controlling interest in player

	/* Route competition */
	PLAYER_UNDERCUT_ROUTE,       ///< Player established competing route
	AI_UNDERCUT_ROUTE,           ///< AI established competing route
	PLAYER_STOLE_INDUSTRY,       ///< Player connected to AI's industry

	/* Financial events */
	PLAYER_BANKRUPTCY_NEAR,      ///< Player company struggling
	AI_BANKRUPTCY_NEAR,          ///< AI company struggling
	PLAYER_HUGE_PROFIT,          ///< Player had exceptional quarter
	AI_HUGE_PROFIT,              ///< AI had exceptional quarter

	/* Market events */
	MARKET_CRASH,                ///< Stock market crashed
	MARKET_BOOM,                 ///< Bull market
	SCANDAL_REVEALED,            ///< Company scandal exposed

	/* Milestones */
	PLAYER_FIRST_MILLION,        ///< Player reached $1M
	PLAYER_FIRST_BILLION,        ///< Player reached $1B
	AI_FIRST_MILLION,            ///< AI reached $1M
	GAME_START,                  ///< New game started
	YEAR_END,                    ///< Year summary

	NUM_TRIGGERS
};

/** Dialogue entry with context */
struct CEODialogue {
	std::string text;
	CEOPersonality personality;
	DialogueTrigger trigger;
	int8_t mood_modifier;        ///< How this affects player opinion of AI
};

/** Get dialogue for a trigger and personality */
std::string GetCEODialogue(CEOPersonality personality, DialogueTrigger trigger, const std::string &player_name = "");

/** Get a taunt when AI is feeling aggressive */
std::string GetCEOTaunt(CEOPersonality personality);

/** Get a compliment when AI respects player */
std::string GetCEOCompliment(CEOPersonality personality);

/** Get dialogue for when AI is defeated/taken over */
std::string GetCEODefeatDialogue(CEOPersonality personality);

/** Get dialogue for when AI achieves victory */
std::string GetCEOVictoryDialogue(CEOPersonality personality);

/** Trigger dialogue event and show to player */
void TriggerCEODialogue(CompanyID ai_company, DialogueTrigger trigger);

/** CEO quotes database - iconic lines by personality */
namespace CEOQuotes {

	/* Gordon Gekko style */
	constexpr const char *GEKKO_GREED[] = {
		"Greed, for lack of a better word, is good.",
		"Money never sleeps, pal.",
		"If you need a friend, get a dog.",
		"It's not a question of enough. It's a zero-sum game.",
		"I create nothing. I own.",
		"You're not naive enough to think we're living in a democracy, are you?",
		"This is your wake-up call, pal. Go to work.",
	};

	constexpr const char *GEKKO_TAKEOVER[] = {
		"I don't throw darts at a board. I bet on sure things.",
		"Ever wonder why fund managers can't beat the S&P 500? Because they're sheep.",
		"The point is, ladies and gentlemen, that greed clarifies.",
		"You want another chance? You got it. Don't disappoint me.",
	};

	constexpr const char *GEKKO_THREAT[] = {
		"You gonna do the right thing here, or do we have a problem?",
		"I'm gonna make you rich. Or I'm gonna destroy you.",
		"When I get a hold of you, you're gonna wish you never came out of that cave.",
		"You could have been one of the great ones. Instead, you're nothing.",
	};

	/* Warren Buffett style */
	constexpr const char *BUFFETT_WISDOM[] = {
		"Price is what you pay. Value is what you get.",
		"Be fearful when others are greedy, and greedy when others are fearful.",
		"Rule No. 1: Never lose money. Rule No. 2: Never forget rule No. 1.",
		"Time is the friend of the wonderful company, the enemy of the mediocre.",
		"It's far better to buy a wonderful company at a fair price.",
		"Our favorite holding period is forever.",
		"Risk comes from not knowing what you're doing.",
	};

	constexpr const char *BUFFETT_PATIENT[] = {
		"The stock market is designed to transfer money from the active to the patient.",
		"I will tell you how to become rich: Be fearful when others are greedy.",
		"Opportunities come infrequently. When it rains gold, put out the bucket.",
		"We simply attempt to be fearful when others are greedy.",
	};

	/* Elon Musk style */
	constexpr const char *MUSK_VISIONARY[] = {
		"When something is important enough, you do it even if the odds are against you.",
		"I think it's possible for ordinary people to choose to be extraordinary.",
		"Persistence is very important. You should not give up unless you are forced to.",
		"Some people don't like change, but you need to embrace it.",
		"The first step is to establish that something is possible; then probability will occur.",
		"I'd rather be optimistic and wrong than pessimistic and right.",
	};

	constexpr const char *MUSK_BOLD[] = {
		"If you're not failing, you're not innovating enough.",
		"I think that's the single best piece of advice: constantly think about how you could be doing things better.",
		"Starting a company is like staring into the abyss and eating glass.",
		"Work like hell. Put in 100 hour weeks. This improves the odds of success.",
	};

	/* Vanderbilt style */
	constexpr const char *VANDERBILT_RUTHLESS[] = {
		"What do I care about the law? I got the power, ain't I?",
		"You have undertaken to cheat me. I won't sue you, for the law is too slow. I'll ruin you.",
		"I don't care half so much about making money as I do about making my point.",
		"Never tell anyone what you're going to do till you've done it.",
	};

	constexpr const char *VANDERBILT_DOMINANT[] = {
		"I have been insane on the subject of moneymaking all my life.",
		"What do I care about law? Ain't I got the power?",
		"You have undertaken to ruin me. I will not sue you, I will ruin you.",
	};

	/* Rockefeller style */
	constexpr const char *ROCKEFELLER_MONOPOLY[] = {
		"Competition is a sin.",
		"The way to make money is to buy when blood is running in the streets.",
		"I always tried to turn every disaster into an opportunity.",
		"Don't be afraid to give up the good to go for the great.",
		"I believe in the supreme worth of the individual.",
		"The growth of a large business is merely a survival of the fittest.",
	};

	constexpr const char *ROCKEFELLER_CALCULATING[] = {
		"I have ways of making money that you know nothing of.",
		"Good leadership consists of showing average people how to do the work of superior people.",
		"The secret of success is to do common things uncommonly well.",
	};

	/* Richard Branson style */
	constexpr const char *BRANSON_ADVENTURER[] = {
		"Screw it, let's do it!",
		"Business opportunities are like buses; there's always another one coming.",
		"You don't learn to walk by following rules. You learn by doing and falling over.",
		"Train people well enough so they can leave. Treat them well enough so they don't want to.",
		"Respect is how to treat everyone, not just those you want to impress.",
		"The best way of learning about anything is by doing.",
	};

	constexpr const char *BRANSON_FUN[] = {
		"Fun is one of the most important – and underrated – ingredients in any successful venture.",
		"I don't think of work as work and play as play. It's all living.",
		"My general attitude to life is to enjoy every minute of every day.",
	};

	/* Generic responses by emotion */
	constexpr const char *GENERIC_ANGRY[] = {
		"This isn't over.",
		"You've made a powerful enemy today.",
		"Enjoy it while it lasts.",
		"I'll remember this.",
		"You have no idea who you're dealing with.",
	};

	constexpr const char *GENERIC_DEFEATED[] = {
		"You win this round.",
		"Well played. Well played indeed.",
		"I underestimated you.",
		"This changes nothing in the long run.",
		"We'll see who's laughing next quarter.",
	};

	constexpr const char *GENERIC_VICTORIOUS[] = {
		"As I expected.",
		"This is just the beginning.",
		"The market has spoken.",
		"Perhaps now you understand.",
		"I told you so.",
	};

	constexpr const char *GENERIC_RESPECT[] = {
		"You're a worthy competitor.",
		"I see potential in you.",
		"Not bad. Not bad at all.",
		"You remind me of myself, years ago.",
		"Keep this up and you'll go far.",
	};
}

#endif /* CEO_DIALOGUE_H */
