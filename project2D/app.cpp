#include "app.h"

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <cmath>
#include <windowsx.h>
#include <numbers>
#include <string>
#include <wincodec.h>


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
        write_factory(nullptr),
        wic_factory(nullptr),
        main_brush(nullptr),
        board_background_brush(nullptr),
        brush1(nullptr),
        brush2(nullptr),
        background_gradient_brush(nullptr),
        arrows_gradient_brush(nullptr),
        star_gradient_brush(nullptr),
        heart_gradient_brush(nullptr),
        text_format(nullptr),
        board_bitmap(nullptr),
        normal_brick_bitmap(nullptr),
        upgrade_brick_bitmap(nullptr),
        award_brick_bitmap(nullptr),
        paddle_bitmap(nullptr) {
    game = new Game();
}

App::~App() {
    SafeRelease(&direct2d_factory);
    SafeRelease(&direct2d_render_target);
    SafeRelease(&write_factory);
    SafeRelease(&wic_factory);
    delete game;
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
            bool result = game->game_step({mouse_x, mouse_y}, mouse_pressed, simulation_step_window_size,
                                          simulation_step_board_start, simulation_step_board_size,
                                          simulation_step_board_scale);

            if (result && (!game->get_won())) {
                delete game;
                game = new Game();
            }

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
            // Create board background brush.
            hr = direct2d_render_target->CreateSolidColorBrush(
                    background_color,
                    &board_background_brush
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

        ID2D1GradientStopCollection* background_gradient_stops = nullptr;
        UINT const NUM_BACKGROUND_GRADIENT_STOPS = 2;
        D2D1_GRADIENT_STOP background_gradient_stops_data[NUM_BACKGROUND_GRADIENT_STOPS];

        /*if (SUCCEEDED(hr)) {
            background_gradient_stops_data[0] =
                    { .position = 0.0f, .color = ColorF(0.39f, 0.69f, 0.83f, 1.0f) };
            background_gradient_stops_data[1] =
                    { .position = 0.4f, .color = ColorF(0.15f, 0.28f, 0.45f, 1.0f) };
            background_gradient_stops_data[2] =
                    { .position = 1.0f, .color = ColorF(0.06f, 0.04f, 0.22f, 1.0f) };
            hr = direct2d_render_target->CreateGradientStopCollection(
                    background_gradient_stops_data, NUM_BACKGROUND_GRADIENT_STOPS, &background_gradient_stops);
        }*/

        if (SUCCEEDED(hr)) {
            background_gradient_stops_data[0] =
                    { .position = 0.0f, .color = ColorF(0.29f, 0.29f, 0.29f, 1.0f) };
            background_gradient_stops_data[1] =
                    { .position = 1.0f, .color = ColorF(0.0f, 0.0f, 0.0f, 1.0f) };
            hr = direct2d_render_target->CreateGradientStopCollection(
                    background_gradient_stops_data, NUM_BACKGROUND_GRADIENT_STOPS, &background_gradient_stops);
        }

        if (SUCCEEDED(hr)) {
            hr = direct2d_render_target->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(
                            Point2F(0, 0),
                            Point2F(0, WINDOW_HEIGHT)),
                    background_gradient_stops, &background_gradient_brush);
        }

        ID2D1GradientStopCollection* arrows_gradient_stops = nullptr;
        UINT const NUM_ARROWS_GRADIENT_STOPS = 2;
        D2D1_GRADIENT_STOP arrows_gradient_stops_data[NUM_ARROWS_GRADIENT_STOPS];

        if (SUCCEEDED(hr)) {
            arrows_gradient_stops_data[0] =
                    { .position = 0.0f, .color = ColorF(0.71f, 0.27f, 0.96f, 1.0f) };
            arrows_gradient_stops_data[1] =
                    { .position = 0.8f, .color = ColorF(0.66f, 0.15f, 0.33f, 1.0f) };
            hr = direct2d_render_target->CreateGradientStopCollection(
                    arrows_gradient_stops_data, NUM_ARROWS_GRADIENT_STOPS, &arrows_gradient_stops);
        }

        if (SUCCEEDED(hr)) {
            hr = direct2d_render_target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                            Point2F(32, 32),
                            Point2F(0, 0),
                            32, 32),
                    arrows_gradient_stops, &arrows_gradient_brush);
        }

        ID2D1GradientStopCollection* star_gradient_stops = nullptr;
        UINT const NUM_STAR_GRADIENT_STOPS = 2;
        D2D1_GRADIENT_STOP star_gradient_stops_data[NUM_STAR_GRADIENT_STOPS];

        if (SUCCEEDED(hr)) {
            star_gradient_stops_data[0] =
                    { .position = 0.0f, .color = ColorF(0.98f, 0.82f, 0.38f, 1.0f) };
            star_gradient_stops_data[1] =
                    { .position = 1.0f, .color = ColorF(0.9f, 0.53f, 0.27f, 1.0f) };
            hr = direct2d_render_target->CreateGradientStopCollection(
                    star_gradient_stops_data, NUM_STAR_GRADIENT_STOPS, &star_gradient_stops);
        }

        if (SUCCEEDED(hr)) {
            hr = direct2d_render_target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                            Point2F(40, 40),
                            Point2F(0, 0),
                            40, 40),
                    star_gradient_stops, &star_gradient_brush);
        }

        ID2D1GradientStopCollection* heart_gradient_stops = nullptr;
        UINT const NUM_HEART_GRADIENT_STOPS = 2;
        D2D1_GRADIENT_STOP heart_gradient_stops_data[NUM_HEART_GRADIENT_STOPS];

        if (SUCCEEDED(hr)) {
            heart_gradient_stops_data[0] =
                    { .position = 0.0f, .color = ColorF(1.0f, 0.28f, 0.31f, 1.0f) };
            heart_gradient_stops_data[1] =
                    { .position = 1.0f, .color = ColorF(0.55f, 0.13f, 0.13f, 1.0f) };
            hr = direct2d_render_target->CreateGradientStopCollection(
                    heart_gradient_stops_data, NUM_HEART_GRADIENT_STOPS, &heart_gradient_stops);
        }

        if (SUCCEEDED(hr)) {
            hr = direct2d_render_target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                            Point2F(32, 32),
                            Point2F(0, 0),
                            32, 32),
                    heart_gradient_stops, &heart_gradient_brush);
        }

        if (SUCCEEDED(hr)) {
            hr = write_factory->CreateTextFormat(
                    L"Consolas",
                    nullptr,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    100.0f,
                    L"en-us",
                    &text_format
            );
        }

        if (SUCCEEDED(hr)) {
            hr = LoadBitmapFromFile(L"assets\\board.png", &board_bitmap);
        }

        if (SUCCEEDED(hr)) {
            hr = LoadBitmapFromFile(L"assets\\brick.png", &normal_brick_bitmap);
        }

        if (SUCCEEDED(hr)) {
            hr = LoadBitmapFromFile(L"assets\\brick1.png", &upgrade_brick_bitmap);
        }

        if (SUCCEEDED(hr)) {
            hr = LoadBitmapFromFile(L"assets\\brick2.png", &award_brick_bitmap);
        }

        if (SUCCEEDED(hr)) {
            hr = LoadBitmapFromFile(L"assets\\paddle.png", &paddle_bitmap);
        }
    }

    return hr;
}

void App::DiscardDeviceResources() {
    SafeRelease(&direct2d_render_target);
    SafeRelease(&main_brush);
    SafeRelease(&board_background_brush);
    SafeRelease(&brush1);
    SafeRelease(&brush2);
    SafeRelease(&background_gradient_brush);
    SafeRelease(&arrows_gradient_brush);
    SafeRelease(&star_gradient_brush);
    SafeRelease(&heart_gradient_brush);
    SafeRelease(&text_format);
    SafeRelease(&board_bitmap);
    SafeRelease(&normal_brick_bitmap);
    SafeRelease(&upgrade_brick_bitmap);
    SafeRelease(&award_brick_bitmap);
    SafeRelease(&paddle_bitmap);
}

HRESULT App::OnRender() {
    HRESULT hr = S_OK;

    hr = CreateDeviceResources();

    if (SUCCEEDED(hr)) {
        direct2d_render_target->BeginDraw();
        direct2d_render_target->Clear(background_color);

        auto background = D2D1::RectF(
                0,
                0,
                simulation_step_window_size.first,
                simulation_step_window_size.second
        );

        //direct2d_render_target->FillRectangle(background, background_gradient_brush);

        auto board_border = D2D1::RectF(
                simulation_step_board_start.first,
                simulation_step_board_start.second,
                simulation_step_board_start.first + simulation_step_board_size.first,
                simulation_step_board_start.second + simulation_step_board_size.second
        );

        direct2d_render_target->DrawBitmap(
                board_bitmap,
                board_border,
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
        );

        //direct2d_render_target->FillRectangle(board_border, board_background_brush);
        //direct2d_render_target->DrawRectangle(board_border, main_brush);

        draw_bricks();
        draw_paddle();
        draw_balls();
        draw_drops();
        draw_lives();
        draw_points();

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

    // Create a DirectWrite factory.
    if (SUCCEEDED(hr)) {
        hr = DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&write_factory)
        );
    }

    // Create a WIC factory.
    if (SUCCEEDED(hr)) {
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }

    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                __uuidof(IWICImagingFactory),
                reinterpret_cast<LPVOID*>(&wic_factory)
        );
    }

    return hr;
}

HRESULT App::LoadBitmapFromFile(PCWSTR uri, ID2D1Bitmap **bitmap) {
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *source = NULL;
    IWICStream *stream = NULL;
    IWICFormatConverter *converter = NULL;
    IWICBitmapScaler *scaler = NULL;

    HRESULT hr = wic_factory->CreateDecoderFromFilename(
            uri,
            NULL,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder
    );

    if (SUCCEEDED(hr)) {
        // Create the initial frame.
        hr = decoder->GetFrame(0, &source);
    }

    if (SUCCEEDED(hr)) {
        // Convert the image format to 32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
        hr = wic_factory->CreateFormatConverter(&converter);
    }

    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(
                source,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeMedianCut
        );
    }

    if (SUCCEEDED(hr)) {
        // Create a Direct2D bitmap from the WIC bitmap.
        hr = direct2d_render_target->CreateBitmapFromWicBitmap(
                converter,
                NULL,
                bitmap
        );
    }

    SafeRelease(&decoder);
    SafeRelease(&source);
    SafeRelease(&stream);
    SafeRelease(&converter);
    SafeRelease(&scaler);

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
    simulation_step_visual_gap = simulation_step_board_size.second / 150.0;
}

void App::draw_bricks() {
    auto bricks = game->get_bricks_to_draw(simulation_step_window_size, simulation_step_board_start, simulation_step_board_scale);

    for (const auto &brick : bricks) {
        auto brick_rect = D2D1::RectF(
                brick.position.first,
                brick.position.second,
                brick.position.first + brick.width,
                brick.position.second + brick.height
        );

        if (brick.type == BrickType::Upgrade) {
            //direct2d_render_target->FillRectangle(brick_rect, brush1);
            direct2d_render_target->DrawBitmap(
                    upgrade_brick_bitmap,
                    brick_rect,
                    1.0f,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        }
        else if (brick.type == BrickType::Award) {
            //direct2d_render_target->FillRectangle(brick_rect, brush2);
            direct2d_render_target->DrawBitmap(
                    award_brick_bitmap,
                    brick_rect,
                    1.0f,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        }
        else {
            //direct2d_render_target->FillRectangle(brick_rect, main_brush);
            direct2d_render_target->DrawBitmap(
                    normal_brick_bitmap,
                    brick_rect,
                    1.0f,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        }
    }
}

void App::draw_paddle() {
    auto paddle = game->get_paddle_to_draw(simulation_step_window_size, simulation_step_board_start, simulation_step_board_scale);

    auto paddle_rect = D2D1::RectF(
            paddle.position.first,
            paddle.position.second,
            paddle.position.first + paddle.WIDTH,
            paddle.position.second + paddle.HEIGHT
    );

    //direct2d_render_target->FillRectangle(paddle_rect, main_brush);
    direct2d_render_target->DrawBitmap(
            paddle_bitmap,
            paddle_rect,
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
    );
}

void App::draw_balls() {
    auto balls = game->get_balls_to_draw(simulation_step_window_size, simulation_step_board_start, simulation_step_board_scale);

    for (const auto &ball : balls) {
        auto ball_ellipse = D2D1::Ellipse(
                D2D1::Point2F(ball.position.first, ball.position.second),
                ball.radius,
                ball.radius
        );

        direct2d_render_target->FillEllipse(ball_ellipse, main_brush);
        direct2d_render_target->DrawEllipse(ball_ellipse, board_background_brush);
    }
}

void App::draw_drops() {
    auto arrows = create_arrows();
    auto star = create_star();
    auto drops = game->get_drops_to_draw(simulation_step_window_size, simulation_step_board_start, simulation_step_board_scale);

    for (const auto &drop : drops) {
        if (drop.type == DropType::Arrows) {
            draw_arrows(arrows, {drop.position.first, drop.position.second}, {drop.width, drop.width});
        }
        else if (drop.type == DropType::Star) {
            draw_star(star, {drop.position.first, drop.position.second}, {drop.width, drop.width});
        }
    }
}

void App::draw_lives() {
    auto heart = create_heart();
    auto lives = game->get_lives();
    auto max_lives = game->get_max_lives();
    auto field_size = simulation_step_board_size.first / 6;
    auto heart_size = field_size / max_lives;
    Position start_point = {simulation_step_board_start.first + simulation_step_board_size.first,
                            simulation_step_board_start.second - heart_size - simulation_step_visual_gap};

    for (auto i = 0; i < lives; i++) {
        draw_heart(
                heart,
                {(start_point.first - i * (heart_size + simulation_step_visual_gap)) - heart_size / 2, start_point.second + heart_size / 2},
                {heart_size, heart_size}
        );
    }
}

void App::draw_points() {
    auto star = create_star();
    auto star_size = simulation_step_board_size.first / 18;

    draw_star(
            star,
            {simulation_step_board_start.first + star_size / 2, simulation_step_board_start.second - star_size / 2 - simulation_step_visual_gap},
            {star_size, star_size}
    );

    auto points = game->get_points();
    auto points_str = std::to_string(points);
    std::int16_t points_str_it = points_str.length() - 1;
    auto TEXT = new WCHAR [CHARACTERS_IN_POINTS_TEXT];
    TEXT[CHARACTERS_IN_POINTS_TEXT - 1] = '\0';

    for (auto i = CHARACTERS_IN_POINTS_TEXT - 2; i >= 0; i--) {
        if (points_str_it >= 0) {
            TEXT[i] = points_str[points_str_it];
            points_str_it--;
        }
        else {
            TEXT[i] = '0';
        }
    }

    auto transformation = D2D1::Matrix3x2F::Scale(star_size / 100.0f, star_size / 100.0f) *
                          D2D1::Matrix3x2F::Translation(
                                  simulation_step_board_start.first + star_size + simulation_step_visual_gap * 2,
                                  simulation_step_board_start.second - star_size - simulation_step_visual_gap
                          );

    direct2d_render_target->SetTransform(transformation);
    direct2d_render_target->DrawText(
            TEXT,
            CHARACTERS_IN_POINTS_TEXT,
            text_format,
            D2D1::RectF(
                    0.0f, 0.0f,
                    simulation_step_window_size.first,
                    simulation_step_window_size.second
            ),
            main_brush
    );
    direct2d_render_target->SetTransform(D2D1::Matrix3x2F::Identity());
}

void App::draw_arrows(ID2D1PathGeometry* path, D2DSize position, D2DSize size) {
    auto transformation = D2D1::Matrix3x2F::Translation(-32.0f, -32.0f) *
                          D2D1::Matrix3x2F::Scale(size.first / 45.0f, size.second / 45.0f) *
                          D2D1::Matrix3x2F::Translation(position.first, position.second);

    direct2d_render_target->SetTransform(transformation);
    direct2d_render_target->FillGeometry(path, arrows_gradient_brush);
    //direct2d_render_target->DrawGeometry(path, main_brush);
    direct2d_render_target->SetTransform(D2D1::Matrix3x2F::Identity());
}

void App::draw_star(ID2D1PathGeometry* path, D2DSize position, D2DSize size) {
    auto transformation = D2D1::Matrix3x2F::Translation(-40.0f, -40.0f) *
                          D2D1::Matrix3x2F::Scale(size.first / 80.0f, size.second / 80.0f) *
                          D2D1::Matrix3x2F::Translation(position.first, position.second);

    direct2d_render_target->SetTransform(transformation);
    direct2d_render_target->FillGeometry(path, star_gradient_brush);
    direct2d_render_target->DrawGeometry(path, main_brush);
    direct2d_render_target->SetTransform(D2D1::Matrix3x2F::Identity());
}

void App::draw_heart(ID2D1PathGeometry *path, D2DSize position, D2DSize size) {
    auto transformation = D2D1::Matrix3x2F::Translation(-32.0f, -32.0f) *
                          D2D1::Matrix3x2F::Scale(size.first / 64.0f, size.second / 64.0f) *
                          D2D1::Matrix3x2F::Translation(position.first, position.second);

    direct2d_render_target->SetTransform(transformation);
    direct2d_render_target->FillGeometry(path, heart_gradient_brush);
    direct2d_render_target->DrawGeometry(path, board_background_brush);
    direct2d_render_target->SetTransform(D2D1::Matrix3x2F::Identity());
}

ID2D1PathGeometry *App::create_arrows() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    ID2D1PathGeometry *path = nullptr;
    ID2D1GeometrySink *path_sink = nullptr;
    direct2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

    path_sink->BeginFigure(Point2F(32.0f, 23.9f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(32.0f, 23.9f), Point2F(19.9f, 35.0f), Point2F(19.9f, 35.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(19.9f, 35.0f), Point2F(12.9f, 28.0f), Point2F(12.9f, 28.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(12.9f, 28.0f), Point2F(31.9f, 11.0f), Point2F(31.9f, 11.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(31.9f, 11.0f), Point2F(51.0f, 28.0f), Point2F(51.0f, 28.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(51.0f, 28.0f), Point2F(44.0f, 35.0f), Point2F(44.0f, 35.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(44.0f, 35.0f), Point2F(32.0f, 23.9f), Point2F(32.0f, 23.9f)));
    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);

    path_sink->BeginFigure(Point2F(32.0f, 29.0f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(32.0f, 29.0f), Point2F(51.0f, 47.0f), Point2F(51.0f, 47.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(51.0f, 47.0f), Point2F(44.0f, 53.0f), Point2F(44.0f, 53.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(44.0f, 53.0f), Point2F(32.0f, 42.0f), Point2F(32.0f, 42.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(32.0f, 42.0f), Point2F(19.9f, 53.0f), Point2F(19.9f, 53.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(19.9f, 53.0f), Point2F(13.0f, 46.9f), Point2F(13.0f, 46.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(13.0f, 46.9f), Point2F(32.0f, 29.0f), Point2F(32.0f, 29.0f)));
    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);

    path_sink->Close();

    return path;
}

ID2D1PathGeometry *App::create_star() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    ID2D1PathGeometry *path = nullptr;
    ID2D1GeometrySink *path_sink = nullptr;
    direct2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

    path_sink->BeginFigure(Point2F(40.0f, 3.0f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(40.0f, 3.0f), Point2F(27.8f, 27.0f), Point2F(27.8f, 27.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(27.8f, 27.0f), Point2F(0.0f, 31.5f), Point2F(0.0f, 31.5f)));
    path_sink->AddBezier(BezierSegment(Point2F(0.0f, 31.5f), Point2F(19.9f, 50.0f), Point2F(19.9f, 50.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(19.9f, 50.0f), Point2F(15.0f, 78.0f), Point2F(15.0f, 78.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(15.0f, 78.0f), Point2F(39.9f, 65.0f), Point2F(39.9f, 65.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(39.9f, 65.0f), Point2F(64.9f, 77.9f), Point2F(64.9f, 77.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(64.9f, 77.9f), Point2F(60.0f, 50.1f), Point2F(60.0f, 50.1f)));
    path_sink->AddBezier(BezierSegment(Point2F(60.0f, 50.1f), Point2F(80.0f, 31.4f), Point2F(80.0f, 31.4f)));
    path_sink->AddBezier(BezierSegment(Point2F(80.0f, 31.4f), Point2F(52.0f, 27.0f), Point2F(52.0f, 27.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(52.0f, 27.0f), Point2F(40.0f, 3.0f), Point2F(40.0f, 3.0f)));
    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);

    path_sink->Close();

    return path;
}

ID2D1PathGeometry *App::create_heart() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    ID2D1PathGeometry *path = nullptr;
    ID2D1GeometrySink *path_sink = nullptr;
    direct2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

    path_sink->BeginFigure(Point2F(32.0f, 59.9f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(16.9f, 43.7f), Point2F(2.6f, 29.9f), Point2F(2.6f, 18.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(2.6f, 6.9f), Point2F(10.8f, 2.9f), Point2F(16.7f, 2.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(20.2f, 2.9f), Point2F(27.8f, 4.3f), Point2F(32.0f, 15.8f)));
    path_sink->AddBezier(BezierSegment(Point2F(36.2f, 4.3f), Point2F(43.9f, 2.9f), Point2F(47.2f, 2.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(54.0f, 2.9f), Point2F(61.3f, 7.6f), Point2F(61.3f, 18.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(61.3f, 29.8f), Point2F(47.6f, 43.1f), Point2F(32.0f, 59.91f)));
    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);

    path_sink->Close();

    return path;
}
