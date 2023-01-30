#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d2d1_3.h>
#include "game.h"

using D2DSize = std::pair<float, float>;

template<class Interface>
inline void SafeRelease(
        Interface **ppInterfaceToRelease) {
    if (*ppInterfaceToRelease != NULL) {
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

INT WINAPI wWinMain(_In_ HINSTANCE instance,
                    _In_opt_ HINSTANCE prev_instance,
                    _In_ PWSTR cmd_line,
                    _In_ INT cmd_show);

class App {
public:
    App();

    ~App();

    HRESULT Initialize(HINSTANCE instance, INT cmd_show);

    void RunMessageLoop();

private:
    const FLOAT WINDOW_HEIGHT = 600;
    const FLOAT WINDOW_WIDTH = 600;
    const FLOAT BOARD_SCALE = 0.7;

    HWND hwnd;
    ID2D1Factory *direct2d_factory;
    ID2D1HwndRenderTarget *direct2d_render_target;
    ID2D1SolidColorBrush *main_brush;
    ID2D1SolidColorBrush *brush1;
    ID2D1SolidColorBrush *brush2;

    HRESULT CreateDeviceIndependentResources();

    HRESULT CreateDeviceResources();

    void DiscardDeviceResources();

    HRESULT OnRender();

    void OnResize(
            UINT width,
            UINT height
    );

    static LRESULT CALLBACK WindowProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
    );

    INT mouse_x = 0;
    INT mouse_y = 0;
    bool mouse_pressed = false;

    D2DSize simulation_step_window_size;
    D2DSize simulation_step_board_start;
    D2DSize simulation_step_board_size;
    D2DSize simulation_step_board_scale;


    Game game;

    void update_simulation_step_sizes();

    void draw_bricks();

    void draw_paddle();

    void draw_ball();

    static constexpr D2D1_COLOR_F background_color =
            {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f};
    static constexpr D2D1_COLOR_F brush_color =
            {.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f};
    static constexpr D2D1_COLOR_F color1 =
            {.r = 1.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f};
    static constexpr D2D1_COLOR_F color2 =
            {.r = 1.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f};

};
