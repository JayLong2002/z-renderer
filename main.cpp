#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "algorithm"
#include "iostream"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);

Model *model = NULL;
const int width  = 500;
const int height = 500;
const int depth  = 255;

Vec3f light_dir(0,0,-1);
Vec3f camera(0,0,3);


void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
	bool steep = false; //标记当前斜率的绝对值是否大于1
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		//斜率绝对值>1了，此时将线段端点各自的x,y坐标对调。
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1) {  //x0>x1时，对线段端点坐标进行对调
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	for (int x = x0; x <= x1; x++) {
		float t = (x - x0) / (float)(x1 - x0);
		int y = y0 * (1. - t) + y1 * t;
		if (steep) {
			//如果线段是斜率大于1的，那么线段上的点原本坐标应该是(y,x)
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
	}
}

void line(Vec2i t0,Vec2i t1, TGAImage& image, TGAColor color){
	line(t0.x,t0.y,t1.x,t1.y,image,color);
}

// compute the barycentric for one point x in triangle t0,t1,t2
Vec3f barycentric(Vec2i p ,Vec3f *triangle){
	int x = p.x,y = p.y;
	int x_a = triangle[0].x, y_a = triangle[0].y;
	int x_b = triangle[1].x, y_b = triangle[1].y;
	int x_c = triangle[2].x, y_c = triangle[2].y;
	
	float x_1 = 1.f *(-(x-x_b)*(y_c-y_b) + (y-y_b)*(x_c-x_b))
			  /(-(x_a-x_b)*(y_c-y_b) + (y_a-y_b)*(x_c-x_b)); 

	float x_2 = 1.f *(-(x-x_c)*(y_a-y_c) + (y-y_c)*(x_a-x_c))
			  /(-(x_b-x_c)*(y_a-y_c) + (y_b-y_c)*(x_a-x_c)); 	

	return Vec3f(x_1,x_2,1.f - x_1 - x_2);
}

// if one braycentric has negtive num , then not inside
bool inside(Vec3f bc){
	float x = bc[0];
	float y = bc[1];
	float z = bc[2];
	return x >= 0.f && y >= 0.f && z >= 0.f ;
}

//  how to draw triangle 
//  法1：重心坐标 ，法2：cross product

// zbuffer
void triangle(Vec3f *pts, const std::unique_ptr<martix>& zbuffer, TGAImage &image, TGAColor color){
	
	Vec3f t0 = pts[0];
	Vec3f t1 = pts[1];
	Vec3f t2 = pts[2];

	//finding binding box
	float minx = std::min({pts[0].x, pts[1].x, pts[2].x });
    float maxx = std::max({pts[0].x, pts[1].x, pts[2].x });
    float miny = std::min({pts[0].y, pts[1].y, pts[2].y });
    float maxy = std::max({pts[0].y, pts[1].y, pts[2].y });

    int min_x = (int)std::floor(minx);
    int max_x = (int)std::ceil(maxx);
    int min_y = (int)std::floor(miny);
    int max_y = (int)std::ceil(maxy);

	// shading
	Vec3f trangle[3] = {t0,t1,t2};
	
	for(int x = min_x ; x < max_x ; x++){
		for(int y = min_y; y < max_y; y++){
			Vec3f bc =barycentric(Vec2i{x,y},trangle);
			if(inside(bc)){
				float zvalue = 0.f;
				//对z值进行插值,compute this points z-value
				for (int i=0; i<3; i++) zvalue += pts[i].z*bc[i];
				if((*zbuffer)[x][y] < zvalue){
					(*zbuffer)[x][y] = zvalue;
					image.set(x,y,color);
				}
			}
		}
	}
}

TGAColor getTextureColor(TGAImage &texture, float u, float v) {
    // 纹理坐标限制在(0, 1)
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));
    // 将u, v坐标乘以纹理的宽度和高度，以获取纹理中的像素位置
    int x = u * texture.get_width();
    int y = v * texture.get_height();
    // 从纹理中获取颜色
    TGAColor color = texture.get(x, y);
    // tga使用的是BGRA通道
    return TGAColor(color[2],color[1],color[0], 255);
}


// 使用texture
void triangle(Vec3f *pts,Vec2f *textures,const std::unique_ptr<martix>& zbuffer,TGAImage &image,TGAImage &texture)
{
	Vec3f t0 = pts[0];
	Vec3f t1 = pts[1];
	Vec3f t2 = pts[2];

	Vec2f v0 = textures[0];
	Vec2f v1 = textures[1];
	Vec2f v2 = textures[2];

	Vec3f trangle[3] = {t0,t1,t2};
	// binding box
    int min_x = (int)std::floor(std::min({pts[0].x, pts[1].x, pts[2].x }));
    int max_x = (int)std::ceil(std::max({pts[0].x, pts[1].x, pts[2].x }));
    int min_y = (int)std::floor(std::min({pts[0].y, pts[1].y, pts[2].y }));
    int max_y = (int)std::ceil(std::max({pts[0].y, pts[1].y, pts[2].y }));

	for(int x = min_x ; x < max_x ; x++){
		for(int y = min_y; y < max_y; y++){
			Vec3f bc =barycentric(Vec2i{x,y},trangle);
			if(inside(bc)){
				float zvalue = 0.f;
				//对z值进行插值,compute this points z-value
				
				for (int i=0; i<3; i++) zvalue += pts[i].z*bc[i];
				Vec2f uv = bc.x*v0+bc.y*v1+bc.z*v2;
				// get from texture,why 1-uv.y?
				TGAColor color =getTextureColor(texture,uv.x,1-uv.y) ;
				if((*zbuffer)[x][y] < zvalue){
					(*zbuffer)[x][y] = zvalue;
					image.set(x,y,color);
				}
			}
		}
	}

}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

// impl z-buffer
int main(int argc, char** argv) {
	TGAImage image(width, height, TGAImage::RGB);
	model = new Model("./obj/african_head/african_head.obj");
	TGAImage texture;
	if(texture.read_tga_file("./obj/african_head/african_head_diffuse.tga")){
        std::cout << "Image successfully loaded!" << std::endl;
        // 可以做一些图像处理
    } else {
        std::cerr << "Error loading the image." << std::endl;
    }
	auto z_buffer = zbuffer(height,width).z_buffer;
	for (int i=0; i<model->nfaces(); i++) {
        Face face = model->face(i);
        Vec3f screen_coords[3], world_coords[3];
        Vec2f tex_coords[3];
        for (int j=0; j<3; j++) {
            world_coords[j]  = model->vert(face.vertexIndices[j]);
            screen_coords[j] = world2screen(world_coords[j]);
            tex_coords[j] = model->getTexCoord(face.texcoordIndices[j]);
        }
        triangle(screen_coords,tex_coords,z_buffer,image,texture);
		//triangle(screen_coords,z_buffer,image,white);
    }
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
	delete model;   
    return 0;
}
