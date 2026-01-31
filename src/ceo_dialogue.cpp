/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file ceo_dialogue.cpp Implementation of CEO Dialogue system. */

#include "stdafx.h"
#include "ceo_dialogue.h"
#include "core/random_func.hpp"
#include "company_base.h"

#include "safeguards.h"

/** Helper to pick random string from array */
template<size_t N>
static const char *PickRandom(const char *const (&arr)[N])
{
	return arr[RandomRange(N)];
}

/** Get dialogue based on personality and trigger */
std::string GetCEODialogue(CEOPersonality personality, DialogueTrigger trigger, const std::string &player_name)
{
	std::string dialogue;

	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "So, you want a piece of the action? Smart move... or very stupid.";
					break;
				case DialogueTrigger::PLAYER_TOOK_CONTROL:
					dialogue = PickRandom(CEOQuotes::GEKKO_THREAT);
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "You think you've won? I created this company from nothing!";
					break;
				case DialogueTrigger::AI_BOUGHT_PLAYER_SHARES:
					dialogue = "I'm not just buying shares. I'm buying YOUR future.";
					break;
				case DialogueTrigger::AI_TOOK_CONTROL:
					dialogue = PickRandom(CEOQuotes::GEKKO_TAKEOVER);
					break;
				case DialogueTrigger::MARKET_CRASH:
					dialogue = "Blood in the streets. Time to go shopping.";
					break;
				case DialogueTrigger::PLAYER_HUGE_PROFIT:
					dialogue = "Not bad, kid. But one good quarter doesn't make you a player.";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = PickRandom(CEOQuotes::GEKKO_GREED);
					break;
				default:
					dialogue = PickRandom(CEOQuotes::GEKKO_GREED);
					break;
			}
			break;

		case CEOPersonality::WARREN_BUFFETT:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "A wise investment, if you're in it for the long haul.";
					break;
				case DialogueTrigger::PLAYER_TOOK_CONTROL:
					dialogue = PickRandom(CEOQuotes::BUFFETT_PATIENT);
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "I see. Sometimes the tide goes out, and you see who's swimming naked.";
					break;
				case DialogueTrigger::AI_BOUGHT_PLAYER_SHARES:
					dialogue = "I like what I see in your company. Shall we build something together?";
					break;
				case DialogueTrigger::MARKET_CRASH:
					dialogue = PickRandom(CEOQuotes::BUFFETT_WISDOM);
					break;
				case DialogueTrigger::PLAYER_BANKRUPTCY_NEAR:
					dialogue = "Cash is king during troubled times. Remember that.";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = PickRandom(CEOQuotes::BUFFETT_WISDOM);
					break;
				default:
					dialogue = PickRandom(CEOQuotes::BUFFETT_WISDOM);
					break;
			}
			break;

		case CEOPersonality::ELON_MUSK:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "Welcome aboard! Hope you're ready for the ride.";
					break;
				case DialogueTrigger::PLAYER_TOOK_CONTROL:
					dialogue = "Interesting move. Let's see if you can handle the chaos.";
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "Whatever. I'll just start something new. Something better.";
					break;
				case DialogueTrigger::AI_TOOK_CONTROL:
					dialogue = "I have plans for your company. Big plans. Revolutionary plans.";
					break;
				case DialogueTrigger::MARKET_CRASH:
					dialogue = "Perfect. Now we can buy everything cheap and rebuild it properly.";
					break;
				case DialogueTrigger::PLAYER_HUGE_PROFIT:
					dialogue = "Nice! But profits are just fuel for the real mission.";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = PickRandom(CEOQuotes::MUSK_VISIONARY);
					break;
				default:
					dialogue = PickRandom(CEOQuotes::MUSK_BOLD);
					break;
			}
			break;

		case CEOPersonality::JOHN_ROCKEFELLER:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "So you wish to be partners? Very well. But I set the terms.";
					break;
				case DialogueTrigger::PLAYER_UNDERCUT_ROUTE:
					dialogue = PickRandom(CEOQuotes::ROCKEFELLER_MONOPOLY);
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "You've won a battle. The war continues.";
					break;
				case DialogueTrigger::AI_TOOK_CONTROL:
					dialogue = "Your company now serves a greater purpose. My purpose.";
					break;
				case DialogueTrigger::PLAYER_STOLE_INDUSTRY:
					dialogue = PickRandom(CEOQuotes::ROCKEFELLER_CALCULATING);
					break;
				case DialogueTrigger::GAME_START:
					dialogue = PickRandom(CEOQuotes::ROCKEFELLER_MONOPOLY);
					break;
				default:
					dialogue = PickRandom(CEOQuotes::ROCKEFELLER_CALCULATING);
					break;
			}
			break;

		case CEOPersonality::RICHARD_BRANSON:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "Brilliant! Welcome to the family!";
					break;
				case DialogueTrigger::PLAYER_TOOK_CONTROL:
					dialogue = "Well, this is an adventure I didn't expect!";
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "You got me! Fair play. Now, what's your next adventure?";
					break;
				case DialogueTrigger::AI_BOUGHT_PLAYER_SHARES:
					dialogue = "I love what you're building! Mind if I come along?";
					break;
				case DialogueTrigger::PLAYER_HUGE_PROFIT:
					dialogue = "Fantastic quarter! Success tastes sweet, doesn't it?";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = PickRandom(CEOQuotes::BRANSON_ADVENTURER);
					break;
				default:
					dialogue = PickRandom(CEOQuotes::BRANSON_FUN);
					break;
			}
			break;

		case CEOPersonality::CORNELIUS_VANDERBILT:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "Taking an interest in my railroad? Bold.";
					break;
				case DialogueTrigger::PLAYER_UNDERCUT_ROUTE:
					dialogue = PickRandom(CEOQuotes::VANDERBILT_RUTHLESS);
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "You've stolen my empire! This offense will not stand!";
					break;
				case DialogueTrigger::AI_TOOK_CONTROL:
					dialogue = PickRandom(CEOQuotes::VANDERBILT_DOMINANT);
					break;
				case DialogueTrigger::AI_UNDERCUT_ROUTE:
					dialogue = "Your route? I don't see your name on it.";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = PickRandom(CEOQuotes::VANDERBILT_RUTHLESS);
					break;
				default:
					dialogue = PickRandom(CEOQuotes::VANDERBILT_DOMINANT);
					break;
			}
			break;

		case CEOPersonality::STEADY_EDDIE:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "A solid investment. We run a tight ship here.";
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "I see. Well, I hope you maintain our standards.";
					break;
				case DialogueTrigger::MARKET_CRASH:
					dialogue = "This is why we maintain conservative reserves.";
					break;
				case DialogueTrigger::PLAYER_HUGE_PROFIT:
					dialogue = "Congratulations. Steady growth is still better, though.";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = "Let's build something reliable. No flash, just results.";
					break;
				default:
					dialogue = "Slow and steady wins the race.";
					break;
			}
			break;

		case CEOPersonality::PENNY_PINCHER:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "Fine. But we don't waste money on dividends here.";
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "At least I saved enough to retire comfortably.";
					break;
				case DialogueTrigger::AI_BOUGHT_PLAYER_SHARES:
					dialogue = "Your company has... acceptable cost structures.";
					break;
				case DialogueTrigger::MARKET_CRASH:
					dialogue = "I knew this would happen. That's why I save everything.";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = "Every penny counts. Every. Single. Penny.";
					break;
				default:
					dialogue = "Do we really need to spend money on that?";
					break;
			}
			break;

		case CEOPersonality::TECH_TITAN:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "Welcome! Wait until you see our new locomotive designs!";
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "I hope you'll continue investing in R&D.";
					break;
				case DialogueTrigger::AI_BOUGHT_PLAYER_SHARES:
					dialogue = "Your network could benefit from our technology.";
					break;
				case DialogueTrigger::PLAYER_HUGE_PROFIT:
					dialogue = "Nice! Now reinvest in better equipment!";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = "The future of transport is efficiency. Let's build it.";
					break;
				default:
					dialogue = "Have you seen the specs on the new engines?";
					break;
			}
			break;

		case CEOPersonality::RANDOM_CHAOS:
		default:
			switch (trigger) {
				case DialogueTrigger::PLAYER_BOUGHT_SHARES:
					dialogue = "Cool. Or is it? I haven't decided yet.";
					break;
				case DialogueTrigger::PLAYER_HOSTILE_TAKEOVER:
					dialogue = "Plot twist! I didn't see that coming!";
					break;
				case DialogueTrigger::MARKET_CRASH:
					dialogue = "CHAOS! I LOVE IT!";
					break;
				case DialogueTrigger::GAME_START:
					dialogue = "Let's see what happens if I press ALL the buttons!";
					break;
				default:
					dialogue = "What's the worst that could happen?";
					break;
			}
			break;
	}

	/* Substitute player name if provided */
	if (!player_name.empty()) {
		size_t pos = dialogue.find("{PLAYER}");
		if (pos != std::string::npos) {
			dialogue.replace(pos, 8, player_name);
		}
	}

	return dialogue;
}

/** Get a taunt */
std::string GetCEOTaunt(CEOPersonality personality)
{
	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
			return PickRandom(CEOQuotes::GEKKO_THREAT);
		case CEOPersonality::CORNELIUS_VANDERBILT:
			return PickRandom(CEOQuotes::VANDERBILT_RUTHLESS);
		case CEOPersonality::JOHN_ROCKEFELLER:
			return PickRandom(CEOQuotes::ROCKEFELLER_MONOPOLY);
		default:
			return PickRandom(CEOQuotes::GENERIC_ANGRY);
	}
}

/** Get a compliment */
std::string GetCEOCompliment(CEOPersonality personality)
{
	switch (personality) {
		case CEOPersonality::WARREN_BUFFETT:
			return "You've got good instincts. I respect that.";
		case CEOPersonality::RICHARD_BRANSON:
			return "Brilliant work! You're a natural!";
		case CEOPersonality::ELON_MUSK:
			return "You're thinking big. I like it.";
		default:
			return PickRandom(CEOQuotes::GENERIC_RESPECT);
	}
}

/** Get defeat dialogue */
std::string GetCEODefeatDialogue(CEOPersonality personality)
{
	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
			return "You think you've won? I'll be back. They always come back to Gekko.";
		case CEOPersonality::CORNELIUS_VANDERBILT:
			return "You've taken my railroad. But you'll never have my legacy.";
		case CEOPersonality::ELON_MUSK:
			return "Whatever. This just frees me up for my next venture.";
		case CEOPersonality::WARREN_BUFFETT:
			return "Well played. True value always reveals itself in time.";
		case CEOPersonality::RICHARD_BRANSON:
			return "What an adventure! No regrets!";
		default:
			return PickRandom(CEOQuotes::GENERIC_DEFEATED);
	}
}

/** Get victory dialogue */
std::string GetCEOVictoryDialogue(CEOPersonality personality)
{
	switch (personality) {
		case CEOPersonality::GORDON_GEKKO:
			return "Another company added to the portfolio. Who's next?";
		case CEOPersonality::CORNELIUS_VANDERBILT:
			return "The empire grows. As it always should.";
		case CEOPersonality::JOHN_ROCKEFELLER:
			return "One step closer to total market control.";
		case CEOPersonality::ELON_MUSK:
			return "This is just phase one. Wait until you see phase two!";
		case CEOPersonality::WARREN_BUFFETT:
			return "A wonderful company at a fair price. Exactly as planned.";
		default:
			return PickRandom(CEOQuotes::GENERIC_VICTORIOUS);
	}
}

/** Trigger dialogue and show notification */
void TriggerCEODialogue(CompanyID ai_company, DialogueTrigger trigger)
{
	/* Get company and personality */
	Company *c = Company::GetIfValid(ai_company);
	if (c == nullptr || !c->is_ai) return;

	/* TODO: Get personality from company's AI profile */
	/* For now, use a default based on company ID */
	CEOPersonality personality = static_cast<CEOPersonality>(
		static_cast<uint8_t>(ai_company) % static_cast<uint8_t>(CEOPersonality::NUM_PERSONALITIES)
	);

	std::string dialogue = GetCEODialogue(personality, trigger);

	/* TODO: Show as news message or notification popup */
	/* For now, this would integrate with the news system */
}
