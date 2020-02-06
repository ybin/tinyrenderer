#include "tgaimage.h"
#include "model.h"

Model *model = nullptr;

const int width = 800;
const int height = 800;
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

Vec2i world2screen(Vec3f v) {
    return Vec2i(int((v.x+1.)*width/2.), int((v.y+1.)*height/2.));
}

int main(int argc, char **argv) {
    TGAImage image(width, height, TGAImage::RGB);
    model = new Model("../obj/african_head/african_head.obj");
    for (int i = 0; i != model->nverts(); i++) {
        Vec3f v = model->vert(i);
        Vec2i p = world2screen(v);
        image.set(p.x, p.y, white);
    }
    delete model;

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("points.tga");
    return 0;
}