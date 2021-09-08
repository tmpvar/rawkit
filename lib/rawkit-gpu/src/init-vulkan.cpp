#include <rawkit/core.h>
#include <rawkit-gpu-internal.h>
#include <rawkit/hot.h>
#include <termcolor.h>
#include <vulkan/vulkan.h>

#include <vector>
using namespace std;


static VkBool32 VKAPI_CALL debug_utils_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)	{
    printf(ANSI_CODE_YELLOW "WARNING:" ANSI_CODE_RESET "\n  id(%u)\n  name(%s)\n  desc(%s)\n",
      callback_data->messageIdNumber,
      callback_data->pMessageIdName,
      callback_data->pMessage
    );
	} else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)	{
    printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET "\n  id(%u)\n  name(%s)\n  desc(%s)\n",
      callback_data->messageIdNumber,
      callback_data->pMessageIdName,
      callback_data->pMessage
    );
	} else {
    printf(ANSI_CODE_GRAY "DEBUG:" ANSI_CODE_RESET "\n  id(%u)\n  name(%s)\n  desc(%s)\n",
      callback_data->messageIdNumber,
      callback_data->pMessageIdName,
      callback_data->pMessage
    );
  }

	return VK_FALSE;
}

rawkit_gpu_t *rawkit_gpu_init(const char** extensions, uint32_t extensions_count, bool validation, PFN_vkDebugReportCallbackEXT debug_callback) {
  rawkit_gpu_t *gpu = rawkit_hot_resource("rawkit::gpu::default", rawkit_gpu_t);
  if (!gpu) {
    return NULL;
  }

  if (gpu->resource_version) {
    return gpu;
  }

  VkResult err;

  auto state = new GPUState;

  if (gpu->_state) {
    auto old_state = (GPUState *)gpu->_state;
    delete old_state;
  }

  gpu->_state = (void *)state;

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
    create_info.pApplicationInfo = &app;

    vector<const char*> layers = {
      // "VK_LAYER_LUNARG_api_dump",
      "VK_LAYER_KHRONOS_validation",
    };

    vector <VkValidationFeatureEnableEXT> validation_feature_enables;

    if (validation) {
      // Enabling multiple validation layers grouped as LunarG standard validation
      create_info.enabledLayerCount = layers.size();
      create_info.ppEnabledLayerNames = layers.data();

      validation_feature_enables.push_back(
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
      );

    }

    vector<const char *> extensions_ext;
    for (int i=0; i<extensions_count; i++) {
      extensions_ext.push_back(extensions[i]);
    }

    if (debug_callback) {
      extensions_ext.push_back("VK_EXT_debug_report");
    }

    extensions_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    create_info.enabledExtensionCount = extensions_ext.size();
    create_info.ppEnabledExtensionNames = extensions_ext.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
    };

    debug_utils_create_info.messageSeverity = (
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    );
    debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;


    VkValidationFeaturesEXT validation_features = {};
    validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validation_features.enabledValidationFeatureCount = validation_feature_enables.size();
    validation_features.pEnabledValidationFeatures = validation_feature_enables.data();

    create_info.pNext = &validation_features;
    validation_features.pNext = &debug_utils_create_info;

    // for (uint32_t i=0; i<create_info.enabledExtensionCount; i++) {
    //   printf("extension: %s\n", create_info.ppEnabledExtensionNames[i]);
    // }

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

    // debug utils messenger
    {
      VkDebugUtilsMessengerEXT debug_utils_messenger;

      PFN_vkCreateDebugUtilsMessengerEXT pvkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        gpu->instance,
        "vkCreateDebugUtilsMessengerEXT"
      );
      if (pvkCreateDebugUtilsMessengerEXT) {
        err = pvkCreateDebugUtilsMessengerEXT(gpu->instance, &debug_utils_create_info, nullptr, &debug_utils_messenger);
        if (err) {
          printf("failed to create debug utils messenger\n");
        }
      } else {
        printf("failed to load debug utils messenger\n");
      }
    }

    if (debug_callback) {
      // Get the function pointer (required for any extensions)
      auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(gpu->instance, "vkCreateDebugReportCallbackEXT");
      if (vkCreateDebugReportCallbackEXT) {
        // Setup the debug report callback
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = (
          VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
          VK_DEBUG_REPORT_DEBUG_BIT_EXT |
          VK_DEBUG_REPORT_ERROR_BIT_EXT |
          VK_DEBUG_REPORT_WARNING_BIT_EXT |
          VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
        );
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
      printf("ERROR: rawkit_gpu_init: failed to allocate physical device list - out of memory\n");
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

    // initialize the device properties structs
    {
      gpu->physical_device_subgroup_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
      gpu->physical_device_subgroup_properties.pNext = nullptr;

      gpu->physical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
      gpu->physical_device_properties.pNext = &gpu->physical_device_subgroup_properties;
    }

    vkGetPhysicalDeviceProperties2(
      gpu->physical_device,
      &gpu->physical_device_properties
    );

    free(gpus);
  }

  // Create Logical Device (with 1 queue per queue family)
  {
    // build the queue cache for this gpu
    rawkit_gpu_populate_queue_cache(gpu);

    int device_extension_count = 1;
    const char* device_extensions[] = { "VK_KHR_swapchain" };
    const float queue_priority[] = { 1.0f };
    std::vector<VkDeviceQueueCreateInfo> queue_info(gpu->queue_count);

    {
      u32 most_generic_queue = 0xFFFFFFFF;
      u32 most_queue_flag_bits = 0;

      for (u32 i=0; i<gpu->queue_count; i++) {
        queue_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[i].queueFamilyIndex = i;
        queue_info[i].queueCount = 1;
        queue_info[i].pQueuePriorities = queue_priority;

        u32 bits = countBits(gpu->queue_family_properties[i].queueFlags);
        if (bits > most_queue_flag_bits) {
          most_queue_flag_bits = bits;
          most_generic_queue = i;
        }
      }

      state->default_queue = most_generic_queue;
    }
    VkPhysicalDeviceFeatures features = {};

    // TODO: make this configurable
    features.shaderInt64 = true;
    features.shaderFloat64 = true;
    features.fragmentStoresAndAtomics = true;
    features.fillModeNonSolid = true;

    VkPhysicalDeviceVulkan12Features vk12_features = {};
    vk12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vk12_features.timelineSemaphore = true;


    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &vk12_features;
    create_info.queueCreateInfoCount = queue_info.size();
    create_info.pQueueCreateInfos = queue_info.data();
    create_info.enabledExtensionCount = device_extension_count;
    create_info.ppEnabledExtensionNames = device_extensions;
    create_info.pEnabledFeatures = &features;
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
