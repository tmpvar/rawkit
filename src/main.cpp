#include <stdio.h>          // printf, fprintf, wcstombs_s
#include <stdlib.h>         // abort
#include <uv.h>
#include <termcolor.h>
#include <flags/flags.h>

#include <GLFW/glfw3.h>

#pragma warning( push, 0 )
  #include <vulkan/vulkan.h>
#pragma warning( pop )

#include <serial/serial.h>

#include <algorithm>
#include <iostream>
#include <cctype>
#include <random>


#include <stdio.h>
#include <locale.h>

#include <hot/host/hot.h>

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include <rawkit/jit.h>
#include <rawkit/vg.h>

#include <rawkit/window-internal.h>

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;


#include <thread>
using namespace std;
// #define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
//#define IMGUI_VULKAN_DEBUG_REPORT
#endif

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int                      g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;
static int                      g_SwapChainResizeWidth = 0;
static int                      g_SwapChainResizeHeight = 0;
static rawkit_vg_t            * g_RawkitVG = nullptr;

void rawkit_wassert(wchar_t const* m, wchar_t const* f, unsigned l) {
  printf("ASSERT: %ls (%ls:%u)\n", m, f, l);
}

static void check_vk_result_(VkResult err, const char *file, const uint32_t line)
{
    if (err == 0)
        return;
    fprintf(stderr, "%s:%u [vulkan] Error: VkResult = %d\n", file, line, err);
    if (err < 0)
        abort();
}

#define check_vk_result(err) check_vk_result_(err, __FILE__, __LINE__)


#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(rawkit_gpu_t *gpu, ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(gpu->physical_device, gpu->graphics_queue_family_index, wd->Surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(gpu->physical_device, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
  #ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
  #else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
  #endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(gpu->physical_device, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);
    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateWindow(gpu->instance, gpu->physical_device, gpu->device, wd, gpu->graphics_queue_family_index, gpu->allocator, width, height, g_MinImageCount);
}

static void CleanupVulkan(rawkit_gpu_t *gpu)
{
    vkDestroyDescriptorPool(gpu->device, gpu->default_descriptor_pool, gpu->allocator);

  #ifdef IMGUI_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(gpu->instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(gpu->instance, gpu->debug_report, gpu->allocator);
  #endif // IMGUI_VULKAN_DEBUG_REPORT

    vkDestroyDevice(gpu->device, gpu->allocator);
    vkDestroyInstance(gpu->instance, gpu->allocator);
}

static void CleanupVulkanWindow(rawkit_gpu_t *gpu)
{
    ImGui_ImplVulkanH_DestroyWindow(gpu->instance, gpu->device, &g_MainWindowData, gpu->allocator);
}

void ResizeSwapChain(rawkit_gpu_t *gpu) {
  if (g_SwapChainRebuild && g_SwapChainResizeWidth > 0 && g_SwapChainResizeHeight > 0) {
    g_SwapChainRebuild = false;
    ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
    ImGui_ImplVulkanH_CreateWindow(
      gpu->instance,
      gpu->physical_device,
      gpu->device,
      &g_MainWindowData,
      gpu->graphics_queue_family_index,
      gpu->allocator,
      g_SwapChainResizeWidth,
      g_SwapChainResizeHeight,
      g_MinImageCount
    );
    g_MainWindowData.FrameIndex = 0;
  }
}

static VkResult BeginMainRenderPass(rawkit_gpu_t *gpu, ImGui_ImplVulkanH_Window* wd, int depth = 0) {
    VkResult err;

    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(gpu->device, wd->Swapchain, UINT64_MAX-1, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err < 0) {
      printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " rawkit::BeginMainRenderPass: unable to acquire next image (%i)\n", err);
      // Resize swap chain
      ResizeSwapChain(gpu);
      if (depth > 1) {
        return err;
      }
      return BeginMainRenderPass(gpu, wd, depth+1);
    }

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(gpu->device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        if (err) {
          printf("rawkit::BeginMainRenderPass: vkWaitForFences failed\n");
          exit(1);
          return err;
        }

        err = vkResetFences(gpu->device, 1, &fd->Fence);
        if (err) {
          printf("rawkit::BeginMainRenderPass: vkResetFences failed\n");
          return err;
        }
    }
    {
        gpu->command_pool = fd->CommandPool;
        err = vkResetCommandPool(gpu->device, fd->CommandPool, 0);
        if (err) {
          printf("rawkit::BeginMainRenderPass: vkResetCommandPool failed\n");
          return err;
        }

        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        if (err) {
          printf("rawkit::BeginMainRenderPass: vkBeginCommandBuffer failed\n");
          return err;
        }

    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 2;
        info.pClearValues = wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }
    return VK_SUCCESS;
}

static void EndMainRenderPass(rawkit_gpu_t *gpu, ImGui_ImplVulkanH_Window* wd, VkResult renderpass_err) {
  VkResult err;
  VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
  if (renderpass_err) {
    return;
  }

  // Submit command buffer
  if (!renderpass_err) {
    vkCmdEndRenderPass(fd->CommandBuffer);
  }

  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &fd->CommandBuffer;
    info.pWaitDstStageMask = &wait_stage;

    if (!renderpass_err) {
      info.waitSemaphoreCount = 1;
      info.pWaitSemaphores = &image_acquired_semaphore;
      info.signalSemaphoreCount = 1;
      info.pSignalSemaphores = &render_complete_semaphore;
    }

    err = vkEndCommandBuffer(fd->CommandBuffer);
    check_vk_result(err);
    err = vkQueueSubmit(gpu->graphics_queue, 1, &info, fd->Fence);
    check_vk_result(err);
  }
}

static void FramePresent(rawkit_gpu_t *gpu, ImGui_ImplVulkanH_Window* wd)
{
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(gpu->graphics_queue, &info);
    if (err) {
      printf("rawkit::FramePresent: vkQueuePresent failed\n");
    }
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void glfw_resize_callback(GLFWwindow*, int w, int h)
{
    g_SwapChainRebuild = true;
    g_SwapChainResizeWidth = w;
    g_SwapChainResizeHeight = h;
}

inline std::string trim(const std::string &s)
{
   auto wsfront=std::find_if_not(s.begin(),s.end(),[](int c){return std::isspace(c);});
   auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](int c){return std::isspace(c);}).base();
   return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
}

VkDevice rawkit_vulkan_device() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
  if (!gpu) {
    return VK_NULL_HANDLE;
  }
  return gpu->device;
}

VkPhysicalDevice rawkit_vulkan_physical_device() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
  if (!gpu) {
    return VK_NULL_HANDLE;
  }

  return gpu->physical_device;
}

VkCommandBuffer rawkit_vulkan_command_buffer() {
  ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
  return wd->Frames[wd->FrameIndex].CommandBuffer;
}

VkCommandPool rawkit_vulkan_command_pool() {
  ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
  return wd->Frames[wd->FrameIndex].CommandPool;
}

VkDescriptorPool rawkit_vulkan_descriptor_pool() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
  if (!gpu) {
    return VK_NULL_HANDLE;
  }

  return gpu->default_descriptor_pool;
}

typedef struct rawkit_imgui_texture_t {
  RAWKIT_RESOURCE_FIELDS

  ImTextureID handle;
} rawkit_imgui_texture_t;

ImTextureID rawkit_imgui_texture(rawkit_texture_t *texture, const rawkit_texture_sampler_t *sampler) {
  if (!texture) {
    return nullptr;
  }

  const uint32_t resource_count = 2;
  rawkit_resource_t *resources[resource_count] = {
    (rawkit_resource_t *)texture,
    (rawkit_resource_t *)sampler
  };

  const char *resource_name = "rawkit::imgui::texture";

  uint64_t id = rawkit_hash_resources(
    resource_name,
    resource_count,
    (const rawkit_resource_t **)resources
  );


  rawkit_imgui_texture_t* imgui_texture = rawkit_hot_resource_id(
    resource_name,
    id,
    rawkit_imgui_texture_t
  );

  bool dirty = rawkit_resource_sources_array(
    (rawkit_resource_t *)imgui_texture,
    resource_count,
    resources
  );

  if (dirty) {
    imgui_texture->handle = ImGui_ImplVulkan_AddTexture(
      sampler ? sampler->handle : texture->default_sampler->handle,
      texture->image_view,
      texture->image_layout
    );
  }

  return imgui_texture->handle;
}

VkQueue rawkit_vulkan_queue() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
  if (!gpu) {
    return VK_NULL_HANDLE;
  }

  return gpu->graphics_queue;
}

uint32_t rawkit_window_width() {
  ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
  return wd->Width;
}

uint32_t rawkit_window_height() {
  ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
  return wd->Height;
}


VkFramebuffer rawkit_current_framebuffer() {
  ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
  VkFramebuffer fb = wd->Frames[wd->FrameIndex].Framebuffer;
  return fb;
}


float rawkit_randf() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0, 1.0);
  return dis(gen);
}

VkPipelineCache rawkit_vulkan_pipeline_cache() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
  if (!gpu) {
    return VK_NULL_HANDLE;
  }

  return gpu->pipeline_cache;
}

VkRenderPass rawkit_vulkan_renderpass() {
  return g_MainWindowData.RenderPass;
}

int32_t rawkit_vulkan_queue_family() {
  rawkit_gpu_t *gpu = rawkit_default_gpu();
  if (!gpu) {
    return -1;
  }

  return gpu->graphics_queue_family_index;
}

rawkit_vg_t *rawkit_default_vg() {
  return g_RawkitVG;
}

int main(int argc, char **argv) {

    const flags::args args(argc, argv);

    rawkit_jit_t *jit = rawkit_jit_create(argv[1]);
    if (!jit) {
      return -1;
    }
    rawkit_set_default_jit(jit);
    rawkit_jit_set_debug(jit, args.get<bool>("rawkit-jit-debug", false));

    #if defined(_WIN32)
      // add guest support for dirent.h
      rawkit_jit_add_export(jit, "FindClose", FindClose);
      rawkit_jit_add_export(jit, "FindFirstFileExW", FindFirstFileExW);
      rawkit_jit_add_export(jit, "FindNextFileW", FindNextFileW);
      rawkit_jit_add_export(jit, "GetFullPathNameW", GetFullPathNameW);
      rawkit_jit_add_export(jit, "GetLastError", GetLastError);
      rawkit_jit_add_export(jit, "_set_errno", _set_errno);
      rawkit_jit_add_export(jit, "setlocale", setlocale);
      rawkit_jit_add_export(jit, "_errno", _errno);
      rawkit_jit_add_export(jit, "_wassert", rawkit_wassert);
    #endif

    rawkit_jit_add_export(jit, "rawkit_vulkan_device", rawkit_vulkan_device);
    rawkit_jit_add_export(jit, "rawkit_vulkan_physical_device", rawkit_vulkan_physical_device);
    rawkit_jit_add_export(jit, "rawkit_vulkan_command_buffer", rawkit_vulkan_command_buffer);
    rawkit_jit_add_export(jit, "rawkit_vulkan_command_pool", rawkit_vulkan_command_pool);
    rawkit_jit_add_export(jit, "rawkit_imgui_texture", rawkit_imgui_texture);
    rawkit_jit_add_export(jit, "rawkit_vulkan_queue", rawkit_vulkan_queue);
    rawkit_jit_add_export(jit, "rawkit_vulkan_pipeline_cache", rawkit_vulkan_pipeline_cache);
    rawkit_jit_add_export(jit, "rawkit_vulkan_renderpass", rawkit_vulkan_renderpass);
    rawkit_jit_add_export(jit, "rawkit_vulkan_descriptor_pool", rawkit_vulkan_descriptor_pool);
    rawkit_jit_add_export(jit, "rawkit_vulkan_queue_family", rawkit_vulkan_queue_family);
    rawkit_jit_add_export(jit, "rawkit_current_framebuffer", rawkit_current_framebuffer);
    rawkit_jit_add_export(jit, "rawkit_randf", rawkit_randf);

    host_hot_init(jit);

    auto list = serial::list_ports();
    auto found = find_if(list.begin(), list.end(), [](const serial::PortInfo& obj) {
      // cout << "arduino: " << obj.description << " :: " << obj.hardware_id << endl;
      return obj.description.find("Arduino") == 0 || obj.description.find("USB Serial Device") == 0;
    });

    string port = "/dev/null";
    serial::Serial sp;
    if (port != "/dev/null") {
      sp.setPort(port);
      sp.setBaudrate(115200);
      // TODO: WTF MAC?
      // sp.setTimeout(serial::Timeout::simpleTimeout(1));
      sp.open();
    }

    vector<string> sp_rx_lines;

    // Setup GLFW window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        return 1;
    }

    if (!glfwVulkanSupported()) {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    uint32_t extensions_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    rawkit_gpu_t *gpu = rawkit_gpu_init(
      extensions,
      extensions_count,
      args.get<bool>("rawkit-vulkan-validation", false),
      #ifdef IMGUI_VULKAN_DEBUG_REPORT
      debug_report
      #else
      nullptr
      #endif
    );

    if (args.get<bool>("headless")) {
      double headless_start = rawkit_now();
      double avg_cycle_time = -1.0;
      double min_cycle_time = FLT_MAX;
      double max_cycle_time = -1.0;
      u32 cycles = 0;
      while(1) {
        cycles++;
        rawkit_gpu_tick(gpu);

        // TODO: rerun if any of the dependencies change - file, shader, etc..
        if (rawkit_jit_tick(jit)) {
          rawkit_jit_call_setup(jit);
        }

        double cycle_start = rawkit_now();
        rawkit_jit_call_loop(jit);
        double diff = rawkit_now() - cycle_start;
        if (diff < min_cycle_time) {
          min_cycle_time = diff;
        }

        if (diff > max_cycle_time) {
          max_cycle_time = diff;
        }

        if (avg_cycle_time < 0.0) {
          avg_cycle_time = diff;
        } else {
          avg_cycle_time = (diff + avg_cycle_time) * 0.5;
        }

        if (args.get<bool>("once")) {
          break;
        }

        u32 sleep_time = args.get<u32>("rawkit-loop-sleep-ms", 250);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
      }
      fprintf(stderr, "done %fms\n", (rawkit_now() - headless_start) * 1000.0);
      fprintf(stderr, "cycle time (%u samples): \n avg: %fms\n min: %fms\n max: %fms\n",
        cycles,
        avg_cycle_time * 1000.0,
        min_cycle_time * 1000.0,
        max_cycle_time * 1000.0
      );
      return 0;
    }

    string title = "rawkit " + string(rawkit_jit_program_path(jit));


    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    uint32_t width = 1280;
    uint32_t height = 720;
    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

    rawkit_window_internal_set_glfw_window(window);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(gpu->instance, window, gpu->allocator, &surface);
    check_vk_result(err);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glfwSetFramebufferSizeCallback(window, glfw_resize_callback);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(gpu, wd, surface, w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = gpu->instance;
    init_info.PhysicalDevice = gpu->physical_device;
    init_info.Device = gpu->device;
    init_info.QueueFamily = gpu->graphics_queue_family_index;
    init_info.Queue = gpu->graphics_queue;
    init_info.PipelineCache = gpu->pipeline_cache;
    init_info.DescriptorPool = gpu->default_descriptor_pool;
    init_info.Allocator = gpu->allocator;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.CheckVkResultFn = [](VkResult err) {
      check_vk_result(err);
    };


    ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        // Use any command queue
        VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
        VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

        err = vkResetCommandPool(gpu->device, command_pool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(command_buffer, &begin_info);
        check_vk_result(err);

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        err = vkEndCommandBuffer(command_buffer);
        check_vk_result(err);
        err = vkQueueSubmit(gpu->graphics_queue, 1, &end_info, VK_NULL_HANDLE);
        check_vk_result(err);

        err = vkDeviceWaitIdle(gpu->device);
        check_vk_result(err);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }


    gpu->command_pool = wd->Frames[0].CommandPool;
    printf("set default gpu pool(%p)\n", gpu->command_pool);
    rawkit_set_default_gpu(gpu);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;


    int32_t prev_width = width;
    int32_t prev_height = height;
    int32_t prev_x = 0;
    int32_t prev_y = 0;
    bool fullscreen = false;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // handle fullscreen toggle
        {
          if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
            fullscreen = !fullscreen;

            if (fullscreen) {
              glfwGetWindowSize(window, &prev_width, &prev_height);
              glfwGetWindowPos(window, &prev_x, &prev_y);

              const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
              glfwSetWindowMonitor(
                window,
                glfwGetPrimaryMonitor(),
                0,
                0,
                mode->width,
                mode->height,
                GLFW_DONT_CARE
              );
            } else {
              glfwSetWindowMonitor(
                window,
                NULL,
                prev_x,
                prev_y,
                prev_width,
                prev_height,
                GLFW_DONT_CARE
              );
            }
          }
        }

        rawkit_gpu_tick(gpu);

        if (rawkit_jit_tick(jit)) {
          rawkit_jit_call_setup(jit);
        }



        // service libuv
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);


        // Poll the serialport
        if (sp.isOpen()) {
          string line = trim(sp.readline());
          if (line.length()) {
            cout << "recv: " << line << endl;
            sp_rx_lines.push_back("<< " + line);
          }
        }

        // Start the Dear ImGui frame
        ImGui_ImplGlfw_NewFrame();

        VkResult render_pass_err = BeginMainRenderPass(gpu, wd);
        if (render_pass_err) {
          continue;
        }

        ImGui::NewFrame();
        if (!g_RawkitVG)
        {
          g_RawkitVG = rawkit_vg(
              gpu,
              rawkit_vulkan_renderpass(),
              "default",
              nullptr);
        }

        // show compilation errors
        {
          uint32_t i = 0;
          rawkit_jit_message_t msg;
          while (rawkit_jit_get_message(jit, i, &msg)) {
            if (i == 0) {
              ImGui::Begin("program compilation issues");
            }

            const char *level_str = "<null>";
            switch (msg.level) {
              case RAWKIT_JIT_MESSAGE_LEVEL_NOTE: level_str = "note"; break;
              case RAWKIT_JIT_MESSAGE_LEVEL_WARNING: level_str = "warning"; break;
              case RAWKIT_JIT_MESSAGE_LEVEL_REMARK: level_str = "remark"; break;
              case RAWKIT_JIT_MESSAGE_LEVEL_ERROR: level_str = "error"; break;
              case RAWKIT_JIT_MESSAGE_LEVEL_FATAL: level_str = "fatal"; break;
              default:
                level_str = "none";
            }

            ImGui::Text("%s %s:%u:%u %s", level_str, msg.filename, msg.line, msg.column, msg.str);

            i++;
          }

          if (i > 0) {
            ImGui::End();
          }
        }


        rawkit_window_internal_set_frame_index(g_MainWindowData.FrameIndex);
        rawkit_window_internal_set_frame_count(g_MainWindowData.ImageCount);

        VkViewport viewport = {};
        viewport.width = (float)g_MainWindowData.Width;
        viewport.height = (float)g_MainWindowData.Height;
        vkCmdSetViewport(
          rawkit_vulkan_command_buffer(),
          0,
          1,
          &viewport
        );

        VkRect2D scissor = {};
        scissor.extent.width = g_MainWindowData.Width;
        scissor.extent.height = g_MainWindowData.Height;
        vkCmdSetScissor(
          rawkit_vulkan_command_buffer(),
          0,
          1,
          &scissor
        );

        ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];

        // TODO: handle device pixel ratio (fb.width / window.width)
        rawkit_vg_begin_frame(g_RawkitVG, fd->CommandBuffer, g_MainWindowData.Width, g_MainWindowData.Height, 1.0);

        rawkit_jit_call_loop(jit);

        rawkit_vg_end_frame(g_RawkitVG);

        // Render ImGui
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

        EndMainRenderPass(gpu, wd, render_pass_err);
        // TODO: this is silly. use glfwGetWindowAttrib(window, GLFW_ICONIFIED);
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (!is_minimized) {
          FramePresent(gpu, wd);
        }
    }

    // Cleanup
    err = vkDeviceWaitIdle(gpu->device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkanWindow(gpu);
    CleanupVulkan(gpu);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
