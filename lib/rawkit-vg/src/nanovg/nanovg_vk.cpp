#include "nanovg_vk.h"

#include <rawkit/window.h>

void vknvg_set_current_command_buffer(VKNVGcontext *vk, VkCommandBuffer cmdBuffer) {
  vk->command_buffer = cmdBuffer;
}

int vknvg_maxi(int a, int b) { return a > b ? a : b; }

void vknvg_xformToMat3x4(float *m3, float *t) {
  m3[0] = t[0];
  m3[1] = t[1];
  m3[2] = 0.0f;
  m3[3] = 0.0f;
  m3[4] = t[2];
  m3[5] = t[3];
  m3[6] = 0.0f;
  m3[7] = 0.0f;
  m3[8] = t[4];
  m3[9] = t[5];
  m3[10] = 1.0f;
  m3[11] = 0.0f;
}

NVGcolor vknvg_premulColor(NVGcolor c) {
  c.r *= c.a;
  c.g *= c.a;
  c.b *= c.a;
  return c;
}

VKNVGtexture *vknvg_findTexture(VKNVGcontext *vk, int id) {
  if (id > vk->ntextures || id <= 0) {
    printf("ERROR: could not find texture %i - invalid\n", id);
    return NULL;
  }
  VKNVGtexture *tex = vk->textures + id - 1;
  return tex;
}
VKNVGtexture *vknvg_allocTexture(VKNVGcontext *vk) {
  VKNVGtexture *tex = NULL;
  int i;

  for (i = 0; i < vk->ntextures; i++) {
    if (vk->textures[i].resource == VK_NULL_HANDLE) {
      tex = &vk->textures[i];
      break;
    }
  }
  if (tex == NULL) {
    if (vk->ntextures + 1 > vk->ctextures) {
      VKNVGtexture *textures;
      int ctextures = vknvg_maxi(vk->ntextures + 1, 4) + vk->ctextures / 2; // 1.5x Overallocate
      textures = (VKNVGtexture *)realloc(vk->textures, sizeof(VKNVGtexture) * ctextures);
      if (textures == NULL) {
        return NULL;
      }
      vk->textures = textures;
      vk->ctextures = ctextures;
    }
    tex = &vk->textures[vk->ntextures++];
  }
  memset(tex, 0, sizeof(*tex));
  return tex;
}
int vknvg_textureId(VKNVGcontext *vk, VKNVGtexture *tex) {
  ptrdiff_t id = tex - vk->textures;
  if (id < 0 || id > vk->ntextures) {
    return 0;
  }
  return (int)id + 1;
}
int vknvg_deleteTexture(VKNVGcontext *vk, VKNVGtexture *tex) {
  if (tex) {
    rawkit_texture_destroy(tex->resource);
    return 1;
  }
  return 0;
}

VKNVGPipeline *vknvg_allocPipeline(VKNVGcontext *vk) {
  VKNVGPipeline *ret = NULL;
  if (vk->npipelines + 1 > vk->cpipelines) {
    VKNVGPipeline *pipelines;
    int cpipelines = vknvg_maxi(vk->npipelines + 1, 128) + vk->cpipelines / 2; // 1.5x Overallocate
    pipelines = (VKNVGPipeline *)realloc(vk->pipelines, sizeof(VKNVGPipeline) * cpipelines);
    if (pipelines == NULL)
      return NULL;
    vk->pipelines = pipelines;
    vk->cpipelines = cpipelines;
  }
  ret = &vk->pipelines[vk->npipelines++];
  memset(ret, 0, sizeof(VKNVGPipeline));
  return ret;
}
int vknvg_compareCreatePipelineKey(const VKNVGCreatePipelineKey *a, const VKNVGCreatePipelineKey *b) {
  if (a->topology != b->topology) {
    return a->topology - b->topology;
  }
  if (a->stencilFill != b->stencilFill) {
    return a->stencilFill - b->stencilFill;
  }

  if (a->stencilTest != b->stencilTest) {
    return a->stencilTest - b->stencilTest;
  }
  if (a->edgeAA != b->edgeAA) {
    return a->edgeAA - b->edgeAA;
  }
  if (a->edgeAAShader != b->edgeAAShader) {
    return a->edgeAAShader - b->edgeAAShader;
  }

  if (a->compositOperation.srcRGB != b->compositOperation.srcRGB) {
    return a->compositOperation.srcRGB - b->compositOperation.srcRGB;
  }
  if (a->compositOperation.srcAlpha != b->compositOperation.srcAlpha) {
    return a->compositOperation.srcAlpha - b->compositOperation.srcAlpha;
  }
  if (a->compositOperation.dstRGB != b->compositOperation.dstRGB) {
    return a->compositOperation.dstRGB - b->compositOperation.dstRGB;
  }
  if (a->compositOperation.dstAlpha != b->compositOperation.dstAlpha) {
    return a->compositOperation.dstAlpha - b->compositOperation.dstAlpha;
  }
  return 0;
}

VKNVGPipeline *vknvg_findPipeline(VKNVGcontext *vk, VKNVGCreatePipelineKey *pipelinekey) {
  VKNVGPipeline *pipeline = NULL;
  for (int i = 0; i < vk->npipelines; i++) {
    if (vknvg_compareCreatePipelineKey(&vk->pipelines[i].create_key, pipelinekey) == 0) {
      pipeline = &vk->pipelines[i];
      break;
    }
  }
  return pipeline;
}

VkResult vknvg_memory_type_from_properties(VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
  // Search memtypes to find first index with those properties
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      // Type is available, does it match user properties?
      if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
        *typeIndex = i;
        return VK_SUCCESS;
      }
    }
    typeBits >>= 1;
  }
  // No memory types matched, return failure
  return VK_ERROR_FORMAT_NOT_SUPPORTED;
}

int vknvg_convertPaint(VKNVGcontext *vk, VKNVGfragUniforms *frag, NVGpaint *paint,
                              NVGscissor *scissor, float width, float fringe, float strokeThr) {
  VKNVGtexture *tex = NULL;
  float invxform[6];

  memset(frag, 0, sizeof(*frag));

  frag->innerCol = vknvg_premulColor(paint->innerColor);
  frag->outerCol = vknvg_premulColor(paint->outerColor);

  if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
    memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
    frag->scissorExt[0] = 1.0f;
    frag->scissorExt[1] = 1.0f;
    frag->scissorScale[0] = 1.0f;
    frag->scissorScale[1] = 1.0f;
  } else {
    nvgTransformInverse(invxform, scissor->xform);
    vknvg_xformToMat3x4(frag->scissorMat, invxform);
    frag->scissorExt[0] = scissor->extent[0];
    frag->scissorExt[1] = scissor->extent[1];
    frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
    frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
  }

  memcpy(frag->extent, paint->extent, sizeof(frag->extent));
  frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
  frag->strokeThr = strokeThr;

  if (paint->image != 0) {
    tex = vknvg_findTexture(vk, paint->image);
    if (tex == NULL)
      return 0;
    if ((tex->flags & NVG_IMAGE_FLIPY) != 0) {
      float m1[6], m2[6];
      nvgTransformTranslate(m1, 0.0f, frag->extent[1] * 0.5f);
      nvgTransformMultiply(m1, paint->xform);
      nvgTransformScale(m2, 1.0f, -1.0f);
      nvgTransformMultiply(m2, m1);
      nvgTransformTranslate(m1, 0.0f, -frag->extent[1] * 0.5f);
      nvgTransformMultiply(m1, m2);
      nvgTransformInverse(invxform, m1);
    } else {
      nvgTransformInverse(invxform, paint->xform);
    }
    frag->type = NSVG_SHADER_FILLIMG;

    if (tex->type == NVG_TEXTURE_RGBA)
      frag->texType = (tex->flags & NVG_IMAGE_PREMULTIPLIED) ? 0 : 1;
    else
      frag->texType = 2;
    //		printf("frag->texType = %d\n", frag->texType);
  } else if (paint->texture) {
    frag->type = NSVG_SHADER_FILLIMG;
    frag->texType = 1;
    nvgTransformInverse(invxform, paint->xform);
  } else {
    frag->type = NSVG_SHADER_FILLGRAD;
    frag->radius = paint->radius;
    frag->feather = paint->feather;
    nvgTransformInverse(invxform, paint->xform);
  }

  vknvg_xformToMat3x4(frag->paintMat, invxform);

  return 1;
}

VKNVGBuffer vknvg_createBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties memoryProperties, const VkAllocationCallbacks *allocator, VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memory_type, void *data, uint32_t size) {

  const VkBufferCreateInfo buf_createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, size, usage};

  VkBuffer buffer;
  NVGVK_CHECK_RESULT(vkCreateBuffer(device, &buf_createInfo, allocator, &buffer));
  VkMemoryRequirements mem_reqs = {0};
  vkGetBufferMemoryRequirements(device, buffer, &mem_reqs);

  uint32_t memoryTypeIndex;
  VkResult res = vknvg_memory_type_from_properties(memoryProperties, mem_reqs.memoryTypeBits, memory_type, &memoryTypeIndex);
  assert(res == VK_SUCCESS);
  VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mem_reqs.size, memoryTypeIndex};

  VkDeviceMemory mem;
  NVGVK_CHECK_RESULT(vkAllocateMemory(device, &mem_alloc, NULL, &mem));

  void *mapped;
  NVGVK_CHECK_RESULT(vkMapMemory(device, mem, 0, mem_alloc.allocationSize, 0, &mapped));
  memcpy(mapped, data, size);
  vkUnmapMemory(device, mem);
  NVGVK_CHECK_RESULT(vkBindBufferMemory(device, buffer, mem, 0));
  VKNVGBuffer buf = {buffer, mem, mem_alloc.allocationSize};
  return buf;
}

void vknvg_destroyBuffer(VkDevice device, const VkAllocationCallbacks *allocator, VKNVGBuffer *buffer) {

  vkDestroyBuffer(device, buffer->buffer, allocator);
  vkFreeMemory(device, buffer->mem, allocator);
}

void vknvg_UpdateBuffer(VkDevice device, const VkAllocationCallbacks *allocator, VKNVGBuffer *buffer, VkPhysicalDeviceMemoryProperties memoryProperties, VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memory_type, void *data, uint32_t size) {

  if (buffer->size < size) {
    vknvg_destroyBuffer(device, allocator, buffer);
    *buffer = vknvg_createBuffer(device, memoryProperties, allocator, usage, memory_type, data, size);
  } else {
    void *mapped;
    NVGVK_CHECK_RESULT(vkMapMemory(device, buffer->mem, 0, size, 0, &mapped));
    memcpy(mapped, data, size);
    vkUnmapMemory(device, buffer->mem);
  }
}

VkShaderModule vknvg_createShaderModule(VkDevice device, const void *code, size_t size, const VkAllocationCallbacks *allocator) {

  VkShaderModuleCreateInfo moduleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, size, (const uint32_t *)code};
  VkShaderModule module;
  NVGVK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, allocator, &module));
  return module;
}

VkBlendFactor vknvg_NVGblendFactorToVkBlendFactor(enum NVGblendFactor factor) {
  switch (factor) {
  case NVG_ZERO:
    return VK_BLEND_FACTOR_ZERO;
  case NVG_ONE:
    return VK_BLEND_FACTOR_ONE;
  case NVG_SRC_COLOR:
    return VK_BLEND_FACTOR_SRC_COLOR;
  case NVG_ONE_MINUS_SRC_COLOR:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  case NVG_DST_COLOR:
    return VK_BLEND_FACTOR_DST_COLOR;
  case NVG_ONE_MINUS_DST_COLOR:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  case NVG_SRC_ALPHA:
    return VK_BLEND_FACTOR_SRC_ALPHA;
  case NVG_ONE_MINUS_SRC_ALPHA:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  case NVG_DST_ALPHA:
    return VK_BLEND_FACTOR_DST_ALPHA;
  case NVG_ONE_MINUS_DST_ALPHA:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  case NVG_SRC_ALPHA_SATURATE:
    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
  default:
    return VK_BLEND_FACTOR_MAX_ENUM;
  }
}

VkPipelineColorBlendAttachmentState vknvg_compositOperationToColorBlendAttachmentState(NVGcompositeOperationState compositeOperation) {
  VkPipelineColorBlendAttachmentState state = {0};
  state.blendEnable = VK_TRUE;
  state.colorBlendOp = VK_BLEND_OP_ADD;
  state.alphaBlendOp = VK_BLEND_OP_ADD;
  state.colorWriteMask = 0xf;

  state.srcColorBlendFactor = vknvg_NVGblendFactorToVkBlendFactor((enum NVGblendFactor)compositeOperation.srcRGB);
  state.srcAlphaBlendFactor = vknvg_NVGblendFactorToVkBlendFactor((enum NVGblendFactor)compositeOperation.srcAlpha);
  state.dstColorBlendFactor = vknvg_NVGblendFactorToVkBlendFactor((enum NVGblendFactor)compositeOperation.dstRGB);
  state.dstAlphaBlendFactor = vknvg_NVGblendFactorToVkBlendFactor((enum NVGblendFactor)compositeOperation.dstAlpha);

  if (state.srcColorBlendFactor == VK_BLEND_FACTOR_MAX_ENUM ||
      state.srcAlphaBlendFactor == VK_BLEND_FACTOR_MAX_ENUM ||
      state.dstColorBlendFactor == VK_BLEND_FACTOR_MAX_ENUM ||
      state.dstAlphaBlendFactor == VK_BLEND_FACTOR_MAX_ENUM) {
    //default blend if failed convert
    state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  }

  return state;
}

VkDescriptorSetLayout vknvg_createDescriptorSetLayout(VkDevice device, const VkAllocationCallbacks *allocator) {
  const VkDescriptorSetLayoutBinding layout_binding[3] = {
      {
          0,
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          1,
          VK_SHADER_STAGE_VERTEX_BIT,
          NULL,
      },
      {
          1,
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          1,
          VK_SHADER_STAGE_FRAGMENT_BIT,
          NULL,
      },
      {
          2,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          1,
          VK_SHADER_STAGE_FRAGMENT_BIT,
          NULL,
      }};
  const VkDescriptorSetLayoutCreateInfo descriptor_layout = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, NULL, 0, 3, layout_binding};

  VkDescriptorSetLayout descLayout;
  NVGVK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptor_layout, allocator, &descLayout));

  return descLayout;
}

VkDescriptorPool vknvg_createDescriptorPool(VkDevice device, uint32_t count, const VkAllocationCallbacks *allocator) {

  const VkDescriptorPoolSize type_count[3] = {
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2 * count},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4 * count},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * count},
  };
  const VkDescriptorPoolCreateInfo descriptor_pool = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, NULL, 0, count * 2, 3, type_count};
  VkDescriptorPool descPool;
  NVGVK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptor_pool, allocator, &descPool));
  return descPool;
}

VkPipelineLayout vknvg_createPipelineLayout(VkDevice device, VkDescriptorSetLayout descLayout, const VkAllocationCallbacks *allocator) {
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descLayout;

  VkPipelineLayout pipelineLayout;

  NVGVK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, allocator,
                                            &pipelineLayout));

  return pipelineLayout;
}

VkPipelineDepthStencilStateCreateInfo initializeDepthStencilCreateInfo(VKNVGCreatePipelineKey *pipelinekey) {

  VkPipelineDepthStencilStateCreateInfo ds = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  ds.depthWriteEnable = VK_FALSE;
  ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  ds.depthBoundsTestEnable = VK_FALSE;
  ds.stencilTestEnable = VK_FALSE;
  ds.back.failOp = VK_STENCIL_OP_KEEP;
  ds.back.passOp = VK_STENCIL_OP_KEEP;
  ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
  if (pipelinekey->stencilFill) {
    ds.stencilTestEnable = VK_TRUE;
    ds.front.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.front.failOp = VK_STENCIL_OP_KEEP;
    ds.front.depthFailOp = VK_STENCIL_OP_KEEP;
    ds.front.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
    ds.front.reference = 0x0;
    ds.front.writeMask = 0xff;
    ds.back = ds.front;
    ds.back.passOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
  } else if (pipelinekey->stencilTest) {
    ds.stencilTestEnable = VK_TRUE;
    if (pipelinekey->edgeAA) {
      ds.front.compareOp = VK_COMPARE_OP_EQUAL;
      ds.front.reference = 0x0;
      ds.front.compareMask = 0xff;
      ds.front.writeMask = 0xff;
      ds.front.failOp = VK_STENCIL_OP_KEEP;
      ds.front.depthFailOp = VK_STENCIL_OP_KEEP;
      ds.front.passOp = VK_STENCIL_OP_KEEP;
      ds.back = ds.front;
    } else {
      ds.front.compareOp = VK_COMPARE_OP_NOT_EQUAL;
      ds.front.reference = 0x0;
      ds.front.compareMask = 0xff;
      ds.front.writeMask = 0xff;
      ds.front.failOp = VK_STENCIL_OP_ZERO;
      ds.front.depthFailOp = VK_STENCIL_OP_ZERO;
      ds.front.passOp = VK_STENCIL_OP_ZERO;
      ds.back = ds.front;
    }
  }

  return ds;
}
VKNVGPipeline *vknvg_createPipeline(VKNVGcontext *vk, VKNVGCreatePipelineKey *pipelinekey) {

  VkDevice device = vk->createInfo.gpu->device;
  VkPipelineLayout pipelineLayout = vk->pipelineLayout;
  VkRenderPass renderpass = vk->createInfo.renderpass;
  const VkAllocationCallbacks *allocator = vk->createInfo.gpu->allocator;

  VkDescriptorSetLayout descLayout = vk->descLayout;
  VkShaderModule vert_shader = vk->fillVertShader;
  VkShaderModule frag_shader = vk->fillFragShader;
  VkShaderModule frag_shader_aa = vk->fillFragShaderAA;

  VkVertexInputBindingDescription vi_bindings[1] = {{0}};
  vi_bindings[0].binding = 0;
  vi_bindings[0].stride = sizeof(NVGvertex);
  vi_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription vi_attrs[2] = {
      {0},
  };
  vi_attrs[0].binding = 0;
  vi_attrs[0].location = 0;
  vi_attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
  vi_attrs[0].offset = 0;
  vi_attrs[1].binding = 0;
  vi_attrs[1].location = 1;
  vi_attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
  vi_attrs[1].offset = (2 * sizeof(float));

  VkPipelineVertexInputStateCreateInfo vi = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vi.vertexBindingDescriptionCount = 1;
  vi.pVertexBindingDescriptions = vi_bindings;
  vi.vertexAttributeDescriptionCount = 2;
  vi.pVertexAttributeDescriptions = vi_attrs;

  VkPipelineInputAssemblyStateCreateInfo ia = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  ia.topology = pipelinekey->topology;

  VkPipelineRasterizationStateCreateInfo rs = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rs.polygonMode = VK_POLYGON_MODE_FILL;
  rs.cullMode = VK_CULL_MODE_BACK_BIT;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.depthClampEnable = VK_FALSE;
  rs.rasterizerDiscardEnable = VK_FALSE;
  rs.depthBiasEnable = VK_FALSE;
  rs.lineWidth = 1.0f;

  VkPipelineColorBlendAttachmentState colorblend = vknvg_compositOperationToColorBlendAttachmentState(pipelinekey->compositOperation);

  if (pipelinekey->stencilFill) {
    rs.cullMode = VK_CULL_MODE_NONE;
    colorblend.colorWriteMask = 0;
  }

  VkPipelineColorBlendStateCreateInfo cb = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  cb.attachmentCount = 1;
  cb.pAttachments = &colorblend;

  VkPipelineViewportStateCreateInfo vp = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  vp.viewportCount = 1;
  vp.scissorCount = 1;

  VkDynamicState dynamicStateEnables[2] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStateEnables;

  VkPipelineDepthStencilStateCreateInfo ds = initializeDepthStencilCreateInfo(pipelinekey);

  VkPipelineMultisampleStateCreateInfo ms = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  ms.pSampleMask = NULL;
  ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineShaderStageCreateInfo shaderStages[2] = {{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}, {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vert_shader;
  shaderStages[0].pName = "main";

  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = frag_shader;
  shaderStages[1].pName = "main";
  if (pipelinekey->edgeAAShader) {
    shaderStages[1].module = frag_shader_aa;
  }

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stageCount = 2;
  pipelineCreateInfo.pStages = shaderStages;
  pipelineCreateInfo.pVertexInputState = &vi;
  pipelineCreateInfo.pInputAssemblyState = &ia;
  pipelineCreateInfo.pRasterizationState = &rs;
  pipelineCreateInfo.pColorBlendState = &cb;
  pipelineCreateInfo.pMultisampleState = &ms;
  pipelineCreateInfo.pViewportState = &vp;
  pipelineCreateInfo.pDepthStencilState = &ds;
  pipelineCreateInfo.renderPass = renderpass;
  pipelineCreateInfo.pDynamicState = &dynamicState;

  VkPipeline pipeline;
  NVGVK_CHECK_RESULT(vkCreateGraphicsPipelines(device, 0, 1, &pipelineCreateInfo, allocator, &pipeline));

  VKNVGPipeline *ret = vknvg_allocPipeline(vk);

  ret->create_key = *pipelinekey;
  ret->pipeline = pipeline;
  return ret;
}

VkPipeline vknvg_bindPipeline(VKNVGcontext *vk, VkCommandBuffer cmdBuffer, VKNVGCreatePipelineKey *pipelinekey) {
  VKNVGPipeline *pipeline = vknvg_findPipeline(vk, pipelinekey);
  if (!pipeline) {
    pipeline = vknvg_createPipeline(vk, pipelinekey);
  }
  if (pipeline != vk->currentPipeline) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vk->currentPipeline = pipeline;
  }
  return pipeline->pipeline;
}
#pragma optimize( "", off )
int vknvg_UpdateTexture(VkDevice device, VKNVGtexture *tex, int dx, int dy, int w, int h, const unsigned char *data) {
  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(device, tex->resource->image, &mem_reqs);
  VkImageSubresource subres = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
  VkSubresourceLayout layout;
  void *bindptr;
  /* Get the subresource layout so we know what the row pitch is */
  vkGetImageSubresourceLayout(device, tex->resource->image, &subres, &layout);
  NVGVK_CHECK_RESULT(vkMapMemory(device, tex->resource->image_memory, 0, mem_reqs.size, 0, &bindptr));
  int comp_size = (tex->type == NVG_TEXTURE_RGBA) ? 4 : 1;
  for (int y = 0; y < h; ++y) {
    char *src = (char *)data + ((dy + y) * (tex->resource->options.width * comp_size)) + dx;
    char *dest = (char *)bindptr + ((dy + y) * layout.rowPitch) + dx;
    memcpy(dest, src, w * comp_size);
  }
  vkUnmapMemory(device, tex->resource->image_memory);
  return 1;
}

int vknvg_maxVertCount(const NVGpath *paths, int npaths) {
  int i, count = 0;
  for (i = 0; i < npaths; i++) {
    count += paths[i].nfill;
    count += paths[i].nstroke;
  }
  return count;
}

VKNVGcall *vknvg_allocCall(VKNVGcontext *vk) {
  VKNVGcall *ret = NULL;
  if (vk->ncalls + 1 > vk->ccalls) {
    VKNVGcall *calls;
    int ccalls = vknvg_maxi(vk->ncalls + 1, 128) + vk->ccalls / 2; // 1.5x Overallocate
    calls = (VKNVGcall *)realloc(vk->calls, sizeof(VKNVGcall) * ccalls);
    if (calls == NULL)
      return NULL;
    vk->calls = calls;
    vk->ccalls = ccalls;
  }
  ret = &vk->calls[vk->ncalls++];
  memset(ret, 0, sizeof(VKNVGcall));
  return ret;
}

int vknvg_allocPaths(VKNVGcontext *vk, int n) {
  int ret = 0;
  if (vk->npaths + n > vk->cpaths) {
    VKNVGpath *paths;
    int cpaths = vknvg_maxi(vk->npaths + n, 128) + vk->cpaths / 2; // 1.5x Overallocate
    paths = (VKNVGpath *)realloc(vk->paths, sizeof(VKNVGpath) * cpaths);
    if (paths == NULL)
      return -1;
    vk->paths = paths;
    vk->cpaths = cpaths;
  }
  ret = vk->npaths;
  vk->npaths += n;
  return ret;
}

int vknvg_allocVerts(VKNVGcontext *vk, int n) {
  int ret = 0;
  if (vk->nverts + n > vk->cverts) {
    NVGvertex *verts;
    int cverts = vknvg_maxi(vk->nverts + n, 4096) + vk->cverts / 2; // 1.5x Overallocate
    verts = (NVGvertex *)realloc(vk->verts, sizeof(NVGvertex) * cverts);
    if (verts == NULL)
      return -1;
    vk->verts = verts;
    vk->cverts = cverts;
  }
  ret = vk->nverts;
  vk->nverts += n;
  return ret;
}

int vknvg_allocFragUniforms(VKNVGcontext *vk, int n) {
  int ret = 0, structSize = vk->fragSize;
  if (vk->nuniforms + n > vk->cuniforms) {
    unsigned char *uniforms;
    int cuniforms = vknvg_maxi(vk->nuniforms + n, 128) + vk->cuniforms / 2; // 1.5x Overallocate
    uniforms = (unsigned char *)realloc(vk->uniforms, structSize * cuniforms);
    if (uniforms == NULL)
      return -1;
    vk->uniforms = uniforms;
    vk->cuniforms = cuniforms;
  }
  ret = vk->nuniforms * structSize;
  vk->nuniforms += n;
  return ret;
}
VKNVGfragUniforms *vknvg_fragUniformPtr(VKNVGcontext *vk, int i) {
  return (VKNVGfragUniforms *)&vk->uniforms[i];
}

void vknvg_vset(NVGvertex *vtx, float x, float y, float u, float v) {
  vtx->x = x;
  vtx->y = y;
  vtx->u = u;
  vtx->v = v;
}

void vknvg_setUniforms(VKNVGcontext *vk, VkDescriptorSet descSet, int uniformOffset, VKNVGcall *call) {
  VkDevice device = vk->createInfo.gpu->device;

  VkWriteDescriptorSet writes[3] = {{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}};

  VkDescriptorBufferInfo vertUniformBufferInfo = {0};
  vertUniformBufferInfo.buffer = vk->vertUniformBuffer.buffer;
  vertUniformBufferInfo.offset = 0;
  vertUniformBufferInfo.range = sizeof(vk->view);

  writes[0].dstSet = descSet;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].pBufferInfo = &vertUniformBufferInfo;
  writes[0].dstArrayElement = 0;
  writes[0].dstBinding = 0;

  VkDescriptorBufferInfo uniform_buffer_info = {0};
  uniform_buffer_info.buffer = vk->fragUniformBuffer.buffer;
  uniform_buffer_info.offset = uniformOffset;
  uniform_buffer_info.range = sizeof(VKNVGfragUniforms);

  writes[1].dstSet = descSet;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[1].pBufferInfo = &uniform_buffer_info;
  writes[1].dstBinding = 1;

  VkDescriptorImageInfo image_info = {0};
  // default to the error image

  if (call->paint.texture && call->paint.texture->resource_version) {
    rawkit_texture_t *tex = call->paint.texture;
    image_info.imageLayout = tex->image_layout;
    image_info.imageView = tex->image_view;
    if (call->paint.sampler) {
      image_info.sampler = call->paint.sampler->handle;
    } else {
      image_info.sampler = tex->default_sampler->handle;
    }
  }

  if (call->paint.image > 0) {
    VKNVGtexture *tex = vknvg_findTexture(vk, call->paint.image);
    image_info.imageLayout = tex->resource->image_layout;
    image_info.imageView = tex->resource->image_view;
    if (call->paint.sampler) {
      image_info.sampler = call->paint.sampler->handle;
    } else {
      image_info.sampler = tex->sampler->handle;
    }
  }

  // fallback to error image
  if (!image_info.imageLayout) {
    int error_id = nvgErrorImage(vk->ctx);
    VKNVGtexture *tex = vknvg_findTexture(vk, error_id);
    image_info.imageLayout = tex->resource->image_layout;
    image_info.imageView = tex->resource->image_view;
    image_info.sampler = tex->sampler->handle;
    if (call->paint.sampler) {
      image_info.sampler = call->paint.sampler->handle;
    } else {
      image_info.sampler = tex->sampler->handle;
    }
  }

  writes[2].dstSet = descSet;
  writes[2].dstBinding = 2;
  writes[2].descriptorCount = 1;
  writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writes[2].pImageInfo = &image_info;
  vkUpdateDescriptorSets(device, 3, writes, 0, NULL);
}

void vknvg_fill(VKNVGcontext *vk, VKNVGcall *call) {
  VKNVGpath *paths = &vk->paths[call->pathOffset];
  int i, npaths = call->pathCount;

  VkDevice device = vk->createInfo.gpu->device;
  VkCommandBuffer cmdBuffer = vk->command_buffer;

  VKNVGCreatePipelineKey pipelinekey = {0};
  pipelinekey.compositOperation = call->compositOperation;
  pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
  pipelinekey.stencilFill = true;
  pipelinekey.edgeAAShader = vk->flags & NVG_ANTIALIAS;

  vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);

  uint32_t frame_idx = rawkit_window_frame_index();
  VkDescriptorSetAllocateInfo alloc_info[1] = {
      {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, vk->descPools[frame_idx].handle, 1, &vk->descLayout},
  };
  VkDescriptorSet descSet;
  NVGVK_CHECK_RESULT(vkAllocateDescriptorSets(device, alloc_info, &descSet));
  vknvg_setUniforms(vk, descSet, call->uniformOffset, call);
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &descSet, 0, NULL);

  for (i = 0; i < npaths; i++) {
    const VkDeviceSize offsets[1] = {paths[i].fillOffset * sizeof(NVGvertex)};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
    vkCmdDraw(cmdBuffer, paths[i].fillCount, 1, 0, 0);
  }

  VkDescriptorSet descSet2;
  NVGVK_CHECK_RESULT(vkAllocateDescriptorSets(device, alloc_info, &descSet2));
  vknvg_setUniforms(vk, descSet2, call->uniformOffset + vk->fragSize, call);
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &descSet2, 0, NULL);

  if (vk->flags & NVG_ANTIALIAS) {

    pipelinekey.compositOperation = call->compositOperation;
    pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelinekey.stencilFill = false;
    pipelinekey.stencilTest = true;
    pipelinekey.edgeAA = true;
    vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);
    // Draw fringes
    for (int i = 0; i < npaths; ++i) {
      const VkDeviceSize offsets[1] = {paths[i].strokeOffset * sizeof(NVGvertex)};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
      vkCmdDraw(cmdBuffer, paths[i].strokeCount, 1, 0, 0);
    }
  }

  pipelinekey.compositOperation = call->compositOperation;
  pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  pipelinekey.stencilFill = false;
  pipelinekey.stencilTest = true;
  pipelinekey.edgeAA = false;
  vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);

  const VkDeviceSize offsets[1] = {call->triangleOffset * sizeof(NVGvertex)};
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
  vkCmdDraw(cmdBuffer, call->triangleCount, 1, 0, 0);
}

void vknvg_convexFill(VKNVGcontext *vk, VKNVGcall *call) {
  VKNVGpath *paths = &vk->paths[call->pathOffset];
  int npaths = call->pathCount;

  VkDevice device = vk->createInfo.gpu->device;
  VkCommandBuffer cmdBuffer = vk->command_buffer;

  VKNVGCreatePipelineKey pipelinekey = {0};
  pipelinekey.compositOperation = call->compositOperation;
  pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
  pipelinekey.edgeAAShader = vk->flags & NVG_ANTIALIAS;

  vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);
  uint32_t frame_idx = rawkit_window_frame_index();
  VkDescriptorSetAllocateInfo alloc_info[1] = {
      {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, vk->descPools[frame_idx].handle, 1, &vk->descLayout},
  };
  VkDescriptorSet descSet;
  NVGVK_CHECK_RESULT(vkAllocateDescriptorSets(device, alloc_info, &descSet));
  vknvg_setUniforms(vk, descSet, call->uniformOffset, call);

  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &descSet, 0, NULL);

  for (int i = 0; i < npaths; ++i) {
    const VkDeviceSize offsets[1] = {paths[i].fillOffset * sizeof(NVGvertex)};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
    vkCmdDraw(cmdBuffer, paths[i].fillCount, 1, 0, 0);
  }
  if (vk->flags & NVG_ANTIALIAS) {
    pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);

    // Draw fringes
    for (int i = 0; i < npaths; ++i) {
      const VkDeviceSize offsets[1] = {paths[i].strokeOffset * sizeof(NVGvertex)};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
      vkCmdDraw(cmdBuffer, paths[i].strokeCount, 1, 0, 0);
    }
  }
}

void vknvg_stroke(VKNVGcontext *vk, VKNVGcall *call) {
  VkDevice device = vk->createInfo.gpu->device;
  VkCommandBuffer cmdBuffer = vk->command_buffer;

  VKNVGpath *paths = &vk->paths[call->pathOffset];
  int npaths = call->pathCount;

  if (vk->flags & NVG_STENCIL_STROKES) {
    uint32_t frame_idx = rawkit_window_frame_index();
    VkDescriptorSetAllocateInfo alloc_info[1] = {
        {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, vk->descPools[frame_idx].handle, 1, &vk->descLayout},
    };
    VkDescriptorSet descSet;
    NVGVK_CHECK_RESULT(vkAllocateDescriptorSets(device, alloc_info, &descSet));
    vknvg_setUniforms(vk, descSet, call->uniformOffset, call);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &descSet, 0, NULL);
    VKNVGCreatePipelineKey pipelinekey = {0};
    pipelinekey.compositOperation = call->compositOperation;
    pipelinekey.stencilFill = false;
    pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelinekey.edgeAAShader = vk->flags & NVG_ANTIALIAS;
    vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);

    for (int i = 0; i < npaths; ++i) {
      const VkDeviceSize offsets[1] = {paths[i].strokeOffset * sizeof(NVGvertex)};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
      vkCmdDraw(cmdBuffer, paths[i].strokeCount, 1, 0, 0);
    }

    pipelinekey.stencilFill = false;
    pipelinekey.stencilTest = true;
    pipelinekey.edgeAA = true;
    vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);
    for (int i = 0; i < npaths; ++i) {
      const VkDeviceSize offsets[1] = {paths[i].strokeOffset * sizeof(NVGvertex)};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
      vkCmdDraw(cmdBuffer, paths[i].strokeCount, 1, 0, 0);
    }

    pipelinekey.stencilFill = true;
    pipelinekey.stencilTest = true;
    pipelinekey.edgeAAShader = false;
    pipelinekey.edgeAA = false;
    for (int i = 0; i < npaths; ++i) {
      const VkDeviceSize offsets[1] = {paths[i].strokeOffset * sizeof(NVGvertex)};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
      vkCmdDraw(cmdBuffer, paths[i].strokeCount, 1, 0, 0);
    }
  } else {

    VKNVGCreatePipelineKey pipelinekey = {0};
    pipelinekey.compositOperation = call->compositOperation;
    pipelinekey.stencilFill = false;
    pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelinekey.edgeAAShader = vk->flags & NVG_ANTIALIAS;

    vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);
    uint32_t frame_idx = rawkit_window_frame_index();
    VkDescriptorSetAllocateInfo alloc_info[1] = {
        {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, vk->descPools[frame_idx].handle, 1, &vk->descLayout},
    };
    VkDescriptorSet descSet;
    NVGVK_CHECK_RESULT(vkAllocateDescriptorSets(device, alloc_info, &descSet));
    vknvg_setUniforms(vk, descSet, call->uniformOffset, call);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &descSet, 0, NULL);
    // Draw Strokes

    for (int i = 0; i < npaths; ++i) {
      const VkDeviceSize offsets[1] = {paths[i].strokeOffset * sizeof(NVGvertex)};
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);
      vkCmdDraw(cmdBuffer, paths[i].strokeCount, 1, 0, 0);
    }
  }
}

void vknvg_triangles(VKNVGcontext *vk, VKNVGcall *call) {
  if (call->triangleCount == 0) {
    return;
  }
  VkDevice device = vk->createInfo.gpu->device;
  VkCommandBuffer cmdBuffer = vk->command_buffer;

  VKNVGCreatePipelineKey pipelinekey = {0};
  pipelinekey.compositOperation = call->compositOperation;
  pipelinekey.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pipelinekey.stencilFill = false;
  pipelinekey.edgeAAShader = vk->flags & NVG_ANTIALIAS;
  vknvg_bindPipeline(vk, cmdBuffer, &pipelinekey);
  uint32_t frame_idx = rawkit_window_frame_index();
  VkDescriptorSetAllocateInfo alloc_info[1] = {
      {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, vk->descPools[frame_idx].handle, 1, &vk->descLayout},
  };
  VkDescriptorSet descSet;
  NVGVK_CHECK_RESULT(vkAllocateDescriptorSets(device, alloc_info, &descSet));
  vknvg_setUniforms(vk, descSet, call->uniformOffset, call);
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &descSet, 0, NULL);

  const VkDeviceSize offsets[1] = {call->triangleOffset * sizeof(NVGvertex)};
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vk->vertexBuffer.buffer, offsets);

  vkCmdDraw(cmdBuffer, call->triangleCount, 1, 0, 0);
}
///==================================================================================================================
int vknvg_renderCreate(void *uptr) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;
  VkDevice device = vk->createInfo.gpu->device;
  VkRenderPass renderpass = vk->createInfo.renderpass;
  const VkAllocationCallbacks *allocator = vk->createInfo.gpu->allocator;

  vkGetPhysicalDeviceMemoryProperties(vk->createInfo.gpu->physical_device, &vk->memoryProperties);
  vkGetPhysicalDeviceProperties(vk->createInfo.gpu->physical_device, &vk->gpuProperties);

  const uint32_t fillVertShader[] = {
#include "shader/fill.vert.inc"
  };

  const uint32_t fillFragShader[] = {
#include "shader/fill.frag.inc"
  };
  const uint32_t fillFragShaderAA[] = {
#include "shader/fill_edge_aa.frag.inc"
  };

  vk->fillVertShader = vknvg_createShaderModule(device, fillVertShader, sizeof(fillVertShader), allocator);
  vk->fillFragShader = vknvg_createShaderModule(device, fillFragShader, sizeof(fillFragShader), allocator);
  vk->fillFragShaderAA = vknvg_createShaderModule(device, fillFragShaderAA, sizeof(fillFragShaderAA), allocator);
  int align = vk->gpuProperties.limits.minUniformBufferOffsetAlignment;

  vk->fragSize = sizeof(VKNVGfragUniforms) + align - sizeof(VKNVGfragUniforms) % align;

  vk->descLayout = vknvg_createDescriptorSetLayout(device, allocator);
  vk->pipelineLayout = vknvg_createPipelineLayout(device, vk->descLayout, allocator);
  return 1;
}

int vknvg_renderCreateTexture(void *uptr, int type, int w, int h, int imageFlags, const unsigned char *data) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;
  VKNVGtexture *tex = vknvg_allocTexture(vk);
  if (!tex) {
    return 0;
  }

  uint64_t data_size = w * h;
  VkFormat format = VK_FORMAT_R8_UNORM;
  if (type == NVG_TEXTURE_RGBA) {
    format = VK_FORMAT_R8G8B8A8_UNORM;
    data_size = w * h * 4;
  }
  tex->type = type;

  rawkit_gpu_t *gpu = vk->createInfo.gpu;
  char name[255] = "\0";
  sprintf(name, "vknvg-texture-resource#%i", vknvg_textureId(vk, tex));
  tex->resource = _rawkit_texture_mem(gpu, name, w, h, format);
  VkSamplerCreateInfo samplerCreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

  VkFilter magFilter = VK_FILTER_LINEAR;
  VkFilter minFilter = VK_FILTER_LINEAR;
  if (imageFlags & NVG_IMAGE_NEAREST) {
    magFilter = VK_FILTER_NEAREST;
    minFilter = VK_FILTER_NEAREST;
  }

  VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  if (imageFlags & NVG_IMAGE_REPEATX) {
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  }

  VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  if (imageFlags & NVG_IMAGE_REPEATY) {
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  }


  tex->sampler = rawkit_texture_sampler(
    gpu,
    magFilter,
    minFilter,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    addressModeU,
    addressModeV,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    0.0,
    false,
    1,
    false,
    VK_COMPARE_OP_NEVER,
    0.0,
    0.0,
    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    false
  );

  if (data) {
    rawkit_texture_update(tex->resource, (void *)data, data_size);
  }

  VkImageMemoryBarrier barrier = {};
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  rawkit_texture_transition(
    tex->resource,
    vk->command_buffer,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    barrier
  );

  return vknvg_textureId(vk, tex);
}
int vknvg_renderDeleteTexture(void *uptr, int image) {

  VKNVGcontext *vk = (VKNVGcontext *)uptr;

  VKNVGtexture *tex = vknvg_findTexture(vk, image);

  return vknvg_deleteTexture(vk, tex);
}
int vknvg_renderUpdateTexture(void *uptr, int image, int x, int y, int w, int h, const unsigned char *data) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;

  VKNVGtexture *tex = vknvg_findTexture(vk, image);
  vknvg_UpdateTexture(vk->createInfo.gpu->device, tex, x, y, w, h, data);
  return 1;
}
int vknvg_renderGetTextureSize(void *uptr, int image, int *w, int *h) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;
  VKNVGtexture *tex = vknvg_findTexture(vk, image);
  if (tex) {
    *w = tex->resource->options.width;
    *h = tex->resource->options.height;
    return 1;
  }
  return 0;
}
void vknvg_renderViewport(void *uptr, int width, int height, float devicePixelRatio) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;
  vk->view[0] = (float)width;
  vk->view[1] = (float)height;
}
void vknvg_renderCancel(void *uptr) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;

  vk->nverts = 0;
  vk->npaths = 0;
  vk->ncalls = 0;
  vk->nuniforms = 0;
}

void vknvg_renderFlush(void *uptr) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;
  VkDevice device = vk->createInfo.gpu->device;
  VkCommandBuffer cmdBuffer = vk->command_buffer;
  VkRenderPass renderpass = vk->createInfo.renderpass;
  VkPhysicalDeviceMemoryProperties memoryProperties = vk->memoryProperties;
  const VkAllocationCallbacks *allocator = vk->createInfo.gpu->allocator;

  int i;
  if (vk->ncalls > 0) {
    vknvg_UpdateBuffer(device, allocator, &vk->vertexBuffer, memoryProperties, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vk->verts, vk->nverts * sizeof(vk->verts[0]));
    vknvg_UpdateBuffer(device, allocator, &vk->fragUniformBuffer, memoryProperties, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vk->uniforms, vk->nuniforms * vk->fragSize);
    vknvg_UpdateBuffer(device, allocator, &vk->vertUniformBuffer, memoryProperties, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vk->view, sizeof(vk->view));
    vk->currentPipeline = NULL;

    uint32_t frame_idx = rawkit_window_frame_index();

    int frame_count = rawkit_window_frame_count();
    if (vk->descPoolsCount != frame_count) {
      VKNVGDescriptorPool *old = vk->descPools;
      vk->descPools = (VKNVGDescriptorPool*)calloc(
        sizeof(VKNVGDescriptorPool) * frame_count,
        1
      );

      memcpy(vk->descPools, old, sizeof(VKNVGDescriptorPool) * vk->descPoolsCount);
      vk->descPoolsCount = frame_count;
    }

    if (vk->ncalls > vk->descPools[frame_idx].ncalls || !vk->descPools[frame_idx].handle) {
      if (vk->descPools[frame_idx].handle) {
        vkDestroyDescriptorPool(device, vk->descPools[frame_idx].handle, allocator);
      }
      vk->descPools[frame_idx].handle = vknvg_createDescriptorPool(device, vk->ncalls, allocator);
      vk->descPools[frame_idx].ncalls = vk->ncalls;
    } else {
      vkResetDescriptorPool(device, vk->descPools[frame_idx].handle, 0);
    }

    for (i = 0; i < vk->ncalls; i++) {
      VKNVGcall *call = &vk->calls[i];
      if (call->type == VKNVG_FILL)
        vknvg_fill(vk, call);
      else if (call->type == VKNVG_CONVEXFILL)
        vknvg_convexFill(vk, call);
      else if (call->type == VKNVG_STROKE)
        vknvg_stroke(vk, call);
      else if (call->type == VKNVG_TRIANGLES) {
        vknvg_triangles(vk, call);
      }
    }
  }
  // Reset calls
  vk->nverts = 0;
  vk->npaths = 0;
  vk->ncalls = 0;
  vk->nuniforms = 0;
}
void vknvg_renderFill(void *uptr, NVGpaint *paint, NVGcompositeOperationState compositeOperation, NVGscissor *scissor, float fringe,
                             const float *bounds, const NVGpath *paths, int npaths) {

  VKNVGcontext *vk = (VKNVGcontext *)uptr;
  VKNVGcall *call = vknvg_allocCall(vk);
  NVGvertex *quad;
  VKNVGfragUniforms *frag;
  int i, maxverts, offset;

  if (call == NULL)
    return;

  call->type = VKNVG_FILL;
  call->triangleCount = 4;
  call->pathOffset = vknvg_allocPaths(vk, npaths);
  if (call->pathOffset == -1)
    goto error;
  call->pathCount = npaths;
  call->paint = *paint;
  call->compositOperation = compositeOperation;

  if (npaths == 1 && paths[0].convex) {
    call->type = VKNVG_CONVEXFILL;
    call->triangleCount = 0; // Bounding box fill quad not needed for convex fill
  }

  // Allocate vertices for all the paths.
  maxverts = vknvg_maxVertCount(paths, npaths) + call->triangleCount;
  offset = vknvg_allocVerts(vk, maxverts);
  if (offset == -1)
    goto error;

  for (i = 0; i < npaths; i++) {
    VKNVGpath *copy = &vk->paths[call->pathOffset + i];
    const NVGpath *path = &paths[i];
    memset(copy, 0, sizeof(VKNVGpath));
    if (path->nfill > 0) {
      copy->fillOffset = offset;
      copy->fillCount = path->nfill;
      memcpy(&vk->verts[offset], path->fill, sizeof(NVGvertex) * path->nfill);
      offset += path->nfill;
    }
    if (path->nstroke > 0) {
      copy->strokeOffset = offset;
      copy->strokeCount = path->nstroke;
      memcpy(&vk->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
      offset += path->nstroke;
    }
  }

  // Setup uniforms for draw calls
  if (call->type == VKNVG_FILL) {
    // Quad
    call->triangleOffset = offset;
    quad = &vk->verts[call->triangleOffset];
    vknvg_vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
    vknvg_vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
    vknvg_vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
    vknvg_vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

    call->uniformOffset = vknvg_allocFragUniforms(vk, 2);
    if (call->uniformOffset == -1)
      goto error;
    // Simple shader for stencil
    frag = vknvg_fragUniformPtr(vk, call->uniformOffset);
    memset(frag, 0, sizeof(*frag));
    frag->strokeThr = -1.0f;
    frag->type = NSVG_SHADER_SIMPLE;
    // Fill shader
    vknvg_convertPaint(vk, vknvg_fragUniformPtr(vk, call->uniformOffset + vk->fragSize), paint, scissor, fringe, fringe, -1.0f);
  } else {
    call->uniformOffset = vknvg_allocFragUniforms(vk, 1);
    if (call->uniformOffset == -1)
      goto error;
    // Fill shader
    vknvg_convertPaint(vk, vknvg_fragUniformPtr(vk, call->uniformOffset), paint, scissor, fringe, fringe, -1.0f);
  }

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (vk->ncalls > 0)
    vk->ncalls--;
}

void vknvg_renderStroke(void *uptr, NVGpaint *paint, NVGcompositeOperationState compositeOperation, NVGscissor *scissor, float fringe,
                               float strokeWidth, const NVGpath *paths, int npaths) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;
  VKNVGcall *call = vknvg_allocCall(vk);
  int i, maxverts, offset;

  if (call == NULL)
    return;

  call->type = VKNVG_STROKE;
  call->pathOffset = vknvg_allocPaths(vk, npaths);
  if (call->pathOffset == -1)
    goto error;
  call->pathCount = npaths;
  call->paint = *paint;
  call->compositOperation = compositeOperation;

  // Allocate vertices for all the paths.
  maxverts = vknvg_maxVertCount(paths, npaths);
  offset = vknvg_allocVerts(vk, maxverts);
  if (offset == -1)
    goto error;

  for (i = 0; i < npaths; i++) {
    VKNVGpath *copy = &vk->paths[call->pathOffset + i];
    const NVGpath *path = &paths[i];
    memset(copy, 0, sizeof(VKNVGpath));
    if (path->nstroke) {
      copy->strokeOffset = offset;
      copy->strokeCount = path->nstroke;
      memcpy(&vk->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
      offset += path->nstroke;
    }
  }

  if (vk->flags & NVG_STENCIL_STROKES) {
    // Fill shader
    call->uniformOffset = vknvg_allocFragUniforms(vk, 2);
    if (call->uniformOffset == -1)
      goto error;

    vknvg_convertPaint(vk, vknvg_fragUniformPtr(vk, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
    vknvg_convertPaint(vk, vknvg_fragUniformPtr(vk, call->uniformOffset + vk->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f / 255.0f);

  } else {
    // Fill shader
    call->uniformOffset = vknvg_allocFragUniforms(vk, 1);
    if (call->uniformOffset == -1)
      goto error;
    vknvg_convertPaint(vk, vknvg_fragUniformPtr(vk, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
  }

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (vk->ncalls > 0)
    vk->ncalls--;
}

void vknvg_renderTriangles(void *uptr, NVGpaint *paint, NVGcompositeOperationState compositeOperation, NVGscissor *scissor,
                                  const NVGvertex *verts, int nverts, float fringe) {
  VKNVGcontext *vk = (VKNVGcontext *)uptr;

  VKNVGcall *call = vknvg_allocCall(vk);
  VKNVGfragUniforms *frag;

  if (call == NULL)
    return;

  call->type = VKNVG_TRIANGLES;
  call->paint = *paint;
  call->compositOperation = compositeOperation;

  // Allocate vertices for all the paths.
  call->triangleOffset = vknvg_allocVerts(vk, nverts);
  if (call->triangleOffset == -1)
    goto error;
  call->triangleCount = nverts;

  memcpy(&vk->verts[call->triangleOffset], verts, sizeof(NVGvertex) * nverts);

  // Fill shader
  call->uniformOffset = vknvg_allocFragUniforms(vk, 1);
  if (call->uniformOffset == -1)
    goto error;
  frag = vknvg_fragUniformPtr(vk, call->uniformOffset);
  vknvg_convertPaint(vk, frag, paint, scissor, 1.0f, fringe, -1.0f);
  frag->type = NSVG_SHADER_IMG;

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (vk->ncalls > 0)
    vk->ncalls--;
}

void vknvg_renderDelete(void *uptr) {

  VKNVGcontext *vk = (VKNVGcontext *)uptr;

  VkDevice device = vk->createInfo.gpu->device;
  const VkAllocationCallbacks *allocator = vk->createInfo.gpu->allocator;

  for (int i = 0; i < vk->ntextures; i++) {
    if (vk->textures[i].resource) {
      vknvg_deleteTexture(vk, &vk->textures[i]);
    }
  }

  vknvg_destroyBuffer(device, allocator, &vk->vertexBuffer);
  vknvg_destroyBuffer(device, allocator, &vk->fragUniformBuffer);
  vknvg_destroyBuffer(device, allocator, &vk->vertUniformBuffer);

  vkDestroyShaderModule(device, vk->fillVertShader, allocator);
  vkDestroyShaderModule(device, vk->fillFragShader, allocator);
  vkDestroyShaderModule(device, vk->fillFragShaderAA, allocator);

  int frame_count = rawkit_window_frame_count();
  for (int i=0; i<frame_count; i++) {
    if (vk->descPools[i].handle) {
      vkDestroyDescriptorPool(device, vk->descPools[i].handle, allocator);
    }
  }

  vkDestroyDescriptorSetLayout(device, vk->descLayout, allocator);
  vkDestroyPipelineLayout(device, vk->pipelineLayout, allocator);

  for (int i = 0; i < vk->npipelines; i++) {
    vkDestroyPipeline(device, vk->pipelines[i].pipeline, allocator);
  }

  free(vk->textures);
  free(vk);
}

NVGcontext *nvgCreateVk(VKNVGCreateInfo createInfo, int flags) {
  NVGparams params;
  NVGcontext *ctx = NULL;
  VKNVGcontext *vk = (VKNVGcontext *)malloc(sizeof(VKNVGcontext));
  if (vk == NULL) {
    return NULL;
  }
  memset(vk, 0, sizeof(VKNVGcontext));

  memset(&params, 0, sizeof(params));
  params.renderCreate = vknvg_renderCreate;
  params.renderCreateTexture = vknvg_renderCreateTexture;
  params.renderDeleteTexture = vknvg_renderDeleteTexture;
  params.renderUpdateTexture = vknvg_renderUpdateTexture;
  params.renderGetTextureSize = vknvg_renderGetTextureSize;
  params.renderViewport = vknvg_renderViewport;
  params.renderCancel = vknvg_renderCancel;
  params.renderFlush = vknvg_renderFlush;
  params.renderFill = vknvg_renderFill;
  params.renderStroke = vknvg_renderStroke;
  params.renderTriangles = vknvg_renderTriangles;
  params.renderDelete = vknvg_renderDelete;
  params.userPtr = vk;
  params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;

  vk->flags = flags;
  vk->createInfo = createInfo;

  int frame_count = rawkit_window_frame_count();
  vk->descPools = (VKNVGDescriptorPool *)calloc(
    sizeof(VKNVGDescriptorPool) * frame_count,
    1
  );

  ctx = nvgCreateInternal(&params);
  if (!ctx) {
    free(vk);
    return NULL;
  }

  vk->ctx = ctx;
  return ctx;
}

void nvgDeleteVk(NVGcontext *ctx) {
  nvgDeleteInternal(ctx);
}