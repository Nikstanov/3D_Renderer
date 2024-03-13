// Lab1.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "Lab1.h"
#include "geometry.h"
#include "engine.h"
#include "camera.h"
#include "tgaimage.h"

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
//std::string filePath = "C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\DoomCombatScene.obj";
//std::string filePath = "C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\Models\\Mimic Chest\\Mimic.obj";
//std::string filePath = "C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\Models\\Box\\Box.obj";
//std::string filePath = "C:\\Nikstanov\\Study\\6sem\\KG\\TestObjects\\Models\\Lanterne\\Lanterne.obj";

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
std::vector<Vec3f> texturesCord;

TGAImage diffusemap_;
TGAImage normalmap_;
TGAImage specularmap_;
TGAImage lightmap_;

void load_texture(std::string filename, const char* suffix, TGAImage& img) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix);
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
}

Vec3f diffuse(Vec2f uvf) {
    Vec2i uv(uvf[0] * diffusemap_.get_width(), uvf[1] * diffusemap_.get_height());
    TGAColor color = diffusemap_.get(uv[0], uv[1]);
    return Vec3f(color[0] / 255.f, color[1] / 255.f, color[2] / 255.f);
}

Vec3f light_map(Vec2f uvf) {
    Vec2i uv(uvf[0] * lightmap_.get_width(), uvf[1] * lightmap_.get_height());
    TGAColor color = lightmap_.get(uv[0], uv[1]);
    return Vec3f(color[0] / 255.f, color[1] / 255.f, color[2] / 255.f);
}

Vec3f normal(Vec2f uvf) {
    Vec2i uv(uvf[0] * normalmap_.get_width(), uvf[1] * normalmap_.get_height());
    TGAColor c = normalmap_.get(uv[0], uv[1]);
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f;
    return res;
}

float specular(Vec2f uvf) {
    Vec2i uv(uvf[0] * specularmap_.get_width(), uvf[1] * specularmap_.get_height());
    return specularmap_.get(uv[0], uv[1])[0] / 1.f;
}

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
float constLight = .1f;

Camera camera{10.0f,  3.141557f / 2 ,  3.141557f / 2 , Vec4f(0.0f , 0.0f , 0.0f, 1)};
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

    HRESULT             CreateGraphicsResources(float scale);
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
            if (line.substr(0, 3) == "vt ") {
                Vec3f texture;
                sscanf_s(line.c_str(), "vt %f %f %f", &texture.x, &texture.y, &texture.z);
                texturesCord.push_back(texture);
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
                    p.textureInd = p.textureInd - 1;
                    if (normalInd > 0) p.normal = normalies[normalInd - 1];
                    face.points[ind] = p;
                    ind++;
                }
                faces.push_back(face);
            }
        }
        in.close();
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

    load_texture(filePath, "_diffuse.tga", diffusemap_);
    load_texture(filePath, "_nm.tga", normalmap_);
    load_texture(filePath, "_spec.tga", specularmap_);
    load_texture(filePath, "_emi.tga", lightmap_);

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

//float weight[5]{ 0.25508352f, 0.11098164f, 0.01330373f, 0.0038771 };
float weight[5]{ 0.259027, 0.2195946, 0.1376216, 0.069054, 0.030216 };

const int downscaling_iterations_amount_max = 8;

HRESULT MainWindow::CreateGraphicsResources(float scale)
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        static const WCHAR msc_fontName[] = L"Verdana";
        FLOAT msc_fontSize = 40.0f * scale;
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

        D2D1_SIZE_U size = D2D1::SizeU(rc.right * scale, rc.bottom * scale);

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

            bloom_buffer = new Vec3f*[downscaling_iterations_amount_max*2];
            for (int i = 0; i < downscaling_iterations_amount_max * 2; i++) {
                bloom_buffer[i] = new Vec3f[width * height];
            }
            
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
    SafeRelease(&bitmap);

    delete[] buffer;
    delete[] bufferOut;
    delete[] zbuffer;
    delete[] sync_buffer;

    for (int i = 0; i < 10; i++) {
        delete[] bloom_buffer[i];
    }
    delete[] bloom_buffer;
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

            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

            Vec2f textureCoords{ textures[0] * bc_clip, textures[1] * bc_clip };
            Vec3f new_color = diffuse(textureCoords);

            Vec3f Norm{ vn[0] * bc_clip, vn[1] * bc_clip , vn[2] * bc_clip };
            Norm = Norm.normalize();
            Norm = normal(textureCoords);
                        
            Vec3f globalCoords{ global[0] * bc_clip, global[1] * bc_clip , global[2] * bc_clip };
            Vec3f negLight = (lightPos - globalCoords).normalize();

            float diff = 1.0f * (Norm * negLight);
            if (diff < 0) diff = 0.0f;

            Vec3f viewDir = ((Vec3f)camera.getEye() - globalCoords).normalize();
            Vec3f R = (Norm * (Norm * negLight) - negLight).normalize();
            float spec = 10.0f * pow(max((viewDir * R), 0.0f), specular(textureCoords));
            //float spec = 1.0f * pow(max((viewDir * R), 0.0f), 64.0f);
            if (spec < 0) spec = 0.0f;

            for (int i = 0; i < 3; i++) {
                new_color[i] = 0.1f + new_color[i] * diff + specColor[i] * spec;
            }

            Vec3f light_color = light_map(textureCoords);
            //Vec3f light_color = Vec3f(0, 0, 0);

            int ind = P.x + P.y * width;
            EnterCriticalSection(&sync_buffer[ind]);
            if (zbuffer[ind] <= frag_depth) {
                zbuffer[ind] = frag_depth;
                
                if (light_color[0] > 0.0f || light_color[1] > 0.0f || light_color[2] > 0.0f) {
                    bloom_buffer[0][ind] = light_color * 20.0f;
                    new_color = new_color + light_color;
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
            Vec3f P = Vec3f(A) + C * phi;
            Vec3i _P = P;
            if (_P.x < 0 || _P.y < 0 || _P.x >= width || _P.y >= height) {
                continue;
            }
            int idx = _P.x + _P.y * width;

            EnterCriticalSection(&sync_buffer[idx]);
            if (zbuffer[idx] < P.z) {
                zbuffer[idx] = P.z;
                buffer[idx] = color;
            }
            LeaveCriticalSection(&sync_buffer[idx]);
        }
    }
}


Vec3f texture(Vec3f* buf, Vec2i coords, Vec2i scale) {
    if (coords.x < 0 || coords.y < 0 || coords.x >= scale.x || coords.y >= scale.y) {
        return Vec3i(0, 0, 0);
    }
    return buf[coords.x + coords.y * width];
}

void downsampling4x(Vec3f* buf, Vec3f* bufOut, int buf_width, int buf_height) {

    for (int y = 0; y < buf_height; y++) {
        for (int x = 0; x < buf_width; x++) {
            int newInd = x + y * width;
            int ind = newInd * 2;
            bufOut[newInd] = (buf[ind] + buf[ind + 1] + buf[ind + width] + buf[ind + 1 + width]) / 4;
            newInd++;
        }
    }
}

void guassian(Vec3f* buf, Vec3f* bufOut, Vec2i TexCoords, bool horizontal, Vec2i new_scale) {
    Vec3f result = texture(buf, TexCoords, new_scale) * weight[0];
    int tempY = TexCoords.y * width;

    if (horizontal)
    {
        for (int i = 1; i < 5; ++i)
        {
            if (TexCoords.x + i < width) {
                result = result + buf[TexCoords.x + i + tempY] * weight[i];
            }
            if (TexCoords.x - i >= 0) {
                result = result + buf[TexCoords.x - i + tempY] * weight[i];
            }
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
            if (TexCoords.y + i < height) {
                result = result + buf[TexCoords.x + (TexCoords.y + i) * width] * weight[i];
            }
            if (TexCoords.y - i >= 0) {
                result = result + buf[TexCoords.x + (TexCoords.y - i) * width] * weight[i];
            }
        }
    }
    bufOut[TexCoords.x + tempY] = result;
}

bool horizontal = true, first_iteration = true;

void upsampling4x(Vec3f* buf, Vec3f* bufOut, int buf_width, int buf_height) {

    for (unsigned int ind = 0; ind < width * height; ind++) {
        bufOut[ind] = Vec3f(0.0f, 0.0f, 0.0f);
    }

    for (unsigned int x = 0; x < buf_width; x++) {
        for (unsigned int y = 0; y < buf_height; y++) {
            int ind = x + y * width;
            bufOut[ind * 2] = buf[ind];
        }
    }
        
    int amount = 2;

    Vec2i new_scale(buf_width * 2, buf_height * 2);

    for (unsigned int j = 0; j < amount; j++)
    {
        const BS::multi_future<void> loop_future = pool.submit_blocks<size_t>(0, new_scale.x,
            [new_scale, bufOut, buf](const unsigned int start, const unsigned int end)
            {
                for (unsigned int x = start; x < end; x++) {
                    for (int y = 0; y < new_scale.y; y++) {
                        guassian(first_iteration ? bufOut : (horizontal) ? bufOut : buf, (horizontal) ? buf : bufOut, Vec2i(x, y), horizontal, new_scale);
                    }
                }
            });
        loop_future.wait();
        
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
}

void addBuffers(Vec3f* bufA, Vec3f* bufB, int buf_width, int buf_height) {
    for (unsigned int x = 0; x < buf_width; x++) {
        for (unsigned int y = 0; y < buf_height; y++) {
            int ind = x + y * width;
            bufA[ind] = bufA[ind] + bufB[ind];
        }
    }
}

void bloom() {

    int downscaling_iterations_amount = 0;
    Vec2i new_scale(width, height);
    for (unsigned int i = 0; i < downscaling_iterations_amount_max; i++) {

        if (new_scale.x % 2 == 1 || new_scale.y % 2 == 1) break;
        downscaling_iterations_amount++;

        new_scale.x /= 2;
        new_scale.y /= 2;
        if (i == 0) {
            downsampling4x(bloom_buffer[0], bloom_buffer[0], new_scale.x, new_scale.y);
        }
        else {
            downsampling4x(bloom_buffer[2*(i - 1)], bloom_buffer[2*i], new_scale.x, new_scale.y);
        }

        bool horizontal = true, first_iteration = true;
        int amount = 4;

        for (unsigned int j = 0; j < amount; j++)
        {
            const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, new_scale.x,
                [first_iteration, horizontal, new_scale, i](const size_t x)
                {
                    for (int y = 0; y < new_scale.y; y++) {
                        guassian(first_iteration ? bloom_buffer[2*i] : bloom_buffer[2*i + !horizontal], bloom_buffer[2 * i + horizontal], Vec2i(x, y), horizontal, new_scale);
                    }
                });
            loop_future.wait();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }


    }
    for (unsigned int i = downscaling_iterations_amount - 1; i > 0; i--) {
        upsampling4x(bloom_buffer[i * 2], bloom_buffer[i * 2 - 1], new_scale.x, new_scale.y);
        new_scale.x *= 2;
        new_scale.y *= 2;
        addBuffers(bloom_buffer[(i - 1) * 2], bloom_buffer[i * 2 - 1], new_scale.x, new_scale.y);
    }

    upsampling4x(bloom_buffer[0], bloom_buffer[1], new_scale.x, new_scale.y);
}

double clockToMilliseconds(clock_t ticks) {
    return (ticks / (double)CLOCKS_PER_SEC) * 1000.0;
}

clock_t deltaTime = 0;
unsigned int frames = 0;
double  frameRate = 60;
double framerate = 0;
double  averageFrameTimeMilliseconds = 33.333;
bool rerender = true;
int scaleMode = 2;
float scaleSizes[]{ 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };


bool chained = true;
int lightMode = 0;

Matrix finalMatrix;
Matrix modelViewProjection;


void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources(scaleSizes[scaleMode]);
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
                    buffer[ind] = Vec3f(0.0f,0.0f,0.0f);
                    zbuffer[ind] = -100000.0f;
                    bufferOut[ind] = rgbWhite;
                    for (size_t k = 0; k < downscaling_iterations_amount_max * 2; k++) {
                        bloom_buffer[k][ind] = Vec3f(0.0f, 0.0f, 0.0f);
                    }
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
                        if ((Vec4f(face.normal) * (camera.getEye() - vertexes[face.points[0].vertexInd])) > 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            screen_coords[j] = finalMatrix * vertexes[face.points[j].vertexInd];
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
                        Face face = faces[i];
                        if ((Vec4f(face.normal) * (camera.getEye() - vertexes[face.points[0].vertexInd])) > 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            screen_coords[j] = finalMatrix * vertexes[face.points[j].vertexInd];
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
                        Face face = faces[i];
                        if ((Vec4f(face.normal) * (camera.getEye() - vertexes[face.points[0].vertexInd])) > 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            screen_coords[j] = finalMatrix * vertexes[face.points[j].vertexInd];
                            if (screen_coords[j].w < 0) {
                                return;
                            }
                            screen_coords[j] = screen_coords[j] / screen_coords[j].w;
                            screen_coords[j][2] = (1.0f - screen_coords[j][2]) * 10000.f;
                        }

                        float colorMultiplier = (face.normal *  (light * -1)) + constLight;
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
                        mat<3,3,float> normals;
                        mat<3, 3, float> textures;
                        Face face = faces[i];
                        mat<4,3,float> res;
                        if ((Vec4f(face.normal) * (camera.getEye() - vertexes[face.points[0].vertexInd])) > 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            global_coords.set_col(j, vertexes[face.points[j].vertexInd]);
                            textures.set_col(j, texturesCord[face.points[j].textureInd]);
                            res.set_col(j, modelViewProjection * vertexes[face.points[j].vertexInd]);
                            normals.set_col(j,face.points[j].normal);
                        }
                        rasterize(res, normals, global_coords, textures, Vec3f(1.0f, 1.0f, 1.0f) - gold, white);
                    });
                loop_future.wait();
            }
            if (lightMode == 4) {
                const BS::multi_future<void> loop_future = pool.submit_loop<size_t>(0, size,
                    [](const size_t i)
                    {
                        mat<3, 3, float> global_coords;
                        mat<3, 3, float> normals;
                        mat<3, 3, float> textures;
                        Face face = faces[i];
                        mat<4, 3, float> res;
                        if ((Vec4f(face.normal) * (camera.getEye() - vertexes[face.points[0].vertexInd])) > 0) {
                            return;
                        }
                        for (size_t j = 0; j < 3; j++) {
                            global_coords.set_col(j, vertexes[face.points[j].vertexInd]);
                            textures.set_col(j, texturesCord[face.points[j].textureInd]);
                            res.set_col(j, modelViewProjection * vertexes[face.points[j].vertexInd]);
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
                    const float gamma = 1.0f / 2.2f;
                    const float exposure = 1.0f;
                    for (int y = 0; y < height; y++) {

                        int ind = x + y * width;
                        
                        Vec4f rgb = buffer[ind] + bloom_buffer[1][ind];

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

            hr = bitmap->CopyFromMemory(&winSize, bufferOut, width * 4);
            rerender = false;
        }

        pRenderTarget->DrawBitmap(bitmap, &winSizeF, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &winSizeF);

        std::wstring fps =  std::to_wstring(int(frameRate + 0.5)) + std::wstring(L",") + std::to_wstring(int(framerate + 0.5));
        D2D1_RECT_F layoutRect = D2D1::RectF(0, 0, 150 * scaleSizes[scaleMode], 40 * scaleSizes[scaleMode]);
        pRenderTarget->DrawTextW(fps.c_str(), fps.size(), pTextFormat_, layoutRect, pBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }

        EndPaint(m_hwnd, &ps);

        clock_t endFrame = clock();
        deltaTime += endFrame - beginFrame;
        framerate = endFrame - beginFrame;

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
            lightMode = (lightMode + 1) % 5;
        }
        if (wParam == '3') {
            DiscardGraphicsResources();
            scaleMode = (scaleMode + 1) % 5;
            
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