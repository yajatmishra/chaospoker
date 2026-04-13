#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

struct Card {
    int rank; // 2-14
    int suit; // 0-3
};

static inline int parseRank(char c) {
    switch(c) {
        case '2': return 2; case '3': return 3; case '4': return 4;
        case '5': return 5; case '6': return 6; case '7': return 7;
        case '8': return 8; case '9': return 9; case 'T': return 10;
        case 'J': return 11; case 'Q': return 12; case 'K': return 13;
        case 'A': return 14; default: return 0;
    }
}

static inline int parseSuit(char c) {
    switch(c) { case 's': return 0; case 'h': return 1; case 'd': return 2; case 'c': return 3; default: return 0; }
}

static inline Card parseCard(const char* s) {
    return {parseRank(s[0]), parseSuit(s[1])};
}

// Sorting network: 9 compare-swaps for 5 elements, descending by rank
static inline void sort5desc(Card* h) {
    #define CSWAP(i,j) if (h[i].rank < h[j].rank) { Card t=h[i]; h[i]=h[j]; h[j]=t; }
    CSWAP(0,1); CSWAP(3,4); CSWAP(2,4);
    CSWAP(2,3); CSWAP(0,3); CSWAP(0,2);
    CSWAP(1,4); CSWAP(1,3); CSWAP(1,2);
    #undef CSWAP
}

// Scores a 5-card hand into a 64-bit int: category * base + tiebreakers
static long long eval5(Card hand[5]) {
    sort5desc(hand);

    int ranks[5], suits[5];
    for (int i = 0; i < 5; i++) { ranks[i] = hand[i].rank; suits[i] = hand[i].suit; }

    bool isFlush = (suits[0]==suits[1] && suits[1]==suits[2] && suits[2]==suits[3] && suits[3]==suits[4]);

    bool isStraight = false;
    bool aceLow = false;
    if (ranks[0]-ranks[4]==4 && ranks[0]!=ranks[1] && ranks[1]!=ranks[2] && ranks[2]!=ranks[3] && ranks[3]!=ranks[4])
        isStraight = true;
    if (ranks[0]==14 && ranks[1]==5 && ranks[2]==4 && ranks[3]==3 && ranks[4]==2) {
        isStraight = true;
        aceLow = true;
    }

    int freq[15] = {};
    for (int i = 0; i < 5; i++) freq[ranks[i]]++;

    int quads=0, trips=0, pairs=0;
    int quadRank=0, tripRank=0;
    int pairRanks[2] = {0,0};
    int kickers[5]; int nk=0;

    for (int r = 14; r >= 2; r--) {
        if (freq[r]==4) { quads++; quadRank=r; }
        else if (freq[r]==3) { trips++; tripRank=r; }
        else if (freq[r]==2) { pairRanks[pairs++]=r; }
        else if (freq[r]==1) { kickers[nk++]=r; }
    }

    long long score = 0;
    constexpr long long base = 10000000000LL;

    if (isStraight && isFlush) {
        int high = aceLow ? 5 : ranks[0];
        score = (high == 14 ? 10 : 9) * base + high;
    } else if (quads) {
        score = 8*base + (long long)quadRank*100 + kickers[0];
    } else if (trips && pairs) {
        score = 7*base + (long long)tripRank*100 + pairRanks[0];
    } else if (isFlush) {
        score = 6*base;
        long long m = 10000;
        for (int i = 0; i < 5; i++) { score += ranks[i]*m; m /= 100; }
    } else if (isStraight) {
        score = 5*base + (aceLow ? 5 : ranks[0]);
    } else if (trips) {
        score = 4*base + (long long)tripRank*10000 + kickers[0]*100 + kickers[1];
    } else if (pairs==2) {
        score = 3*base + (long long)pairRanks[0]*10000 + pairRanks[1]*100 + kickers[0];
    } else if (pairs==1) {
        score = 2*base + (long long)pairRanks[0]*1000000;
        long long m = 10000;
        for (int i = 0; i < nk; i++) { score += kickers[i]*m; m /= 100; }
    } else {
        score = 1*base;
        long long m = 100000000;
        for (int i = 0; i < 5; i++) { score += ranks[i]*m; m /= 100; }
    }
    return score;
}

// Best 5-card hand from n cards via brute-force C(n,5)
static long long evalBest(Card* cards, int n) {
    if (n < 5) return 0;
    long long best = 0;
    for (int a = 0; a < n; a++)
    for (int b = a+1; b < n; b++)
    for (int c = b+1; c < n; c++)
    for (int d = c+1; d < n; d++)
    for (int e = d+1; e < n; e++) {
        Card hand[5] = {cards[a], cards[b], cards[c], cards[d], cards[e]};
        long long s = eval5(hand);
        if (s > best) best = s;
    }
    return best;
}

// Preflop hand strength: composite score from pair value, suitedness, gap, high cards
static int preflopStrength(Card c1, Card c2) {
    int r1 = std::max(c1.rank, c2.rank);
    int r2 = std::min(c1.rank, c2.rank);
    bool suited = (c1.suit == c2.suit);
    bool paired = (c1.rank == c2.rank);

    if (paired) {
        if (r1 >= 10) return 55 + r1 * 3;
        return 30 + r1 * 4;
    }

    int gap = r1 - r2;
    int score = (r1 + r2) * 2;

    if (suited) score += 8;
    if (gap <= 1) score += 5;
    else if (gap <= 2) score += 3;
    else if (gap <= 3) score += 1;
    if (gap >= 5) score -= 8;
    if (r1 >= 10 && r2 >= 10) score += 10;
    if (r1 == 14) score += 8;

    return std::min(95, std::max(5, score));
}

// Postflop hand category: 0=air, 2=gutshot, 3=OESD/flush draw, 4=pair,
// 5=top pair, 6=overpair, 7=two pair, 8=trips/set, 9-11=straight/flush/full house+
static int postflopCategory(const Card* hole, int nHole, const Card* board, int nBoard) {
    Card all[9];
    int n = 0;
    for (int i = 0; i < nHole; i++) all[n++] = hole[i];
    for (int i = 0; i < nBoard; i++) all[n++] = board[i];

    long long score = evalBest(all, n);
    constexpr long long base = 10000000000LL;
    int cat = (int)(score / base);

    if (cat >= 9) return 11;
    if (cat == 8) return 11;
    if (cat == 7) return 11;
    if (cat == 6) return 10;
    if (cat == 5) return 9;

    int suitCount[4] = {};
    for (int i = 0; i < n; i++) suitCount[all[i].suit]++;
    bool flushDraw = false;
    for (int s = 0; s < 4; s++) if (suitCount[s] == 4) flushDraw = true;

    bool straightDraw = false, gutshot = false;
    {
        bool has[15] = {};
        for (int i = 0; i < n; i++) has[all[i].rank] = true;
        if (has[14]) has[1] = true;
        for (int r = 1; r <= 10; r++) {
            int cnt = 0;
            for (int i = r; i < r+5; i++) if (has[i]) cnt++;
            if (cnt == 4) {
                if (!has[r] || !has[r+4]) straightDraw = true;
                else gutshot = true;
            }
        }
    }

    if (cat == 4) return 8;
    if (cat == 3) return 7;

    if (cat == 2) {
        bool holePair = (hole[0].rank == hole[1].rank);
        if (holePair) {
            int maxBoard = 0;
            for (int i = 0; i < nBoard; i++) if (board[i].rank > maxBoard) maxBoard = board[i].rank;
            if (hole[0].rank > maxBoard) return 6;
            return 4;
        }
        int boardMax = 0;
        for (int i = 0; i < nBoard; i++) if (board[i].rank > boardMax) boardMax = board[i].rank;
        for (int h = 0; h < nHole; h++) {
            if (hole[h].rank == boardMax) {
                for (int b = 0; b < nBoard; b++) {
                    if (board[b].rank == hole[h].rank) return 5;
                }
            }
        }
        return 4;
    }

    if (flushDraw || straightDraw) return 3;
    if (gutshot) return 2;
    return 0;
}

int main() {
    std::ios_base::sync_with_stdio(false);

    int numPlayers = 0, mySeat = 0, startingChips = 0;
    int swapCosts[4] = {};
    int bigBlind = 2;
    Card holeCards[2]; int nHole = 0;
    Card board[5]; int nBoard = 0;
    int street = 0;
    int chips[10] = {};
    int lastSwapIdx = -1;
    int actionCountThisStreet = 0;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        size_t sp = line.find(' ');
        std::string cmd = (sp == std::string::npos) ? line : line.substr(0, sp);

        if (cmd == "GAME_START") {
            std::istringstream iss(line);
            std::string tmp;
            iss >> tmp >> numPlayers >> mySeat >> startingChips;
            iss >> swapCosts[0] >> swapCosts[1] >> swapCosts[2] >> swapCosts[3];
            for (int i = 0; i < numPlayers; i++) chips[i] = startingChips;
        }
        else if (cmd == "HAND_START") {
            std::istringstream iss(line);
            std::string tmp;
            int hn, ds, sb_seat, bb_seat, sb_amt, bb_amt;
            iss >> tmp >> hn >> ds >> sb_seat >> bb_seat >> sb_amt >> bb_amt;
            bigBlind = bb_amt;
            nHole = 0; nBoard = 0; street = 0; lastSwapIdx = -1; actionCountThisStreet = 0;

        }
        else if (cmd == "CHIPS") {
            std::istringstream iss(line);
            std::string tmp; iss >> tmp;
            for (int i = 0; i < numPlayers; i++) { if (!(iss >> chips[i])) chips[i] = 0; }
        }
        else if (cmd == "DEAL_HOLE") {
            size_t p = sp + 1;
            holeCards[0] = parseCard(line.c_str() + p);
            p = line.find(' ', p) + 1;
            holeCards[1] = parseCard(line.c_str() + p);
            nHole = 2;
        }
        else if (cmd == "DEAL_FLOP") {
            street = 1; actionCountThisStreet = 0;            size_t p = sp + 1;
            nBoard = 0;
            board[nBoard++] = parseCard(line.c_str() + p);
            p = line.find(' ', p) + 1;
            board[nBoard++] = parseCard(line.c_str() + p);
            p = line.find(' ', p) + 1;
            board[nBoard++] = parseCard(line.c_str() + p);
        }
        else if (cmd == "DEAL_TURN") {
            street = 2; actionCountThisStreet = 0;            board[nBoard++] = parseCard(line.c_str() + sp + 1);
        }
        else if (cmd == "DEAL_RIVER") {
            street = 3; actionCountThisStreet = 0;            board[nBoard++] = parseCard(line.c_str() + sp + 1);
        }
        else if (cmd == "REDRAW_FLOP") {
            nBoard = 0;
            size_t p = sp + 1;
            board[nBoard++] = parseCard(line.c_str() + p);
            p = line.find(' ', p) + 1;
            board[nBoard++] = parseCard(line.c_str() + p);
            p = line.find(' ', p) + 1;
            board[nBoard++] = parseCard(line.c_str() + p);
        }
        else if (cmd == "REDRAW_TURN") {
            if (nBoard >= 4) board[3] = parseCard(line.c_str() + sp + 1);
        }
        else if (cmd == "REDRAW_RIVER") {
            if (nBoard >= 5) board[4] = parseCard(line.c_str() + sp + 1);
        }
        // Track which card index was swapped to correctly update on SWAP_RESULT
        else if (cmd == "SWAP_RESULT") {
            Card newCard = parseCard(line.c_str() + sp + 1);
            if (nHole == 2 && lastSwapIdx >= 0 && lastSwapIdx < 2) {
                holeCards[lastSwapIdx] = newCard;
            }
            lastSwapIdx = -1;
        }
        // Swap the weaker card preflop if hand is weak and cost is cheap;
        // postflop only swap with complete air at minimal cost
        else if (cmd == "SWAP_PROMPT") {
            std::istringstream iss(line);
            std::string tmp;
            int cost, myChips;
            iss >> tmp >> cost >> myChips;

            if (nHole < 2) {
                std::cout << "STAY" << std::endl;
                continue;
            }

            if (street == 0) {
                bool paired = (holeCards[0].rank == holeCards[1].rank);
                int str = preflopStrength(holeCards[0], holeCards[1]);
                if (paired || str >= 50) {
                    std::cout << "STAY" << std::endl;
                } else if (cost <= myChips && cost <= bigBlind * 3) {
                    int idx = (holeCards[0].rank <= holeCards[1].rank) ? 0 : 1;
                    lastSwapIdx = idx;
                    std::cout << "SWAP " << idx << std::endl;
                } else {
                    std::cout << "STAY" << std::endl;
                }
            } else {
                int cat = postflopCategory(holeCards, nHole, board, nBoard);
                if (cat <= 0 && cost <= bigBlind && cost <= myChips) {
                    int idx = (holeCards[0].rank <= holeCards[1].rank) ? 0 : 1;
                    lastSwapIdx = idx;
                    std::cout << "SWAP " << idx << std::endl;
                } else {
                    std::cout << "STAY" << std::endl;
                }
            }
        }
        // Always vote NO: invest to redraw when weak (second chance on bad boards),
        // free NO vote when strong (nothing to lose, opponents might lose their edge)
        else if (cmd == "VOTE_PROMPT") {
            std::istringstream iss(line);
            std::string tmp;
            int myChips;
            iss >> tmp >> myChips;

            if (nBoard == 0 || nHole < 2) {
                std::cout << "VOTE NO 0" << std::endl;
                continue;
            }

            int cat = postflopCategory(holeCards, nHole, board, nBoard);
            if (cat <= 2) {
                std::cout << "VOTE NO " << std::min(bigBlind * 2, myChips) << std::endl;
            } else if (cat <= 4) {
                std::cout << "VOTE NO " << std::min(bigBlind, myChips) << std::endl;
            } else {
                std::cout << "VOTE NO 0" << std::endl;
            }
        }
        else if (cmd == "ACTION_PROMPT") {
            std::istringstream iss(line);
            std::string tmp;
            int myChips, currentBet, myBet, minRaise, pot;
            iss >> tmp >> myChips >> currentBet >> myBet >> minRaise >> pot;

            int toCall = currentBet - myBet;

            if (nHole < 2) {
                if (toCall <= 0) std::cout << "CHECK" << std::endl;
                else std::cout << "FOLD" << std::endl;
                continue;
            }

            // Preflop: three tiers — raise strong (70+), call playable (40-69), fold weak
            if (street == 0) {
                int str = preflopStrength(holeCards[0], holeCards[1]);
                bool paired = (holeCards[0].rank == holeCards[1].rank);
                if (str >= 70 || (paired && holeCards[0].rank >= 9)) {
                    int raiseAmt = std::min(minRaise + bigBlind * 3, myChips + myBet);
                    if (raiseAmt > currentBet && raiseAmt >= minRaise) {
                        std::cout << "RAISE " << raiseAmt << std::endl;
                    } else if (toCall <= 0) {
                        std::cout << "CHECK" << std::endl;
                    } else if (toCall <= myChips) {
                        std::cout << "CALL" << std::endl;
                    } else {
                        std::cout << "ALLIN" << std::endl;
                    }
                } else if (str >= 40) {
                    if (toCall <= 0) {
                        if (str >= 55 && minRaise <= myChips + myBet) {
                            std::cout << "RAISE " << std::min(minRaise, myChips + myBet) << std::endl;
                        } else {
                            std::cout << "CHECK" << std::endl;
                        }
                    } else if (toCall <= bigBlind * 4 && toCall <= myChips) {
                        std::cout << "CALL" << std::endl;
                    } else if (toCall <= myChips && toCall <= pot / 2) {
                        std::cout << "CALL" << std::endl;
                    } else {
                        std::cout << "FOLD" << std::endl;
                    }
                } else {
                    if (toCall <= 0) {
                        std::cout << "CHECK" << std::endl;
                    } else {
                        std::cout << "FOLD" << std::endl;
                    }
                }
            }
            // Postflop: pure value betting — no bluffs, size by hand category
            else {
                int cat = postflopCategory(holeCards, nHole, board, nBoard);
                actionCountThisStreet++;

                if (cat >= 8) {
                    int raiseAmt = std::min(myChips + myBet, currentBet + pot);
                    raiseAmt = std::max(raiseAmt, minRaise);
                    raiseAmt = std::min(raiseAmt, myChips + myBet);
                    if (raiseAmt >= minRaise && raiseAmt > currentBet) {
                        std::cout << "RAISE " << raiseAmt << std::endl;
                    } else if (toCall <= 0) {
                        if (minRaise <= myChips + myBet)
                            std::cout << "RAISE " << std::min(minRaise + pot, myChips + myBet) << std::endl;
                        else
                            std::cout << "ALLIN" << std::endl;
                    } else if (toCall <= myChips) {
                        std::cout << "CALL" << std::endl;
                    } else {
                        std::cout << "ALLIN" << std::endl;
                    }
                } else if (cat >= 5) {
                    int betSize = pot * 2 / 3;
                    int raiseAmt = currentBet + std::max(betSize, bigBlind * 3);
                    raiseAmt = std::min(raiseAmt, myChips + myBet);
                    if (toCall <= 0) {
                        if (minRaise <= myChips + myBet)
                            std::cout << "RAISE " << std::min(minRaise + pot/2, myChips + myBet) << std::endl;
                        else
                            std::cout << "CHECK" << std::endl;
                    } else if (toCall <= pot && toCall <= myChips) {
                        if (cat >= 6 && raiseAmt >= minRaise && raiseAmt > currentBet)
                            std::cout << "RAISE " << raiseAmt << std::endl;
                        else
                            std::cout << "CALL" << std::endl;
                    } else if (toCall <= myChips) {
                        std::cout << "CALL" << std::endl;
                    } else {
                        std::cout << "ALLIN" << std::endl;
                    }
                } else if (cat >= 4) {
                    if (toCall <= 0) {
                        if (minRaise <= myChips + myBet)
                            std::cout << "RAISE " << std::min(minRaise, myChips + myBet) << std::endl;
                        else
                            std::cout << "CHECK" << std::endl;
                    } else if (toCall <= bigBlind * 4 && toCall <= myChips) {
                        std::cout << "CALL" << std::endl;
                    } else {
                        std::cout << "FOLD" << std::endl;
                    }
                } else if (cat >= 3) {
                    // Semi-bluff draws on flop/turn only
                    if (toCall <= 0) {
                        if (street <= 2 && minRaise <= myChips + myBet) {
                            std::cout << "RAISE " << std::min(minRaise, myChips + myBet) << std::endl;
                        } else {
                            std::cout << "CHECK" << std::endl;
                        }
                    } else if (toCall <= pot / 3 && toCall <= myChips) {
                        std::cout << "CALL" << std::endl;
                    } else {
                        std::cout << "FOLD" << std::endl;
                    }
                } else if (cat >= 2) {
                    if (toCall <= 0) {
                        std::cout << "CHECK" << std::endl;
                    } else if (toCall <= bigBlind * 2 && toCall <= myChips) {
                        std::cout << "CALL" << std::endl;
                    } else {
                        std::cout << "FOLD" << std::endl;
                    }
                } else {
                    if (toCall <= 0) {
                        std::cout << "CHECK" << std::endl;
                    } else {
                        std::cout << "FOLD" << std::endl;
                    }
                }
            }
        }
    }
    return 0;
}
