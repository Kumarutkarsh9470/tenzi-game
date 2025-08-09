#include <ncurses.h>
#include <vector>
#include <deque>
#include <string>
#include <random>
#include <chrono>
#include <algorithm>
#include <cstdlib>

struct Entity {
    int row;
    int col;
    int dr;
    int dc;
    char glyph;
    bool alive;
};

struct Plane {
    int row;
    int col;
    int speedCounter;
    int speedTicks; // lower is faster
    bool alive;
};

struct Bomb {
    int row;
    int col;
    int speedCounter;
    int speedTicks; // lower is faster
    bool alive;
};

struct Bullet {
    int row;
    int col;
    int speedCounter;
    int speedTicks; // lower is faster
    bool alive;
};

static std::mt19937& rng() {
    static std::mt19937 gen(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    return gen;
}

static int rand_int(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng());
}

static bool rects_overlap(int r1, int c1, int r2, int c2) {
    return r1 == r2 && c1 == c2;
}

int main() {
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);   // plane
        init_pair(2, COLOR_YELLOW, COLOR_BLACK); // bomb
        init_pair(3, COLOR_GREEN, COLOR_BLACK);  // player
        init_pair(4, COLOR_RED, COLOR_BLACK);    // bullet/explosions
        init_pair(5, COLOR_WHITE, COLOR_BLACK);  // hud
    }

    int maxRows, maxCols;
    getmaxyx(stdscr, maxRows, maxCols);

    // Player gun at bottom row
    int playerRow = maxRows - 2;
    int playerCol = maxCols / 2;
    int lives = 3;
    int score = 0;

    std::vector<Plane> planes;
    std::vector<Bomb> bombs;
    std::vector<Bullet> bullets;

    // Timing
    const int frameDelayMs = 16; // ~60 fps

    // Spawn control
    int planeSpawnCounter = 0;
    int planeSpawnEvery = 45; // frames

    bool running = true;

    auto draw_hud = [&]() {
        attron(COLOR_PAIR(5));
        mvprintw(0, 1, "Score: %d  Lives: %d  Planes: %zu  Bombs: %zu  Bullets: %zu  (Arrows: move/shoot)",
                 score, lives, planes.size(), bombs.size(), bullets.size());
        attroff(COLOR_PAIR(5));
    };

    auto draw_player = [&]() {
        attron(COLOR_PAIR(3));
        mvaddch(playerRow, playerCol, '^');
        attroff(COLOR_PAIR(3));
    };

    auto draw_planes = [&]() {
        attron(COLOR_PAIR(1));
        for (const auto& p : planes) {
            if (!p.alive) continue;
            if (p.row >= 1 && p.row < maxRows - 1 && p.col >= 0 && p.col < maxCols) {
                mvaddch(p.row, p.col, '>');
            }
        }
        attroff(COLOR_PAIR(1));
    };

    auto draw_bombs = [&]() {
        attron(COLOR_PAIR(2));
        for (const auto& b : bombs) {
            if (!b.alive) continue;
            if (b.row >= 1 && b.row < maxRows - 1 && b.col >= 0 && b.col < maxCols) {
                mvaddch(b.row, b.col, 'o');
            }
        }
        attroff(COLOR_PAIR(2));
    };

    auto draw_bullets = [&]() {
        attron(COLOR_PAIR(4));
        for (const auto& b : bullets) {
            if (!b.alive) continue;
            if (b.row >= 1 && b.row < maxRows - 1 && b.col >= 0 && b.col < maxCols) {
                mvaddch(b.row, b.col, '|');
            }
        }
        attroff(COLOR_PAIR(4));
    };

    auto spawn_plane = [&]() {
        int r = rand_int(1, std::max(1, maxRows / 2));
        Plane p{r, 0, 0, rand_int(2, 5), true};
        planes.push_back(p);
    };

    auto maybe_drop_bomb = [&](Plane& p) {
        // Random chance to drop a bomb
        if (!p.alive) return;
        if (rand_int(0, 99) < 5) { // 5% chance per frame
            bombs.push_back(Bomb{p.row + 1, p.col, 0, rand_int(1, 2), true});
        }
    };

    auto update_planes = [&]() {
        for (auto& p : planes) {
            if (!p.alive) continue;
            p.speedCounter++;
            if (p.speedCounter >= p.speedTicks) {
                p.speedCounter = 0;
                p.col += 1; // move right
                if (p.col >= maxCols) {
                    p.alive = false;
                } else {
                    maybe_drop_bomb(p);
                }
            }
        }
        // remove dead/offscreen
        planes.erase(std::remove_if(planes.begin(), planes.end(), [](const Plane& p){ return !p.alive; }), planes.end());
    };

    auto update_bombs = [&]() {
        for (auto& b : bombs) {
            if (!b.alive) continue;
            b.speedCounter++;
            if (b.speedCounter >= b.speedTicks) {
                b.speedCounter = 0;
                b.row += 1; // fall down
                if (b.row >= maxRows - 1) {
                    b.alive = false;
                }
            }
        }
        bombs.erase(std::remove_if(bombs.begin(), bombs.end(), [](const Bomb& b){ return !b.alive; }), bombs.end());
    };

    auto update_bullets = [&]() {
        for (auto& b : bullets) {
            if (!b.alive) continue;
            b.speedCounter++;
            if (b.speedCounter >= b.speedTicks) {
                b.speedCounter = 0;
                b.row -= 1; // move up
                if (b.row <= 0) {
                    b.alive = false;
                }
            }
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b){ return !b.alive; }), bullets.end());
    };

    auto handle_collisions = [&]() {
        // bullets vs planes
        for (auto& bullet : bullets) {
            if (!bullet.alive) continue;
            for (auto& plane : planes) {
                if (!plane.alive) continue;
                if (rects_overlap(bullet.row, bullet.col, plane.row, plane.col)) {
                    bullet.alive = false;
                    plane.alive = false;
                    score += 10;
                    break;
                }
            }
        }
        // bombs vs player
        for (auto& bomb : bombs) {
            if (!bomb.alive) continue;
            if (rects_overlap(bomb.row, bomb.col, playerRow, playerCol)) {
                bomb.alive = false;
                lives -= 1;
            }
        }
        // bullets vs bombs (optional defense)
        for (auto& bullet : bullets) {
            if (!bullet.alive) continue;
            for (auto& bomb : bombs) {
                if (!bomb.alive) continue;
                if (rects_overlap(bullet.row, bullet.col, bomb.row, bomb.col)) {
                    bullet.alive = false;
                    bomb.alive = false;
                    score += 2;
                    break;
                }
            }
        }
    };

    while (running) {
        // Input
        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                playerCol = std::max(0, playerCol - 1);
                break;
            case KEY_RIGHT:
                playerCol = std::min(maxCols - 1, playerCol + 1);
                break;
            case KEY_UP:
                // fire bullet
                bullets.push_back(Bullet{playerRow - 1, playerCol, 0, 1, true});
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
            default:
                break;
        }

        // Spawn planes periodically
        planeSpawnCounter++;
        if (planeSpawnCounter >= planeSpawnEvery) {
            planeSpawnCounter = 0;
            spawn_plane();
            // make spawning a bit dynamic over time
            if (planeSpawnEvery > 10 && rand_int(0, 3) == 0) {
                planeSpawnEvery -= 1;
            }
        }

        // Update world
        update_planes();
        update_bombs();
        update_bullets();
        handle_collisions();

        // Render
        clear();
        draw_hud();
        draw_player();
        draw_planes();
        draw_bombs();
        draw_bullets();

        if (lives <= 0) {
            attron(COLOR_PAIR(4));
            std::string msg = "GAME OVER - Press Q to Quit";
            mvprintw(maxRows/2, std::max(0, (maxCols - static_cast<int>(msg.size()))/2), "%s", msg.c_str());
            attroff(COLOR_PAIR(4));
            // Stop updates but keep listening for Q
            nodelay(stdscr, FALSE);
            int c;
            while ((c = getch()) != 'q' && c != 'Q') {}
            break;
        }

        refresh();
        napms(frameDelayMs);

        // Window size may change
        getmaxyx(stdscr, maxRows, maxCols);
        playerRow = maxRows - 2;
        playerCol = std::clamp(playerCol, 0, std::max(0, maxCols - 1));
    }

    endwin();
    return 0;
}