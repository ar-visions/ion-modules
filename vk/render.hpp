#pragma once
#include <vk/vk.hpp>

struct iRender;
struct PipelineData;

struct Render {
    sh<iRender> intern;
    static const int max_frames;
    
    void push(PipelineData &pd);
    Render(Device * = null);
    
    void updating();
    void present();
    void update();
    
    //Render(const Render &r);
    Render &operator=(const Render &r);
};
