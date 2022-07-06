#include <string>
#include <fstream>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "colors.h"
#include "tetris.h"

#ifdef AUDIO
#include "audio.h"
Audio * drop_sound;
Audio * clear_sound;
Audio * hiscore_sound;
Audio * pause_sound;
Audio * gameover_sound;
bool play_hiscore = true;
#endif


void fps_init() 
{
    memset(frametimes, 0, sizeof(frametimes));
    framecount = 0;
    framespersecond = 0;
    frametimelast = SDL_GetTicks();
}

void fps_process() 
{
    uint32_t frametimesindex;
    uint32_t getticks;
    uint32_t count;
    uint32_t i;

    frametimesindex = framecount % FRAME_VALUES;
    getticks = SDL_GetTicks();
    frametimes[frametimesindex] = getticks - frametimelast;
    frametimelast = getticks;
    framecount++;

    if (framecount < FRAME_VALUES)
    {
        count = framecount;
    }
    else
    {
        count = FRAME_VALUES;
    }

    framespersecond = 0;
    for (i = 0; i < count; i++)
    {
        framespersecond += frametimes[i];
    }

    framespersecond /= count;
    framespersecond = 1000.f / framespersecond;
}

int32_t read_hiscore()
{
    std::string::size_type sz;
    std::string hiscore_str;
    std::ifstream infile(HISCORE_FILENAME);
    if (!infile.good()) {
        // Highscore file missing.
        return 0;
    }
    std::getline(infile, hiscore_str);
    int32_t hiscore = std::stoi(hiscore_str, &sz);
    return hiscore;
}

void write_hiscore(int32_t hiscore)
{
    std::ofstream outfile(HISCORE_FILENAME);
    std::string hiscore_str = std::to_string(hiscore);
    outfile << hiscore_str;
}

uint8_t matrix_get(const uint8_t *values, int32_t width, int32_t row,
                   int32_t col)
{
    int32_t index = row * width + col;
    return values[index];
}

void matrix_set(uint8_t *values, int32_t width, int32_t row, int32_t col,
                uint8_t value)
{
    int32_t index = row * width + col;
    values[index] = value;
}

uint8_t tetromino_get(const Tetromino *tetromino, int32_t row, int32_t col,
                    int32_t rotation)
{
    int32_t side = tetromino->side;
    switch (rotation)
    {
    case 0:
        return tetromino->data[row * side + col];
    case 1:
        return tetromino->data[(side - col - 1) * side + row];
    case 2:
        return tetromino->data[(side - row - 1) * side + (side - col - 1)];
    case 3:
        return tetromino->data[col * side + (side - row - 1)];
    }
    return 0;
}

uint8_t check_row_filled(const uint8_t *values, int32_t width, int32_t row)
{
    for (int32_t col = 0; col < width; ++col)
    {
        if (!matrix_get(values, width, row, col))
        {
            return 0;
        }
    }
    return 1;
}

uint8_t check_row_empty(const uint8_t *values, int32_t width, int32_t row)
{
    for (int32_t col = 0; col < width; ++col)
    {
        if (matrix_get(values, width, row, col))
        {
            return 0;
        }
    }
    return 1;
}

int32_t find_lines(const uint8_t *values, int32_t width, int32_t height,
                   uint8_t *lines_out)
{
    int32_t count = 0;
    for (int32_t row = 0; row < height; ++row)
    {
        uint8_t filled = check_row_filled(values, width, row);
        lines_out[row] = filled;
        count += filled;
    }
    return count;
}

void clear_lines(uint8_t *values, int32_t width, int32_t height, 
                 const uint8_t *lines)
{
    int32_t src_row = height - 1;
    for (int32_t dst_row = height - 1; dst_row >= 0; --dst_row)
    {
        while (src_row >= 0 && lines[src_row])
        {
            --src_row;
        }

        if (src_row < 0)
        {
            memset(values + dst_row * width, 0, width);
        }
        else 
        {
            if (src_row != dst_row)
            {
                memcpy(values + dst_row * width,
                       values + src_row * width,
                       width);
            }
            --src_row;
        }
    }
}

bool check_piece_valid(const Piece_State *piece, const uint8_t *board,
                       int32_t width, int32_t height)
{
    const Tetromino *tetromino = TETROMINOS + piece->tetromino_index;

    for (int32_t row = 0; row < tetromino->side; ++row)
    {
        for (int32_t col = 0; col < tetromino->side; ++col)
        {
            uint8_t value = tetromino_get(tetromino, row, col,
                                          piece->rotation);
            if (value > 0)
            {
                int32_t board_row = piece->offset_row + row;
                int32_t board_col = piece->offset_col + col;
                if (board_row < 0)
                {
                    return false;
                }
                if (board_row >= height)
                {
                    return false;
                }
                if (board_col < 0)
                {
                    return false;
                }
                if (board_col >= width)
                {
                    return false;
                }
                if (matrix_get(board, width, board_row, board_col))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

void merge_piece(Game_State *game)
{
    const Tetromino *tetromino = TETROMINOS + game->piece.tetromino_index;
    for (int32_t row = 0; row < tetromino->side; ++row)
    {
        for (int32_t col = 0; col < tetromino->side; ++col)
        {
            uint8_t value = tetromino_get(tetromino, row, col,
                                          game->piece.rotation);
            if (value)
            {
                int32_t board_row = game->piece.offset_row + row;
                int32_t board_col = game->piece.offset_col + col;
                matrix_set(game->board, WIDTH, board_row, board_col, value);
            }
        }
    }
}

int32_t random_int(int32_t min, int32_t max)
{
    int32_t range = max - min;
    return min + rand() % range;
}

float get_time_to_next_drop(int32_t level)
{
    if (level > 29)
    {
        level = 29;
    }
    return FRAMES_PER_DROP[level] * TARGET_SECONDS_PER_FRAME;
}

void random_next_piece(Game_State *game)
{
    game->tetromino_next = (uint8_t)random_int(0, ARRAY_COUNT(TETROMINOS));
}

void spawn_piece(Game_State *game)
{
    game->piece = {};
    game->piece.tetromino_index = game->tetromino_next;
    game->piece.offset_col = WIDTH / 2;
    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
}


bool soft_drop(Game_State *game)
{
    ++game->piece.offset_row;
    if (!check_piece_valid(&game->piece, game->board, WIDTH, HEIGHT))
    {
        --game->piece.offset_row;
        merge_piece(game);
        spawn_piece(game);
        random_next_piece(game);

#ifdef AUDIO
        playSoundFromMemory(drop_sound, SDL_MIX_MAXVOLUME / 2);
#endif

        return false;
    }
    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
    return true;
}

int32_t compute_score(int32_t level, int32_t line_count)
{
    switch (line_count)
    {
    case 1:
        return 40 * (level + 1);
    case 2:
        return 100 * (level + 1);
    case 3:
        return 300 * (level + 1);
    case 4:
        return 1200 * (level + 1);
    }
    return 0;
}

int32_t min(int32_t x, int32_t y)
{
    return x < y ? x : y;
}
int32_t max(int32_t x, int32_t y)
{
    return x > y ? x : y;
}

int32_t get_lines_for_next_level(int32_t start_level, int32_t level)
{
    int32_t first_level_up_limit = min(
        (start_level * 10 + 10),
        max(100, (start_level * 10 - 50)));
    if (level == start_level)
    {
        return first_level_up_limit;
    }
    int32_t diff = level - start_level;
    return first_level_up_limit + diff * 10;
}

void update_game_start(Game_State *game, const Input_State *input)
{
    if (input->dup > 0)
    {
        ++game->start_level;
    }

    if (input->ddown > 0 && game->start_level > 0)
    {
        --game->start_level;
    }
    
    if (input->dspace > 0)
    {
        memset(game->board, 0, WIDTH * HEIGHT);
        game->level = game->start_level;
        game->line_count = 0;
        game->score = 0;
        spawn_piece(game);
        random_next_piece(game);
        game->phase = GAME_PHASE_PLAY;
    }
}

void update_game_pause(Game_State *game, const Input_State *input)
{
    if (input->dp > 0)
    {
        game->phase = GAME_PHASE_PLAY;
#ifdef AUDIO
        playSoundFromMemory(pause_sound, SDL_MIX_MAXVOLUME / 2);
#endif
    }
}

void update_game_gameover(Game_State *game, const Input_State *input)
{
    if (input->dspace > 0)
    {
        game->phase = GAME_PHASE_START;
        memset(game->board, 0, WIDTH * HEIGHT);
    }
}

void update_game_line(Game_State *game)
{
    if (game->time >= game->highlight_end_time)
    {
        clear_lines(game->board, WIDTH, HEIGHT, game->lines);
        game->line_count += game->pending_line_count;
        game->score += compute_score(game->level, game->pending_line_count);

        int32_t lines_for_next_level = get_lines_for_next_level(
                                                            game->start_level,
                                                            game->level);
        if (game->line_count >= lines_for_next_level)
        {
            ++game->level;
        }
        
        game->phase = GAME_PHASE_PLAY;
    }
}

void update_game_play(Game_State *game, const Input_State *input)
{
    Piece_State piece = game->piece;
    if (input->da > 0)
    {
        --piece.offset_col;
    }
    if (input->dd > 0)
    {
        ++piece.offset_col;
    }
    if (input->dright > 0)
    {
        piece.rotation = (piece.rotation + 1) % 4;
    }

    if (input->dleft > 0)
    {
        piece.rotation = (piece.rotation + 3) % 4;
    }

    if (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
    {
        game->piece = piece;
    }

    if (input->ds > 0 || input->ddown > 0)
    {
        soft_drop(game);
    }

    if (input->dspace > 0)
    {
        while(soft_drop(game));
    }
    
    while (game->time >= game->next_drop_time)
    {
        soft_drop(game);
    }

    game->pending_line_count = find_lines(game->board, WIDTH, HEIGHT,
                                          game->lines);
    if (game->pending_line_count > 0)
    {
#ifdef AUDIO
        playSoundFromMemory(clear_sound, SDL_MIX_MAXVOLUME / 2);
#endif
        game->phase = GAME_PHASE_LINE;
        game->highlight_end_time = game->time + 0.5f;
    }

    int32_t game_over_row = 2;
    if (!check_row_empty(game->board, WIDTH, game_over_row))
    {
        game->phase = GAME_PHASE_GAMEOVER;
#ifdef AUDIO
        playSoundFromMemory(gameover_sound, SDL_MIX_MAXVOLUME / 2);
#endif
    }

    if (input->dp > 0)
    {
        game->phase = GAME_PHASE_PAUSE;
#ifdef AUDIO
        playSoundFromMemory(pause_sound, SDL_MIX_MAXVOLUME / 2);
#endif
    }

}

void update_game(Game_State *game, const Input_State *input)
{
    switch(game->phase)
    {
    case GAME_PHASE_START:
        update_game_start(game, input);
        break;
    case GAME_PHASE_PLAY:
        update_game_play(game, input);
        break;
    case GAME_PHASE_LINE:
        update_game_line(game);
        break;
    case GAME_PHASE_PAUSE:
        update_game_pause(game, input);
        break;
    case GAME_PHASE_GAMEOVER:
        update_game_gameover(game, input);
        break;
    }
}

void fill_rect(SDL_Renderer *renderer, int32_t x, int32_t y, int32_t width,
               int32_t height, Color color)
{
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}


void draw_rect(SDL_Renderer *renderer, int32_t x, int32_t y, int32_t width,
               int32_t height, Color color)
{
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect);
}

void draw_string(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                 int32_t x, int32_t y, Text_Align alignment, Color color)
{
    SDL_Color sdl_color = SDL_Color { color.r, color.g, color.b, color.a };
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, sdl_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;
    switch (alignment)
    {
    case TEXT_ALIGN_LEFT:
        rect.x = x;
        break;
    case TEXT_ALIGN_CENTER:
        rect.x = x - surface->w / 2;
        break;
    case TEXT_ALIGN_RIGHT:
        rect.x = x - surface->w;
        break;
    }

    SDL_RenderCopy(renderer, texture, 0, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_cell(SDL_Renderer *renderer, int32_t row, int32_t col, uint8_t value,
               int32_t offset_x, int32_t offset_y, bool outline = false)
{
    Color base_color = BASE_COLORS[value];
    Color light_color = LIGHT_COLORS[value];
    Color dark_color = DARK_COLORS[value];
   
    int32_t edge = GRID_SIZE / 8;

    int32_t x = col * GRID_SIZE + offset_x;
    int32_t y = row * GRID_SIZE + offset_y;

    if (outline)
    {
        draw_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, base_color);
        return;
    }
    
    fill_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, dark_color);
    fill_rect(renderer, x + edge, y, GRID_SIZE - edge, GRID_SIZE - edge,
              light_color);
    fill_rect(renderer, x + edge, y + edge,
              GRID_SIZE - edge * 2, GRID_SIZE - edge * 2, base_color); 
}

void draw_preview_cell(SDL_Renderer *renderer, int32_t row, int32_t col,
                       uint8_t value, int32_t offset_x, int32_t offset_y,
                       bool outline = false)
{
    Color base_color = BASE_COLORS[value];
    Color light_color = LIGHT_COLORS[value];
    Color dark_color = DARK_COLORS[value];
   
    int32_t edge = GRID_SIZE / 16;

    int32_t x = col * (GRID_SIZE / 2) + offset_x;
    int32_t y = row * (GRID_SIZE / 2) + offset_y;

    if (outline)
    {
        draw_rect(renderer, x, y, GRID_SIZE / 2, GRID_SIZE / 2, base_color);
        return;
    }
    
    fill_rect(renderer, x, y, GRID_SIZE / 2, GRID_SIZE / 2, dark_color);
    fill_rect(renderer, x + edge, y, (GRID_SIZE / 2) - edge,
              (GRID_SIZE / 2) - edge, light_color);
    fill_rect(renderer, x + edge, y + edge,
              (GRID_SIZE / 2) - edge * 2, (GRID_SIZE / 2) - edge * 2,
              base_color); 
}

void draw_piece(SDL_Renderer *renderer, const Piece_State *piece,
                int32_t offset_x, int32_t offset_y, bool outline = false)
{
    const Tetromino *tetromino = TETROMINOS + piece->tetromino_index;
    for (int32_t row = 0; row < tetromino->side; ++row)
    {
        for (int32_t col = 0; col < tetromino->side; ++col)
        {
            uint8_t value = tetromino_get(tetromino, row, col, piece->rotation);
            if (value)
            {
                draw_cell(renderer,
                          row + piece->offset_row,
                          col + piece->offset_col,
                          value,
                          offset_x, offset_y,
                          outline);
            }
        }
    }
}

void draw_preview(SDL_Renderer *renderer, const Game_State *game,
                int32_t offset_x, int32_t offset_y, bool outline = false)
{
    const Tetromino *tetromino = TETROMINOS + game->tetromino_next;
    for (int32_t row = 0; row < tetromino->side; ++row)
    {
        for (int32_t col = 0; col < tetromino->side; ++col)
        {
            uint8_t value = tetromino_get(tetromino, row, col, 0);
            if (value)
            {
                draw_preview_cell(renderer,
                                  row,
                                  col,
                                  value,
                                  offset_x, offset_y,
                                  outline);
            }
        }
    }
}

void draw_board(SDL_Renderer *renderer, const uint8_t *board, int32_t width,
                int32_t height, int32_t offset_x, int32_t offset_y)
{
    fill_rect(renderer, offset_x, offset_y,
              width * GRID_SIZE, height * GRID_SIZE,
              BASE_COLORS[0]);
    for (int32_t row = 0; row < height; ++row)
    {
        for (int32_t col = 0; col < width; ++col)
        {
            uint8_t value = matrix_get(board, width, row, col);
            if (value)
            {
                draw_cell(renderer, row, col, value, offset_x, offset_y);
            }
        }
    }
}

void render_game(const Game_State *game, SDL_Renderer *renderer,
                 TTF_Font *font, TTF_Font *small_font, TTF_Font *tiny_font)
{
    char buffer[256];
    
    Color highlight_color = color(0xFF, 0xFF, 0xFF, 0xFF);
    Color gray_color = color(0x77, 0x77, 0x77, 0x77);
    Color flash_color; 

    int32_t margin_y = 60;
    
    draw_board(renderer, game->board, WIDTH, HEIGHT, 0, margin_y);

    if (game->phase == GAME_PHASE_PLAY)
    {
        draw_piece(renderer, &game->piece, 0, margin_y);

        Piece_State piece = game->piece;
        while (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
        {
            piece.offset_row++;
        }
        --piece.offset_row;
        draw_piece(renderer, &piece, 0, margin_y, true);
        
    }

    if (game->phase == GAME_PHASE_LINE)
    {
        for (int32_t row = 0; row < HEIGHT; ++row)
        {
            if (game->lines[row])
            {
                int32_t x = 0;
                int32_t y = row * GRID_SIZE + margin_y;

                // Flash effect when clearing line.
                if ((framecount % 2) == 0)
                {
                    flash_color = color(0xFF, 0xFF, 0xFF, 0xFF);
                }
                else
                {
                    flash_color = color(0x0, 0x0, 0x0, 0x0);
                }
                
                fill_rect(renderer, x, y, WIDTH * GRID_SIZE, GRID_SIZE,
                          flash_color);
            }
        }
    }
    else if (game->phase == GAME_PHASE_PAUSE)
    {
        int32_t x = WIDTH * GRID_SIZE / 2;
        int32_t y = (HEIGHT * GRID_SIZE + margin_y) / 2;
        draw_string(renderer, font, "-PAUSED-", x, y, TEXT_ALIGN_CENTER,
                    highlight_color);
    }
    else if (game->phase == GAME_PHASE_GAMEOVER)
    {
        int32_t x = WIDTH * GRID_SIZE / 2;
        int32_t y = (HEIGHT * GRID_SIZE + margin_y) / 2;
        draw_string(renderer, font, "GAME OVER", x, y, TEXT_ALIGN_CENTER,
                    highlight_color);
    }
    else if (game->phase == GAME_PHASE_START)
    {
        int32_t x = WIDTH * GRID_SIZE / 2;
        int32_t y = ((HEIGHT * GRID_SIZE + margin_y) / 2) - 140;

#ifdef AUDIO
        play_hiscore = true;
#endif

        draw_string(renderer, font, "PRESS SPACE TO START",
                    x, y, TEXT_ALIGN_CENTER, highlight_color);

        snprintf(buffer, sizeof(buffer), "STARTING LEVEL: %02d",
                 game->start_level);
        draw_string(renderer, font, buffer, x, y + 30,
                    TEXT_ALIGN_CENTER, highlight_color);

        draw_string(renderer, tiny_font, "CONTROLS",
                    x, y + 80, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, tiny_font, "--------",
                    x, y + 100, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, tiny_font, "MOVE LEFT: A  MOVE RIGHT: D",
                    x, y + 120, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, tiny_font, "MOVE DOWN: S",
                    x, y + 140, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, tiny_font, "ROTATE: LEFT & RIGHT ARROW",
                    x, y + 160, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, tiny_font, "LEVEL SELECT: UP & DOWN ARROW",
                    x, y + 180, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, tiny_font, "FAST DROP: SPACE",
                    x, y + 200, TEXT_ALIGN_CENTER, highlight_color);
        draw_string(renderer, tiny_font, "PAUSE: P",
                    x, y + 220, TEXT_ALIGN_CENTER, highlight_color);

        draw_string(renderer, tiny_font, "CREDITS",
                    x, y + 280, TEXT_ALIGN_CENTER, gray_color);
        draw_string(renderer, tiny_font, "-------",
                    x, y + 300, TEXT_ALIGN_CENTER, gray_color);
        draw_string(renderer, tiny_font, "Game by Robert in 2020",
                    x, y + 320, TEXT_ALIGN_CENTER, gray_color);
        draw_string(renderer, tiny_font, "Based on core design by odyssjii",
                    x, y + 340, TEXT_ALIGN_CENTER, gray_color);
    }
    
    fill_rect(renderer, 0, margin_y, WIDTH * GRID_SIZE,
              (HEIGHT - VISIBLE_HEIGHT) * GRID_SIZE,
              color(0x00, 0x00, 0x00, 0x00));
    

    snprintf(buffer, sizeof(buffer), "LEVEL: %d", game->level);
    draw_string(renderer, font, buffer, 5, 3, TEXT_ALIGN_LEFT,
                highlight_color);

    snprintf(buffer, sizeof(buffer), "LINES: %d", game->line_count);
    draw_string(renderer, font, buffer, 5, 33, TEXT_ALIGN_LEFT,
                highlight_color);
    
    snprintf(buffer, sizeof(buffer), "SCORE: %d", game->score);
    draw_string(renderer, font, buffer, 5, 63, TEXT_ALIGN_LEFT,
                highlight_color);

    snprintf(buffer, sizeof(buffer), "HI: %d", game->hiscore);
    draw_string(renderer, font, buffer, 5, 93, TEXT_ALIGN_LEFT,
                highlight_color);

    draw_string(renderer, font, "NEXT:", 175, 12, TEXT_ALIGN_LEFT,
                highlight_color);

    if (game->phase != GAME_PHASE_START)
    {
        // Next block.
        draw_preview(renderer, game, 234, 5);
        snprintf(buffer, sizeof(buffer), "FPS: %.4f", framespersecond);
        draw_string(renderer, tiny_font, buffer, 175, 70, TEXT_ALIGN_LEFT,
                    gray_color);

        snprintf(buffer, sizeof(buffer), "DTIME: %.4fs",
                                get_time_to_next_drop(game->level));
        draw_string(renderer, tiny_font, buffer, 175, 90, TEXT_ALIGN_LEFT,
                    gray_color);
    }
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return 1;
    }

#ifdef AUDIO
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        return 1; 
    }
    initAudio();
    drop_sound = createAudio("sounds/drop.wav", 0, SDL_MIX_MAXVOLUME / 2);
    clear_sound = createAudio("sounds/clear.wav", 0, SDL_MIX_MAXVOLUME / 2);
    hiscore_sound = createAudio("sounds/hiscore.wav", 0,
                                SDL_MIX_MAXVOLUME / 2);
    pause_sound = createAudio("sounds/pause.wav", 0, SDL_MIX_MAXVOLUME / 2);
    gameover_sound = createAudio("sounds/gameover.wav", 0,
                                 SDL_MIX_MAXVOLUME / 2);
#endif

    fps_init();

    if (TTF_Init() < 0)
    {
        return 2;
    }
    
    SDL_Window *window = SDL_CreateWindow(
        "Tetris v1.55",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        300,
        720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Amiga classic.
    const char *font_name = "fonts/P0T-NOoDLE_v1.0.ttf";
    TTF_Font *font = TTF_OpenFont(font_name, 24);
    TTF_Font *small_font = TTF_OpenFont(font_name, 20);
    TTF_Font *tiny_font = TTF_OpenFont(font_name, 16);

    Game_State game = {};
    Input_State input = {};

    random_next_piece(&game);

    game.piece.tetromino_index = 2;
    game.hiscore = read_hiscore();

    input.key_frame_count = 0;
    input.key_skip_count = 0;

    bool quit = false;
    while (!quit)
    {
        fps_process();
        game.time = SDL_GetTicks() / 1000.0f;
    
        if (game.score > game.hiscore && game.phase == GAME_PHASE_PLAY)
        {
            game.hiscore = game.score;
#ifdef AUDIO
            if (play_hiscore)
            {
                playSoundFromMemory(hiscore_sound, SDL_MIX_MAXVOLUME / 2);
                play_hiscore = false;
            }
#endif
        }

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        int32_t key_count;
        const uint8_t *key_states = SDL_GetKeyboardState(&key_count);

        if (key_states[SDL_SCANCODE_ESCAPE])
        {
            quit = true;
        }
        
        Input_State prev_input = input;

        if (e.type == SDL_KEYUP)
        {
            input.key_frame_count = 0;
            input.key_skip_count = 0;
        }
        else
        {
            input.key_frame_count++;
        }

        input.left = key_states[SDL_SCANCODE_LEFT];
        input.right = key_states[SDL_SCANCODE_RIGHT];
        input.a = key_states[SDL_SCANCODE_A];
        input.s = key_states[SDL_SCANCODE_S];
        input.d = key_states[SDL_SCANCODE_D];
        input.p = key_states[SDL_SCANCODE_P];
        input.up = key_states[SDL_SCANCODE_UP];
        input.down = key_states[SDL_SCANCODE_DOWN];
        input.space = key_states[SDL_SCANCODE_SPACE];

        input.dleft = input.left - prev_input.left;
        input.dright = input.right - prev_input.right;

        // NES key repeat rules (16 frames threshold, 6 frames per repeat).
        if (input.key_frame_count >= 10)
        {
            if (input.key_skip_count >= 5)
            {
                input.key_skip_count = 0;
                input.da = input.a;
                input.ds = input.s;
                input.dd = input.d;
                input.dup = input.up;
                input.ddown = input.down;
                
            }
            else
            {
                input.key_skip_count++;
                input.da = 0;
                input.ds = 0;
                input.dd = 0;
                input.dup = 0;
                input.ddown = 0;
            }
        }
        else
        {
            input.da = input.a - prev_input.a;
            input.ds = input.s - prev_input.s;
            input.dd = input.d - prev_input.d;
            input.dup = input.up - prev_input.up;
            input.ddown = input.down - prev_input.down;
        }

        input.dp = input.p - prev_input.p;
        input.dspace = input.space - prev_input.space;
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        update_game(&game, &input);
        render_game(&game, renderer, font, small_font, tiny_font);

        SDL_RenderPresent(renderer);
    }

    write_hiscore(game.hiscore);

#ifdef AUDIO
    freeAudio(drop_sound);
    freeAudio(clear_sound);
    freeAudio(hiscore_sound);
    freeAudio(pause_sound);
    freeAudio(gameover_sound);
#endif

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    return 0;
}
