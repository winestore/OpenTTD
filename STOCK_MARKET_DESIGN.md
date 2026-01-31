# Transport Empire: Stock Market System Design

## Overview

Add a full stock market system to OpenTTD where players can buy/sell shares of any transport company, enabling hostile takeovers, market manipulation, and Gordon Gekko-style corporate warfare.

## Core Mechanics

### 1. Share System

Each company has **100 shares** (representing 100% ownership):
- Founder starts with 100% ownership
- Shares can be sold to raise capital
- Other players/AI can buy available shares
- 51%+ ownership = controlling interest
- 75%+ ownership = forced buyout option

```cpp
// New struct in company_base.h
struct CompanyShares {
    uint8_t shares_owned[MAX_COMPANIES];  // shares owned by each company
    uint8_t shares_available;              // shares on the market
    Money share_price;                     // current market price per share
    Money share_price_history[12];         // monthly price history
    int8_t market_sentiment;               // -100 to +100, affects price
};
```

### 2. Share Price Calculation

```cpp
Money CalculateSharePrice(const Company *c) {
    Money base_value = CalculateCompanyValue(c) / 100;  // value per share

    // Performance modifier (-20% to +30% based on recent quarters)
    int perf_mod = CalculatePerformanceModifier(c);

    // Market sentiment modifier (-15% to +15%)
    int sentiment_mod = c->shares.market_sentiment / 7;

    // Speculation/volatility (random -5% to +5%)
    int volatility = RandomRange(11) - 5;

    Money price = base_value * (100 + perf_mod + sentiment_mod + volatility) / 100;
    return std::max(price, Money(1));  // minimum $1 per share
}
```

### 3. Trading Actions

**Buy Shares:**
- Player selects company → number of shares → confirm purchase
- Price = share_price * quantity + 5% broker fee
- Cannot buy more than shares_available

**Sell Shares:**
- Player selects owned shares → number to sell
- Shares go to market (shares_available)
- Price = share_price * quantity - 5% broker fee

**Issue New Shares (raise capital):**
- Company owner can issue up to 25 new shares
- Dilutes existing ownership
- Company receives (share_price * new_shares * 0.9)

### 4. Hostile Takeover

When player owns 51%+:
- Can force management decisions
- Can merge companies
- Can liquidate assets
- Can fire AI and take direct control

When player owns 75%+:
- Can force remaining shareholders to sell at 110% of market price

### 5. Market Events

Random events that affect share prices:

```cpp
enum class MarketEvent {
    SCANDAL,           // -20% price, -30 sentiment
    RECORD_PROFITS,    // +15% price, +20 sentiment
    CEO_DEPARTURE,     // -10% price, -15 sentiment
    EXPANSION_NEWS,    // +10% price, +15 sentiment
    ACCIDENT,          // -15% price, -25 sentiment
    SUBSIDY_AWARDED,   // +8% price, +10 sentiment
    STRIKE,            // -12% price, -20 sentiment
    MERGER_RUMOR,      // +5% price, +30 sentiment (volatile)
};
```

### 6. Market Manipulation (Gordon Gekko Mode)

**Pump and Dump:**
- Buy shares quietly (limit 5% per month to avoid detection)
- Spend money on "advertising" to boost sentiment
- Sell when price peaks
- Risk: Investigation if too obvious, fines

**Bear Raid:**
- Short sell (borrow shares, sell, buy back cheaper)
- Spread negative news (costs money)
- Buy back at lower price
- Risk: Unlimited losses if price rises

**Insider Trading:**
- Occasionally get tips about upcoming events
- Act on them for profit
- Risk: Investigation, heavy fines, reputation damage

### 7. Reputation System

```cpp
struct CompanyReputation {
    int8_t public_opinion;      // -100 to +100
    int8_t investor_confidence; // -100 to +100
    int8_t regulatory_standing; // -100 to +100
    uint8_t investigation_heat; // 0-100, chance of investigation
};
```

Actions that affect reputation:
- Good service → +opinion
- Accidents → -opinion, -confidence
- Market manipulation → -regulatory, +heat
- Charitable donations → +opinion, -heat
- Environmental damage → -opinion

## UI Design

### Stock Market Window

```
+------------------------------------------+
|  STOCK MARKET                       [x]  |
+------------------------------------------+
| Company        Price   Change  You Own   |
|------------------------------------------|
| Smith Trans    $1,234  +5.2%   15%       |
| Jones Cargo    $892    -2.1%   0%        |
| AI Rail Co     $567    +0.8%   51%  [!]  |
| Mega Freight   $2,101  -8.3%   5%        |
+------------------------------------------+
| [Buy] [Sell] [Company Details] [History] |
+------------------------------------------+
```

### Portfolio Window

```
+------------------------------------------+
|  YOUR PORTFOLIO                     [x]  |
+------------------------------------------+
| Total Value: $4,523,000                  |
| Today's Change: +$123,000 (+2.8%)        |
|------------------------------------------|
| Holdings:                                |
| Smith Trans:  15 shares @ $1,234 = $18k  |
| AI Rail Co:   51 shares @ $567  = $29k   |
| Cash: $4,476,000                         |
+------------------------------------------+
```

## Implementation Plan

### Phase 1: Core Stock System
1. Add `CompanyShares` to `company_base.h`
2. Implement share price calculation in `economy.cpp`
3. Add buy/sell commands in `economy_cmd.cpp`
4. Create basic stock market GUI

### Phase 2: Trading Features
1. Implement broker fees
2. Add share issuance for companies
3. Implement 51%/75% takeover mechanics
4. Price history tracking

### Phase 3: Market Events
1. Random event system
2. Sentiment tracking
3. News integration

### Phase 4: Manipulation & Reputation
1. Insider trading mechanics
2. Investigation system
3. Reputation effects
4. AI response to market

## Files to Modify

- `src/company_base.h` - Add CompanyShares struct
- `src/economy.cpp` - Share price calculation
- `src/economy_cmd.cpp` - Buy/sell commands
- `src/company_gui.cpp` - Stock market window
- `src/news_gui.cpp` - Market event news
- `src/saveload/company_sl.cpp` - Save/load shares
- `src/script/api/` - AI API for stock trading

## New Files to Create

- `src/stock_market.cpp` - Core stock market logic
- `src/stock_market.h` - Stock market declarations
- `src/stock_gui.cpp` - Stock market GUI
- `src/market_events.cpp` - Random market events
