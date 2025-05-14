#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <vector>

#include "refRenderer.h"
#include "image.h"
#include "noise.h"
#include "sceneLoader.h"
#include "util.h"


RefRenderer::RefRenderer() {
    image = NULL;

    numCircles = 0;
    position = NULL;
    velocity = NULL;
    color = NULL;
    radius = NULL;
}


RefRenderer::~RefRenderer() {

    if (image) {
        delete image;
    }

    if (position) {
        delete [] position;
        delete [] velocity;
        delete [] color;
        delete [] radius;
    }
}

// 获取当前渲染结果图像
const Image*
RefRenderer::getImage() {
    return image;
}

void
RefRenderer::setup() {
    // 这里无需做任何初始化
}

// 分配输出图像缓冲区，避免内存泄漏
void
RefRenderer::allocOutputImage(int width, int height) {

    if (image)
        delete image;
    image = new Image(width, height);
}

// 清空图像，根据场景决定清空颜色
void
RefRenderer::clearImage() {

    // 如果是雪花场景，使用渐变色清空
    if (sceneName == SNOWFLAKES || sceneName == SNOWFLAKES_SINGLE_FRAME) {

        for (int j=0; j<image->height; j++) {
            float* ptr = image->data + (4 * j * image->width);
            float shade = .4f + .45f * static_cast<float>(image->height-j) / image->height;
            for (int i=0; i<image->width; i++) {
                ptr[0] = ptr[1] = ptr[2] = shade;
                ptr[3] = 1.f;
                ptr += 4;
            }
        }
    } else {
        // 其他场景清空为白色
        image->clear(1.f, 1.f, 1.f, 1.f);
    }
}

// 加载场景，初始化圆的参数
void
RefRenderer::loadScene(SceneName scene) {
    sceneName = scene;
    loadCircleScene(sceneName, numCircles, position, velocity, color, radius);
}

// 推进动画一帧，更新所有圆的位置和速度
void
RefRenderer::advanceAnimation() {

    // 只有雪花场景有动画
    if (sceneName == SNOWFLAKES) {

        const float dt = 1.f / 60.f;
        const float kGravity = -1.8f; // 重力加速度
        const float kDragCoeff = 2.f; // 阻力系数

        for (int i=0; i<numCircles; i++) {

            int index3 = 3 * i;

            // 远处的雪花移动更慢，产生视差效果
            float forceScaling = CLAMP(1.f - position[index3+2], .1f, 1.f);

            // 添加噪声让雪花飘动更自然
            float noiseInput[3];
            float noiseForce[2];
            noiseInput[0] = 10.f * position[index3];
            noiseInput[1] = 10.f * position[index3+1];
            noiseInput[2] = 255.f * position[index3+2];
            vec2CellNoise(noiseInput, noiseForce, i);
            noiseForce[0] *= 7.5f;
            noiseForce[1] *= 5.f;

            // 计算阻力
            float dragForce[3];
            dragForce[0] = -1.f * kDragCoeff * velocity[index3];
            dragForce[1] = -1.f * kDragCoeff * velocity[index3+1];

            // 更新位置
            position[index3]   += velocity[index3] * dt;
            position[index3+1] += velocity[index3+1] * dt;
            position[index3+2] += velocity[index3+2] * dt;

            // 更新速度
            velocity[index3]   += forceScaling * (noiseForce[0] + dragForce[0]) * dt;
            velocity[index3+1] += forceScaling * (kGravity + noiseForce[1] + dragForce[1]) * dt;

            // 雪花飞出屏幕后重置到顶部
            if ( (position[index3+1] + radius[i] < 0.f) ||
                 (position[index3]+radius[i]) < -0.f ||
                 (position[index3]-radius[i]) > 1.f)
            {
                noiseInput[0] = 255.f * position[index3];
                noiseInput[1] = 255.f * position[index3+1];
                noiseInput[2] = 255.f * position[index3+2];
                vec2CellNoise(noiseInput, noiseForce, i);

                position[index3] = .5f + .5f * noiseForce[0];
                position[index3+1] = 1.35f + radius[i];

                // 重新设置速度
                velocity[index3] = 2.f * noiseForce[1];
                velocity[index3+1] = 0.f;
            }
        }
    } else if (sceneName == BOUNCING_BALLS) {
        // 弹跳小球场景
        const float dt = 1.f / 60.f;
        const float kGravity = -2.8f; // 重力加速度
        const float kDragCoeff = -0.8f; // 反弹阻力
        const float epsilon = 0.001f; // 停止阈值

        for (int i=0; i<numCircles; i++) {
            int index3 = 3 * i;

            // 记录旧速度和位置
            float oldVelocity = velocity[index3+1]; 
            float oldPosition = position[index3+1]; 

            if (oldVelocity == 0.f && oldPosition == 0.f) { // 已停止
                continue; 
            }

            if (position[index3+1] < 0 && oldVelocity < 0.f) { // 反弹
                velocity[index3+1] *= kDragCoeff; 
            }

            // 更新速度
            velocity[index3+1] += kGravity * dt; 

            // 更新位置
            position[index3+1] += velocity[index3+1] * dt;

            // 判断是否停止
            if (fabsf(velocity[index3+1] - oldVelocity) < epsilon 
                    && oldPosition < 0.0f 
                    && fabsf(position[index3+1]-oldPosition) < epsilon) { 
                velocity[index3+1] = 0.f; 
                position[index3+1] = 0.f; 
            } 
        }
    } else if (sceneName == HYPNOSIS) { 
        // 催眠圈场景，半径不断增大，超过阈值后重置
        float cutOff = 0.5f;  
        for (int i = 0; i < numCircles; i++) { 
            if (radius[i] > cutOff) { 
                radius[i] = 0.02f; 
            } else { 
                radius[i] += 0.01f; 
            }
        }
    } else if (sceneName == FIREWORKS) {
        // 烟花场景，火花超出距离后重置
        const float dt = 1.f / 60.f;
        const float pi = 3.14159;
        const float maxDist = 0.25f; 

        for (int i = 0; i < NUM_FIREWORKS; i++) { 
            int index3i = 3 * i;
            float cx = position[index3i]; 
            float cy = position[index3i+1]; 
            for (int j = 0; j < NUM_SPARKS; j++) { 
                int sIdx = NUM_FIREWORKS + i * NUM_SPARKS + j;
                int index3j = 3 * sIdx;
                
                // 更新火花位置
                position[index3j] += velocity[index3j] * dt;  
                position[index3j+1] += velocity[index3j+1] * dt; 

                float sx = position[index3j]; 
                float sy = position[index3j+1];

                // 计算火花与中心的距离
                float cxsx = sx - cx; 
                float cysy = sy - cy;
                float dist = sqrt(cxsx * cxsx + cysy * cysy);
                if (dist > maxDist) { // 超出距离重置
                    float angle = (j * 2 * pi)/NUM_SPARKS;
                    float sinA = sin(angle); 
                    float cosA = cos(angle); 
                    float x = cosA * radius[i]; 
                    float y = sinA * radius[i]; 

                    position[index3j] = position[index3i] + x;  
                    position[index3j+1] = position[index3i+1] + y;  
                    position[index3j+2] = 0.0f; 

                    velocity[index3j] = cosA/5.0;  
                    velocity[index3j+1] = sinA/5.0; 
                    velocity[index3j+2] = 0.0f;  
                } 
            }
        } 

    } 
}

// 查表获取颜色，coord为归一化距离
static inline void
lookupColor(float coord, float& r, float& g, float& b) {

    const int N = 5;

    float lookupTable[N][3] = {
        {1.f, 1.f, 1.f},
        {1.f, 1.f, 1.f},
        {.8f, .9f, 1.f},
        {.8f, .9f, 1.f},
        {.8f, 0.8f, 1.f},
    };

    float scaledCoord = coord * (N-1);

    int base = std::min(static_cast<int>(scaledCoord), N-1);

    // 线性插值获取颜色
    float weight = scaledCoord - static_cast<float>(base);
    float oneMinusWeight = 1.f - weight;

    r = (oneMinusWeight * lookupTable[base][0]) + (weight * lookupTable[base+1][0]);
    g = (oneMinusWeight * lookupTable[base][1]) + (weight * lookupTable[base+1][1]);
    b = (oneMinusWeight * lookupTable[base][2]) + (weight * lookupTable[base+1][2]);
}

// shadePixel -- 计算指定圆对像素的贡献
// circleIndex: 圆的索引
// pixelCenterX, pixelCenterY: 像素中心归一化坐标
// px, py, pz: 圆心坐标
// pixelData: 指向像素数据的指针
void
RefRenderer::shadePixel(
    int circleIndex,
    float pixelCenterX, float pixelCenterY,
    float px, float py, float pz,
    float* pixelData)
{
    float diffX = px - pixelCenterX;
    float diffY = py - pixelCenterY;
    float pixelDist = diffX * diffX + diffY * diffY;

    float rad = radius[circleIndex];
    float maxDist = rad * rad;

    // 圆对像素无贡献，直接返回
    if (pixelDist > maxDist)
        return;

    float colR, colG, colB;
    float alpha;

    // 计算颜色和透明度
    if (sceneName == SNOWFLAKES || sceneName == SNOWFLAKES_SINGLE_FRAME) {

        // 雪花透明度随距离衰减，颜色查表
        const float kCircleMaxAlpha = .5f;
        const float falloffScale = 4.f;

        float normPixelDist = sqrt(pixelDist) / rad;
        lookupColor(normPixelDist, colR, colG, colB);

        float maxAlpha = kCircleMaxAlpha * CLAMP(.6f + .4f * (1.f-pz), 0.f, 1.f);
        alpha = maxAlpha * exp(-1.f * falloffScale * normPixelDist * normPixelDist);

    } else {

        // 其他场景直接取圆的颜色
        int index3 = 3 * circleIndex;
        colR = color[index3];
        colG = color[index3+1];
        colB = color[index3+2];
        alpha = .5f;
    }

    // 重要：混合当前圆对像素的贡献，保证顺序一致
    float oneMinusAlpha = 1.f - alpha;
    pixelData[0] = alpha * colR + oneMinusAlpha * pixelData[0];
    pixelData[1] = alpha * colG + oneMinusAlpha * pixelData[1];
    pixelData[2] = alpha * colB + oneMinusAlpha * pixelData[2];
    pixelData[3] += alpha;
}

// 渲染所有圆
void
RefRenderer::render() {

    // 遍历所有圆
    for (int circleIndex=0; circleIndex<numCircles; circleIndex++) {

        int index3 = 3 * circleIndex;

        float px = position[index3];
        float py = position[index3+1];
        float pz = position[index3+2];
        float rad = radius[circleIndex];

        // 计算圆的包围盒（归一化坐标）
        float minX = px - rad;
        float maxX = px + rad;
        float minY = py - rad;
        float maxY = py + rad;

        // 转换为屏幕像素坐标，并裁剪到屏幕边界
        int screenMinX = CLAMP(static_cast<int>(minX * image->width), 0, image->width);
        int screenMaxX = CLAMP(static_cast<int>(maxX * image->width)+1, 0, image->width);
        int screenMinY = CLAMP(static_cast<int>(minY * image->height), 0, image->height);
        int screenMaxY = CLAMP(static_cast<int>(maxY * image->height)+1, 0, image->height);

        float invWidth = 1.f / image->width;
        float invHeight = 1.f / image->height;

        // 遍历包围盒内所有像素，计算圆对像素的贡献
        for (int pixelY=screenMinY; pixelY<screenMaxY; pixelY++) {

            // 指向当前行的像素数据
            float* imgPtr = &image->data[4 * (pixelY * image->width + screenMinX)];

            for (int pixelX=screenMinX; pixelX<screenMaxX; pixelX++) {

                // 计算像素中心归一化坐标
                float pixelCenterNormX = invWidth * (static_cast<float>(pixelX) + 0.5f);
                float pixelCenterNormY = invHeight * (static_cast<float>(pixelY) + 0.5f);
                shadePixel(circleIndex, pixelCenterNormX, pixelCenterNormY, px, py, pz, imgPtr);
                imgPtr += 4;
            }
        }
    }
}

// 导出所有粒子参数到文件
void RefRenderer::dumpParticles(const char* filename) {

    FILE* output = fopen(filename, "w");

    fprintf(output, "%d\n", numCircles);
    for (int i=0; i<numCircles; i++) {
        fprintf(output, "%f %f %f   %f %f %f   %f\n",
                position[3*i+0], position[3*i+1], position[3*i+2],
                velocity[3*i+0], velocity[3*i+1], velocity[3*i+2],
                radius[i]);
    }
    fclose(output);

}