/*
 * Rock Paper Scissors — SDL2 C++ Game
 * =====================================
 * Build (Linux/macOS):
 *   g++ rps_game.cpp -o rps_game $(sdl2-config --cflags --libs) -lSDL2_ttf -std=c++17
 *
 * Build (Windows with vcpkg):
 *   cl rps_game.cpp /I<vcpkg>/include /link SDL2.lib SDL2_ttf.lib SDL2main.lib /SUBSYSTEM:WINDOWS
 *
 * Dependencies: SDL2, SDL2_ttf
 *   Ubuntu:  sudo apt install libsdl2-dev libsdl2-ttf-dev
 *   macOS:   brew install sdl2 sdl2_ttf
 *   Windows: vcpkg install sdl2 sdl2-ttf
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>

// ── Window & layout ──────────────────────────────────────────────────────────
static constexpr int WIN_W  = 800;
static constexpr int WIN_H  = 600;
static constexpr int FPS    = 60;

// ── Colours ──────────────────────────────────────────────────────────────────
struct Col { Uint8 r, g, b, a; };
static constexpr Col BG        { 15,  12,  30, 255 };   // deep navy
static constexpr Col PANEL     { 28,  24,  55, 255 };   // dark purple panel
static constexpr Col ACCENT    {130,  80, 255, 255 };   // vivid violet
static constexpr Col TEXT_LT   {240, 235, 255, 255 };   // near-white
static constexpr Col TEXT_DIM  {140, 130, 180, 255 };   // muted purple
static constexpr Col WIN_COL   { 80, 230, 140, 255 };   // green win
static constexpr Col LOSE_COL  {230,  70,  80, 255 };   // red lose
static constexpr Col TIE_COL   {250, 200,  60, 255 };   // amber tie
static constexpr Col BTN_IDLE  { 45,  38,  80, 255 };   // button rest
static constexpr Col BTN_HOV   { 70,  55, 120, 255 };   // button hover
static constexpr Col BTN_PRESS { 95,  70, 160, 255 };   // button pressed
static constexpr Col BTN_BDR   {110,  80, 220, 255 };   // button border

// ── Helpers ──────────────────────────────────────────────────────────────────
static void setColor(SDL_Renderer* r, Col c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static void fillRect(SDL_Renderer* r, int x, int y, int w, int h) {
    SDL_Rect rc{x, y, w, h};
    SDL_RenderFillRect(r, &rc);
}

static void drawRoundedRect(SDL_Renderer* r, int x, int y, int w, int h, int radius, Col fill, Col border) {
    // Filled body (approximate rounded rect via overlapping rects + circles)
    setColor(r, fill);
    fillRect(r, x + radius, y,          w - 2*radius, h);
    fillRect(r, x,          y + radius, w,            h - 2*radius);

    auto fillCircle = [&](int cx, int cy, int rad, Col c) {
        setColor(r, c);
        for (int dy = -rad; dy <= rad; dy++)
            for (int dx = -rad; dx <= rad; dx++)
                if (dx*dx + dy*dy <= rad*rad)
                    SDL_RenderDrawPoint(r, cx+dx, cy+dy);
    };
    fillCircle(x+radius,   y+radius,   radius, fill);
    fillCircle(x+w-radius, y+radius,   radius, fill);
    fillCircle(x+radius,   y+h-radius, radius, fill);
    fillCircle(x+w-radius, y+h-radius, radius, fill);

    // Border outline
    setColor(r, border);
    SDL_RenderDrawLine(r, x+radius,   y,         x+w-radius, y);
    SDL_RenderDrawLine(r, x+radius,   y+h,       x+w-radius, y+h);
    SDL_RenderDrawLine(r, x,          y+radius,  x,          y+h-radius);
    SDL_RenderDrawLine(r, x+w,        y+radius,  x+w,        y+h-radius);
}

static SDL_Texture* makeText(SDL_Renderer* r, TTF_Font* font, const std::string& s, Col c) {
    SDL_Color sc{c.r, c.g, c.b, c.a};
    SDL_Surface* sf = TTF_RenderUTF8_Blended(font, s.c_str(), sc);
    if (!sf) return nullptr;
    SDL_Texture* tx = SDL_CreateTextureFromSurface(r, sf);
    SDL_FreeSurface(sf);
    return tx;
}

static void renderText(SDL_Renderer* r, TTF_Font* font,
                       const std::string& s, Col c,
                       int cx, int cy, bool centred = true)
{
    SDL_Texture* tx = makeText(r, font, s, c);
    if (!tx) return;
    int tw, th;
    SDL_QueryTexture(tx, nullptr, nullptr, &tw, &th);
    int x = centred ? cx - tw/2 : cx;
    int y = centred ? cy - th/2 : cy;
    SDL_Rect dst{x, y, tw, th};
    SDL_RenderCopy(r, tx, nullptr, &dst);
    SDL_DestroyTexture(tx);
}

// ── Emoji-style hand shapes drawn with SDL primitives ─────────────────────────
//
// Each hand is drawn in a 160×160 bounding box centred on (cx, cy).
// offset_y: vertical "bounce" displacement during animation.

static void drawRock(SDL_Renderer* r, int cx, int cy, float scale = 1.0f) {
    int s = static_cast<int>(60 * scale);
    // Fist: large oval
    setColor(r, {245, 200, 120, 255});
    for (int dy = -s; dy <= s; dy++)
        for (int dx = -s; dx <= s; dx++) {
            float ex = (float)dx / (s * 1.0f);
            float ey = (float)dy / (s * 0.9f);
            if (ex*ex + ey*ey <= 1.0f)
                SDL_RenderDrawPoint(r, cx+dx, cy+dy);
        }
    // Knuckle lines
    setColor(r, {200, 150, 80, 255});
    for (int i = -1; i <= 1; i++)
        SDL_RenderDrawLine(r, cx + i*18, cy - s + 6, cx + i*18, cy - s + 18);
    // Thumb bump
    setColor(r, {245, 200, 120, 255});
    for (int dy = -14; dy <= 14; dy++)
        for (int dx = -10; dx <= 10; dx++)
            if (dx*dx + dy*dy < 180)
                SDL_RenderDrawPoint(r, cx - s + 12 + dx, cy + 10 + dy);
}

static void drawPaper(SDL_Renderer* r, int cx, int cy, float scale = 1.0f) {
    int hw = static_cast<int>(48 * scale);
    int hh = static_cast<int>(66 * scale);
    // Palm
    setColor(r, {245, 200, 120, 255});
    fillRect(r, cx - hw, cy - 10, hw*2, hh);
    // Four fingers
    int fw = static_cast<int>(16 * scale);
    for (int i = 0; i < 4; i++) {
        int fx = cx - hw + i*(fw + 5);
        int fy = cy - 10 - static_cast<int>(50 * scale);
        setColor(r, {245, 200, 120, 255});
        fillRect(r, fx, fy, fw, static_cast<int>(54 * scale));
        // Fingertip rounded hint
        for (int dy = -fw/2; dy <= 0; dy++)
            for (int dx = -fw/2; dx <= fw/2; dx++)
                if (dx*dx + dy*dy <= (fw/2)*(fw/2))
                    SDL_RenderDrawPoint(r, fx + fw/2 + dx, fy + dy);
    }
    // Thumb
    setColor(r, {245, 200, 120, 255});
    fillRect(r, cx + hw + 2, cy + 10, static_cast<int>(22 * scale), static_cast<int>(30 * scale));
    // Knuckle lines
    setColor(r, {200, 150, 80, 255});
    for (int i = 0; i < 4; i++) {
        int fx = cx - hw + i*(fw + 5) + fw/2;
        SDL_RenderDrawLine(r, fx, cy - 6, fx, cy + 10);
    }
}

static void drawScissors(SDL_Renderer* r, int cx, int cy, float scale = 1.0f) {
    int hw = static_cast<int>(48 * scale);
    int hh = static_cast<int>(66 * scale);
    // Palm
    setColor(r, {245, 200, 120, 255});
    fillRect(r, cx - hw, cy - 10, hw*2, hh);
    // Two extended fingers
    int fw = static_cast<int>(17 * scale);
    int fh = static_cast<int>(55 * scale);
    for (int i = 0; i < 2; i++) {
        int fx = cx - hw/2 + i*(fw + 6);
        int fy = cy - 10 - fh;
        // slight V-spread
        int spread = (i == 0) ? -8 : 8;
        setColor(r, {245, 200, 120, 255});
        // Rotated finger approximation via shifting x
        for (int row = 0; row < fh; row++) {
            int shift = spread * row / fh;
            SDL_RenderDrawLine(r, fx + shift, fy + row, fx + fw + shift, fy + row);
        }
        // Fingertip
        for (int dy = -fw/2; dy <= 0; dy++)
            for (int dx = -fw/2; dx <= fw/2; dx++)
                if (dx*dx + dy*dy <= (fw/2)*(fw/2)) {
                    int topSpread = spread;
                    SDL_RenderDrawPoint(r, fx + fw/2 + dx + topSpread, fy + dy);
                }
    }
    // Folded fingers (smaller bumps)
    setColor(r, {220, 170, 100, 255});
    fillRect(r, cx + hw - static_cast<int>(30*scale), cy - 14, static_cast<int>(26*scale), static_cast<int>(22*scale));
    fillRect(r, cx - hw + 4, cy - 14, static_cast<int>(20*scale), static_cast<int>(18*scale));
    // Thumb
    setColor(r, {245, 200, 120, 255});
    fillRect(r, cx + hw + 2, cy + 10, static_cast<int>(22*scale), static_cast<int>(30*scale));
    // Knuckle lines on visible fingers
    setColor(r, {200, 150, 80, 255});
    for (int i = 0; i < 2; i++) {
        int fx = cx - hw/2 + i*(fw + 6) + fw/2;
        SDL_RenderDrawLine(r, fx, cy - 6, fx, cy + 10);
    }
}

// ── Game State ────────────────────────────────────────────────────────────────
enum class Choice { NONE = -1, ROCK = 0, PAPER = 1, SCISSORS = 2 };
enum class GameState { IDLE, ANIMATING, RESULT };
enum class Result { NONE, WIN, LOSE, TIE };

static const char* choiceName(Choice c) {
    switch (c) {
        case Choice::ROCK:     return "Rock";
        case Choice::PAPER:    return "Paper";
        case Choice::SCISSORS: return "Scissors";
        default:               return "...";
    }
}

static Result evaluate(Choice player, Choice cpu) {
    if (player == cpu) return Result::TIE;
    if ((player == Choice::ROCK     && cpu == Choice::SCISSORS) ||
        (player == Choice::SCISSORS && cpu == Choice::PAPER)    ||
        (player == Choice::PAPER    && cpu == Choice::ROCK))
        return Result::WIN;
    return Result::LOSE;
}

// ── Button ────────────────────────────────────────────────────────────────────
struct Button {
    int x, y, w, h;
    Choice choice;
    bool hovered = false;
    bool pressed = false;

    bool contains(int mx, int my) const {
        return mx >= x && mx <= x+w && my >= y && my <= y+h;
    }

    void draw(SDL_Renderer* r, TTF_Font* font, TTF_Font* bigFont) const {
        Col fill   = pressed ? BTN_PRESS : (hovered ? BTN_HOV : BTN_IDLE);
        drawRoundedRect(r, x, y, w, h, 14, fill, BTN_BDR);

        // Mini hand icon
        float scale = 0.55f;
        int iconCY = y + h/2 - 16;
        switch (choice) {
            case Choice::ROCK:     drawRock(r,     x+w/2, iconCY, scale); break;
            case Choice::PAPER:    drawPaper(r,    x+w/2, iconCY, scale); break;
            case Choice::SCISSORS: drawScissors(r, x+w/2, iconCY, scale); break;
            default: break;
        }
        // Label
        renderText(r, font, choiceName(choice), TEXT_LT, x+w/2, y+h - 24);
    }
};

// ── Main ──────────────────────────────────────────────────────────────────────
int main(int /*argc*/, char** /*argv*/) {
    srand(static_cast<unsigned>(time(nullptr)));

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow(
        "Rock · Paper · Scissors",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, 0
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Fonts — tries common system paths; falls back to default if missing
    auto tryFont = [](const char* path, int pt) -> TTF_Font* {
        TTF_Font* f = TTF_OpenFont(path, pt);
        return f;
    };
    TTF_Font* fontLg = tryFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 32);
    TTF_Font* fontMd = tryFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",      20);
    TTF_Font* fontSm = tryFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",      15);
    // macOS fallback
    if (!fontLg) fontLg = tryFont("/Library/Fonts/Arial Bold.ttf", 32);
    if (!fontMd) fontMd = tryFont("/Library/Fonts/Arial.ttf",      20);
    if (!fontSm) fontSm = tryFont("/Library/Fonts/Arial.ttf",      15);
    // Last-resort: open any font we find
    if (!fontLg) fontLg = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", 32);
    if (!fontMd) fontMd = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 20);
    if (!fontSm) fontSm = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 15);

    // Buttons — three hand choices, centred horizontally
    const int BTN_W = 160, BTN_H = 160, BTN_GAP = 24;
    const int totalW = 3*BTN_W + 2*BTN_GAP;
    const int startX = (WIN_W - totalW) / 2;
    const int btnY   = WIN_H - BTN_H - 40;

    Button buttons[3] = {
        {startX + 0*(BTN_W+BTN_GAP), btnY, BTN_W, BTN_H, Choice::ROCK},
        {startX + 1*(BTN_W+BTN_GAP), btnY, BTN_W, BTN_H, Choice::PAPER},
        {startX + 2*(BTN_W+BTN_GAP), btnY, BTN_W, BTN_H, Choice::SCISSORS},
    };

    // Game state
    GameState state     = GameState::IDLE;
    Result    result    = Result::NONE;
    Choice    playerChoice = Choice::NONE;
    Choice    cpuChoice    = Choice::NONE;
    int       playerScore  = 0;
    int       cpuScore     = 0;

    // Animation
    float animT      = 0.0f;  // 0..1
    float animBounce = 0.0f;  // pixel offset for player hand
    float cpuRevealT = 0.0f;  // 0..1, cpu hand fade-in
    bool  cpuShown   = false;

    const float ANIM_SPEED  = 2.0f;   // bounces per second
    const float ANIM_DUR    = 1.8f;   // seconds of animation
    const float REVEAL_DUR  = 0.6f;

    Uint32 lastTick = SDL_GetTicks();

    bool running = true;
    SDL_Event e;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTick) / 1000.0f;
        lastTick = now;

        // ── Events ───────────────────────────────────────────────────────────
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;

            if (state == GameState::IDLE || state == GameState::RESULT) {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    for (auto& btn : buttons) {
                        if (btn.contains(e.button.x, e.button.y)) {
                            playerChoice = btn.choice;
                            cpuChoice    = static_cast<Choice>(rand() % 3);
                            state        = GameState::ANIMATING;
                            animT        = 0.0f;
                            cpuRevealT   = 0.0f;
                            cpuShown     = false;
                        }
                    }
                }
                // Reset button area — click anywhere in top half resets scores
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
                    playerScore = cpuScore = 0;
                    state = GameState::IDLE;
                    playerChoice = cpuChoice = Choice::NONE;
                    result = Result::NONE;
                }
            }

            // Button hover/press states
            for (auto& btn : buttons) {
                btn.hovered = btn.contains(mx, my);
                btn.pressed = btn.hovered &&
                              (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK);
            }
        }

        // ── Animation update ─────────────────────────────────────────────────
        if (state == GameState::ANIMATING) {
            animT += dt;

            // Bounce: sin wave
            float progress = animT / ANIM_DUR;
            float bounceFreq = 3.5f;
            animBounce = sinf(progress * bounceFreq * 3.14159f * 2.0f) * 28.0f * (1.0f - progress*0.6f);

            if (animT >= ANIM_DUR) {
                animBounce = 0;
                cpuShown   = true;
                result = evaluate(playerChoice, cpuChoice);
                if (result == Result::WIN)  playerScore++;
                else if (result == Result::LOSE) cpuScore++;
                state = GameState::RESULT;
            }
        }
        if (state == GameState::RESULT) {
            cpuRevealT = std::min(cpuRevealT + dt / REVEAL_DUR, 1.0f);
        }

        // ── Render ───────────────────────────────────────────────────────────
        setColor(renderer, BG);
        SDL_RenderClear(renderer);

        // Title
        if (fontLg)
            renderText(renderer, fontLg, "Rock · Paper · Scissors", TEXT_LT, WIN_W/2, 42);

        // Score board
        drawRoundedRect(renderer, WIN_W/2 - 120, 74, 240, 50, 10, PANEL, BTN_BDR);
        if (fontMd) {
            renderText(renderer, fontMd, std::to_string(playerScore), WIN_COL, WIN_W/2 - 50, 99);
            renderText(renderer, fontMd, "—", TEXT_DIM, WIN_W/2, 99);
            renderText(renderer, fontMd, std::to_string(cpuScore),    LOSE_COL, WIN_W/2 + 50, 99);
        }
        if (fontSm) {
            renderText(renderer, fontSm, "YOU", TEXT_DIM, WIN_W/2 - 50, 118);
            renderText(renderer, fontSm, "CPU", TEXT_DIM, WIN_W/2 + 50, 118);
        }

        // Arena panels
        const int ARENA_Y = 148, ARENA_H = 240, ARENA_W = 280;
        drawRoundedRect(renderer, WIN_W/2 - ARENA_W - 20, ARENA_Y, ARENA_W, ARENA_H, 16, PANEL, BTN_BDR);
        drawRoundedRect(renderer, WIN_W/2 + 20,           ARENA_Y, ARENA_W, ARENA_H, 16, PANEL, BTN_BDR);

        if (fontSm) {
            renderText(renderer, fontSm, "YOU", TEXT_DIM, WIN_W/2 - ARENA_W/2 - 20, ARENA_Y + 18);
            renderText(renderer, fontSm, "CPU", TEXT_DIM, WIN_W/2 + ARENA_W/2 + 20, ARENA_Y + 18);
        }

        // Player hand
        if (playerChoice != Choice::NONE) {
            int handCX = WIN_W/2 - ARENA_W/2 - 20;
            int handCY = ARENA_Y + ARENA_H/2 + static_cast<int>(animBounce);
            switch (playerChoice) {
                case Choice::ROCK:     drawRock(renderer,     handCX, handCY); break;
                case Choice::PAPER:    drawPaper(renderer,    handCX, handCY); break;
                case Choice::SCISSORS: drawScissors(renderer, handCX, handCY); break;
                default: break;
            }
        }

        // CPU hand (fade in after reveal)
        if (cpuShown && cpuChoice != Choice::NONE) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            Uint8 alpha = static_cast<Uint8>(std::min(255.0f, cpuRevealT * 255.0f));
            SDL_SetTextureAlphaMod(nullptr, alpha); // alpha modulation via tint trick below
            int handCX = WIN_W/2 + ARENA_W/2 + 20;
            int handCY = ARENA_Y + ARENA_H/2;
            // Draw cpu hand (flipped: mirror image via negative offset)
            switch (cpuChoice) {
                case Choice::ROCK:     drawRock(renderer,     handCX, handCY); break;
                case Choice::PAPER:    drawPaper(renderer,    handCX, handCY); break;
                case Choice::SCISSORS: drawScissors(renderer, handCX, handCY); break;
                default: break;
            }
            // Overlay fade
            if (cpuRevealT < 1.0f) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, PANEL.r, PANEL.g, PANEL.b,
                                       static_cast<Uint8>((1.0f - cpuRevealT) * 220));
                SDL_Rect coverRect{WIN_W/2 + 20, ARENA_Y, ARENA_W, ARENA_H};
                SDL_RenderFillRect(renderer, &coverRect);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        } else if (!cpuShown && state == GameState::ANIMATING) {
            // Show "?" during animation
            if (fontLg)
                renderText(renderer, fontLg, "?", TEXT_DIM,
                           WIN_W/2 + ARENA_W/2 + 20, ARENA_Y + ARENA_H/2);
        }

        // Animating — show "throwing" text
        if (state == GameState::ANIMATING && fontMd) {
            int dots = (static_cast<int>(animT * 3)) % 4;
            std::string throwing = "Throwing";
            for (int i = 0; i < dots; i++) throwing += ".";
            renderText(renderer, fontMd, throwing, TEXT_DIM, WIN_W/2, ARENA_Y + ARENA_H + 16);
        }

        // Result display
        if (state == GameState::RESULT && fontLg && fontMd) {
            const char* resultStr = "Tie!";
            Col rCol = TIE_COL;
            if (result == Result::WIN)  { resultStr = "You Win!";  rCol = WIN_COL; }
            if (result == Result::LOSE) { resultStr = "You Lose!"; rCol = LOSE_COL; }
            renderText(renderer, fontLg, resultStr, rCol, WIN_W/2, ARENA_Y + ARENA_H + 18);

            // Choice names
            std::string sub = std::string(choiceName(playerChoice)) +
                              " vs " + choiceName(cpuChoice);
            renderText(renderer, fontSm, sub, TEXT_DIM, WIN_W/2, ARENA_Y + ARENA_H + 52);

            // Hint
            renderText(renderer, fontSm, "Click a hand below to play again",
                       TEXT_DIM, WIN_W/2, ARENA_Y + ARENA_H + 76);
        }

        if (state == GameState::IDLE && fontMd)
            renderText(renderer, fontMd, "Choose your hand to begin!",
                       TEXT_DIM, WIN_W/2, ARENA_Y + ARENA_H/2);

        // Buttons
        for (const auto& btn : buttons)
            btn.draw(renderer, fontSm, fontMd);

        // Right-click hint
        if (fontSm)
            renderText(renderer, fontSm, "Right-click to reset scores",
                       TEXT_DIM, WIN_W/2, WIN_H - 14);

        SDL_RenderPresent(renderer);
        SDL_Delay(1000/FPS);
    }

    if (fontLg) TTF_CloseFont(fontLg);
    if (fontMd) TTF_CloseFont(fontMd);
    if (fontSm) TTF_CloseFont(fontSm);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
