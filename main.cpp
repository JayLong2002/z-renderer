#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model *model     = NULL;
const int width  = 800;
const int height = 800;

Vec3f light_dir(1,1,1);
// carmer postion
Vec3f       eye(0.5,1,2);
Vec3f    center(0,0,0);
Vec3f        up(0,1,0);

struct GouraudShader : public IShader {
    // 存储每个顶点的光照强度
    Vec3f varying_intensity; 

    virtual Vec4f vertex(int iface, int nthvert) {
        // iface 第i个三角形，nthvert 第j个顶点
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = Viewport*Projection*ModelView*gl_Vertex;     // transform it to screen coordinates
        auto normal_vec = model->normal(iface, nthvert);
        varying_intensity[nthvert] = std::max(0.f, normal_vec*light_dir); // get diffuse lighting intensity
        return gl_Vertex;
    }

    /**
     * @brief change the color
     * @param bar: 重心坐标
     * @param color : the point's color
     * 
    */
    virtual bool fragment(Vec3f bar, TGAColor &color) {
		float intensity = varying_intensity * bar;
        color = TGAColor(
				255 * intensity,
				(int)(160 * std::sqrt(intensity)),
				0
		);
		return false;
	}
};

inline void init(){
    lookat(eye, center, up);
    viewport(width/8, height/8, width*3/4, height*3/4);
    projection(-1.f/(eye-center).norm());
    light_dir.normalize();
}



struct Shader : public IShader {
    Vec3f          varying_intensity; // written by vertex shader, read by fragment shader
    mat<2,3,float> varying_uv;        // same as above    
    
    Vec4f vertex(int iface, int nthvert) override {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert)*light_dir); // get diffuse lighting intensity
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        return Viewport*Projection*ModelView*gl_Vertex; // transform it to screen coordinates
    }

    bool fragment(Vec3f bar, TGAColor &color) override {
        float intensity = varying_intensity*bar;   // interpolate intensity for the current pixel
        Vec2f uv = varying_uv * bar;                 // interpolate uv for the current pixel
        color = model->diffuse(uv)*intensity;      // well duh
        return false;                              // no, we do not discard this pixel
    }
};

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head/african_head.obj");
    }
    
    init();
    
    TGAImage image  (width, height, TGAImage::RGB);

    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    GouraudShader shader;
    // i : 三角形，j：坐标
    for (int i=0; i<model->nfaces(); i++) {
        Vec4f screen_coords[3];
        for (int j=0; j<3; j++) {
            // how the vertex going?
            screen_coords[j] = shader.vertex(i, j);
        }
        triangle(screen_coords, shader, image, zbuffer);
    }

    image.  flip_vertically(); // to place the origin in the bottom left corner of the image
    zbuffer.flip_vertically();
    image.  write_tga_file("output.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    delete model;
    return 0;
}