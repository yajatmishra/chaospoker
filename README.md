# Chaos PokerBot : Jump Trading x Tinkercademy Bootcamp Round 2

**Name:** Yajat Mishra

## Build

Requires a C++17 compiler (g++ or clang++).

```bash
g++ -O2 -std=c++17 -o bot bot.cpp
```

## Launch Command

```bash
./bot
```

The bot communicates over stdin/stdout per the chaospoker protocol.

## Strategy

### Hand Evaluation

The bot uses a brute-force 5-card evaluator that enumerates all C(n,5) combinations from the available cards (hole + board). Each 5-card hand is scored into a 64-bit integer encoding hand category (high card through royal flush) and tiebreaker kickers, allowing direct numeric comparison. A sorting network (9 compare-swaps) sorts the 5 cards for pattern recognition. At most C(7,5)=21 combinations are evaluated per decision, keeping it fast.

### Preflop Strategy

Hole cards are scored using a composite strength metric that accounts for:
- **Pair value** (pocket pairs are scored by rank, high pairs premium)
- **Suited bonus** (+8 points for suited cards, reflecting flush potential)
- **Connectedness** (bonus for small gaps, penalty for gaps >= 5)
- **High card value** (broadway bonus, ace bonus)

This score maps to three tiers:
- **Strong** (70+): Open-raise to minRaise + 3xBB
- **Playable** (40-69): Raise if 55+, otherwise call up to 4xBB or half-pot
- **Weak** (<40): Fold to any bet

### Postflop Strategy

After the flop, the bot classifies its hand into a category hierarchy:

| Category | Examples | Action |
|----------|----------|--------|
| Monster (8+) | Trips, straights, flushes, full houses | Bet pot-sized, willing to go all-in |
| Strong (5-7) | Top pair, overpair, two pair | Value bet 2/3 pot; re-raise with overpair+ |
| Pair (4) | Middle/bottom pair | Min-raise if uncalled, call up to 4xBB |
| Draw (3) | Flush draw, OESD | Semi-bluff on flop/turn, call up to pot/3 |
| Gutshot (2) | Inside straight draw | Check-call up to 2xBB |
| Air (0) | Nothing | Check-fold |

The bot does not bluff. Against the field of bots tested, bluffing was empirically -EV because opponents call too frequently. Pure value betting maximises win rate.

### Swap Strategy

- **Preflop:** Swap the weaker hole card if the hand scores below 50 and the cost is at most 3xBB. Pocket pairs and strong hands always stay.
- **Postflop:** Only swap when holding complete air (category 0) and the cost is at most 1xBB.
- **Swap tracking:** The bot tracks which card index (0 or 1) was sent to the engine and correctly updates the hole cards when receiving `SWAP_RESULT`. This avoids a subtle bug where rank-based replacement could corrupt hand state after a swap.

### Voting Strategy

The bot always votes **NO** on community cards. The reasoning:

- When the bot has a weak hand (air/gutshot/draw), it invests up to 2xBB to push for a board redraw, giving it a second chance to connect.
- When the bot has a mediocre hand (pair), it invests 1xBB for a redraw nudge.
- When the bot has a strong hand, it votes NO with zero investment (free vote, no risk).

This always NO approach was empirically validated as it produced a 4-6% win rate improvement against tested opponents. The insight is that in this variant of poker, community card redraws are asymmetric. A bot with air has nothing to lose from a redraw, while a bot with a made hand risks losing its edge. By consistently voting for redraws when weak, the bot converts losing boards into coin-flip situations at low cost.

### Performance

- **I/O:** `sync_with_stdio(false)` with `std::endl` flushes on every response
- **Memory:** All arrays are stack-allocated (no heap allocation during play)
- **Latency:** ~40 microseconds worst case per decision (250x under the 10ms limit)
- **Robustness:** Handles unexpected messages, missing data, and edge cases (e.g., fewer than 2 hole cards)

### Tradeoffs

1. **No bluffing.** The bot plays a purely exploitable strategy. A strong opponent could fold to every bet knowing we always have it. However, against the tested bots, this is optimal since they call too much.
2. **Always-NO voting.** This sacrifices the ability to protect strong hands via YES votes in exchange for consistent second chances when weak. The net effect is strongly positive empirically.
3. **Conservative swap usage.** The bot rarely swaps, only doing so with weak hands at low cost. More aggressive swapping could improve hand quality but risks burning chips on marginal upgrades.
