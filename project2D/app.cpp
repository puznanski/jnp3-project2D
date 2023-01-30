#include "app.h"

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <cmath>
#include <windowsx.h>
#include <numbers>


INT WINAPI wWinMain(_In_ [[maybe_unused]] HINSTANCE instance,
                    _In_opt_ [[maybe_unused]] HINSTANCE prev_instance,
                    _In_ [[maybe_unused]] PWSTR cmd_line,
                    _In_ [[maybe_unused]] INT cmd_show) {

    App app;

    if (SUCCEEDED(app.Initialize(instance, cmd_show))) {
        app.RunMessageLoop();
    }

    return 0;
}

LRESULT CALLBACK App::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    if (msg == WM_CREATE) {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
        App *app = (App *) pcs->lpCreateParams;

        ::SetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(app)
        );

        result = 1;
    } else {
        App *app = reinterpret_cast<App *>(static_cast<LONG_PTR>(
                ::GetWindowLongPtrW(
                        hwnd,
                        GWLP_USERDATA
                )));

        bool wasHandled = false;

        if (app) {
            switch (msg) {
                case WM_SIZE: {
                    UINT width = LOWORD(lParam);
                    UINT height = HIWORD(lParam);
                    app->OnResize(width, height);
                }
                    result = 0;
                    wasHandled = true;
                    break;

                case WM_DISPLAYCHANGE: {
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                    result = 0;
                    wasHandled = true;
                    break;

                case WM_PAINT: {
                    app->OnRender();
                    ValidateRect(hwnd, NULL);
                }
                    result = 0;
                    wasHandled = true;
                    break;

                case WM_DESTROY: {
                    PostQuitMessage(0);
                }
                    result = 1;
                    wasHandled = true;
                    break;
                case WM_MOUSEMOVE: {
                    app->mouse_x = GET_X_LPARAM(lParam);
                    app->mouse_y = GET_Y_LPARAM(lParam);
                }
                    result = 0;
                    wasHandled = true;
                    break;
            }
        }

        if (!wasHandled) {
            result = DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    return result;
}

App::App() :
        hwnd(nullptr),
        direct2d_factory(nullptr),
        direct2d_render_target(nullptr),
        main_brush(nullptr) {}

App::~App() {
    SafeRelease(&direct2d_factory);
    SafeRelease(&direct2d_render_target);
}

void App::RunMessageLoop() {
    MSG msg;

    do {
        mouse_pressed = (GetAsyncKeyState(VK_LBUTTON) < 0);

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message != WM_QUIT) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } else {
            update_simulation_step_sizes();
            game.game_step({mouse_x, mouse_y}, mouse_pressed, simulation_step_window_size, simulation_step_board_start,
                           simulation_step_board_size, simulation_step_board_scale);
            OnRender();
        }
    } while (msg.message != WM_QUIT);
}

HRESULT App::Initialize(HINSTANCE instance, INT cmd_show) {
    HRESULT hr = CreateDeviceIndependentResources();

    if (SUCCEEDED(hr)) {
        // Register the window class.
        WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = App::WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        wcex.hInstance = instance;
        wcex.hbrBackground = nullptr;
        wcex.lpszMenuName = nullptr;
        wcex.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
        wcex.lpszClassName = L"D2DApp";

        ATOM register_result = RegisterClassEx(&wcex);
        if (register_result == 0) {
            return 1;
        }

        hwnd = CreateWindowEx(
                0,                              // Optional window styles.
                L"D2DApp",                      // Window class
                L"JNP3 - 2D project",           // Window text
                WS_OVERLAPPEDWINDOW,            // Window style

                // Size and position
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                nullptr,       // Parent window
                nullptr,       // Menu
                instance,  // Instance handle
                this        // Additional application data
        );

        if (hwnd) {
            // Because the SetWindowPos function takes its size in pixels, we
            // obtain the window's DPI, and use it to scale the window size.
            float dpi = GetDpiForWindow(hwnd);

            SetWindowPos(
                    hwnd,
                    NULL,
                    NULL,
                    NULL,
                    static_cast<int>(ceil(WINDOW_HEIGHT * dpi / 96.f) * 2),
                    static_cast<int>(ceil(WINDOW_WIDTH * dpi / 96.f) * 2),
                    SWP_NOMOVE);
            ShowWindow(hwnd, SW_SHOWNORMAL);
            UpdateWindow(hwnd);
        } else {
            return 1;
        }
    }

    return hr;
}

HRESULT App::CreateDeviceResources() {
    using D2D1::RadialGradientBrushProperties;
    using D2D1::ColorF;
    using D2D1::Point2F;

    HRESULT hr = S_OK;

    if (!direct2d_render_target) {
        RECT rc;
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
                rc.right - rc.left,
                rc.bottom - rc.top
        );

        // Create a Direct2D render target.
        hr = direct2d_factory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(hwnd, size),
                &direct2d_render_target
        );

        if (SUCCEEDED(hr)) {
            // Create main brush.
            hr = direct2d_render_target->CreateSolidColorBrush(
                    brush_color,
                    &main_brush
            );
        }

        if (SUCCEEDED(hr)) {
            // Create brush 1.
            hr = direct2d_render_target->CreateSolidColorBrush(
                    color1,
                    &brush1
            );
        }

        if (SUCCEEDED(hr)) {
            // Create brush 2.
            hr = direct2d_render_target->CreateSolidColorBrush(
                    color2,
                    &brush2
            );
        }
    }

    return hr;
}

void App::DiscardDeviceResources() {
    SafeRelease(&direct2d_render_target);
    SafeRelease(&main_brush);
    SafeRelease(&brush1);
    SafeRelease(&brush2);
}

HRESULT App::OnRender() {
    HRESULT hr = S_OK;

    hr = CreateDeviceResources();

    if (SUCCEEDED(hr)) {
        direct2d_render_target->BeginDraw();
        direct2d_render_target->Clear(background_color);

        auto board_border = D2D1::RectF(
                simulation_step_board_start.first,
                simulation_step_board_start.second,
                simulation_step_board_start.first + simulation_step_board_size.first,
                simulation_step_board_start.second + simulation_step_board_size.second
        );

        direct2d_render_target->DrawRectangle(board_border, main_brush);

        draw_bricks();
        draw_paddle();
        draw_ball();

        hr = direct2d_render_target->EndDraw();
    }

    if (hr == D2DERR_RECREATE_TARGET) {
        hr = S_OK;
        DiscardDeviceResources();
    }

    return hr;
}

void App::OnResize(UINT width, UINT height) {
    if (direct2d_render_target) {
        // Note: This method can fail, but it's okay to ignore the
        // error here, because the error will be returned again
        // the next time EndDraw is called.
        direct2d_render_target->Resize(D2D1::SizeU(width, height));

        update_simulation_step_sizes();
        OnRender();
    }
}

HRESULT App::CreateDeviceIndependentResources() {
    HRESULT hr = S_OK;

    // Create a Direct2D factory.
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &direct2d_factory);

    return hr;
}

void App::update_simulation_step_sizes() {
    D2D1_SIZE_F window_size = direct2d_render_target->GetSize();
    simulation_step_window_size = {
            window_size.width,
            window_size.height
    };
    simulation_step_board_size = {
            simulation_step_window_size.second * BOARD_SCALE,
            simulation_step_window_size.second * BOARD_SCALE
    };
    simulation_step_board_start = {
            (simulation_step_window_size.first - simulation_step_board_size.first) / 2,
            (simulation_step_window_size.second - simulation_step_board_size.second) / 2
    };
    simulation_step_board_scale = {
            simulation_step_board_size.first / simulation_step_window_size.first,
            BOARD_SCALE
    };
}

void App::draw_bricks() {
    auto bricks = game.get_bricks_to_draw(simulation_step_window_size, simulation_step_board_start, simulation_step_board_scale);

    for (const auto &brick : bricks) {
        auto brick_rect = D2D1::RectF(
                brick.position.first,
                brick.position.second,
                brick.position.first + brick.BRICK_WIDTH,
                brick.position.second + brick.BRICK_HEIGHT
        );

        if (brick.type == BrickType::Unbreakable) {
            direct2d_render_target->FillRectangle(brick_rect, brush1);
        }
        else if (brick.type == BrickType::Award) {
            direct2d_render_target->FillRectangle(brick_rect, brush2);
        }
        else {
            direct2d_render_target->FillRectangle(brick_rect, main_brush);
        }
    }
}

void App::draw_paddle() {
    auto paddle = game.get_paddle_to_draw(simulation_step_window_size, simulation_step_board_start, simulation_step_board_scale);

    auto paddle_rect = D2D1::RectF(
            paddle.position.first,
            paddle.position.second,
            paddle.position.first + paddle.WIDTH,
            paddle.position.second + paddle.HEIGHT
    );

    direct2d_render_target->FillRectangle(paddle_rect, main_brush);
}

void App::draw_ball() {
    auto ball = game.get_ball_to_draw(simulation_step_window_size, simulation_step_board_start, simulation_step_board_scale);

    auto ball_ellipse = D2D1::Ellipse(
            D2D1::Point2F(ball.position.first, ball.position.second),
            ball.BALL_RADIUS,
            ball.BALL_RADIUS
    );

    direct2d_render_target->FillEllipse(ball_ellipse, main_brush);
}


