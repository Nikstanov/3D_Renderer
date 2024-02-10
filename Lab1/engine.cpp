#include "engine.h"

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

void viewport(int width, int height)
{
    Viewport = Matrix::identity();
    Viewport[0][0] = width / 2.0f;
    Viewport[1][1] = -height / 2.0f;
    Viewport[0][3] = width / 2.0f;
    Viewport[1][3] = height / 2.0f;
}

void projection(float aspect, float FOV, float zfar, float znear)
{
    Projection = Matrix::identity();
    Projection[0][0] = 1.0f / (aspect * tan(FOV / 2.0f * 0.0174533f));
    Projection[1][1] = 1.0f / tan(FOV / 2.0f * 0.0174533f);
    Projection[2][2] = zfar / (znear - zfar);
    Projection[2][3] = zfar * znear / (znear - zfar);
    Projection[3][2] = -1.0f;
    Projection[3][3] = 0.0f;
}

void view(Vec3f eye, Vec3f target, Vec3f up)
{
    Vec3f z = (eye - target).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    ModelView = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        //ModelView[i][3] = -front[i];
    }
    ModelView[0][3] = -(x * eye);
    ModelView[1][3] = -(y * eye);
    ModelView[2][3] = -(z * eye);
}
