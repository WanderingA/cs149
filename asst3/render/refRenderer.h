#ifndef __REF_RENDERER_H__
#define __REF_RENDERER_H__

#include "circleRenderer.h"


class RefRenderer : public CircleRenderer {

private:

    Image* image;
    SceneName sceneName;

    int numCircles;
    float* position;
    float* velocity;
    float* color;
    float* radius;

public:

    RefRenderer();
    virtual ~RefRenderer();
    // return the image
    const Image* getImage();
    // nothing to do here
    void setup();
    // loads the scene into the renderer
    void loadScene(SceneName name);
    // allocate buffer the renderer will render into
    void allocOutputImage(int width, int height);
    // clearImage -- clear the renderer's target image
    void clearImage();
    // 将模拟时间提前一步，更新所有圆的位置和速度
    void advanceAnimation();
    // render all circles
    void render();
    // dump particles to a file
    void dumpParticles(const char* filename);
    // 计算每个像素的颜色和不透明度
    void shadePixel(
        int circleIndex,
        float pixelCenterX, float pixelCenterY,
        float px, float py, float pz,
        float* pixelData);
};


#endif
