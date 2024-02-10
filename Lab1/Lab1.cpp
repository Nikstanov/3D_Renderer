// Lab1.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "Lab1.h"
#include "geometry.h"
#include "engine.h"
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
std::string filePath = "C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\teapot.obj";

struct Point {
    Vec4f* vertex = 0;
    int textureInd = -1;
    Vec3f normal{ -1.1111f,-1,-1 };
};
struct Face {
    Point points[3];
    Vec3f normal;
};

Vec3f calculateNormal(Face face) {
    return cross((Vec3f)(*face.points[2].vertex - *face.points[0].vertex), (Vec3f)(*face.points[1].vertex - *face.points[0].vertex)).normalize();
}

std::vector<Face> faces;
std::vector<Vec4f> vertexes;
std::vector<Vec3f> normalies;

int width = 1920;
int height = 1080;
float aspect = width / height;
float FOV = 90.0f;
float zfar = 100.0f;
float znear = 0.1f;
float lastX = 400, lastY = 300;
float constLight = .2f;

Vec3f eye{ 0.0f, 2.f, 2.0f };
Vec3f up{ 0.0f , 1.0f , 0.0f };
Vec3f target{ 0.0f , 1.0f , 0.0f };
Vec3f front;
//Vec3f front{ 0.0f , 0.0f , -1.0f };

float cameraR = 5.0f;
float cameraZeta = 3.141557f/2;
float cameraPhi = 3.141557f / 2;
float minScaleSize = 0.0001f;
float maxScaleSize = cameraR * 3.0f;

Vec3f light{ 1.0f, 0.0f, 1.0f};
Vec3f lightView;

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
    DWORD* buffer;
    int* zbuffer;
    int maxInd;
    D2D1_RECT_U winSize;
    D2D1_RECT_F winSizeF;

    float                  fontSize = 10.0f;

    HRESULT             CreateGraphicsResources();
    void                DiscardGraphicsResources();
    void                OnPaint();
    void                rasterize(Vec3i t0, Vec3i t1, Vec3i t2, Vec3f vn0, Vec3f vn1, Vec3f vn2, COLORREF color);
    void                rasterize(Vec3i t0, Vec3i t1, Vec3i t2, COLORREF color);
    //Vec2f*              rasterization(Vec4f* verts, COLORREF color);
    inline void         setPixel(int x, int y, COLORREF color);
    void                drawLine(int x1, int y1, int x2, int y2, COLORREF color);

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
                    int vertexInd = -1;
                    int normalInd = -1;
                    sscanf_s(token.c_str(), "%i/%i/%i", &vertexInd, &p.textureInd, &normalInd);
                    p.vertex = &vertexes[vertexInd - 1];
                    if (normalInd > 0) p.normal = normalies[normalInd - 1];
                    face.points[ind] = p;
                    ind++;
                }
                faces.push_back(face);
            }
        }
        in.close();
        vertexes.clear();
    }

    
    std::map<Vec4f*, std::vector<Face*>> map;
    for (size_t i = 0; i < faces.size(); i++) {
        faces[i].normal = calculateNormal(faces[i]);
        for (size_t j = 0; j < 3; j++) {
            map[faces[i].points[j].vertex].push_back(&faces[i]);
        }
    }
    

    for (size_t i = 0; i < faces.size(); i++) {
        for (size_t j = 0; j < 3; j++) {
            if (faces[i].points[j].normal.x == -1.1111f) {
                std::vector<Face*> a = map[faces[i].points[j].vertex];
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

            maxInd = width * height;

            winSize.right = width;
            winSize.bottom = height;

            winSizeF.right = width;
            winSizeF.bottom = height;
        }
        viewport(width, height);
        projection(aspect, FOV, zfar, znear);
        view(eye, target, up);
    }

    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void MainWindow::drawLine(int x0, int y0, int x1, int y1, COLORREF color) {
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

void inline MainWindow::setPixel(int x, int y, COLORREF color) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    buffer[y * width + x] = color;
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

void MainWindow::rasterize(Vec3i t0, Vec3i t1, Vec3i t2, Vec3f vn0, Vec3f vn1, Vec3f vn2, COLORREF color){
    if (t0.y == t1.y && t0.y == t2.y) return; 
    if (t0.y > t1.y) {
        std::swap(t0, t1); std::swap(vn0, vn1);
    }
    if (t0.y > t2.y) {
        std::swap(t0, t2); std::swap(vn0, vn2);
    }
    if (t1.y > t2.y) {
        std::swap(t1, t2); std::swap(vn1, vn2);
    }
    int total_height = t2.y - t0.y;
    
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
        
        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; 
        Vec3i A = (Vec3f)t0 + Vec3f(t2 - t0) * alpha;
        Vec3i B = second_half ? (Vec3f)t1 + Vec3f(t2 - t1) * beta : (Vec3f)t0 + Vec3f(t1 - t0) * beta;

        Vec3f NormA = vn0 + (vn2 - vn0) * alpha;
        Vec3f NormB = second_half ? vn1 + (vn2 - vn1) * beta : vn0 + (vn1 - vn0) * beta;

        if (A.x > B.x) {
            std::swap(A, B);
            std::swap(NormA, NormB);
        }
        Vec3f C = B - A;    
        Vec3f NormC = NormB - NormA;

        for (int j = 0; j <= C.x; j++) {
            float phi = j == C.x ? 1.f : (float)j / C.x;
            Vec3i P = Vec3f(A) + C * phi;
            Vec3f Norm = (NormA + NormC * phi).normalize();

            if (P.x < 0 || P.y < 0 || P.x >= width || P.y >= height) {
                continue;
            }

            
            float Id = 0.5f * (Norm * light);

            Vec3f R = (Norm * (Norm * light * 2.f) - light).normalize();
            float Is = (R * front);

            int idx = P.x + P.y * width;
            if (zbuffer[idx] < P.z) {
                zbuffer[idx] = P.z;
                buffer[idx] = addColors(addColors(getNewColor(color, Is), getNewColor(color, Id)), getNewColor(color, constLight));
                //buffer[idx] = addColors(getNewColor(color, Id), getNewColor(color, constLight));
            }
        }
    }
}


void MainWindow::rasterize(Vec3i t0, Vec3i t1, Vec3i t2, COLORREF color) {
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
                }
            }
            
            eye.x = cameraR * cos(cameraPhi) * sin(cameraZeta);
            eye.z = cameraR * sin(cameraPhi) * sin(cameraZeta);
            eye.y = cameraR * cos(cameraZeta);
            view(eye, target, up);
            front = target - eye;

            Matrix finalMatrix = Viewport * Projection * ModelView;
            Vec4f drawFace[3];
            Vec3f normals[3];
            Face face;
            size_t size = faces.size();
            for (size_t i = 0; i < size; i++) {
                face = faces[i];
    
                if ((face.normal * front) + 1.0f < 0) {
                    continue;
                }
                
                for (size_t j = 0; j < 3; j++) {
                    drawFace[j] = finalMatrix * *face.points[j].vertex;
                    drawFace[j] = drawFace[j] / drawFace[j].w;
                    drawFace[j][2] = (1.0f - drawFace[j][2]) * 10000.f;
                    normals[j] = face.points[j].normal;
                    //normals[j] = ModelView * embed<4>(*face.points[j].normal);
                }
                
                //rasterize(drawFace[0], drawFace[1], drawFace[2], normals[0], normals[1], normals[2], rgbWhite);
                
                // 2 lab
                
                float colorMultiplier = (face.normal * light) + constLight;
                if (colorMultiplier > 1.0f) colorMultiplier = 1.0f;
                COLORREF color = RGB(GetRValue(rgbWhite) * colorMultiplier, GetGValue(rgbWhite) * colorMultiplier, GetBValue(rgbWhite) * colorMultiplier);
                rasterize(drawFace[0], drawFace[1], drawFace[2], rgbWhite);
                
                
                // 1 lab
                /*
                for (int j = 0; j < 3; j++) {
                    Vec3i a1 = drawFace[j];
                    Vec3i a2 = drawFace[(j + 1) % 3];
                    drawLine(a1[0], a1[1], a2[0], a2[1], rgbBlack);
                }
                */
                
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
        float newCameraR = cameraR + zDelta / 2400.0f;
        if (newCameraR > minScaleSize && newCameraR < maxScaleSize) {
            cameraR = newCameraR;
            rerender = true;
        }
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

            cameraPhi = cameraPhi + xoffset;
            cameraZeta = cameraZeta + yoffset;

            rerender = true;
        }
        break;
    }
    case WM_KEYDOWN:
    {
        
        float cameraSpeed = 0.05f;
        if (wParam == VK_LEFT) {
            target.x = target.x - cameraSpeed;
        }
        if (wParam == VK_RIGHT) {
            target.x = target.x + cameraSpeed;
        }
        if (wParam == VK_UP) {
            target.y = target.y + cameraSpeed;
        }
        if (wParam == VK_DOWN) {
            target.y = target.y - cameraSpeed;
        }
        if (wParam == 'A') {
            target.x = target.x - cameraSpeed;
        }
        if (wParam == 'D') {
            target.x = target.x + cameraSpeed;
        }
        if (wParam == 'W') {
            target.y = target.y + cameraSpeed;
        }
        if (wParam == 'S') {
            target.y = target.y - cameraSpeed;
        }
        
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