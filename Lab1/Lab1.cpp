// Lab1.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "Lab1.h"
#include "geometry.h"
#include "engine.h"
#include "camera.h"

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
std::string filePath = "C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\car.obj";

BS::thread_pool pool(12);
struct Point {
    int vertexInd = 0;
    int textureInd = -1;
    Vec3f normal{ -1.1111f,-1,-1 };
};
struct Face {
    Point points[3];
    Vec3f normal;
};

std::vector<Face> faces;
std::vector<Vec4f> vertexes;
std::vector<Vec3f> normalies;

Vec3f calculateNormal(Face face) {
    return cross((Vec3f)(vertexes[face.points[2].vertexInd] - vertexes[face.points[0].vertexInd]), (Vec3f)(vertexes[face.points[1].vertexInd] - vertexes[face.points[0].vertexInd])).normalize();
}

int width = 1920;
int height = 1080;
float aspect = width / height;
float FOV = 90.0f;
float zfar = 100.0f;
float znear = 0.1f;
float lastX = 400, lastY = 300;
float constLight = .2f;

Camera camera{5,  3.141557f / 2 ,  3.141557f / 2 , Vec3f(0.0f , 1.0f , 0.0f)};
Vec3f light{ 1.0f, 1.0f, 1.0f};

DWORD* buffer;
int* zbuffer;
float* zbuffer_f;

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

    if (!win.Create(L"Engine", WS_POPUP))
    {
        return 0;
    }

    std::ifstream in;
    in.open(filePath);
    if (in.is_open()) {

        std::string line;
        while (std::getline(in, line)) {
            if (line.substr(0, 2) == "v ") {
                Vec4f vert;
                vert[3] = 1.0f;
                sscanf_s(line.c_str(), "v %f %f %f", &vert[0], &vert[1], &vert[2]);
                vertexes.push_back(vert);
            }
            if (line.substr(0, 3) == "vn ") {
                Vec3f normal;
                sscanf_s(line.c_str(), "vn %f %f %f", &normal.x, &normal.y, &normal.z);
                normalies.push_back(normal.normalize());
            }
            if (line.substr(0, 2) == "f ") {
                Face face;
                std::istringstream v(line.substr(2));
                std::string token;
                int ind = 0;
                while (v >> token) {
                    Point p;
                    int normalInd = -1;
                    sscanf_s(token.c_str(), "%i/%i/%i", &p.vertexInd, &p.textureInd, &normalInd);
                    p.vertexInd = p.vertexInd - 1;
                    if (normalInd > 0) p.normal = normalies[normalInd - 1];
                    face.points[ind] = p;
                    ind++;
                }
                faces.push_back(face);
            }
        }
        in.close();
        //vertexes.clear();
    }

    
    std::map<int, std::vector<Face*>> map;
    for (size_t i = 0; i < faces.size(); i++) {
        Face face = faces[i];
        faces[i].normal = calculateNormal(faces[i]);
        for (size_t j = 0; j < 3; j++) {
            map[faces[i].points[j].vertexInd].push_back(&faces[i]);
        }
    }
    

    for (size_t i = 0; i < faces.size(); i++) {
        for (size_t j = 0; j < 3; j++) {
            if (faces[i].points[j].normal.x == -1.1111f) {
                std::vector<Face*> a = map[faces[i].points[j].vertexInd];
                Vec3f norm{};
                for (size_t k = 0; k < a.size(); k++) {
                    norm = norm + a[k]->normal;
                }
                norm = (norm / a.size()).normalize();
                faces[i].points[j].normal = norm;
            }
        }
    }

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
            // Center align (horizontally) the text.
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

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 1.0f);
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
            buffer = new DWORD[width * height];
            zbuffer = new int[width * height];
            zbuffer_f = new float[width * height];

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

void inline setPixel(int x, int y, COLORREF color) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    buffer[y * width + x] = color;
}

void drawLine(int x0, int y0, int x1, int y1, COLORREF color) {
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



COLORREF getNewColor(COLORREF color, float koef) {
    if (koef > 1.f) koef = 1.0f;
    if (koef < 0) koef = 0.0f;
    return RGB(GetRValue(color) * koef, GetGValue(color) * koef, GetBValue(color) * koef);
}

COLORREF addColors(COLORREF a1, COLORREF a2) {
    BYTE R = GetRValue(a1);
    BYTE G = GetGValue(a1);
    BYTE B = GetBValue(a1);

    BYTE R1 = GetRValue(a2);
    BYTE G1 = GetGValue(a2);
    BYTE B1 = GetBValue(a2);

    int(R) + R1 > 254 ? R = 254 : R = R + R1;
    int(G) + G1 > 254 ? G = 254 : G = G + G1;
    int(B) + B1 > 254 ? B = 254 : B = B + B1;

    return RGB(R, G, B);
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    for (int i = 2; i--; ) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void rasterize(mat<4,3,float> &clipc, mat<3,3,float> vn, mat<4,3,float> global, COLORREF color) {
    mat<3, 4, float> pts = (Viewport * clipc).transpose(); // transposed to ease access to each of the points
    mat<3, 2, float> pts2;
    for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);

    
    Vec2i t0 = pts2[0], t1 = pts2[1], t2 = pts2[2];
    if (t0.y == t1.y && t0.y == t2.y) return;
    if (t0.y > t1.y) { std::swap(t0, t1); }
    if (t0.y > t2.y) { std::swap(t0, t2); }
    if (t1.y > t2.y) { std::swap(t1, t2); }
    int total_height = t2.y - t0.y;

    for (int i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;

        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
        Vec2f A = (Vec2f)t0 + Vec2f(t2 - t0) * alpha;
        Vec2f B = second_half ? (Vec2f)t1 + Vec2f(t2 - t1) * beta : (Vec2f)t0 + Vec2f(t1 - t0) * beta;

        if (A.x > B.x) { std::swap(A, B); }
        Vec2f C = B - A;

        for (int j = 0; j <= C.x; j++) {
            float phi = j == C.x ? 1.f : (float)j / C.x;
            Vec2i P = Vec2f(A) + C * phi;

            if (P.x < 0 || P.y < 0 || P.x >= width || P.y >= height) {
                continue;
            }

            Vec3f bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);
            Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
            bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
            float frag_depth = 1.0f - clipc[2] * bc_clip;

            Vec3f Norm{ vn[0] * bc_clip, vn[1] * bc_clip , vn[2] * bc_clip };
            Vec3f globalCoords{ global[0] * bc_clip, global[1] * bc_clip , global[2] * bc_clip };

            //Vec3f negLight = (light - globalCoords);
            Vec3f negLight = light;
            float Id = (Norm * negLight);
            Vec3f R = (Norm * (Norm * negLight * 2.f) - negLight).normalize();
            float Is = 0.5f * (R * (camera.getEye() - globalCoords).normalize());

            int ind = P.x + P.y * width;
            if (zbuffer_f[ind]>frag_depth) continue;
            zbuffer_f[ind] = frag_depth;
            //buffer[ind] = addColors(addColors(getNewColor(color, Is), getNewColor(color, Id)), getNewColor(color, constLight));
            buffer[ind] = addColors(getNewColor(color, Id), getNewColor(color, constLight));
        }
    }
}

void rasterize(Vec3i t0, Vec3i t1, Vec3i t2, COLORREF color) {
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

        if (A.x > B.x) {std::swap(A, B);}
        Vec3f C = B - A;

        for (int j = 0; j <= C.x; j++) {
            float phi = j == C.x ? 1.f : (float)j / C.x;
            Vec3i P = Vec3f(A) + C * phi;

            if (P.x < 0 || P.y < 0 || P.x >= width || P.y >= height) {
                continue;
            }
            int idx = P.x + P.y * width;
            if (zbuffer[idx] < P.z) {
                zbuffer[idx] = P.z;
                buffer[idx] = color;
            }
        }
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
const COLORREF rgbWhite = 0x00FFFFFF;
const COLORREF rgbBlack = 0x00000000;

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
                    buffer[ind] = 255;
                    zbuffer[ind] = -100000;
                    zbuffer_f[ind] = -100000.f;
                }
            }

            camera.updateViewMatrix();
            finalMatrix = Viewport * Projection * ModelView;
            modelViewProjection = Projection * ModelView;
            size_t size = faces.size();
            if (chained) {
                const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                    [](const size_t i)
                    {
                        Vec4f screen_coords[3];
                        Face face = faces[i];
                        if ((face.normal * camera.getFront()) + 1.0f < 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            screen_coords[j] = finalMatrix * vertexes[face.points[j].vertexInd];
                            screen_coords[j] = screen_coords[j] / screen_coords[j].w;
                        }
                        for (int j = 0; j < 3; j++) {
                            Vec3i a1 = screen_coords[j];
                            Vec3i a2 = screen_coords[(j + 1) % 3];
                            drawLine(a1[0], a1[1], a2[0], a2[1], rgbBlack);
                        }
                    },8 );
                loop_future.wait();
            }
            if (lightMode == 1) {
                const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                    [](const size_t i)
                    {
                        Vec4f screen_coords[3];
                        Face face = faces[i];
                        if ((face.normal * camera.getFront()) + 1.0f < 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            screen_coords[j] = finalMatrix * vertexes[face.points[j].vertexInd];
                            screen_coords[j] = screen_coords[j] / screen_coords[j].w;
                            screen_coords[j][2] = (1.0f - screen_coords[j][2]) * 10000.f;
                        }
                        rasterize(screen_coords[0], screen_coords[1], screen_coords[2], rgbWhite);

                    }, 8);
                loop_future.wait();
            }
            if (lightMode == 2) {
                const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                    [](const size_t i)
                    {
                        Vec4f screen_coords[3];
                        Face face = faces[i];
                        if ((face.normal * camera.getFront()) + 1.0f < 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            screen_coords[j] = finalMatrix * vertexes[face.points[j].vertexInd];
                            screen_coords[j] = screen_coords[j] / screen_coords[j].w;
                            screen_coords[j][2] = (1.0f - screen_coords[j][2]) * 10000.f;
                        }
                        float colorMultiplier = (face.normal * (light * -1)) + constLight;
                        if (colorMultiplier > 1.0f) colorMultiplier = 1.0f;
                        if (colorMultiplier < constLight) colorMultiplier = constLight;
                        COLORREF color = RGB(GetRValue(rgbWhite) * colorMultiplier, GetGValue(rgbWhite) * colorMultiplier, GetBValue(rgbWhite) * colorMultiplier);
                        rasterize(screen_coords[0], screen_coords[1], screen_coords[2], color);
                    }, 8);
                loop_future.wait();
            }
            if (lightMode == 3) {
                const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                    [](const size_t i)
                    {
                        /*
                        Vec4f global_coords[3];
                        Vec4f not_norm_screen_coords[3];
                        Vec4f screen_coords[3];
                        Vec3f normals[3];
                        Face face = faces[i];
                        if ((face.normal * camera.getFront()) + 1.0f < 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            global_coords[j] = vertexes[face.points[j].vertexInd];
                            not_norm_screen_coords[j] = finalMatrix * global_coords[j];
                            screen_coords[j] = not_norm_screen_coords[j] / not_norm_screen_coords[j].w;
                            screen_coords[j][2] = (1.0f - screen_coords[j][2]) * 10000.f;
                            normals[j] = face.points[j].normal;
                        }
                        rasterize(global_coords, not_norm_screen_coords, screen_coords[0], screen_coords[1], screen_coords[2], normals[0], normals[1], normals[2], rgbWhite);
                        */

                        mat<4, 3, float> global_coords;
                        mat<3,3,float> normals;
                        Face face = faces[i];
                        mat<4,3,float> res;
                        if ((face.normal * camera.getFront()) + 1.0f < 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            global_coords.set_col(j, vertexes[face.points[j].vertexInd]);
                            res.set_col(j, modelViewProjection * vertexes[face.points[j].vertexInd]);
                            normals.set_col(j,face.points[j].normal);
                        }
                        rasterize(res, normals, global_coords, rgbWhite);
                    }, 8);
                loop_future.wait();
            }
            hr = bitmap->CopyFromMemory(&winSize, buffer, width * 4);
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
    case WM_MOUSEWHEEL:
    {
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (camera.updateRadius(zDelta / 2400.0f)) rerender = true;
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