// @todo refactor

// @todo frame rate and sleep, dt
// @todo handle controller input


#include <windows.h>
#include <stdio.h>

#include "iml_general.h"
#include "iml_types.h"

#include "random.h"

#define BLOCK_SIZE 32
#define GRID_WIDTH 16
#define GRID_HEIGHT 32
#define WIDTH  (GRID_WIDTH * BLOCK_SIZE)
#define HEIGHT (GRID_HEIGHT * BLOCK_SIZE)


struct Win32_Offscreen_Buffer {
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct Win32_Window_Dimension {
    int width;
    int height;
};

struct Vector2 {
    int x;
    int y;
};

struct Vector3 {
    union {
        struct {
            u32 x;
            u32 y;
            u32 z;
        };
        struct {
            u32 r;
            u32 g;
            u32 b;
        };
    };
};

internal Vector3
rgb(u32 r, u32 g, u32 b) {
    Vector3 rgb { r, g, b };
    return rgb;
}

enum Block_Type {
    EMPTY = 0,
    
    // @note descriptions from wikipedia:https://tetris.wiki/Tetromino
    I, // Light blue; shaped like a capital I; four Minos in a straight line. Other names include straight, stick, and long. This is the only tetromino that can clear four lines outside of cascade games.
    O, // Yellow; a square shape; four Minos in a 2×2 square. Other names include square and block.
    T, // Purple; shaped like a capital T; a row of three Minos with one added above the center.
    S, // Green; shaped like a capital S; two stacked horizontal diminos with the top one offset to the right. Other names include inverse skew and right snake.
    Z, // Red; shaped like a capital Z; two stacked horizontal diminos with the top one offset to the left. Other names include skew and left snake.
    J, // Blue; shaped like a capital J; a row of three Minos with one added above the left side. Other names include gamma, inverse L, or left gun.
    L, // Orange; shaped like a capital L; a row of three Minos with one added above the right side. Other names include right gun.
    
    ENUM_SIZE,
};

struct Block {
    Vector2 pos[4];
    enum32(Block_Type) type;
};

struct Game_State {
    Block current_block;
    int grid[GRID_HEIGHT][GRID_WIDTH]; // @todo enum for the color of the block
};


global Win32_Offscreen_Buffer global_backbuffer;
global b32 global_running;


internal void
win32_resize_dib_section(Win32_Offscreen_Buffer *buffer, int width, int height) {
    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    
    buffer->width  = width;
    buffer->height = height;
    
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    
    int bytes_per_pixel = 4;
    int bitmap_memory_size = (buffer->width * buffer->height) * bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * bytes_per_pixel;
    buffer->bytes_per_pixel = bytes_per_pixel;
}

internal Win32_Window_Dimension
win32_get_window_dimension(HWND window) {
    Win32_Window_Dimension result;
    
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;
    
    return result;
}

internal void
win32_display_buffer_in_window(Win32_Offscreen_Buffer *buffer, HDC device_context, int window_width, int window_height) {
    StretchDIBits(device_context,
                  0, 0, window_width, window_height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
win32_main_window_callback(HWND window,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam) {
    LRESULT result = 0;
    
    switch (message) {
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_DESTROY:
        case WM_CLOSE: {
            global_running = false;
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            assert(!"Keyboard input came in through a non-dispatch message!");
        } break;
        
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width  = paint.rcPaint.right  - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            Win32_Window_Dimension dimension = win32_get_window_dimension(window);
            win32_display_buffer_in_window(&global_backbuffer, device_context, dimension.width, dimension.height);
            EndPaint(window, &paint);
        } break;
        
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    
    return result;
}

internal void
win32_clear_buffer(Win32_Offscreen_Buffer *buffer, Game_State *game_state) {
    u8 *row = (u8 *)buffer->memory;
    for (int y = 0; y < buffer->height; ++y) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; ++x) {
            *pixel++ = 0;
        }
        
        row += buffer->pitch;
    }
}

internal void
win32_render_block(Win32_Offscreen_Buffer *buffer, Vector2 block_pos, enum32(Block_Type) type) {
    Vector3 color_rgb = {};
    if (type == Block_Type::I)      { color_rgb = rgb(  0, 191, 255); }
    else if (type == Block_Type::O) { color_rgb = rgb(255, 255, 0  ); }
    else if (type == Block_Type::T) { color_rgb = rgb(128,   0, 128); }
    else if (type == Block_Type::S) { color_rgb = rgb(  0, 255, 0  ); }
    else if (type == Block_Type::Z) { color_rgb = rgb(255,   0, 0  ); }
    else if (type == Block_Type::J) { color_rgb = rgb(  0,   0, 255); }
    else if (type == Block_Type::L) { color_rgb = rgb(255, 165, 0  ); }
    else {
        color_rgb = rgb(255, 255, 255);
    }
    
    //
    s32 min_x = block_pos.x * BLOCK_SIZE;
    s32 min_y = block_pos.y * BLOCK_SIZE;
    s32 max_x = min_x + BLOCK_SIZE;
    s32 max_y = min_y + BLOCK_SIZE;
    
    if (min_x < 0)  min_x = 0;
    if (min_y < 0)  min_y = 0;
    if (max_x > buffer->width)   max_x = buffer->width;
    if (max_y > buffer->height)  max_y = buffer->height;
    
    u32 color = ((color_rgb.r << 16) |
                 (color_rgb.g << 8)  |
                 (color_rgb.b << 0));
    
    u8 *row = ((u8 *)buffer->memory +
               min_x * buffer->bytes_per_pixel +
               min_y * buffer->pitch);
    for (int y = min_y; y < max_y; ++y) {
        u32 *pixel = (u32 *)row;
        for (int x = min_x; x < max_x; ++x) {
            *pixel = color;
            ++pixel;
        }
        row += buffer->pitch;
    }
}

internal void
win32_process_pending_messages(Game_State *game_state) {
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        switch (message.message) {
            case WM_QUIT: {
                global_running = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                u32 vk_code = (u32)message.wParam;
                
                // @note Since we are comparing was_down to is_down,
                // we MUST use == and != to convert these bit tests to actual 0 or 1 values.
                b32 was_down = ((message.lParam & (1 << 30)) != 0);
                b32 is_down = ((message.lParam & (1 << 31)) == 0);
                if (was_down != is_down && is_down) {
                    if ((vk_code == 'W') || (vk_code == VK_UP)) {
                        // --game_state->current_block.pos.y;
                    }
                    else if ((vk_code == 'A') || (vk_code == VK_LEFT)) {
                        b32 can_move = true;
                        for (int i = 0; i < 4; ++i) {
                            if (game_state->current_block.pos[i].x <= 0) {
                                can_move = false;
                            }
                        }
                        if (can_move)  {
                            for (int i = 0; i < 4; ++i) {
                                --game_state->current_block.pos[i].x;
                            }
                        }
                    }
                    else if ((vk_code == 'S') || (vk_code == VK_DOWN)) {
                        // ++game_state->current_block.pos.y;
                    }
                    else if ((vk_code == 'D') || (vk_code == VK_RIGHT)) {
                        b32 can_move = true;
                        for (int i = 0; i < 4; ++i) {
                            if (game_state->current_block.pos[i].x >= GRID_WIDTH-1) {
                                can_move = false;
                            }
                        }
                        if (can_move)  {
                            for (int i = 0; i < 4; ++i) {
                                ++game_state->current_block.pos[i].x;
                            }
                        }
                    }
                    else if (vk_code == VK_ESCAPE) {
                        global_running = false;
                    }
                }
                
                if (is_down) {
                    b32 alt_key_was_down = (message.lParam & (1 << 29));
                    if ((vk_code == VK_F4) && alt_key_was_down) {
                        global_running = false;
                    }
                }
            } break;
            
            default: {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            } break;
        }
    }
}

internal void
reset_game(Game_State *game_state, b32 clear_grid) {
    // clear grid
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            game_state->grid[y][x] = Block_Type::EMPTY;
        }
    }
    
    
    int half_screen = ((int)GRID_WIDTH/2);
    game_state->current_block.pos[0] = {half_screen,0};
    game_state->current_block.pos[1] = {half_screen,1};
    game_state->current_block.pos[2] = {half_screen-1,0};
    game_state->current_block.pos[3] = {half_screen-1,1};
    game_state->current_block.type = Block_Type::O;
}

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prev_instance,
        LPSTR     cmd_line,
        int       show_cmd) {
    win32_resize_dib_section(&global_backbuffer, WIDTH, HEIGHT);
    
    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "TetrisWindowClass";
    
    if (!RegisterClassA(&window_class))  {
        // @todo
    }
    HWND window = CreateWindowExA(0, window_class.lpszClassName,
                                  "Tetris",
                                  WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  WIDTH, HEIGHT,
                                  0, 0, instance, 0);
    if (!window)  {
        // @todo
    }
    
    HDC device_context = GetDC(window);
    
    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    s64 perf_count_frequency = perf_count_frequency_result.QuadPart;
    
    Game_State game_state = {};
    reset_game(&game_state, false);
    
    global_running = true;
    
    LARGE_INTEGER last_counter;
    QueryPerformanceCounter(&last_counter);
    u64 last_cycle_count = __rdtsc();
    while (global_running) {
        //
        // simulate
        //
        auto add_block_to_grid = [](Game_State *game_state, Block *block) {
            for (int i = 0; i < 4; ++i) {
                game_state->grid[block->pos[i].y][block->pos[i].x] = block->type;
            }
        };
        auto make_new_current_block = [](Game_State *game_state, Vector2 *old_block_pos = nullptr) {
            // @todo generate different blocks
            enum32(Block_Type) type = (get_next_random_number() % Block_Type::ENUM_SIZE) + 1;
            game_state->current_block.type = type;
            
            int half_screen = ((int)GRID_WIDTH/2);
            Vector2 p0, p1, p2, p3;
            
            if (type == Block_Type::I)      { p0={-1,0}; p1={0,0} ;p2={1,0} ;p3={2,0}; }
            else if (type == Block_Type::O) { p0={0,0}; p1={1,0} ;p2={0,1} ;p3={1,1}; }
            else if (type == Block_Type::T) { p0={0,0}; p1={-1,1} ;p2={0,1} ;p3={1,1}; }
            else if (type == Block_Type::S) { p0={-1,1}; p1={0,1} ;p2={0,0} ;p3={1,0}; }
            else if (type == Block_Type::Z) { p0={-1,0}; p1={0,0} ;p2={0,1} ;p3={1,1}; }
            else if (type == Block_Type::J) { p0={-1,0}; p1={-1,1} ;p2={0,1} ;p3={1,1}; }
            else if (type == Block_Type::L) { p0={-1,1}; p1={0,1} ;p2={1,1} ;p3={1,0}; }
            else {
                // @todo assert
            }
            game_state->current_block.pos[0] = {p0.x+half_screen, p0.y+half_screen};
            game_state->current_block.pos[1] = {p1.x+half_screen, p1.y+half_screen};
            game_state->current_block.pos[2] = {p2.x+half_screen, p2.y+half_screen};
            game_state->current_block.pos[3] = {p3.x+half_screen, p3.y+half_screen};
            
            // @copynpaste
            b32 hit = false;
            for (int i = 0; i < 4; ++i) {
                if (game_state->grid[game_state->current_block.pos[i].y][game_state->current_block.pos[i].x] != Block_Type::EMPTY)  {
                    // hit a block
                    hit = true;
                }
            }
            if (hit)  {
                reset_game(game_state, true);
            }
        };
        
        
        Block old_block = {};
        old_block = game_state.current_block;
        for (int i = 0; i < 4; ++i) {
            old_block.pos[i] = game_state.current_block.pos[i];
        }
        
        // @note handle input
        win32_process_pending_messages(&game_state);
        
        
        // @note check if current_block hit other blocks because of the player input
        b32 hit = false;
        for (int i = 0; i < 4; ++i) {
            if (game_state.grid[game_state.current_block.pos[i].y][game_state.current_block.pos[i].x] != Block_Type::EMPTY)  {
                // hit a block
                hit = true;
            }
        }
        if (hit)  {
            for (int i = 0; i < 4; ++i) {
                game_state.current_block.pos[i] = old_block.pos[i];
            }
        }
        
        // @note check if current_block hit the bottom
        hit = false;
        for (int i = 0; i < 4; ++i) {
            if (game_state.current_block.pos[i].y >= GRID_HEIGHT-1) {
                hit = true;
            }
        }
        if (hit)  {
            add_block_to_grid(&game_state, &old_block);
            make_new_current_block(&game_state, old_block.pos);
        }
        else {
            // @note move the current_block downward
            for (int i = 0; i < 4; ++i) {
                ++game_state.current_block.pos[i].y;
            }
        }
        
        // @note check if current block hit other blocks after moving downward
        // @copynpaste
        hit = false;
        for (int i = 0; i < 4; ++i) {
            if (game_state.grid[game_state.current_block.pos[i].y][game_state.current_block.pos[i].x] != Block_Type::EMPTY)  {
                // hit a block
                hit = true;
            }
        }
        if (hit)  {
            for (int i = 0; i < 4; ++i) {
                game_state.current_block.pos[i] = old_block.pos[i];
            }
            add_block_to_grid(&game_state, &old_block);
            make_new_current_block(&game_state, old_block.pos);
        }
        
        //
        // @note check for tetris
        //
        // @todo for_each line check if full, and move everything down
        // @todo @note If top most line is full, -> game over
        int line_streak_count = 0;
        for (int current_line = GRID_HEIGHT-1; current_line >= 0; --current_line) {
            b32 line_full = true;
            for (int x = 0; x < GRID_WIDTH; ++x) {
                if (game_state.grid[current_line][x] == Block_Type::EMPTY) {
                    line_full = false;
                    line_streak_count = 0;
                }
                else {
                    ++line_streak_count;
                }
            }
            if (line_full)  {
                if (line_streak_count == 4)  {
                    // @todo BOOM TETRIS
                }
                
                // @note move all blocks done 1, skip last line
                for (int y = current_line-1; y >= 0; --y) {
                    for (int x = 0; x < GRID_WIDTH; ++x) {
                        game_state.grid[y+1][x] = game_state.grid[y][x];
                    }
                }
                // @note clear top most line, because it got moved down by one
                for (int x = 0; x < GRID_WIDTH; ++x) {
                    game_state.grid[0][x] = Block_Type::EMPTY;
                }
            }
        }
        
        //
        // render
        //
        win32_clear_buffer(&global_backbuffer, &game_state);
        
        // @note render grid
        for (int y = 0; y < GRID_HEIGHT; ++y) {
            for (int x = 0; x < GRID_WIDTH; ++x) {
                int type = game_state.grid[y][x];
                if (type > 0)  {
                    win32_render_block(&global_backbuffer, Vector2{x,y}, type);
                }
            }
        }
        // @note render current_block
        for (int i = 0; i < 4; ++i) {
            win32_render_block(&global_backbuffer, game_state.current_block.pos[i], game_state.current_block.type);
        }
        
        Win32_Window_Dimension dimension = win32_get_window_dimension(window);
        win32_display_buffer_in_window(&global_backbuffer, device_context, dimension.width, dimension.height);
        
        //
        // sleep
        //
        DWORD sleep_ms = 80;
        Sleep(sleep_ms);
        
        
        u64 end_cycle_count = __rdtsc();
        LARGE_INTEGER end_counter;
        QueryPerformanceCounter(&end_counter);
        
        u64 cycles_elapsed = end_cycle_count - last_cycle_count;
        s64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
        f64 ms_per_frame = (f64)((1000.0f * (f64)counter_elapsed) / (f64)perf_count_frequency);
        f64 fps = (f64)perf_count_frequency / (f64)counter_elapsed;
        f64 mcpf = (f64)cycles_elapsed / (1000.0f * 1000.0f);
        
        char string_buffer[256];
        sprintf(string_buffer, "%.02fms/f, %.02fFPS, %.02fmc/f\n", ms_per_frame, fps, mcpf);
        OutputDebugStringA(string_buffer);
        
        last_counter = end_counter;
        last_cycle_count = end_cycle_count;
    }
}