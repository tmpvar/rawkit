#include <rawkit/core.h>
#include <rawkit/gpu.h>
#include <rawkit/hot.h>

#include <vulkan/vulkan.h>

rawkit_gpu_t *rawkit_gpu_init(const char** extensions, uint32_t extensions_count, bool validation, PFN_vkDebugReportCallbackEXT debug_callback) {
  rawkit_gpu_t *gpu = rawkit_hot_resource("rawkit::gpu::default", rawkit_gpu_t);
  if (!gpu) {
    return NULL;
  }

  if (gpu->resource_version) {
    return gpu;
  }

  VkResult err;

  // Create Vulkan Instance
  {
    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = NULL;
    app.pApplicationName = "rawkit";
    app.applicationVersion = 0;
    app.pEngineName = "rawkit";
    app.engineVersion = 0;
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.enabledExtensionCount = extensions_count;
    create_info.ppEnabledExtensionNames = extensions;
    create_info.pApplicationInfo = &app;

    if (validation) {
      // Enabling multiple validation layers grouped as LunarG standard validation
      const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
      create_info.enabledLayerCount = 1;
      create_info.ppEnabledLayerNames = layers;
    }

    const char** extensions_ext = NULL;
    if (debug_callback) {
      // Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
      extensions_ext = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
      if (!extensions_ext) {
        printf("ERROR: unable to init vulkan - out of memory while allocating extension list");
        return gpu;
      }
      memcpy(extensions_ext, extensions, extensions_count * sizeof(const char*));
      extensions_ext[extensions_count] = "VK_EXT_debug_report";
      create_info.enabledExtensionCount = extensions_count + 1;
      create_info.ppEnabledExtensionNames = extensions_ext;
    }

    for (uint32_t i=0; i<create_info.enabledExtensionCount; i++) {
      printf("extension: %s\n", create_info.ppEnabledExtensionNames[i]);
    }

    // Create Vulkan Instance
    err = vkCreateInstance(&create_info,  gpu->allocator, &gpu->instance);
    if (err) {
      // if this was caused by a layer, disable all of the layers and try again
      if (err == VK_ERROR_LAYER_NOT_PRESENT) {
        create_info.enabledLayerCount = 0;
        err = vkCreateInstance(&create_info,  gpu->allocator, &gpu->instance);
      }

      if (err) {
        printf("ERROR: rawkit_gpu_init: failed to create vulkan instance (%i)\n", err);
        return gpu;
      }
    }

    if (debug_callback) {
      free(extensions_ext);
      // Get the function pointer (required for any extensions)
      auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(gpu->instance, "vkCreateDebugReportCallbackEXT");
      if (vkCreateDebugReportCallbackEXT) {
        // Setup the debug report callback
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_callback;
        debug_report_ci.pUserData = NULL;
        err = vkCreateDebugReportCallbackEXT(gpu->instance, &debug_report_ci, gpu->allocator, &gpu->debug_report);
        // This is not a hard error, but deserves a log
        if (err) {
          printf("ERROR: rawkit_gpu_init: failed to create debug reporter (%i)\n", err);
        }
      }
    }
  }

  // Select GPU
  {
    uint32_t gpu_count;
    err = vkEnumeratePhysicalDevices(gpu->instance, &gpu_count, NULL);
    if (err) {
      printf("ERROR: rawkit_gpu_init: could not find a viable physical device (%i)\n", err);
      return gpu;
    }

    VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
    if (!gpus) {
      printf("ERROR: rawkit_gpu_init: failed to allocate physical device list - out of memory\n", err);
      return gpu;
    }

    err = vkEnumeratePhysicalDevices(gpu->instance, &gpu_count, gpus);
    if (err) {
      printf("ERROR: rawkit_gpu_init: device enumeration failed (%i)\n", err);
      return gpu;
    }

    // If a number >1 of GPUs got reported, you should find the best fit GPU for your purpose
    // e.g. VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU if available, or with the greatest memory available, etc.
    // for sake of simplicity we'll just take the first one, assuming it has a graphics queue family.
    gpu->physical_device = gpus[0];
    free(gpus);
  }

  // find and store the graphics queue
  {
    int32_t index = rawkit_vulkan_find_queue_family_index(gpu, VK_QUEUE_GRAPHICS_BIT);
    if (index < 0) {
      printf("ERROR: rawkit_gpu_init: could not find graphics queue\n");
      return gpu;
    }
    gpu->graphics_queue_family_index = index;
  }

  // Create Logical Device (with 1 queue)
  {
    int device_extension_count = 1;
    const char* device_extensions[] = { "VK_KHR_swapchain" };
    const float queue_priority[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_info[1] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = gpu->graphics_queue_family_index;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = queue_priority;
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledExtensionCount = device_extension_count;
    create_info.ppEnabledExtensionNames = device_extensions;
    err = vkCreateDevice(gpu->physical_device, &create_info, gpu->allocator, &gpu->device);
    if (err) {
      printf("ERROR: rawkit_gpu_init: could not create device (%i)\n", err);
      return gpu;
    }
    gpu->graphics_queue = rawkit_vulkan_find_queue(gpu, VK_QUEUE_GRAPHICS_BIT);
  }

  // Create Descriptor Pool
  {
    const uint32_t pool_sizes_len = 11;
    VkDescriptorPoolSize pool_sizes[pool_sizes_len] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * pool_sizes_len;
    pool_info.poolSizeCount = pool_sizes_len;
    pool_info.pPoolSizes = pool_sizes;
    err = vkCreateDescriptorPool(gpu->device, &pool_info, gpu->allocator, &gpu->default_descriptor_pool);
    if (err) {
      printf("ERROR: rawkit_gpu_init: could not create descriptor pool (%i)\n", err);
      return gpu;
    }
  }

  gpu->valid = true;
  gpu->resource_version++;
  return gpu;
}
