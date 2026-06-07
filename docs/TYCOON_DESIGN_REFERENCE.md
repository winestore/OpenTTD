# Transport-Tycoon-Inspired Game — Systems Reference

*A clean-room study of how OpenTTD models its economy, companies, and AI rivals — written as a foundation for building our own original game (Path A). This documents **mechanics and formulas**, not code to copy. The goal: learn the systems, then write our own clean implementation, and add the corporate-warfare layer OpenTTD never had.*

---

## 0. The big strategic finding

OpenTTD already contains the seed of a corporate-warfare game, but stops short of the part we care about:

| Mechanic | In OpenTTD? | Our opportunity |
| --- | --- | --- |
| Hostile takeover of a rival | **Yes** — `CalculateHostileTakeoverValue()` | Make it the *core loop*, not an afterthought |
| Bankruptcy → buyout timeline | **Yes** — month 4/7/10 stages | Keep — it's good drama |
| Company valuation (assets + profit) | **Yes** | Keep the formula shape |
| Loans / interest / debt-driven bankruptcy | **Yes** | Extend into **leverage** as a strategy |
| **Fractional shares (25/50/75% stakes)** | **No** (removed years ago) | **This is our headline feature** |
| **Leveraged buyouts (debt-financed raids)** | **No** | **Our headline feature** |
| Creeping acquisition / proxy fights | **No** | Net-new design space |

**Takeaway:** we are not reinventing the genre. We are bolting a *financial / ownership layer* onto a proven transport-sim economy. The transport sim is the "physics"; the corporate warfare is the new game on top.

---

## 1. The Economy (the "physics" layer — borrow this wholesale)

OpenTTD's economy is deliberately simple: **no supply/demand curves, just thresholds and feedback loops.** That's a feature — it's legible to players and cheap to simulate. Recommend we keep this philosophy.

### 1.1 The core feedback loop
```
deliver cargo → industry produces more → town grows → more demand → repeat
```
Everything below serves this loop.

### 1.2 Cargo payment (the money faucet)
Payment for a delivered load:
```
payment = distance × time_factor × num_pieces × cargo_rate / 2^21
```
- **distance** — tiles traveled, capped at straight-line source→destination.
- **time_factor** — decays with transit time. Fast delivery pays full (max 255); slow delivery decays in tiers down to a floor (~31). This is what makes speed matter.
- **cargo_rate** — per-cargo base rate (passengers 3185, valuables 7509, etc.), scaled by inflation.
- Each cargo has two "transit period" thresholds controlling how fast its payout decays — e.g. valuables `[1,32]` decay fast (deliver quick or it's worthless), coal `[7,255]` is patient.

**Design note:** this single formula creates the whole risk/reward of route design. Worth replicating almost exactly, then tuning the cargo table for our new business types.

### 1.3 Industries (production with a throttle)
- Each industry has a `prod_level` (0–128, default 16). Actual output = `base_rate × prod_level / 16`.
- Delivering input cargo converts to output via integer multiplier tables.
- **Production reacts to how much you transport:**
  - Transport >60% of output → production tends to *grow*.
  - Transport little → production *shrinks*, and processing industries that get no cargo for ~5 years *close*.
- Two modes: "Original" (chunky monthly 2×/÷2 jumps) and "Smooth" (gradual ±3–23% drift). Smooth feels more modern.

### 1.4 Towns (the demand sink)
- Towns grow only when **cargo delivery goals are met** — passengers, mail, goods, and (in some climates) food/water each have separate goals.
- Hit the goals → town expands → more stations' worth of demand. Miss them → stagnation.
- This is the lever that makes passenger/city networks self-reinforcing.

### 1.5 The periodic loop
Runs monthly:
- **Inflation** — two separate tracks (build prices vs. cargo payments), compounded monthly, frozen after ~170 years to stay playable.
- **Company finances** — interest charged, station maintenance deducted.
- **Recession cycles** — a counter-based random walk that globally dampens demand ~20% during downturns. *Note: this global economic cycle is a natural hook for our LBO timing game — buy cheap in the recession.*
- **Vehicle running costs** — accrue daily per vehicle, scaled by difficulty.

### 1.6 Difficulty = global multipliers
Almost all difficulty is uniform multipliers: construction cost ×0.5–4, running cost ×0.5–4, interest 2–12%, subsidy bonus +0% to ×4. Simple and effective — recommend we copy this approach rather than hand-tuning per-system.

---

## 2. Companies, Finance & Takeovers (extend this — it's our game)

### 2.1 What a company tracks
- **Cash** (`money`), **loan** (`current_loan`), **loan cap** (`max_loan`, inflation-scaled).
- **Rolling history**: last 24 quarters of income/expenses/delivered-cargo/performance/value, plus 3 years of categorized expenses.
- **Bankruptcy state**: `months_of_bankruptcy`, who's been offered a buyout, offer timeout, offer price.
- **Infrastructure counts** (rail/road/stations/etc.) — costs derived from these, not stored.

### 2.2 Loans & interest (the seed of leverage)
- Start with ~100,000 (inflation-adjusted), borrow up to `max_loan` in 10,000 steps.
- Interest charged monthly: `loan × rate / 100`, apportioned across the year. **Negative cash is also charged interest** — you can't hide a deficit.
- **Bankruptcy trigger:** `cash − loan < −max_loan`. Recover above that line and the counter resets.

> **Our extension:** OpenTTD caps leverage at a flat loan limit. Our game makes *leverage itself the strategy* — borrow against a target's own assets to buy it (an LBO), with the debt load and interest as the risk. The bankruptcy formula above is exactly the failure condition for an over-leveraged raider.

### 2.3 Three valuations (this is the heart of the financial game)

**Asset value** (book value of what you physically own):
```
asset_value = Σ stations(facilities × station_price × 25)
            + Σ vehicles(vehicle_value × 1.5)
```

**Company value** (net worth, used for score):
```
company_value = asset_value − current_loan + cash   (min 1)
```

**Hostile-takeover value** (what a raider pays):
```
takeover_value = asset_value
               + current_loan        (you assume their debt)
               + max(0, −cash)        (you cover their deficit)
               + Σ last 4 quarters: max(quarterly_profit, 0) × 2   (≈ 2 yrs forward profit)
```

The takeover price is *much* higher than the bankruptcy price because you pay for **future earnings**, not just steel on the ground. **This single formula is the most reusable idea in the whole codebase for us** — it already prices a company like a real acquisition (assets + assumed debt + a profit multiple). Our share system just sells *fractions* of this number.

### 2.4 The bankruptcy → buyout drama (keep this timeline)

| Month insolvent | What happens |
| --- | --- |
| 0–3 | Counter ticks up, no public action |
| **4** | Public **warning**; rivals/AI notified |
| **7** | Company **put up for sale** at its (loan-free) asset value; offered to the **best-performing rival first** |
| 8–9 | Each rival gets a timed window to accept; on decline, next-best is asked |
| **10** | If unsold → **liquidated** (assets deleted) |

On acceptance, *everything* transfers atomically: vehicles (renumbered, regrouped, recolored), stations/track, town ratings (best-of), subsidies, exclusive rights. Acquired company is deleted — no "spin back out."

> **Our extensions to this drama:**
> - Insert **shares**: instead of all-or-nothing, accumulate a stake over time (creeping takeover).
> - **Proxy fights / board control** at 50%.
> - **White-knight rescues**: a third party outbids the raider during the month 7–10 window.
> - **Asset stripping**: buy, sell the valuable routes, let the husk fail — classic LBO villainy.

### 2.5 What's missing (our whole design space)
OpenTTD has **no** shares, no partial ownership, no debt-financed acquisition, no board/voting. Hostile takeover there is single-player-only, AI-target-only, and all-or-nothing. **Every one of those limits is a feature we get to invent.**

---

## 3. AI Rivals (architecture to learn from, not necessarily copy)

### 3.1 How OpenTTD does it
Each AI company is a **sandboxed Squirrel script** running in its own VM inside the company object. The engine:
- Calls every AI's `GameLoop()` each tick, **rate-limited by a "competitor speed" setting** (run every 1–16 frames).
- Gives each AI a finite **opcode budget** per tick; when spent, the script *suspends* and resumes next tick. This guarantees one AI can't stall the game.
- Exposes a huge API (~100 classes: `AIRail`, `AIVehicle`, `AICompany`, `AIIndustry`, events, plus a **test mode** to dry-run a command's cost before committing).
- A separate single **GameScript** (neutral, not a company) sets goals/events for everyone.

### 3.2 What's worth borrowing
- **The opcode/tick-budget pattern** — clean way to run many autonomous agents in a real-time sim without frame spikes. Worth replicating conceptually.
- **Test-mode commands** — agents (and our tooling) can price an action before taking it. Maps perfectly onto "evaluate this acquisition before bidding."
- **Event system** — AIs react to `CompanyInTrouble`, `AskMerger`, etc. Our raiders react to "rival over-leveraged" / "stake crossed 25%."

### 3.3 What we'd do differently
- A full embedded scripting VM is heavyweight for a v1. Consider **native behavior-tree / utility-AI rivals** with a few distinct "personalities" (the cautious operator, the aggressive raider, the cash-hoarder) rather than a moddable script platform. Scriptable AI is a great *post-launch / modding* feature, not a launch requirement.
- For the corporate-warfare game, the AI's interesting decisions are *financial* (when to raid, when to defend, when to issue debt), not *pathfinding*. The hard AI problem moves up the stack.

---

## 4. Recommended build order (v1 scope)

1. **Transport physics** (§1): cargo payment formula, a handful of industries, town growth. One climate, ~6 cargo types. This is the "is it fun to run a route" core.
2. **Single-company finance** (§2.1–2.2): cash, loans, interest, running costs, bankruptcy condition.
3. **Valuation + AI rivals** (§2.3, §3): rivals that build routes and can go bankrupt. Now it's OpenTTD-lite — already shippable.
4. **The new layer** (§2.4–2.5): shares, leverage/LBOs, hostile stakes, board control. *This* is the differentiator and where most design iteration goes.
5. **Defer**: extra business types (casinos/data centers/etc.), moddable scripting, multiplayer. Each is a post-v1 expansion, not launch scope.

**Engine reminder:** Godot or Unity for a 2.5D management game — not Unreal. Lean on a deliberate minimalist art style (think *Mini Motorways*) so we ship without a full art team.

---

## 5. File map (where each system lives in OpenTTD, for further study)

| System | Key files |
| --- | --- |
| Cargo payment | `src/economy.cpp` (956–1015, 1210–1242), `src/table/cargo_const.h` |
| Industries | `src/industry_cmd.cpp` (2592–3044), `src/industrytype.h` |
| Towns | `src/town_cmd.cpp` (3929–3965) |
| Monthly loop / inflation / interest | `src/economy.cpp` (725–855, 1961–1977) |
| Company model | `src/company_base.h`, `src/company_type.h` |
| Valuation & takeover | `src/economy.cpp` (115–192, 550–635), `src/company_cmd.cpp` (718–769) |
| AI framework | `src/ai/` (`ai_core.cpp`, `ai_instance.cpp`), `src/script/`, `src/script/api/` |

---

*Next suggested step: turn §2.4–2.5 into a one-page design doc for the share/LBO loop — the part of the game that doesn't exist anywhere yet.*
