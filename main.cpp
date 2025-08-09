#include <ncurses.h>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>
#include <string>

struct Plane {
    double x;
    double y;
    double speedX;
    bool alive;
};

struct Bomb {
    double x;
    double y;
    double speedY;
    bool active;
};

struct Bullet {
    double x;
    double y;
    double speedY;
    bool active;
};

struct Player {
    int x;
    int y;
};

static std::mt19937& rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

static int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

int main() {
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    timeout(0);

    int maxY = 0, maxX = 0;
    getmaxyx(stdscr, maxY, maxX);

    Player player { maxX / 2, maxY - 2 };

    std::vector<Plane> planes;
    std::vector<Bomb> bombs;
    std::vector<Bullet> bullets;

    int score = 0;
    int lives = 3;
    bool quit = false;
    bool gameOver = false;

    const double planeSpawnIntervalSec = 1.5;
    double planeSpawnAccumulator = 0.0;

    const double bulletCooldownSec = 0.2;
    double bulletCooldownAccumulator = 0.0;

    std::uniform_int_distribution<int> planeRowDist(1, 6);
    std::uniform_real_distribution<double> planeSpeedDist(8.0, 14.0); // cells per second
    std::uniform_real_distribution<double> bombDropChancePerSecond(0.65, 1.2); // varied drop chance per sec

    const double bombSpeed = 16.0;   // cells per second
    const double bulletSpeed = -28.0; // negative: moves up

    auto lastFrameTime = std::chrono::steady_clock::now();

    while (!quit) {
        // Timing
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - lastFrameTime;
        lastFrameTime = now;
        double dt = std::min(elapsed.count(), 0.05); // clamp dt to avoid huge jumps

        // Handle window resize
        int newMaxY = 0, newMaxX = 0;
        getmaxyx(stdscr, newMaxY, newMaxX);
        if (newMaxY != maxY || newMaxX != maxX) {
            maxY = newMaxY;
            maxX = newMaxX;
            player.y = maxY - 2;
            player.x = clampInt(player.x, 1, std::max(1, maxX - 2));
            clear();
        }

        // Input
        int ch;
        while ((ch = getch()) != ERR) {
            if (ch == 'q' || ch == 'Q') {
                quit = true;
            }
            if (gameOver) {
                continue;
            }
            switch (ch) {
                case KEY_LEFT:
                    player.x = clampInt(player.x - 2, 1, std::max(1, maxX - 2));
                    break;
                case KEY_RIGHT:
                    player.x = clampInt(player.x + 2, 1, std::max(1, maxX - 2));
                    break;
                case KEY_UP:
                    if (bulletCooldownAccumulator <= 0.0) {
                        bullets.push_back(Bullet{ static_cast<double>(player.x), static_cast<double>(player.y - 1), bulletSpeed, true });
                        bulletCooldownAccumulator = bulletCooldownSec;
                    }
                    break;
                default:
                    break;
            }
        }

        if (gameOver) {
            // Just render game over screen and continue loop for quit
            erase();
            std::string gameOverMsg = "Game Over! Press 'q' to quit.";
            mvprintw(maxY / 2, std::max(0, (maxX - static_cast<int>(gameOverMsg.size())) / 2), "%s", gameOverMsg.c_str());
            std::string scoreMsg = "Final Score: " + std::to_string(score);
            mvprintw(maxY / 2 + 1, std::max(0, (maxX - static_cast<int>(scoreMsg.size())) / 2), "%s", scoreMsg.c_str());
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // Update cooldowns
        if (bulletCooldownAccumulator > 0.0) bulletCooldownAccumulator -= dt;
        planeSpawnAccumulator += dt;

        // Spawn planes
        if (planeSpawnAccumulator >= planeSpawnIntervalSec) {
            planeSpawnAccumulator -= planeSpawnIntervalSec;
            int rowTop = 1;
            int rowBottom = std::max(1, std::min(maxY - 8, 6));
            if (rowBottom < rowTop) rowBottom = rowTop;
            std::uniform_int_distribution<int> dynamicRowDist(rowTop, rowBottom);
            int spawnRow = dynamicRowDist(rng());
            double speed = planeSpeedDist(rng());
            planes.push_back(Plane{0.0, static_cast<double>(spawnRow), speed, true});
        }

        // Planes update and bomb drops
        for (Plane &plane : planes) {
            if (!plane.alive) continue;
            plane.x += plane.speedX * dt;
            if (plane.x >= maxX - 2) {
                plane.alive = false;
            } else {
                // probabilistic bomb drop per second
                double chance = bombDropChancePerSecond(rng()) * dt;
                std::uniform_real_distribution<double> p(0.0, 1.0);
                if (p(rng()) < chance) {
                    bombs.push_back(Bomb{ plane.x, plane.y + 1.0, bombSpeed, true });
                }
            }
        }

        // Bullets update
        for (Bullet &bullet : bullets) {
            if (!bullet.active) continue;
            bullet.y += bullet.speedY * dt;
            if (bullet.y < 1) {
                bullet.active = false;
            }
        }

        // Bombs update
        for (Bomb &bomb : bombs) {
            if (!bomb.active) continue;
            bomb.y += bomb.speedY * dt;
            if (bomb.y >= maxY - 1) {
                bomb.active = false;
            }
        }

        // Collisions: bullets with planes
        for (Bullet &bullet : bullets) {
            if (!bullet.active) continue;
            for (Plane &plane : planes) {
                if (!plane.alive) continue;
                int bx = static_cast<int>(bullet.x + 0.5);
                int by = static_cast<int>(bullet.y + 0.5);
                int px = static_cast<int>(plane.x + 0.5);
                int py = static_cast<int>(plane.y + 0.5);
                if (by == py && bx == px) {
                    plane.alive = false;
                    bullet.active = false;
                    score += 10;
                    break;
                }
            }
        }

        // Collisions: bombs with player
        for (Bomb &bomb : bombs) {
            if (!bomb.active) continue;
            int bx = static_cast<int>(bomb.x + 0.5);
            int by = static_cast<int>(bomb.y + 0.5);
            if (by == player.y && bx == player.x) {
                bomb.active = false;
                lives -= 1;
                if (lives <= 0) {
                    gameOver = true;
                }
            }
        }

        // Cleanup inactive/dead entities
        planes.erase(std::remove_if(planes.begin(), planes.end(), [](const Plane &p){ return !p.alive; }), planes.end());
        bombs.erase(std::remove_if(bombs.begin(), bombs.end(), [](const Bomb &b){ return !b.active; }), bombs.end());
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet &b){ return !b.active; }), bullets.end());

        // Render
        erase();

        // UI
        std::string hud = "Score: " + std::to_string(score) + "  Lives: " + std::to_string(lives) + "  ('q' to quit)";
        mvprintw(0, 0, "%s", hud.c_str());

        // Draw planes
        for (const Plane &plane : planes) {
            if (!plane.alive) continue;
            int px = static_cast<int>(plane.x + 0.5);
            int py = static_cast<int>(plane.y + 0.5);
            if (py > 0 && py < maxY && px > 0 && px < maxX) {
                mvaddch(py, px, '>');
            }
        }

        // Draw bombs
        for (const Bomb &bomb : bombs) {
            if (!bomb.active) continue;
            int bx = static_cast<int>(bomb.x + 0.5);
            int by = static_cast<int>(bomb.y + 0.5);
            if (by > 0 && by < maxY && bx > 0 && bx < maxX) {
                mvaddch(by, bx, 'o');
            }
        }

        // Draw bullets
        for (const Bullet &bullet : bullets) {
            if (!bullet.active) continue;
            int bx = static_cast<int>(bullet.x + 0.5);
            int by = static_cast<int>(bullet.y + 0.5);
            if (by > 0 && by < maxY && bx > 0 && bx < maxX) {
                mvaddch(by, bx, '^');
            }
        }

        // Draw player gun
        if (!gameOver) {
            mvaddch(player.y, player.x, 'A');
        }

        refresh();

        // Frame cap ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    endwin();
    return 0;
}