# HOSTILE — Design & Build Blueprint (Waves 2 & 3 Synthesis)

*The consolidated output of seven parallel design agents (four on "how we make it
better," three on design/UI/UX), grounded on the Wave 1 engine review. This is the
plan of record for building the game. Engine: **Godot + C# for the deterministic
simulation core.** Companion to `ENGINE_REVIEW.md`, `GAME_DESIGN_V1.md`,
`TYCOON_DESIGN_REFERENCE.md`.*

---

## 0. The blueprint in one page

**What we're building:** a transport-and-business tycoon where you win by **building
the best network OR raiding everyone else's** — the two paths in permanent tension. The
transport sim is the proven "physics"; the corporate-warfare layer (shares, leverage,
hostile takeovers) is the differentiator no competitor has.

**The decisions that now hold across all seven reviews:**

1. **C# deterministic core, hard-walled from Godot presentation.** Command pattern
   (test→execute→log), integer money (cents), 30 ticks/sec, seeded RNG. Rendering can
   never touch sim state. This makes replays/undo/AI/multiplayer additive later.
2. **The economy gets a heartbeat: the recession cycle.** A visible economic indicator
   ties the transport economy and the takeover game into one rhythm — raiders hoard in
   the boom and strike when valuations crater.
3. **The financial layer is fully specced, including its failure modes.** Share price,
   ownership ladder, LBOs with leverage-scaled interest and a hard cap, defender
   counterplay, a reputation system, and explicit mitigations for the five ways the
   game could break.
4. **Rivals are utility-AI "personalities," not a scripting VM.** Difficulty scales by
   tuning behavior, not handing the AI free money. Raiders become raid targets when
   over-leveraged.
5. **The art is the financial dashboard.** Clean 2.5D minimalism, ~60 library meshes
   recolored per company, no custom-art team. The takeover screen is the Steam
   screenshot.
6. **Onboarding teaches transport first, finance second.** Premium **$19.99 on Steam**,
   DLC for the deferred business types. Free-to-play/ads/metaverse stay cut.
7. **~6-9 month v1 with a 5-6 person team, on a 7-sprint plan** that needs no mid-project
   rework. There is a real ~$900K funding-gap reality check with four ways to close it.

---

## 1. Simulation architecture (the C# spine)

**The skeleton.** One `GameState` holds all deterministic state. Every mutation is a
`Command` with `Test(state) → (success, costCents)` then `Execute(state)`, and is logged.
The tick loop runs: process command queue → simulate (cargo/production/towns/finance) →
AI (budgeted) → advance clock. **No rendering in the loop.**

**Determinism conventions (non-negotiable):**
- Money in **cents** (`long`); position in **1/256-tile** units. No floats in sim state.
- RNG seeded from `(tick, category)` — never `randf()`.
- Sort before iterating any dictionary/collection in the sim.
- Side-effects deferred to tick boundaries; `Test` and `Execute` share logic, only
  mutations are guarded by `if (!test)`.

**Save/load:** chunked, **named fields** (reflection-based, never positional), per-chunk
schema version, a `FixupPointers` pass after all chunks load. Hand-rolled binary (5-10 MB
vs. 200 MB JSON), disciplined versioning, old saves kept in CI.

**Presentation boundary:** a `SimulationBridge` exposes a read-only state delta to Godot
each tick and accepts `Command` objects from the UI. Godot interpolates between tick
states for smooth 60 FPS over a 30 Hz sim.

**Multiplayer-ready without building it:** commands are already serializable + deterministic,
so a future `NetworkCommandDispatcher` (lockstep, server-ordered) bolts on with **zero
changes** to `GameState`, `Command`, or the tick loop.

**Highest-risk calls & rulings:** money=cents (ruled), tick=30/sec (ruled), save=hand-rolled
binary (ruled), determinism audit = daily state-hash + quarterly CI replay test (ruled),
dispatch = local-optimistic for v1 with rollback (ruled).

---

## 2. Economy & gameplay depth (better than OpenTTD, still legible)

Keep OpenTTD's legible threshold model; add depth in five places. **Phase-1 (v1)** picks
are the high-ROI ones; the rest are post-launch.

| Improvement | What it adds | Phase |
| --- | --- | --- |
| **Demand-based cargo rates** | A visible demand multiplier (0.5×–2×) turns routing into timing/portfolio decisions | v1 |
| **5-tier industry response** | Depleted→Struggling→Healthy→Booming→Saturated (vs. binary), shown as a health meter — lets you optimize to *any* level | v1 |
| **Economic indicator + visible recession** | The game's heartbeat: a readable gauge so downturns are a *tool* (buy low) not a random debuff | v1 |
| **Rival crisis events** | Telegraphed financial stress on rivals → teachable takeover openings, no surprise bankruptcies | v1 |
| **Town growth momentum** | Boom towns and ghost towns; cities worth fighting over | v1 |
| Contracts, perishables, seasonal swings, regional booms, city specialization | More texture | post-v1 |

**Legibility safeguards (mandatory):** every hidden number is surfaced with a tooltip;
all changes (recession, demand shifts, closures) get 3-12 month lead-time warnings; a
quarterly news summary narrates what changed. Depth must never become spreadsheet soup.

---

## 3. The financial layer (the headline — fully specced)

**Share price** = `(net_worth / shares) × profitability_multiplier(0.8–1.2) × sentiment_jitter(±~10%)`.
Profitable operators trade at a premium → they become expensive, juicy targets.

**Liquidity:** fixed-impact model — buying N shares moves price by `+0.15·√(N/total)`.
Creeping toward 25% gets visibly, increasingly expensive and telegraphs intent.

**Ownership ladder:** 5% **toehold** (public disclosure — a real tell) → 25% **blocking**
(veto big moves, board seat) → 50%+1 **control** (merge / operate / asset-strip) → 100%
**absorb**. Disclosure at 5/25/50% mirrors real securities law.

**LBO (signature):** three financing paths — cash, corporate loan (against *your* assets),
or LBO (against the *target's* assets, debt transfers on control). Interest scales with
leverage: `base 3% × (1 + 0.5·leverage_ratio)`; **hard cap at 200% debt-to-assets** (bank
refuses beyond). Over-leverage + recession = death spiral. Buy a company with its own money
— if its cash flow can service the debt.

**Defender counterplay:** share buybacks (can shove a raider past a threshold
involuntarily), poison-pill dilution, white-knight rescue, staggered board.

**Reputation system (0-100):** asset-stripping and failed raids cost reputation; discipline
and successful defense gain it. Reputation gates white-knight cooperation and loan rates —
turning "villain" play into a real, costed choice.

**The five degeneracy risks, each mitigated:** raider snowball (leverage cap tightens with
portfolio size), infinite cheap leverage (credit rating caps total debt), asset-strip
free-wins (reputation hit + clawback + annual sale limit), kingmaking/collusion (white-knight
cooldown + forced bid auctions), and "building = being a target" (dividends + staggered
board + reputation give operators real defenses).

**Scoring ties both paths together:** `score = net_worth + Σ(stake% × company_value)`. By
year 50 a master operator (~$50M net worth) and a disciplined raider (a holding company of
stakes) land in the same ballpark — the "both equally" balance, quantified.

---

## 4. Rival AI (utility personalities)

**Architecture:** hybrid utility AI (chosen over behavior trees and a scripting VM — fastest
to ship and tune in C#). Each rival is a small parameter struct. Borrow two patterns from
OpenTTD: a per-tick action budget (smooth with many rivals) and test-mode pricing (evaluate
an acquisition before committing — the same `Command.Test` the player UI uses).

**The interesting decisions are financial,** scored by utility: build vs. reinforce, when to
raid a weak rival, when to take leverage, when to defend own stock.

**Four launch personalities:** Operator (builds fat, becomes a target), Raider (hoards cash,
pounces — your direct rival), Turtle (over-defends, hard to crack), Gambler (over-levers,
blows up spectacularly in recessions). Their interactions create varied, replayable games.

**Difficulty = behavior tuning, not free money** (parameter shifts + personality mix + loan
cap), so you beat the AI by outplaying it. **Defense closes the loop:** raid an AI and it
buys back, dilutes, or finds a white knight — and an over-leveraged raider is itself prey.

---

## 5. Art direction (premium look, no artist)

**Style:** clean 2.5D isometric minimalism — *Mini Motorways* meets a Bloomberg Terminal.
Flat-shaded geometry, crisp outlines, no gradients. **The game's visual story is its
financial dashboard,** not the map — the animated takeover screen (ownership bar filling,
a rival's leverage ratio turning orange) is the hero screenshot.

**Asset strategy:** ~60 base meshes from libraries (Kenney/Synty/Quaternius), recolored per
company via a shader override (one mesh → many liveries). Procedural terrain/trees. **AI 3D
generators are unreliable for assets** — use AI only for mood boards.

**Color is the UI language:** company colors for ownership; green/red for profit/loss; orange
escalating to red for leverage risk. Don't color-code cargo (icons + company color instead).
Monospace numbers so values don't wobble when they update.

**Look-dev path:** ~10 weeks from our colored-shapes prototype to three Steam-ready hero
screenshots (gameplay, the takeover modal, the holding-company tree).

---

## 6. UI/UX (and the signature takeover flow)

**Five core screens:** the **Map** (persistent, left 60-70%), the **Financial Cockpit**
(right panel: financials / graphs / vehicles / infrastructure), the **Share Market** (company
table + cap tables), the **Takeover Panel** (slides up when you cross 5%), and a minimal
**Operating Panel**. Dark theme, responsive, sparklines everywhere, a "?" on every formula.

**The takeover experience — the signature moment — as an 8-beat flow:**
1. **Spot** a vulnerable rival in the Share Market (price dipping, debt high).
2. **Creep** — accumulate quietly toward 5% (slow-buy option to mask price impact).
3. **Disclosure** — crossing 5% fires a breaking-news banner; the AI is notified and reacts.
4. **Build the stake** toward 25%, choosing cash / loan / LBO financing with live cost+risk.
5. **The fight** — defender deploys poison pill / white knight / buyback in real time.
6. **Pressure cooker** — hold and let interest bleed them, with a price alert to re-engage.
7. **Checkmate** — escalate past 50%+1.
8. **Resolution modal** — choose **Merge / Operate / Strip**, each with clear consequences.

**Build-vs-raid tension is always on screen:** a "Strategic Position" view splits your
capital into build vs. raid pools; your net-worth card color-codes fragility (green operator
→ red fragile raider); a vulnerability scanner ranks rivals and predicts the next fire-sale.

**Onboarding via progressive disclosure:** days 1-5 transport only → days 6-20 finance →
days 21+ the stock market → day 30+ an optional first-takeover tutorial. A "Finance Depth"
setting (Simple/Moderate/Complex) lets operators hide LBO machinery.

**Godot mapping:** native Control nodes; custom `_draw()` controls for sparklines, the cap-table
pie, and the escalation ladder; a clean signal architecture (GameState signals → UI listeners,
UI → Command objects).

---

## 7. Player experience & business plan

**Session shape (50-year campaign):** years 1-5 foundation (passive rivals) → 6-15 the board
wakes up (first raids, first recession) → 16-30 the raid era → 31-50 consolidation endgame.
Recessions are the recurring stress test; a tension beat every ~5 years.

**Replayability:** procedural map seeds + random personality mixes + four difficulty tiers +
hand-crafted scenarios + cosmetic (not pay-to-win) progression.

**Monetization (honest):** premium **$19.99** base; **$9.99 DLC** expansions for the deferred
business types (Casinos, Data Centers/Logistics, Agriculture+futures), optional season pass.
Comps: Mini Metro, Two Point Hospital (6 DLCs over 3 years). **Free-to-play/ads/metaverse stay
cut** — ads kill long-session flow; niche sims never reach the DAU an ad model needs; blockchain
adds cost and kills word-of-mouth.

**The reality check (stated plainly):** a naive 5.75-FTE / 30-month plan costs ~$2.1M and a
realistic Y1-3 net is ~$1.2M — a **~$900K gap**. Four real ways to close it: leaner team +
longer timeline, founder sweat equity, an indie-publisher advance (~$300-500K for ~20% of rev),
or an **Early Access** launch (~$300-400K to fund full polish + DLC). 50K Y1 units is a *good*
outcome, not a home run; the bet is that "transport + raiding" is novel enough to clear it.

---

## 8. Consolidated build plan

Synthesizing the sim-architecture sprint plan with everyone's "what's in v1":

1. **Engine spine** — `GameState`, tick loop, `Command` (test/execute/log), seeded RNG, chunked save/load. *Determinism tests from day one.*
2. **World** — tile array, Perlin heightmap, place towns/industries (no growth at gen).
3. **Operator loop** — cargo payment, 5-tier industry response, town-growth momentum, in the tick loop. *(Prototype already proves it's fun-shaped.)*
4. **Vehicles/stations/orders** — simplified ~5k-line version, **road first**, A\* with safe-waiting.
5. **Finance + rivals** — valuations, loans, the economic indicator, 4 AI personalities.
6. **The differentiator** — share market, ownership ladder, LBOs, defenses, reputation, the takeover UI flow.
7. **Polish + business** — finance cockpit, onboarding, scenarios, art look-dev, Steam page. *Defer: rail signaling, ships/air, multiplayer, modding.*

Roughly **6-9 months to a v1 alpha** at 5-6 people; no mid-project rework because the
determinism spine is paid up front.

---

## 9. Decisions still open for you

- **Funding path** (lean+slow / sweat equity / publisher / Early Access) — shapes timeline and team.
- **C# everywhere vs. C# core + GDScript UI** — the sim must be C#; the UI could be either.
- **Scope of "Finance Depth: Simple" mode** — how much of the financial game a pure operator can ignore.
- **Which deferred business type leads the first DLC** (the agents lean Casinos for contrast/flavor).
- **Title** — "HOSTILE" is the working name throughout; worth validating before a Steam page.

---

*Next concrete step: stop designing and start the **engine spine** (item 1) — a real C#
`GameState` + tick loop + `Command` with the determinism tests. That's where this stack of
blueprints becomes a running game.*
