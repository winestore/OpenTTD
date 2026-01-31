# Transport Empire

A modified OpenTTD with stock market trading, hostile takeovers, CEO personalities, and Gordon Gekko-style corporate warfare.

## Features

- **Stock Market System** - Buy and sell shares in competing companies
- **Hostile Takeovers** - Acquire 75% of a company to take it over
- **CEO Personalities** - 10 distinct AI personalities (Gordon Gekko, Warren Buffett, Elon Musk, etc.)
- **Dynamic Weather** - Rain, snow, fog, thunderstorms affecting gameplay
- **Day/Night Cycle** - Visual time progression with lighting effects
- **Achievement System** - 35+ achievements across wealth, empire, and stock market categories
- **Campaign Mode** - Story-driven scenarios like "Rise of an Empire" and "Gordon Gekko: Corporate Raider"
- **Screen Effects** - Visual feedback for major events (takeovers, market crashes, achievements)

---

## Building & Launching

### Prerequisites

**All Platforms:**
- CMake 3.16+
- C++20 compatible compiler
- SDL2
- zlib
- liblzma
- libpng (optional, for PNG screenshots)

### Windows (PC)

**Option 1: Visual Studio**
```cmd
git clone https://github.com/winestore/OpenTTD.git
cd OpenTTD
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Launch: `build\Release\openttd.exe`

**Option 2: MSYS2/MinGW**
```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2
git clone https://github.com/winestore/OpenTTD.git
cd OpenTTD
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

Launch: `build/openttd.exe`

### macOS

**Using Homebrew:**
```bash
# Install dependencies
brew install cmake sdl2 xz libpng

# Clone and build
git clone https://github.com/winestore/OpenTTD.git
cd OpenTTD
mkdir build && cd build
cmake ..
cmake --build . -j$(sysctl -n hw.ncpu)
```

Launch: `build/openttd` or `open build/openttd.app` (if bundle created)

**Creating macOS App Bundle:**
```bash
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build .
cpack -G Bundle
```

### Linux

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake libsdl2-dev zlib1g-dev liblzma-dev libpng-dev

# Fedora
sudo dnf install cmake SDL2-devel zlib-devel xz-devel libpng-devel

# Build
git clone https://github.com/winestore/OpenTTD.git
cd OpenTTD
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

Launch: `build/openttd`

---

## Game Rules

### Stock Market

| Action | Description |
|--------|-------------|
| **Buy Shares** | Purchase up to 10% at a time. 5% broker fee applies. |
| **Sell Shares** | Sell your holdings. 5% broker fee applies. |
| **Controlling Interest** | Own 51%+ to gain voting control over a company |
| **Hostile Takeover** | Own 75%+ to force a complete acquisition |
| **Share Price** | Based on company value, performance, and market sentiment |

**Market Events:**
- Scandals (-20% price)
- Record profits (+15% price)
- Market crashes (all stocks -15%)
- Bull markets (all stocks +10%)

### CEO Personalities

| Personality | Trading Style |
|-------------|---------------|
| **Gordon Gekko** | Hyper-aggressive, relentless takeover attempts |
| **Warren Buffett** | Value investor, patient, holds long-term |
| **Elon Musk** | Bold moves, high risk tolerance, unpredictable |
| **Cornelius Vanderbilt** | Infrastructure-focused, territorial |
| **John Rockefeller** | Monopoly-seeking, methodical expansion |
| **Steady Eddie** | Conservative, slow and steady growth |
| **Penny Pincher** | Extreme cost-cutting, avoids risk |
| **Risk Taker** | High volatility, boom or bust |
| **Green Evangelist** | Prioritizes eco-friendly transport |
| **Tech Visionary** | Invests in cutting-edge vehicles |

### Rivalry System

- **Relationship Score**: -100 (Nemesis) to +100 (Friendly)
- Actions like takeover attempts, route stealing, and share purchases affect relationships
- Nemesis CEOs are 2x more aggressive toward you
- Revenge arcs can trigger when significantly wronged

### Achievements

**Wealth:**
- First Dollar, Millionaire, Centi-Millionaire, Billionaire, From Rags to Riches

**Empire:**
- Vehicle milestones (10, 50, 100, 500, 1000 vehicles)
- Multi-Modal Master (10+ of each vehicle type)

**Stock Market:**
- First Shares, Controlling Interest, Hostile Takeover
- Gordon Gekko Award (own shares in every AI company)
- Monopoly (be the only company remaining)

**Special (Hidden):**
- Revenge is Sweet (take over someone who tried to take you over)
- Underdog (smallest company takes over the largest)
- Speed Runner (reach $100M in under 5 years)

### Weather Effects

| Weather | Speed Modifier | Station Rating |
|---------|----------------|----------------|
| Clear | 100% | +2 |
| Light Rain/Snow | -5% | -2 |
| Heavy Rain/Snow | -10% to -25% | -10 |
| Storm | -20% | -15 |
| Fog | -12% | -5 |

### Day/Night Cycle

- Time advances with game ticks
- Sunrise/sunset times vary by season
- Streetlights and vehicle headlights activate at dusk
- Visual color tinting (golden hour, blue nights)

---

## Campaigns

### Rise of an Empire
1. **Humble Beginnings** - Build your first profitable route
2. **Competition Arrives** - Outmaneuver Steady Eddie
3. **Wall Street Calling** - Learn the stock market
4. **Hostile Waters** - Survive Gordon Gekko's attacks
5. **Empire** - Achieve total monopoly

### Gordon Gekko: Corporate Raider
Play as the villain - make money through takeovers, not transport.

---

## Controls

| Key | Action |
|-----|--------|
| Mouse Wheel | Zoom in/out |
| Right-click + Drag | Pan camera |
| Arrow Keys | Move camera |
| Space | Pause/unpause |

Stock Market window accessible via company toolbar.

---

## Configuration

Settings are stored in:
- **Windows**: `%USERPROFILE%\Documents\OpenTTD\openttd.cfg`
- **macOS**: `~/Documents/OpenTTD/openttd.cfg`
- **Linux**: `~/.local/share/openttd/openttd.cfg`

---

## Credits

Transport Empire is built on [OpenTTD](https://www.openttd.org/), which is based on Transport Tycoon Deluxe by Chris Sawyer.

**Transport Empire additions by:** winestore

---

## License

GNU General Public License v2.0 - See [LICENSE](LICENSE) for details.
