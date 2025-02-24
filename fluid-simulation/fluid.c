#include <stdio.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000
#define COLOR_BLUE 0x03A9F4
#define COLOR_BROWN 0x201513
#define CELL_SIZE 10
#define LINE_WIDTH 2
#define COLUMNS SCREEN_WIDTH/CELL_SIZE
#define ROWS SCREEN_HEIGHT/CELL_SIZE
#define SOLID_TYPE 1
#define WATER_TYPE 0

struct Cell 
{
    int type;
    double fill_level; // 0 empty, 1 full
    int x;
    int y;
};

void draw_cell(SDL_Surface* surface, struct Cell cell, int fill_cell){
    int pixel_x = cell.x*CELL_SIZE;
    int pixel_y = cell.y*CELL_SIZE;
    SDL_Rect cell_rect = (SDL_Rect){pixel_x, pixel_y, CELL_SIZE, CELL_SIZE};
    // Background color
    SDL_FillRect(surface, &cell_rect, COLOR_BLACK);
    // Water fill level
    if(cell.type == WATER_TYPE){        
        if(fill_cell == 0){
            double water_height = cell.fill_level > 1 ? CELL_SIZE : cell.fill_level * CELL_SIZE; // Avoid overflowing weird behavior
            double empty_height = CELL_SIZE - water_height;
            SDL_Rect water_rect = (SDL_Rect){pixel_x, (pixel_y + empty_height + 1), CELL_SIZE, water_height};
            SDL_FillRect(surface, &water_rect, COLOR_BLUE);
        }
        else{
            SDL_FillRect(surface,&cell_rect, COLOR_BLUE);   
        }
    }
    // Solid color
    if(cell.type == SOLID_TYPE){
        SDL_FillRect(surface, &cell_rect, COLOR_WHITE);
    }
}

void draw_environment(SDL_Surface* surface, struct Cell environment[ROWS*COLUMNS]){
    for(int i=0; i<ROWS*COLUMNS; i++){
        draw_cell(surface,environment[i], 0);
    }
    // Making the fountain be continuous 
    for(int i=0; i<ROWS; i++){
        for(int j=0; j<COLUMNS; j++){
            struct Cell above_cell = environment[j+COLUMNS*(i-1)];
            struct Cell current_cell = environment[j+COLUMNS*i];

            if(i>0 && above_cell.type == WATER_TYPE && above_cell.fill_level > 0.015 && current_cell.fill_level > 0.015 && current_cell.type == WATER_TYPE){
                draw_cell(surface, environment[j+COLUMNS*i], 1);
            }
        }
    }
}

void draw_grid(SDL_Surface* surface) {
    for (int i=0; i<COLUMNS; i++){
        SDL_Rect column = (SDL_Rect){i*CELL_SIZE, 0, LINE_WIDTH, SCREEN_HEIGHT};
        SDL_FillRect(surface, &column, COLOR_BROWN);
    }
    for (int j=0; j<ROWS; j++){
        SDL_Rect row = (SDL_Rect){0, j*CELL_SIZE, SCREEN_WIDTH, LINE_WIDTH};
        SDL_FillRect(surface, &row, COLOR_BROWN);
    }
}

void initialize_environment(struct Cell environment[ROWS*COLUMNS]){
    for (int i=0; i<ROWS; i++){
        for(int j=0; j<COLUMNS; j++){
            environment[j + COLUMNS*i] = (struct Cell){WATER_TYPE, 0, j, i};
        }
    }
}

void water_flow_down(struct Cell environment[ROWS*COLUMNS]) {
    struct Cell environment_copy[ROWS*COLUMNS];
    for (int i=0; i<ROWS*COLUMNS; i++){
        environment_copy[i] = environment[i];
    }

    for (int i = ROWS-2; i >= 0; i--) {  // Starts upside down
        for (int j = 0; j < COLUMNS; j++) {
            struct Cell current_cell = environment[j + COLUMNS*i];
            struct Cell bottom_cell = environment[j + COLUMNS*(i+1)];
            
            if (current_cell.type == WATER_TYPE && current_cell.fill_level > 0) {
                if (bottom_cell.type == WATER_TYPE && bottom_cell.fill_level < 1) {
                    double transfer_amount = 0.2; // Sets a speed for it to flow
                    if (current_cell.fill_level < transfer_amount) {
                        transfer_amount = current_cell.fill_level;
                    }
                    if (bottom_cell.fill_level + transfer_amount > 1) {
                        transfer_amount = 1 - bottom_cell.fill_level;
                    }
                    
                    environment_copy[j + COLUMNS*i].fill_level -= transfer_amount;
                    environment_copy[j + COLUMNS*(i+1)].fill_level += transfer_amount;
                }
            }
        }
    }

    for (int i=0; i<ROWS*COLUMNS; i++){
        environment[i] = environment_copy[i];
    }
}

void water_flow_horizntally(struct Cell environment[ROWS*COLUMNS]){

    struct Cell environment_copy[ROWS*COLUMNS];
    for (int i=0; i<ROWS*COLUMNS; i++){
        environment_copy[i] = environment[i];
    }

    for (int i=0; i<ROWS; i++){
        for (int j=0; j<COLUMNS; j++){
            // Water flow left and right
            struct Cell current_cell = environment[j + COLUMNS*i];
            struct Cell bottom_cell = environment[j + COLUMNS*(i+1)];
            if ((i+1 == ROWS || bottom_cell.fill_level >= current_cell.fill_level ) || bottom_cell.type == SOLID_TYPE)
            {
                // Left
                if(current_cell.type == WATER_TYPE && j>0)
                {
                    struct Cell destination_cell = environment[(j-1) + COLUMNS*i];
                    if(destination_cell.type == WATER_TYPE && destination_cell.fill_level < current_cell.fill_level)
                    {
                        double delta_fill = current_cell.fill_level - destination_cell.fill_level;
                        environment_copy[j+COLUMNS*i].fill_level -= delta_fill/3;
                        environment_copy[(j-1)+COLUMNS*i].fill_level += delta_fill/3;
                    }
                }
                // Right
                if(current_cell.type == WATER_TYPE && j<COLUMNS-1)
                {
                    struct Cell destination_cell = environment[(j+1) + COLUMNS*i];
                    if(destination_cell.type == WATER_TYPE && destination_cell.fill_level < current_cell.fill_level)
                    {
                        double delta_fill = current_cell.fill_level - destination_cell.fill_level;
                        environment_copy[j+COLUMNS*i].fill_level -= delta_fill/3;
                        environment_copy[(j+1)+COLUMNS*i].fill_level += delta_fill/3;
                    }
                }
            }
        }
    }
    for (int i=0; i<ROWS*COLUMNS; i++){
        environment[i] = environment_copy[i];
    }
}

void water_pressure(struct Cell environment[ROWS*COLUMNS]){

    struct Cell environment_copy[ROWS*COLUMNS];
    for (int i=0; i<ROWS*COLUMNS; i++){
        environment_copy[i] = environment[i];
    }

    for (int i=0; i<ROWS; i++){
        for (int j=0; j<COLUMNS; j++){
            struct Cell current_cell = environment[j + COLUMNS*i];
            struct Cell above_cell = environment[j + COLUMNS*(i-1)];

            if(current_cell.type == WATER_TYPE && current_cell.fill_level > 1 && i > 0 && above_cell.type == WATER_TYPE && current_cell.fill_level > above_cell.fill_level)
            {
                double delta_fill = (current_cell.fill_level - 1 ) - above_cell.fill_level;   
                environment_copy[j + COLUMNS*i].fill_level -= delta_fill;
                environment_copy[j + COLUMNS*(i-1)].fill_level += delta_fill;
            }
            
        }
    }

    for (int i=0; i<ROWS*COLUMNS; i++){
        environment[i] = environment_copy[i];
    }
}

void simulation_step(struct Cell environment[ROWS*COLUMNS]){

    water_flow_down(environment);
    
    water_flow_horizntally(environment);

    water_pressure(environment);
}

int main(){
    SDL_Init(SDL_INIT_VIDEO);       
    SDL_Window* window = SDL_CreateWindow("Liquid Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    SDL_Surface* surface = SDL_GetWindowSurface(window);

    struct Cell environment[ROWS * COLUMNS];
    initialize_environment(environment);

    int simulation_running = 1;
    int current_type = SOLID_TYPE;
    int delete_mode = 0;
    SDL_Event event;
    
    while(simulation_running)
    {
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT){ simulation_running = 0; } // Close Program
            if (event.type == SDL_MOUSEMOTION) // Draw on screen
            {
                if(event.motion.state != 0)
                {
                    int cell_x = event.motion.x / CELL_SIZE;
                    int cell_y = event.motion.y / CELL_SIZE;
                    int fill_level = delete_mode ? 0 : 1;
                    struct Cell cell;
                    if (delete_mode != 0)
                    {
                        current_type = WATER_TYPE;
                        fill_level = 0;
                        cell = (struct Cell){current_type,fill_level,cell_x,cell_y};
                    }
                    else
                    {
                        fill_level = 1;
                        struct Cell cell = (struct Cell){current_type,fill_level,cell_x,cell_y};
                    }
                    environment[cell_x + COLUMNS*cell_y] = cell;
                }
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_SPACE)
                {
                    current_type = !current_type;
                }
                if (event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    delete_mode = !delete_mode;
                }
            }
        }

        // Simulation
        simulation_step(environment);
        draw_environment(surface,environment);
        // draw_grid(surface);
        SDL_UpdateWindowSurface(window);
        SDL_Delay(30);
    }

    
}