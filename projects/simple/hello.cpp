#include <stdio.h>

#include <cimgui.h>

#include <rawkit/rawkit.h>

#include <vulkan/vulkan.h>

#include <rawkit/grbl/parser.h>
#include <rawkit/gcode/parser.h>

void setup() {
  VkImage pImage;
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.format = VK_FORMAT_R8G8B8A8_UNORM;
  info.extent.width = 1024;
  info.extent.height = 768;
  info.extent.depth = 1;
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkDevice device = rawkit_vulkan_device();
  printf("vulkan device: %p\n", device);

  VkResult result = vkCreateImage(
    device,
    &info,
    NULL,
    &pImage
  );

  if (result == VK_SUCCESS) {
    printf("created an image\n");
  } else {
    printf("failed to create image\n");
  }


}
void loop() {
  bool show_demo_window = true;
  // igShowDemoWindow(NULL);
  igBegin("simple window", 0, 0);
    igTextUnformatted("hello!", NULL);
  igEnd();
}
