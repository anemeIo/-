#include <SDL.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

#define RESOURCES_DIR "./resources/"

class Sprite {
public:
    Sprite(SDL_Renderer* renderer, const std::string& filename, int width) : width(width) {
        SDL_Surface* surface = SDL_LoadBMP((std::string(RESOURCES_DIR) + filename).c_str());
        if (!surface) {
            std::cerr << "Error in SDL_LoadBMP: " << SDL_GetError() << std::endl;
            return;
        }
        if (!(surface->w % width) && surface->w / width) { // image width must be a multiple of sprite width
            height = surface->h;
            nframes = surface->w / width;
            texture = SDL_CreateTextureFromSurface(renderer, surface);
        }
        else {
            std::cerr << "Incorrect sprite size" << std::endl;
        }
        SDL_FreeSurface(surface);
    }

    SDL_Rect rect(int idx) const { // choose the sprite number idx from the texture
        return { idx * width, 0, width, height };
    }

    ~Sprite() { // do not forget to free the memory!
        if (texture) SDL_DestroyTexture(texture);
    }

    SDL_Texture* texture = nullptr; // the image is to be stored here
    int width = 0; // single sprite width (texture width = width * nframes)
    int height = 0; // sprite height
    int nframes = 0; // number of frames in the animation sequence
};

class Cockroach {
public:
    Cockroach(const std::string& name, int lane, SDL_Renderer* renderer)
        : name(name), lane(lane), renderer(renderer), x(0), stopped(false), stopTime(0), sprite(renderer, "cockroach.bmp", 50) {
        y = 50 + lane * 100;
        speed = rand() % 5 + 1;
        rect = { x, y, sprite.width, sprite.height };
    }

    void update() {
        if (!stopped) {
            x += speed;
            rect.x = x;
        }
    }

    void render() const {
        if (sprite.texture) {
            SDL_Rect srcRect = sprite.rect(0); // Use the first frame for simplicity
            SDL_RenderCopy(renderer, sprite.texture, &srcRect, &rect);
        }
    }

    bool hasFinished() const {
        return x >= 750;
    }

    std::string getName() const {
        return name;
    }

    bool isStopped() const {
        return stopped;
    }

    int getStopTime() const {
        return stopTime;
    }

    void decrementStopTime() {
        --stopTime;
    }

    void stop(int duration) {
        stopped = true;
        stopTime = duration;
    }

    void resume() {
        stopped = false;
        stopTime = 0;
    }

private:
    std::string name;
    int lane;
    int x;
    int y;
    int speed;
    SDL_Renderer* renderer;
    SDL_Rect rect;
    Sprite sprite; // Sprite ��� �����������

    bool stopped;
    int stopTime;
};

class Player {
public:
    Player(const std::string& name, int cockroachIndex)
        : name(name), cockroachIndex(cockroachIndex) {}

    std::string name;
    int cockroachIndex;
};

class Game {
public:
    Game() : window(nullptr), renderer(nullptr), running(false), gameStarted(false), numPlayers(0), currentInput(0) {}

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        window = SDL_CreateWindow("Cockroach Race", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        srand(static_cast<unsigned int>(time(0)));

        cockroaches.push_back(Cockroach("Cockroach 1", 0, renderer));
        cockroaches.push_back(Cockroach("Cockroach 2", 1, renderer));
        cockroaches.push_back(Cockroach("Cockroach 3", 2, renderer));
        cockroaches.push_back(Cockroach("Cockroach 4", 3, renderer));
        cockroaches.push_back(Cockroach("Cockroach 5", 4, renderer));

        running = true;
        return true;
    }

    void run() {
        while (running) {
            if (!gameStarted) {
                handleStartScreenEvents();
                renderStartScreen();
            }
            else {
                handleEvents();
                update();
                render();
                SDL_Delay(16); // ����������� FPS �� �������� 60
            }
        }
    }

    void clean() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    bool gameStarted;
    int numPlayers;
    std::vector<Player> players;
    std::vector<Cockroach> cockroaches;

    int currentInput;
    std::vector<std::string> playerInputs;

    void handleStartScreenEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                if (x >= 350 && x <= 450 && y >= 500 && y <= 550) {
                    // Check if player inputs are valid
                    if (numPlayers > 0 && validatePlayerInputs()) {
                        gameStarted = true;
                    }
                }
                // Detect number of players button click
                for (int i = 0; i < 5; ++i) {
                    if (x >= 50 && x <= 250 && y >= 50 + i * 60 && y <= 100 + i * 60) {
                        numPlayers = i + 1;
                        playerInputs.resize(numPlayers);
                        currentInput = 0;
                    }
                }
            }
            else if (e.type == SDL_TEXTINPUT || e.type == SDL_KEYDOWN) {
                handleTextInput(e);
            }
        }
    }

    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }
    }

    void update() {
        for (auto& cockroach : cockroaches) {
            if (!cockroach.isStopped()) {
                cockroach.update();
            }
            else {
                cockroach.decrementStopTime();
            }

            // Check if the cockroach has finished its stop time
            if (cockroach.isStopped() && cockroach.getStopTime() <= 0) {
                cockroach.resume(); // Resume the cockroach
            }
        }

        for (const auto& cockroach : cockroaches) {
            if (cockroach.hasFinished()) {
                std::cout << cockroach.getName() << " wins!" << std::endl;
                resetRace();
                break;
            }
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // ������ ����
        SDL_RenderClear(renderer);

        for (const auto& cockroach : cockroaches) {
            cockroach.render();
        }

        renderPlayerNames();

        SDL_RenderPresent(renderer);
    }

    void renderStartScreen() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // ������ ����
        SDL_RenderClear(renderer);

        // Render buttons for number of players
        for (int i = 0; i < 5; ++i) {
            SDL_Rect buttonRect = { 50, 50 + i * 60, 200, 50 };
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // ������� ����
            SDL_RenderFillRect(renderer, &buttonRect);
            renderText("Players: " + std::to_string(i + 1), 60, 60 + i * 60);
        }

        // Render player input fields
        for (int i = 0; i < numPlayers; ++i) {
            renderText("Player " + std::to_string(i + 1) + " Name:", 300, 50 + i * 60);
            renderText(playerInputs[i], 500, 50 + i * 60);
        }

        // Render start button
        SDL_Rect buttonRect = { 350, 500, 100, 50 };
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // ������� ����
        SDL_RenderFillRect(renderer, &buttonRect);
        renderText("Start Game", 360, 510);

        SDL_RenderPresent(renderer);
    }

    void renderPlayerNames() {
        for (int i = 0; i < players.size(); ++i) {
            renderText(players[i].name, 50, 50 + i * 100);
        }
    }

    void renderText(const std::string& message, int x, int y) {
        // Here we simply draw rectangles to simulate text rendering
        SDL_Rect rect = { x, y, static_cast<int>(message.size() * 10), 20 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // ����� ����
        SDL_RenderFillRect(renderer, &rect);
    }

    void resetRace() {
        cockroaches.clear();
        cockroaches.push_back(Cockroach("Cockroach 1", 0, renderer));
        cockroaches.push_back(Cockroach("Cockroach 2", 1, renderer));
        cockroaches.push_back(Cockroach("Cockroach 3", 2, renderer));
        cockroaches.push_back(Cockroach("Cockroach 4", 3, renderer));
        cockroaches.push_back(Cockroach("Cockroach 5", 4, renderer));
    }

    bool validatePlayerInputs() {
        for (int i = 0; i < numPlayers; ++i) {
            if (playerInputs[i].empty()) {
                return false;
            }
            players.push_back(Player(playerInputs[i], i));
        }
        return true;
    }

    void handleTextInput(SDL_Event& e) {
        if (e.type == SDL_TEXTINPUT) {
            playerInputs[currentInput] += e.text.text;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE && !playerInputs[currentInput].empty()) {
            playerInputs[currentInput].pop_back();
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
            if (++currentInput >= numPlayers) {
                currentInput = 0;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    Game game;
    if (!game.init()) {
        return -1;
    }

    game.run();
    game.clean();
    return 0;
}
