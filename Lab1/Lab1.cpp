// Lab1.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "Lab1.h"
#include "geometry.h"
#include "engine.h"
#include "camera.h"
#include "tgaimage.h"
#include "model.h"

#include "BS_thread_pool.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include "basewin.h"

#define MAX_LOADSTRING 100  

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND win;
std::string filePath = "C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\Models\\Intergalactic Spaceship\\Intergalactic_Spaceship.obj";

BS::thread_pool pool(12);

Model model;

int width = 1920;
int height = 1080;
float aspect = width / height;
float FOV = 90.0f;
float zfar = 100.0f;
float znear = 0.1f;
float lastX = 400, lastY = 300;
float constLight = .1f;

Camera camera{10,  3.141557f / 2 ,  3.141557f / 2 , Vec4f(0.0f , 0.0f , 0.0f, 1)};
Vec3f light{ 1.0f, 1.0f, 1.0f};
Vec3f lightPos{ 5.0f, 5.0f, 5.0f };

Vec3f* buffer;
float* zbuffer;
DWORD* bufferOut;
Vec3f** bloom_buffer;

CRITICAL_SECTION* sync_buffer;

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class MainWindow : public BaseWindow<MainWindow>
{
    ID2D1Factory* pFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    ID2D1SolidColorBrush* pBrush;
    IDWriteFactory* pDWriteFactory_;
    IDWriteTextFormat* pTextFormat_;
    ID2D1Bitmap* bitmap;
    int maxInd;
    D2D1_RECT_U winSize;
    D2D1_RECT_F winSizeF;

    float                  fontSize = 10.0f;

    HRESULT             CreateGraphicsResources();
    void                DiscardGraphicsResources();
    void                OnPaint();

public:

    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL)
    {
    }

    PCWSTR  ClassName() const { return L"Engine Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {

    MainWindow win;
    hInst = hInstance;

    if (!win.Create(L"Engine", WS_POPUP))
    {
        return 0;
    }

    model = Model("C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\Models\\Shovel Knight", "shovel");
    light = light.normalize();

    ShowWindow(win.Window(), SW_SHOWMAXIMIZED);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB1));
    MSG msg = { };
    RECT rec;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            InvalidateRect(win.Window(), NULL, TRUE);
        }
    }

    return 0;
}

HRESULT MainWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        static const WCHAR msc_fontName[] = L"Verdana";
        static const FLOAT msc_fontSize = 20.0f;
        fontSize = msc_fontSize;

        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&pDWriteFactory_)
        );

        if (SUCCEEDED(hr))
        {
            hr = pDWriteFactory_->CreateTextFormat(
                msc_fontName,                      // Font family name.
                NULL,                       // Font collection (NULL sets it to use the system font collection).
                DWRITE_FONT_WEIGHT_REGULAR,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                msc_fontSize,
                L"en-us",
                &pTextFormat_
            );
            if (SUCCEEDED(hr))
            {
                hr = pTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            }

            if (SUCCEEDED(hr))
            {
                hr = pTextFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
        }


        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right * 2, rc.bottom * 2);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(0.0f, 0.0f, 0.0f);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

            D2D1_SIZE_U size = pRenderTarget->GetPixelSize();
            width = size.width;
            height = size.height;
            aspect = float(width) / height;

            float xSize, ySize;
            pRenderTarget->GetDpi(&xSize, &ySize);
            D2D1_SIZE_U tempSize{ width, height };
            D2D1_BITMAP_PROPERTIES prop{ pRenderTarget->GetPixelFormat(),xSize, ySize };
            pRenderTarget->CreateBitmap(tempSize, prop, &bitmap);

            buffer = new Vec3f[width * height];
            bufferOut = new DWORD[width * height];
            zbuffer = new float[width * height];
            sync_buffer = new CRITICAL_SECTION[width * height];

            bloom_buffer = new Vec3f*[2];
            bloom_buffer[0] = new Vec3f[width * height];
            bloom_buffer[1] = new Vec3f[width * height];

            for (int i = 0; i < width * height; i++) {
                InitializeCriticalSection(&sync_buffer[i]);
            }

            maxInd = width * height;

            winSize.right = width;
            winSize.bottom = height;

            winSizeF.right = width;
            winSizeF.bottom = height;
        }
        viewport(width, height);
        projection(aspect, FOV, zfar, znear);
        camera.updateViewMatrix();
    }

    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void inline setPixel(int x, int y, Vec3f color) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    buffer[y * width + x] = color;
}

void drawLine(int x0, int y0, int x1, int y1, Vec3f color) {
    bool steep = false; 
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    float derror = std::abs(dy / float(dx));
    float error = 0;
    int y = y0;
    if (x0 < 0) x0 = 0;
    if (x1 >= width) x1 = width - 1;
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            setPixel(y, x, color);
        }
        else {
            setPixel(x, y, color);
        }
        error += derror;

        if (error > .5) {
            y += (y1 > y0 ? 1 : -1);
            error -= 1.;
        }
    }
}

Part part;

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    for (int i = 2; i--; ) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) 
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1);
}

const COLORREF rgbWhite = 0x00FFFFFF;
const COLORREF rgbBlack = 0x00000000;

Vec3f blue(0.0f,0.0f,1.0f);
Vec3f black(0.0f, 0.0f, 0.0f);
Vec3f gold(1.0f, 0.6f, 0.3f);
Vec3f white(1.0f, 1.0f, 1.0f);

void rasterize(mat<4, 3, float>& clipc, mat<3, 3, float> vn, mat<3, 3, float> global, mat<3, 3, float> textures, Vec3f defColor, Vec3f specColor) {
    mat<3, 4, float> pts = (Viewport * clipc).transpose();
    mat<3, 2, float> pts2;
    for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);

    Vec2f bboxmin(width, height);
    Vec2f bboxmax(0, 0);
    Vec2f clamp(width - 1, height - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max<float>(0.f, std::min<float>(bboxmin[j], pts2[i][j]));
            bboxmax[j] = std::min<float>(clamp[j], std::max<float>(bboxmax[j], pts2[i][j]));
        }
    }
    Vec2i P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);
            Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
            bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
            float frag_depth = 1.0f - clipc[2] * bc_clip;

            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z<0) continue;

            Vec2f textureCoords{ textures[0] * bc_clip, textures[1] * bc_clip };
            Vec3f new_color = part.diffuse(textureCoords, defColor);

            Vec3f Norm{ vn[0] * bc_clip, vn[1] * bc_clip , vn[2] * bc_clip };
            Norm = Norm.normalize();
            Norm = part.normal(textureCoords, Norm);
                        
            Vec3f globalCoords{ global[0] * bc_clip, global[1] * bc_clip , global[2] * bc_clip };
            Vec3f negLight = (lightPos - globalCoords).normalize();

            float diff = 1.0f * (Norm * negLight);
            if (diff < 0) diff = 0.0f;

            Vec3f viewDir = ((Vec3f)camera.getEye() - globalCoords).normalize();
            Vec3f R = (Norm * (Norm * negLight) - negLight).normalize();
            float spec = 10.0f * pow(max((viewDir * R), 0.0f), part.specular(textureCoords, 32));
            if (spec < 0) spec = 0.0f;

            for (int i = 0; i < 3; i++) {
                new_color[i] = 0.1f + new_color[i] * diff + specColor[i] * spec;
            }

            Vec3f light_color = part.light_map(textureCoords);

            int ind = P.x + P.y * width;
            EnterCriticalSection(&sync_buffer[ind]);
            if (zbuffer[ind] <= frag_depth) {
                zbuffer[ind] = frag_depth;

                if (light_color[0] > 0.0f || light_color[1] > 0.0f || light_color[2] > 0.0f) {
                    new_color = light_color * 5.0f;
                    bloom_buffer[0][ind] = new_color;
                }

                buffer[ind] = new_color;

            }
            LeaveCriticalSection(&sync_buffer[ind]);
        }
    }
}

void rasterize(Vec3i t0, Vec3i t1, Vec3i t2, Vec3f color) {
    if (t0.y == t1.y && t0.y == t2.y) return;
    if (t0.y > t1.y) {std::swap(t0, t1);}
    if (t0.y > t2.y) {std::swap(t0, t2);}
    if (t1.y > t2.y) {std::swap(t1, t2);}
    int total_height = t2.y - t0.y;

    for (int i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;

        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
        Vec3i A = (Vec3f)t0 + Vec3f(t2 - t0) * alpha;
        Vec3i B = second_half ? (Vec3f)t1 + Vec3f(t2 - t1) * beta : (Vec3f)t0 + Vec3f(t1 - t0) * beta;

        if (A.y < 0 || A.y >= height) continue;
        if ((A.x < 0 && B.x < 0) || (A.x >= width && B.x >= width)) continue;
        if (A.x > B.x) {std::swap(A, B);}
        Vec3f C = B - A;

        for (int j = 0; j <= C.x; j++) {
            float phi = j == C.x ? 1.f : (float)j / C.x;
            Vec3i P = Vec3f(A) + C * phi;

            if (P.x < 0 || P.y < 0 || P.x >= width || P.y >= height) {
                continue;
            }
            int idx = P.x + P.y * width;

            EnterCriticalSection(&sync_buffer[idx]);
            if (zbuffer[idx] < P.z) {
                zbuffer[idx] = P.z;
                buffer[idx] = color;
            }
            LeaveCriticalSection(&sync_buffer[idx]);
        }
    }
}


float weight[5]{ 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };


Vec3f texture(Vec3f* buf, Vec2i coords) {
    if (coords.x < 0 || coords.y < 0 || coords.x >= width || coords.y >= height) {
        return Vec3i(0, 0, 0);
    }
    return buf[coords.x + coords.y * width];
}

void guassian(Vec3f* buf, Vec3f* bufOut, Vec2i TexCoords, bool horizontal) {
    Vec3f result = texture(buf, TexCoords) * weight[0];
    if (horizontal)
    {
        for (int i = 1; i < 5; ++i)
        {
            result = result + texture(buf, TexCoords + Vec2i(1 * i, 0)) * weight[i];
            result = result + texture(buf, TexCoords - Vec2i(1 * i, 0)) * weight[i];
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
            result = result + texture(buf, TexCoords + Vec2i(0, 1 * i)) * weight[i];
            result = result + texture(buf, TexCoords - Vec2i(0, 1 * i)) * weight[i];
        }
    }
    bufOut[TexCoords.x + TexCoords.y * width] = result;
}

void bloom() {
    bool horizontal = true, first_iteration = true;
    int amount = 6;
    for (unsigned int i = 0; i < amount; i++)
    {
        const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, width,
            [first_iteration, horizontal](const size_t x)
            {
                for (int y = 0; y < height; y++) {
                    guassian(first_iteration ? bloom_buffer[0] : bloom_buffer[!horizontal], bloom_buffer[horizontal], Vec2i(x,y), horizontal);
                }
            });
        loop_future.wait();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
}



double clockToMilliseconds(clock_t ticks) {
    return (ticks / (double)CLOCKS_PER_SEC) * 1000.0;
}

clock_t deltaTime = 0;
unsigned int frames = 0;
double  frameRate = 60;
double  averageFrameTimeMilliseconds = 33.333;
bool rerender = true;


bool chained = true;
int lightMode = 0;

Matrix finalMatrix;
Matrix modelViewProjection;


void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        clock_t beginFrame = clock();

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        pRenderTarget->BeginDraw();
        if (rerender) {

            
            for (size_t i = 0; i < height; i++) {
                for (size_t j = 0; j < width; j++) {
                    int ind = i * width + j;
                    buffer[ind] = Vec3f(1.0f,1.0f,1.0f);
                    zbuffer[ind] = -100000.0f;
                    bufferOut[ind] = rgbWhite;
                    bloom_buffer[0][ind] = Vec3f(0.0f, 0.0f, 0.0f);
                }
            }

            camera.updateViewMatrix();
            finalMatrix = Viewport * Projection * ModelView;
            modelViewProjection = Projection * ModelView;
            for (int partInd = 0; partInd < model.parts.size(); partInd++) {
                part = model.parts[partInd];
                size_t size = part.faces.size();

                if (chained) {
                    const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                        [](const size_t i)
                        {
                            
                            Vec4f screen_coords[3];
                            Face face = part.faces[i];
                            if ((Vec4f(face.normal) * (camera.getEye() - model.vertexes[face.points[0].vertexInd])) > 0) {
                                return;
                            }
                            for (size_t j = 0; j < 3; j++) {
                                screen_coords[j] = finalMatrix * model.vertexes[face.points[j].vertexInd];
                                if (screen_coords[j].w < 0) {
                                    return;
                                }
                                screen_coords[j] = screen_coords[j] / screen_coords[j].w;
                            }
                            for (int j = 0; j < 3; j++) {
                                Vec3i a1 = screen_coords[j];
                                Vec3i a2 = screen_coords[(j + 1) % 3];
                                drawLine(a1[0], a1[1], a2[0], a2[1], black);
                            }
                        });
                    loop_future.wait();
                }
                if (lightMode == 1) {
                    const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                        [](const size_t i)
                        {
                            Vec4f screen_coords[3];
                            Face face = part.faces[i];
                            if ((Vec4f(face.normal) * (camera.getEye() - model.vertexes[face.points[0].vertexInd])) > 0) {
                                return;
                            }
                            for (size_t j = 0; j < 3; j++) {
                                screen_coords[j] = finalMatrix * model.vertexes[face.points[j].vertexInd];
                                if (screen_coords[j].w < 0) {
                                    return;
                                }
                                screen_coords[j] = screen_coords[j] / screen_coords[j].w;
                                screen_coords[j][2] = (1.0f - screen_coords[j][2]) * 10000.f;
                            }
                            rasterize(screen_coords[0], screen_coords[1], screen_coords[2], Vec3f(1.0f, 1.0f, 1.0f) - blue);

                        });
                    loop_future.wait();
                }
                if (lightMode == 2) {
                    const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                        [](const size_t i)
                        {
                            Vec4f screen_coords[3];
                            Face face = part.faces[i];
                            if ((Vec4f(face.normal) * (camera.getEye() - model.vertexes[face.points[0].vertexInd])) > 0) {
                                return;
                            }
                            for (size_t j = 0; j < 3; j++) {
                                screen_coords[j] = finalMatrix * model.vertexes[face.points[j].vertexInd];
                                if (screen_coords[j].w < 0) {
                                    return;
                                }
                                screen_coords[j] = screen_coords[j] / screen_coords[j].w;
                                screen_coords[j][2] = (1.0f - screen_coords[j][2]) * 10000.f;
                            }

                            float colorMultiplier = (face.normal * (light * -1)) + constLight;
                            if (colorMultiplier < 0) colorMultiplier = 0;
                            rasterize(screen_coords[0], screen_coords[1], screen_coords[2], (Vec3f(1.0f, 1.0f, 1.0f) - gold) * colorMultiplier);
                        });
                    loop_future.wait();
                }
                if (lightMode == 3) {
                    const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                        [](const size_t i)
                        {
                            mat<3, 3, float> global_coords;
                            mat<3, 3, float> normals;
                            mat<3, 3, float> textures;
                            Face face = part.faces[i];
                            mat<4, 3, float> res;
                            if ((Vec4f(face.normal) * (camera.getEye() - model.vertexes[face.points[0].vertexInd])) > 0) {
                                return;
                            }
                            for (size_t j = 0; j < 3; j++) {
                                global_coords.set_col(j, model.vertexes[face.points[j].vertexInd]);
                                textures.set_col(j, model.texturesCord[face.points[j].textureInd]);
                                res.set_col(j, modelViewProjection * model.vertexes[face.points[j].vertexInd]);
                                normals.set_col(j, face.points[j].normal);
                            }
                            rasterize(res, normals, global_coords, textures, Vec3f(1.0f, 1.0f, 1.0f) - gold, white);
                        });
                    loop_future.wait();
                    bloom();
                }

                const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, width,
                    [](const size_t x)
                    {
                        const float gamma = 1.0f;
                        const float exposure = 1.0f;
                        for (int y = 0; y < height; y++) {
                            int ind = x + y * width;
                            //Vec3f rgb = bloom_buffer[0][ind];
                            Vec3f rgb = buffer[ind] + bloom_buffer[0][ind];

                            //Vec3f mapped = Vec3f(rgb[0] / (rgb[0] + 1.0f), rgb[1] / (rgb[1] + 1.0f), rgb[2] / (rgb[2] + 1.0f));
                            //Vec3f mapped = Vec3f(rgb[0], rgb[1], rgb[2]);
                            Vec3f mapped = Vec3f(1.0f - exp(rgb[0] * (-exposure)), 1.0f - exp(rgb[1] * (-exposure)), 1.0f - exp(rgb[2] * (-exposure)));
                            mapped = Vec3f(pow(mapped[0], gamma), pow(mapped[1], gamma), pow(mapped[2], gamma));


                            for (int i = 0; i < 3; i++) {
                                if (mapped[i] < 0.0f) mapped[i] = 0.0f;
                                if (mapped[i] > 1.0f) mapped[i] = 1.0f;
                            }

                            bufferOut[ind] = RGB(mapped[0] * 255, mapped[1] * 255, mapped[2] * 255);
                        }
                    });
                loop_future.wait();
            }
            

            hr = bitmap->CopyFromMemory(&winSize, bufferOut, width * 4);
            rerender = false;
        }

        pRenderTarget->DrawBitmap(bitmap, &winSizeF, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &winSizeF);

        std::wstring fps = std::to_wstring(int(frameRate));
        D2D1_RECT_F layoutRect = D2D1::RectF(0, 0, 50, 40);
        pRenderTarget->DrawTextW(fps.c_str(), fps.size(), pTextFormat_, layoutRect, pBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }

        EndPaint(m_hwnd, &ps);

        clock_t endFrame = clock();
        deltaTime += endFrame - beginFrame;
        frames++;
        if (clockToMilliseconds(deltaTime) > 1000.0) {
            frameRate = (double)frames * 0.5 + frameRate * 0.5;
            frames = 0;
            deltaTime -= CLOCKS_PER_SEC;
            averageFrameTimeMilliseconds = 1000.0 / (frameRate == 0 ? 0.001 : frameRate);
        }
    }
}

bool mouseMove = false;

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SETCURSOR:
        break;
    case WM_MOUSEWHEEL:
    {
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (camera.updateRadius(camera.cameraR * (zDelta / 2400.0f))) rerender = true;
        break;
    }
    case WM_LBUTTONDOWN:
    {
        mouseMove = true;
        lastX = GET_X_LPARAM(lParam);
        lastY = GET_Y_LPARAM(lParam);
        break;
    }
    case WM_LBUTTONUP:
    {
        mouseMove = false;
        break;
    }
    case WM_MOUSEMOVE:
    {
        if (mouseMove) {
            int xpos = GET_X_LPARAM(lParam);
            int ypos = GET_Y_LPARAM(lParam);
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;
            lastX = xpos;
            lastY = ypos;

            float sensitivity = 0.005f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            camera.cameraPhi += xoffset;
            camera.cameraZeta += yoffset;

            rerender = true;
        }
        break;
    }
    case WM_KEYDOWN:
    {
        if (wParam == '1') {
            chained = !chained;
        }
        if (wParam == '2') {
            lightMode = (lightMode + 1) % 4;
        }
        
        float cameraSpeed = 0.05f;
        float offsetX = 0, offsetY = 0, offsetZ = 0;
        if (wParam == VK_LEFT) {
            offsetX -= cameraSpeed;
        }
        if (wParam == VK_RIGHT) {
            offsetX += cameraSpeed;
        }
        if (wParam == VK_UP) {
            offsetY += cameraSpeed;
        }
        if (wParam == VK_DOWN) {
            offsetY -= cameraSpeed;
        }
        if (wParam == 'A') {
            offsetX -= cameraSpeed;
        }
        if (wParam == 'D') {
            offsetX += cameraSpeed;
        }
        if (wParam == 'W') {
            offsetY += cameraSpeed;
        }
        if (wParam == 'S') {
            offsetY -= cameraSpeed;
        }
        camera.updateTarget(offsetX, offsetY, offsetZ);
        if (wParam == VK_ESCAPE) {
            DestroyWindow(m_hwnd);
        }
        rerender = true;
        break;
    }
    case WM_KEYUP:
    {
        break;
    }
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_MULTI_THREADED, &pFactory)))
        {
            return -1;
        }
        SetCursor(LoadCursorW(hInst, L"IDC_ARROW"));
        break;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        break;

    case WM_PAINT:
        OnPaint();
        break;
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}