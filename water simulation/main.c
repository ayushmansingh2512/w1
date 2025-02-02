#include <stdio.h>
#include <SDL.h>
#include <math.h>

#define SCREEN_HEIGHT 800
#define SCREEN_WIDTH 1200 
#define COLOR_WHITE 0xfff 0x00000000fffff
#define COLOR_BLACK
#define COLOR_BLUE 0x34c3eb
#define COLOR_DARK_BLUE 0x1a6199
#define COLOR_GRAY 0x1f1f1f1f
#define CELL_SIZE 10
#define LINE_WIDTH 2
#define COLUMNS (SCREEN_WIDTH / CELL_SIZE)
#define ROWS (SCREEN_HEIGHT / CELL_SIZE)
#define SOLID_TYPE 1
#define WATER_TYPE 0

// Physics constants
#define GRAVITY 0.15
#define FLOW_RATE 0.08
#define MAX_PRESSURE 2.0
#define MIN_FLOW 0.01
#define MAX_VELOCITY 1.5
#define DAMPING 0.9
#define NEIGHBOR_CHECK_RADIUS 2
#define VERTICAL_FLOW_MULTIPLIER 1.1
#define HORIZONTAL_FLOW_MULTIPLIER 0.6

struct Cell {
    int type;           // Type of cell (WATER_TYPE or SOLID_TYPE)
    double fill_level;  // Water fill level (0 to 1)
    double velocity_y;  // Vertical velocity
    double velocity_x;  // Horizontal velocity
    double pressure;    // Water pressure
    int x, y;           // Cell coordinates
};

// Draw a single cell
void draw_cell(SDL_Surface* surface, struct Cell cell) {
    int pixel_x = cell.x * CELL_SIZE;
    int pixel_y = cell.y * CELL_SIZE;
    SDL_Rect cell_rect = { pixel_x, pixel_y, CELL_SIZE, CELL_SIZE };

    if (cell.type == SOLID_TYPE) {
        SDL_FillRect(surface, &cell_rect, COLOR_WHITE);
    }
    else if (cell.type == WATER_TYPE && cell.fill_level > 0) {
        int water_height = (int)(cell.fill_level * CELL_SIZE);
        int empty_height = CELL_SIZE - water_height;
        SDL_Rect water_rect = { pixel_x, pixel_y + empty_height, CELL_SIZE, water_height };

        // Dynamic color gradient
        Uint8 blue = (Uint8)(200 + (55 * cell.fill_level)); // Darker blue for lower levels
        Uint32 color = SDL_MapRGB(surface->format, 0, 0, blue);

        SDL_FillRect(surface, &cell_rect, COLOR_BLACK);
        SDL_FillRect(surface, &water_rect, color);
    }
    else {
        SDL_FillRect(surface, &cell_rect, COLOR_BLACK);
    }
}


// Draw the grid
void draw_grid(SDL_Surface* surface) {
    for (int i = 0; i < COLUMNS; i++) {
        SDL_Rect column = { i * CELL_SIZE, 0, LINE_WIDTH, SCREEN_HEIGHT };
        SDL_FillRect(surface, &column, COLOR_GRAY);
    }
    for (int j = 0; j < ROWS; j++) {
        SDL_Rect row = { 0, j * CELL_SIZE, SCREEN_WIDTH, LINE_WIDTH };
        SDL_FillRect(surface, &row, COLOR_GRAY);
    }
}

// Draw the entire environment
void draw_environment(SDL_Surface* surface, struct Cell environment[ROWS][COLUMNS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            draw_cell(surface, environment[i][j]);
        }
    }
}

// Initialize the environment
void initialize_environment(struct Cell environment[ROWS][COLUMNS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            environment[i][j] = (struct Cell){
                .type = WATER_TYPE,
                .fill_level = 0,
                .velocity_y = 0,
                .velocity_x = 0,
                .pressure = 0,
                .x = j,
                .y = i
            };
        }
    }
}

// Simulate water dynamics
// Simulate water dynamics
void simulation_step(struct Cell environment[ROWS][COLUMNS]) {
    for (int i = ROWS - 1; i >= 0; i--) {
        for (int j = 0; j < COLUMNS; j++) {
            struct Cell* cell = &environment[i][j];

            if (cell->type == WATER_TYPE && cell->fill_level > 0) {
                // Apply gravity and move downwards
                if (i < ROWS - 1) {
                    struct Cell* below = &environment[i + 1][j];
                    if (below->type == WATER_TYPE && below->fill_level < 1.0) {
                        double flow = fmin(cell->fill_level, 1.0 - below->fill_level);
                        flow = fmin(flow, GRAVITY * VERTICAL_FLOW_MULTIPLIER);

                        if (flow > MIN_FLOW) {
                            double actual_flow = fmin(flow, cell->fill_level);
                            cell->fill_level -= actual_flow;
                            below->fill_level += actual_flow;

                            // Add downward velocity for realism
                            below->velocity_y += actual_flow;
                        }
                    }
                }

                // Horizontal spreading with smoothing
                for (int dx = -1; dx <= 1; dx += 2) {
                    if (j + dx >= 0 && j + dx < COLUMNS) {
                        struct Cell* neighbor = &environment[i][j + dx];
                        if (neighbor->type == WATER_TYPE && neighbor->fill_level < cell->fill_level) {
                            double flow = (cell->fill_level - neighbor->fill_level) * FLOW_RATE;
                            flow = fmin(flow, cell->fill_level);
                            flow = fmin(flow, 1.0 - neighbor->fill_level);
                            flow *= HORIZONTAL_FLOW_MULTIPLIER;

                            if (flow > MIN_FLOW) {
                                cell->fill_level -= flow;
                                neighbor->fill_level += flow;

                                // Adjust horizontal velocity
                                neighbor->velocity_x += flow * (dx > 0 ? 1 : -1);
                            }
                        }
                    }
                }

                // Pressure effect: distribute water within neighbors
                double pressure = fmin(cell->fill_level - 0.5, MAX_PRESSURE);
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dy == 0 && dx == 0) continue;
                        int neighbor_x = j + dx;
                        int neighbor_y = i + dy;
                        if (neighbor_x >= 0 && neighbor_x < COLUMNS && neighbor_y >= 0 && neighbor_y < ROWS) {
                            struct Cell* neighbor = &environment[neighbor_y][neighbor_x];
                            if (neighbor->type == WATER_TYPE && neighbor->fill_level < cell->fill_level) {
                                double flow = pressure * FLOW_RATE;
                                flow = fmin(flow, cell->fill_level);
                                flow = fmin(flow, 1.0 - neighbor->fill_level);
                                if (flow > MIN_FLOW) {
                                    cell->fill_level -= flow;
                                    neighbor->fill_level += flow;
                                }
                            }
                        }
                    }
                }

                // Apply damping to velocities
                cell->velocity_y *= DAMPING;
                cell->velocity_x *= DAMPING;
            }
        }
    }
}
int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Liquid Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    struct Cell environment[ROWS][COLUMNS];
    initialize_environment(environment);

    int simulation_running = 1;
    SDL_Event event;
    int current_type = SOLID_TYPE;
    int delete_mode = 0;

    while (simulation_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                simulation_running = 0;
            }
            if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.motion.state) {
                    int cell_x = event.motion.x / CELL_SIZE;
                    int cell_y = event.motion.y / CELL_SIZE;

                    if (cell_x >= 0 && cell_x < COLUMNS && cell_y >= 0 && cell_y < ROWS) {
                        struct Cell* cell = &environment[cell_y][cell_x];
                        if (delete_mode) {
                            cell->type = WATER_TYPE;
                            cell->fill_level = 0;
                        }
                        else {
                            cell->type = current_type;
                            cell->fill_level = (current_type == WATER_TYPE) ? 1.0 : 0;
                        }
                    }
                }
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    current_type = !current_type;
                }
                if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    delete_mode = !delete_mode;
                }
            }
        }

        simulation_step(environment);
        draw_environment(surface, environment);
        draw_grid(surface);
        SDL_UpdateWindowSurface(window);
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
