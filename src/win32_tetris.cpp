#include <windows.h>
#include <stdio.h>

#include "iml_general.h"
#include "iml_types.h"


#define BLOCK_SIZE 32
#define WIDTH  (16 * BLOCK_SIZE)
#define HEIGHT (32 * BLOCK_SIZE)


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

struct Game_State {
    
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
            u32 vk_code  = wparam;
            b32 was_down = ((lparam & (1 << 30)) != 0);
            b32 is_down  = ((lparam & (1 << 31)) == 0);
            
            if (is_down) {
                if (vk_code == 'W') {
                }
                else if (vk_code == 'A') {
                }
                else if (vk_code == 'S') {
                }
                else if (vk_code == 'D') {
                }
                else if (vk_code == VK_ESCAPE) {
                    global_running = false;
                }
            }
            
            b32 alt_key_was_down = (lparam & (1 << 29));
            if ((vk_code == VK_F4) && alt_key_was_down) {
                global_running = false;
            }
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
win32_render_game(Win32_Offscreen_Buffer *buffer, Game_State *game_state) {
    u8 *row = (u8 *)buffer->memory;
    for (int y = 0; y < buffer->height; ++y) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; ++x) {
            u8 red = 0;
            u8 blue = 0;
            u8 green = 0;
            
            *pixel++ = ((red << 16) | ((green << 8) | blue));
        }
        
        row += buffer->pitch;
    }
}

internal void
win32_render_block(Win32_Offscreen_Buffer *buffer, Vector2 block_pos) {
    u32 r = 255;
    u32 g = 0;
    u32 b = 0;
    //
    s32 min_x = block_pos.x * BLOCK_SIZE;
    s32 min_y = block_pos.y * BLOCK_SIZE;
    s32 max_x = min_x + BLOCK_SIZE;
    s32 max_y = min_y + BLOCK_SIZE;
    
    if (min_x < 0)  min_x = 0;
    if (min_y < 0)  min_y = 0;
    if (max_x > buffer->width)   max_x = buffer->width;
    if (max_y > buffer->height)  max_y = buffer->height;
    
    u32 color = ((r << 16) |
                 (g << 8)  |
                 (b << 0));
    
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
    
    global_running = true;
    
    LARGE_INTEGER last_counter;
    QueryPerformanceCounter(&last_counter);
    u64 last_cycle_count = __rdtsc();
    while (global_running) {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                global_running = false;
            }
            
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        
        //
        // simulate
        //
        
        
        //
        // render
        //
        Vector2 block_pos = {2,2};
        win32_render_block(&global_backbuffer, block_pos);
        
        Win32_Window_Dimension dimension = win32_get_window_dimension(window);
        win32_display_buffer_in_window(&global_backbuffer, device_context, dimension.width, dimension.height);
        
        //
        // sleep
        //
        DWORD sleep_ms = 100;
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