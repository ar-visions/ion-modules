#include <vk/vk.hpp>
#include <data/data.hpp>
#include <media/canvas.hpp>

int main() {
    // A UX app could run this, insert an image lambda into the mix.
    // In terms of updating the image, who knows
    Canvas canvas;
    return Vulkan::run([&]() {
        if (!canvas)
             canvas = Canvas(vec2i {1024, 1024}, Canvas::Context2D);
        ///
        /// update the skia VkImage, you can do so manually, just so you know how its done.. ya kno mean.  then you verify that skia does this shit. period. done.
        /// stop sitting at these supposed dead-ends as if they are that; fuck you.
        ///
        rectd r = { 0, 0, 32, 32 };
        canvas.color("#f00");
        canvas.fill(r);
        canvas.flush();
        
        printf("test\n");
    });
}
