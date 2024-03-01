#ifndef __OUR_GL_H__
#define __OUR_GL_H__

#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

// MSAA ,sample ssnum * ssnum each pixel
const int ssnum = 2;

void viewport(int x, int y, int w, int h);
void projection(float coeff=0.f); // coeff = -1/c
void lookat(Vec3f eye, Vec3f center, Vec3f up);
void lookat(Vec3f gaze,Vec3f position,Vec3f up_direction);

struct IShader {
    virtual ~IShader();
    /**
     * @brief handle the points in some way user-define,and save the 
    */
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer);

#endif //__OUR_GL_H__