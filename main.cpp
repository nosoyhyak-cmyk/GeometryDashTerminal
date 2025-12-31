#include <ncurses.h>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>

// --- Game Constants ---
enum class State { MENU, PLAYING, GAMEOVER, WIN };

struct Level {
    std::string name;
    std::string difficulty;
    std::vector<int> map; // 0: empty, 1: spike, 2: block, 3: tall wall
    int bestProgress;
    int color;
};

class GeometryDashPro {
public:
    GeometryDashPro() : running(true), currentState(State::MENU), currentLevelIdx(0) {
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        curs_set(0);

        // Neon Palette
        init_pair(1, COLOR_CYAN, COLOR_BLACK);   // Player
        init_pair(2, COLOR_RED, COLOR_BLACK);    // Spikes
        init_pair(3, COLOR_MAGENTA, COLOR_BLACK);// Menu UI
        init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Gold/Stars
        init_pair(5, COLOR_BLUE, COLOR_BLACK);   // Level 1-2
        init_pair(6, COLOR_GREEN, COLOR_BLACK);  // Level 3-4
        init_pair(7, COLOR_WHITE, COLOR_BLACK);  // Level 5

        setupLevels();
    }

    ~GeometryDashPro() { endwin(); }

    void setupLevels() {
        // Level 1: Stereo Madness (Easy)
        levels.push_back({"STEREO MADNESS", "EASY",
                          {0,0,0,1,0,0,1,0,0,2,2,0,0,1,1,0,0,0,2,0,1,1,1,0,0,2,2,2,0,0,1,0,1,0,1}, 0, 5});

        // Level 2: Back on Track (Easy)
        levels.push_back({"BACK ON TRACK", "EASY",
                          {0,1,0,2,0,1,0,2,2,0,1,1,0,0,2,0,2,0,1,1,0,2,2,2,0,1,0,1,0,2,1,2,0,1}, 0, 5});

        // Level 3: Polargeist (Normal)
        levels.push_back({"POLARGEIST", "NORMAL",
                          {0,1,1,0,2,2,1,0,3,0,1,1,2,2,1,1,3,0,0,1,2,1,2,1,3,3,0,1,1,1,2,0,1,3}, 0, 6});

        // Level 4: Dry Out (Hard)
        levels.push_back({"DRY OUT", "HARD",
                          {0,3,0,1,1,3,0,2,2,3,1,1,1,3,0,3,3,1,0,1,3,1,3,1,3,0,2,2,2,3,1,1,3,1}, 0, 6});

        // Level 5: Base After Base (Insane)
        levels.push_back({"BASE AFTER BASE", "INSANE",
                          {1,1,3,1,1,3,2,2,3,1,3,1,3,3,1,1,1,3,2,3,1,3,1,3,1,3,3,3,1,1,3,1,3,3}, 0, 7});
    }

    void run() {
        while (running) {
            getmaxyx(stdscr, screenH, screenW);
            input();
            update();
            render();
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
    }

private:
    bool running;
    State currentState;
    int screenW, screenH;
    int currentLevelIdx;
    std::vector<Level> levels;

    // Player Vars
    float py, pvy, worldX;
    bool isDashing;
    int dashTimer;

    void input() {
        int ch = getch();
        if (ch == 'q') running = false;

        if (currentState == State::MENU) {
            if (ch == KEY_RIGHT) currentLevelIdx = (currentLevelIdx + 1) % levels.size();
            if (ch == KEY_LEFT) currentLevelIdx = (currentLevelIdx - 1 + levels.size()) % levels.size();
            if (ch == ' ') startLevel();
        } else if (currentState == State::PLAYING) {
            if (ch == ' ' && py >= screenH - 6) pvy = -1.6f; // Jump
            if (ch == 'd' && dashTimer <= 0) { isDashing = true; dashTimer = 30; }
        } else if (currentState == State::GAMEOVER || currentState == State::WIN) {
            if (ch == ' ') currentState = State::MENU;
        }
    }

    void startLevel() {
        py = screenH - 6;
        pvy = 0;
        worldX = 0;
        isDashing = false;
        dashTimer = 0;
        currentState = State::PLAYING;
    }

    void update() {
        if (currentState != State::PLAYING) return;

        // Physics
        pvy += 0.12f;
        py += pvy;
        if (py >= screenH - 6) { py = screenH - 6; pvy = 0; }

        float speed = isDashing ? 1.2f : 0.5f;
        worldX += speed;
        if (dashTimer > 0) {
            dashTimer--;
            if (dashTimer < 20) isDashing = false;
        }

        // Collision Logic
        int playerX = 15;
        Level& cur = levels[currentLevelIdx];

        for (int i = 0; i < cur.map.size(); ++i) {
            float objX = (i * 12) - worldX + playerX;
            if (objX > playerX - 1 && objX < playerX + 2) {
                int type = cur.map[i];
                if (type == 1 && py > screenH - 7) triggerDeath();
                if (type == 2 && py > screenH - 8) triggerDeath();
                if (type == 3 && py > screenH - 9) triggerDeath();
            }
        }

        // Progress
        int progress = (worldX / (cur.map.size() * 12)) * 100;
        if (progress > cur.bestProgress) cur.bestProgress = progress;
        if (progress >= 100) currentState = State::WIN;
    }

    void triggerDeath() {
        currentState = State::GAMEOVER;
    }

    void render() {
        erase();
        if (currentState == State::MENU) drawMenu();
        else if (currentState == State::PLAYING) drawGame();
        else if (currentState == State::GAMEOVER) drawEndScreen("GAME OVER - ATTEMPT FAILED");
        else if (currentState == State::WIN) drawEndScreen("LEVEL COMPLETE! YOU ARE A GOD");
        refresh();
    }

    void drawMenu() {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(screenH/2 - 6, screenW/2 - 10, "GEOMETRY DASH PRO");
        attroff(A_BOLD);

        Level& l = levels[currentLevelIdx];
        attron(COLOR_PAIR(l.color));
        mvprintw(screenH/2 - 2, screenW/2 - 15, "[<]  LEVEL %d: %s  [>]", currentLevelIdx + 1, l.name.c_str());
        mvprintw(screenH/2, screenW/2 - 8, "DIFFICULTY: %s", l.difficulty.c_str());
        mvprintw(screenH/2 + 2, screenW/2 - 7, "BEST: %d%%", l.bestProgress);
        attroff(COLOR_PAIR(l.color));

        mvprintw(screenH/2 + 6, screenW/2 - 12, "PRESS [SPACE] TO START");
    }

    void drawGame() {
        // Ground
        attron(COLOR_PAIR(5));
        mvhline(screenH - 5, 0, '=', screenW);
        attroff(COLOR_PAIR(5));

        // Obstacles (Designed)
        Level& cur = levels[currentLevelIdx];
        for (int i = 0; i < cur.map.size(); ++i) {
            int sx = (i * 12) - worldX + 15;
            if (sx > 0 && sx < screenW) {
                attron(COLOR_PAIR(2));
                if (cur.map[i] == 1) mvprintw(screenH - 6, sx, "^");
                else if (cur.map[i] == 2) mvprintw(screenH - 6, sx, "[X]");
                else if (cur.map[i] == 3) { mvprintw(screenH - 6, sx, "|+|"); mvprintw(screenH - 7, sx, "|+|"); }
                attroff(COLOR_PAIR(2));
            }
        }

        // Player
        attron(COLOR_PAIR(1) | A_BOLD);
        if (isDashing) attron(A_REVERSE);
        mvprintw((int)py - 1, 15, "@@");
        mvprintw((int)py, 15, "@@");
        attroff(A_REVERSE | A_BOLD);

        // Progress Bar
        int prog = (worldX / (cur.map.size() * 12)) * 100;
        attron(COLOR_PAIR(4));
        mvprintw(2, screenW/2 - 10, "PROGRESS: [");
        for(int i=0; i<20; ++i) {
            if (i < prog/5) addch('#'); else addch('.');
        }
        printw("] %d%%", prog);
        attroff(COLOR_PAIR(4));
    }

    void drawEndScreen(std::string msg) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(screenH/2, screenW/2 - msg.length()/2, "%s", msg.c_str());
        mvprintw(screenH/2 + 2, screenW/2 - 12, "PRESS [SPACE] TO MENU");
    }
};

int main() {
    GeometryDashPro game;
    game.run();
    return 0;
}
