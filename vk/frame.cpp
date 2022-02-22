#include <dx/dx.hpp>
#include <vk/vk.hpp>
#include <vk/buffer.hpp>

Frame::Frame(Device *device): device(device) { }

///
void Frame::update() {
    Device &device = *this->device;
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    auto views = array<VkImageView>();
    for (Texture &tx: attachments)
        views += VkImageView(tx);
    VkFramebufferCreateInfo ci {};
    ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass      = device.render_pass;
    ci.attachmentCount = views.size();
    ci.pAttachments    = views.data();
    ci.width           = device.extent.width;
    ci.height          = device.extent.height;
    ci.layers          = 1;
    assert(vkCreateFramebuffer(device, &ci, nullptr, &framebuffer) == VK_SUCCESS);
}

///
void Frame::destroy() {
    auto &device = *this->device;
    vkDestroyFramebuffer(device, framebuffer, nullptr);
}

Frame::operator VkFramebuffer &() {
    return framebuffer;
}
