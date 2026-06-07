# [Working title: HOSTILE] — v1 Game Design Document

*A transport-and-business tycoon where you can win two ways: **build the best empire**, or **take everyone else's.** Operating and raiding are equally valid paths to victory, and they pull against each other — every dollar you spend on a beautiful network is a dollar not defending your stock.*

Status: v1 design draft · Companion to `TYCOON_DESIGN_REFERENCE.md`

---

## 1. The pitch (one paragraph)

You run a company in a living economy of towns, industries, and rivals. Move cargo and passengers to earn money and grow your network — classic transport tycoon. But your company is also **publicly traded**, and so is everyone else's. Profits raise your share price; debt and missed routes sink it. When a rival stumbles, you can **buy in** — a quiet 5% stake, a creeping 25%, a hostile run at the whole board — financed with your own cash or with **leverage** borrowed against the target's assets. Build an empire, or raid your way to the top of a holding company that owns everyone. The economy doesn't care how you win.

---

## 2. Design pillars

1. **Two honest paths.** A pure operator who never buys a share can win. A pure raider who barely lays track can win. Most players blend. The systems must make both viable and neither dominant.
2. **Legible economy.** Thresholds and feedback loops, not opaque curves. A player should always be able to explain *why* the money moved. (We inherit this from OpenTTD on purpose.)
3. **Numbers tell stories.** A share price, a debt ratio, a takeover offer — each is a little narrative. The UI surfaces these as drama, not spreadsheets.
4. **Small and sharp.** One climate, a handful of cargo and business types, one map size. Depth from the *interaction* of systems, not from content volume.

---

## 3. The two loops

### 3.1 The Operator loop (the "physics")
```
build route → deliver cargo → earn revenue → industries grow / towns grow → bigger network → repeat
```
- **Cargo payment** = `distance × speed_decay(transit_time) × cargo_rate`. Fast delivery pays full; slow delivery decays to a floor. (See reference §1.2.)
- **Industries** scale production up when you transport >60% of their output, down (and eventually close) when you neglect them.
- **Towns** grow only when their delivery goals (passengers/mail/goods) are met, creating self-reinforcing city networks.
- **Costs**: build cost up front, running costs daily, loan interest monthly, station upkeep monthly.

This loop alone is a complete, shippable game. Everything below is the second game stacked on top.

### 3.2 The Raider loop (the new layer)
```
spot a weak/undervalued rival → accumulate shares (cash or leverage) →
cross ownership thresholds → influence or seize the company → absorb/strip/hold → repeat
```
- Every company has a **share price** derived from its valuation (reference §2.3).
- You buy shares on an open market. Buying pushes the price up; dumping pushes it down.
- Cross thresholds to unlock power (see §5).
- Finance it with cash, a normal loan, or a **leveraged buyout** — debt secured against the *target's* assets, not yours (see §6).

### 3.3 The tension (why "both equally" works)
- Money spent expanding your network is money **not** spent buying defensive shares in yourself — so the best operators are the most attractive *targets*.
- A raider loaded with acquisition debt has **thin cash** and high interest — vulnerable to a counter-raid or a recession.
- A great network **raises your own share price**, making you expensive to attack but also making a successful raid on you a huge prize.
- Result: building well and raiding well are in constant, deliberate friction. That friction is the game.

---

## 4. Company model (player and rivals are identical)

Each company tracks:
- **Cash**, **loan**, **loan cap**.
- **Shares outstanding** (fixed total, e.g. 1,000) and a **cap table** — who owns what % (including the founder/AI and the open float).
- **Share price**, recomputed from valuation each quarter and nudged live by market trades.
- **Rolling financials** — last several quarters of profit/revenue/cargo, used for valuation and AI decisions.
- **Board control state** — who, if anyone, holds >50%.

### Three valuations (drive price and offers) — from reference §2.3
- **Asset value** = vehicles (×1.5 book) + stations/infrastructure.
- **Net worth** = assets − loan + cash. *Drives the score and the baseline share price.*
- **Takeover value** = assets + assumed debt + covered deficit + ~2 years forward profit. *Drives the premium a raider pays for control.*

The gap between net worth and takeover value **is the control premium** — the extra you pay to own the future, not just the steel. Players learn to feel this gap.

---

## 5. The ownership ladder (core of the new game)

Crossing a threshold of a company's shares unlocks escalating power:

| Stake | Name | What it grants |
| --- | --- | --- |
| 5% | **Toehold** | Visibility into the target's detailed finances; must publicly disclose (alerts them) |
| 25% | **Blocking stake** | Veto big moves (new debt, asset sales); collect dividends; signals intent |
| 50%+1 | **Control** | Win a **board vote**: redirect the company, merge it into yours, or strip it |
| 100% | **Full ownership** | Absorb entirely — assets fold into you, the company dissolves |

- Accumulating quietly toward 25% is a **creeping takeover**. Crossing 5% forces disclosure — a deliberate risk/tell.
- At 50%, you don't auto-merge; you **win control** and choose: operate it as a subsidiary (keep its cash flow), **merge** it (absorb assets, reference §2.4 transfer rules), or **asset-strip** (sell its best routes, pocket the cash, let the husk fail).
- **Defenders** can fight back: buy back their own float, issue new shares to dilute you (a "poison pill"), find a **white knight** third party, or simply out-earn the interest clock you're racing.

This ladder is the entire net-new design space OpenTTD never had. It's where most iteration time goes.

---

## 6. Leverage & the LBO (the signature mechanic)

Three ways to finance a raid:
1. **Cash** — safe, slow, limited by your bank balance.
2. **Corporate loan** — borrow against *your* assets; interest hits *your* books.
3. **Leveraged buyout** — borrow against the **target's** assets to buy the target. The classic move.

**How the LBO resolves:**
- On gaining control, the acquisition debt **transfers onto the acquired company's balance sheet**.
- If the target's cash flow services that debt → you've bought a company essentially with its own money. Brutal and brilliant.
- If it can't → the subsidiary slides toward bankruptcy (reference §2.2 trigger: `cash − loan < −max_loan`), and you either inject cash, strip assets fast, or let it collapse and eat the reputation hit.

**Risk knobs that keep it from being free money:** interest scales with how aggressive the leverage is; over-leveraged companies are visibly fragile (rivals/AI target them); a **recession** (the economy's existing global demand-dip cycle, reference §1.5) can flip a healthy LBO into a death spiral. *Timing raids against the economic cycle becomes a skill.*

---

## 7. AI rivals (personalities, not scripts — for v1)

Rather than OpenTTD's full scripting VM, v1 ships a few hand-built **personalities** that make the world feel alive and create raid targets/threats:

- **The Operator** — builds relentlessly, ignores the stock game. Becomes a fat, profitable *target*.
- **The Raider** — under-builds, hoards cash, pounces on weakness. Your direct rival; will raid *you*.
- **The Turtle** — over-defends its own stock, grows slowly, hard to crack.
- **The Gambler** — over-leverages for fast growth; spectacular or bankrupt.

Borrow two patterns from OpenTTD's framework (reference §3): a **per-tick action budget** so many rivals run smoothly, and **dry-run pricing** so an AI (and the player's UI) can evaluate an acquisition's cost before committing. Scriptable/moddable AI is a great *post-launch* feature — not v1.

---

## 8. Victory & session shape

- **Mode: Hostile (default).** Fixed time horizon (e.g. 50 in-game years). **Score = your net worth + your share of every company you partly own.** This single score rewards both paths: a master builder scores on net worth; a raider scores on the holding-company value of stakes they control.
- **Instant win:** control >50% of every surviving company (you *are* the market).
- **Loss:** your own company goes bankrupt, or you're taken over.
- **Sandbox mode:** no clock, just build — for the operator-at-heart audience.

---

## 9. v1 scope (ruthlessly limited)

**In:**
- One climate, one map size, ~6 cargo types, ~5 industry types, towns.
- Road + rail transport (defer ships/air to keep the build small).
- Full single-company finance: loans, interest, bankruptcy.
- The **share market, ownership ladder, and LBO** — the differentiator.
- 4 AI personalities. Hostile mode + sandbox mode.
- Minimalist art (one cohesive low-poly or clean-2D style; asset libraries; *Mini Motorways*-grade restraint).

**Explicitly deferred to expansions:**
- Extra business types (casinos, data centers, racetracks, restaurants) — each is a DLC-shaped chunk.
- Ships, aircraft, multiple climates.
- Multiplayer.
- Moddable scripting / Steam Workshop.
- Proxy fights, complex derivatives, IPOs of new companies.

**Cut for good (from the original brief):** Unreal Engine, free-to-play, in-game ads, "metaverse." v1 is a **paid premium game on Steam** (~$20), expansions later.

---

## 10. Build order

1. **Operator loop** — cargo/industry/town economy. Prove it's fun to run a route.
2. **Single-company finance** — cash, loans, interest, bankruptcy, basic AI that builds.
3. **Valuation + share price + market** — companies become tradeable. Now the world has a stock layer.
4. **Ownership ladder + control outcomes** — toehold → 25% → 50% → merge/strip.
5. **Leverage / LBO** — the signature risk.
6. **AI personalities + victory/score + polish.**

Steps 1–2 = OpenTTD-lite (already a game). Steps 3–5 = the reason anyone buys ours.

---

## 11. Open questions to resolve next

- **Share liquidity model:** fixed-impact (each trade moves price a set %) or a simple order book? Lean fixed-impact for legibility.
- **How visible is the cap table?** Full transparency creates more drama and counterplay; hidden stakes enable surprise raids. Probably: stakes hidden below 5%, disclosed above.
- **Dividend vs. reinvestment:** do owned subsidiaries pay you cash, or just add to your net worth? Cash dividends make the raider path more *active*.
- **Recession severity:** how punishing should the cycle be? It's the natural counter to reckless leverage — tune it as the difficulty dial.

---

*Suggested next step: prototype the Operator loop (§3.1) as a tiny playable build to confirm the core is fun before we invest in the financial layer — the order that de-risks the project.*
