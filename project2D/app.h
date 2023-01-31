#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
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
    const INT CHARACTERS_IN_POINTS_TEXT = 7;

    HWND hwnd;
    ID2D1Factory *direct2d_factory;
    ID2D1HwndRenderTarget *direct2d_render_target;
    IDWriteFactory *write_factory;
    IWICImagingFactory *wic_factory;

    ID2D1SolidColorBrush *main_brush;
    ID2D1SolidColorBrush *board_background_brush;
    ID2D1SolidColorBrush *brush1;
    ID2D1SolidColorBrush *brush2;

    ID2D1LinearGradientBrush* background_gradient_brush;
    ID2D1RadialGradientBrush* arrows_gradient_brush;
    ID2D1RadialGradientBrush* star_gradient_brush;
    ID2D1RadialGradientBrush* heart_gradient_brush;

    IDWriteTextFormat *text_format;

    ID2D1Bitmap *board_bitmap;
    ID2D1Bitmap *normal_brick_bitmap;
    ID2D1Bitmap *upgrade_brick_bitmap;
    ID2D1Bitmap *award_brick_bitmap;
    ID2D1Bitmap *paddle_bitmap;

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

    HRESULT LoadBitmapFromFile(PCWSTR uri, ID2D1Bitmap **bitmap);

    INT mouse_x = 0;
    INT mouse_y = 0;
    bool mouse_pressed = false;

    D2DSize simulation_step_window_size;
    D2DSize simulation_step_board_start;
    D2DSize simulation_step_board_size;
    D2DSize simulation_step_board_scale;
    FLOAT simulation_step_visual_gap;

    Game* game;

    void update_simulation_step_sizes();

    void draw_bricks();

    void draw_paddle();

    void draw_balls();

    void draw_drops();

    void draw_lives();

    void draw_points();

    void draw_arrows(ID2D1PathGeometry* path, D2DSize position, D2DSize size);

    void draw_star(ID2D1PathGeometry* path, D2DSize position, D2DSize size);

    void draw_heart(ID2D1PathGeometry* path, D2DSize position, D2DSize size);

    ID2D1PathGeometry* create_arrows();

    ID2D1PathGeometry* create_star();

    ID2D1PathGeometry* create_heart();

    static constexpr D2D1_COLOR_F background_color =
            {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f};
    static constexpr D2D1_COLOR_F brush_color =
            {.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f};
    static constexpr D2D1_COLOR_F color1 =
            {.r = 1.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f};
    static constexpr D2D1_COLOR_F color2 =
            {.r = 1.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f};

};
