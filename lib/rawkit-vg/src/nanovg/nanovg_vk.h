#pragma once


#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <nanovg/nanovg.h>
#include <vulkan/vulkan.h>

enum NVGcreateFlags {
  // Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
  NVG_ANTIALIAS = 1 << 0,
  // Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
  // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
  NVG_STENCIL_STROKES = 1 << 1,
  // Flag indicating that additional debug checks are done.
  NVG_DEBUG = 1 << 2,
};

typedef struct VKNVGCreateInfo {
  VkPhysicalDevice gpu;
  VkDevice device;
  VkRenderPass renderpass;
  // VkCommandBuffer cmdBuffer;

  const VkAllocationCallbacks *allocator; //Allocator for vulkan. can be null
} VKNVGCreateInfo;

#define NVGVK_CHECK_RESULT(f)    \
  {                              \
    VkResult res = (f);          \
    if (res != VK_SUCCESS) {     \
      assert(res == VK_SUCCESS); \
    }                            \
  }

enum VKNVGshaderType {
  NSVG_SHADER_FILLGRAD,
  NSVG_SHADER_FILLIMG,
  NSVG_SHADER_SIMPLE,
  NSVG_SHADER_IMG
};

typedef struct VKNVGtexture {
  VkSampler sampler;

  VkImage image;
  VkImageLayout imageLayout;
  VkImageView view;

  VkDeviceMemory mem;
  int32_t width, height;
  int type; //enum NVGtexture
  int flags;
} VKNVGtexture;

enum VKNVGcallType {
  VKNVG_NONE = 0,
  VKNVG_FILL,
  VKNVG_CONVEXFILL,
  VKNVG_STROKE,
  VKNVG_TRIANGLES,
};

typedef struct VKNVGcall {
  int type;
  int pathOffset;
  int pathCount;
  int triangleOffset;
  int triangleCount;
  int uniformOffset;
  NVGcompositeOperationState compositOperation;
  NVGpaint paint;
} VKNVGcall;

typedef struct VKNVGpath {
  int fillOffset;
  int fillCount;
  int strokeOffset;
  int strokeCount;
} VKNVGpath;

typedef struct VKNVGfragUniforms {
  float scissorMat[12]; // matrices are actually 3 vec4s
  float paintMat[12];
  struct NVGcolor innerCol;
  struct NVGcolor outerCol;
  float scissorExt[2];
  float scissorScale[2];
  float extent[2];
  float radius;
  float feather;
  float strokeMult;
  float strokeThr;
  int texType;
  int type;
} VKNVGfragUniforms;

typedef struct VKNVGBuffer {
  VkBuffer buffer;
  VkDeviceMemory mem;
  VkDeviceSize size;
} VKNVGBuffer;

enum VKNVGstencilType {
  VKNVG_STENCIL_NONE = 0,
  VKNVG_STENCIL_FILL,
};
typedef struct VKNVGCreatePipelineKey {
  bool stencilFill;
  bool stencilTest;
  bool edgeAA;
  bool edgeAAShader;
  VkPrimitiveTopology topology;
  NVGcompositeOperationState compositOperation;
} VKNVGCreatePipelineKey;

typedef struct VKNVGPipeline {
  VKNVGCreatePipelineKey create_key;
  VkPipeline pipeline;
} VKNVGPipeline;

typedef struct VKNVGDepthSimplePipeline {
  VkPipeline pipeline;
  VkDescriptorSetLayout descLayout;
  VkPipelineLayout pipelineLayout;
} VKNVGDepthSimplePipeline;

typedef struct VKNVGcontext {
  VKNVGCreateInfo createInfo;

  VkPhysicalDeviceProperties gpuProperties;
  VkPhysicalDeviceMemoryProperties memoryProperties;

  int fragSize;
  int flags;

  //own resources
  VKNVGtexture *textures;
  int ntextures;
  int ctextures;

  VkDescriptorSetLayout descLayout;
  VkPipelineLayout pipelineLayout;

  VKNVGPipeline *pipelines;
  int cpipelines;
  int npipelines;

  float view[2];

  // Per frame buffers
  VKNVGcall *calls;
  int ccalls;
  int ncalls;
  VKNVGpath *paths;
  int cpaths;
  int npaths;
  struct NVGvertex *verts;
  int cverts;
  int nverts;

  VkDescriptorPool descPool;
  int cdescPool;

  unsigned char *uniforms;
  int cuniforms;
  int nuniforms;
  VKNVGBuffer vertexBuffer;
  VKNVGBuffer vertUniformBuffer;
  VKNVGBuffer fragUniformBuffer;
  VKNVGPipeline *currentPipeline;

  VkShaderModule fillFragShader;
  VkShaderModule fillFragShaderAA;
  VkShaderModule fillVertShader;

  VkCommandBuffer current_command_buffer;

  NVGcontext *ctx;
} VKNVGcontext;


#ifdef __cplusplus
extern "C" {
#endif

void vknvg_set_current_command_buffer(VKNVGcontext *vk, VkCommandBuffer cmdBuffer);

int vknvg_maxi(int a, int b);
void vknvg_xformToMat3x4(float *m3, float *t);
NVGcolor vknvg_premulColor(NVGcolor c);
VKNVGtexture *vknvg_findTexture(VKNVGcontext *vk, int id);
VKNVGtexture *vknvg_allocTexture(VKNVGcontext *vk);
int vknvg_textureId(VKNVGcontext *vk, VKNVGtexture *tex);
int vknvg_deleteTexture(VKNVGcontext *vk, VKNVGtexture *tex);
VKNVGPipeline *vknvg_allocPipeline(VKNVGcontext *vk);
int vknvg_compareCreatePipelineKey(const VKNVGCreatePipelineKey *a, const VKNVGCreatePipelineKey *b);
VKNVGPipeline *vknvg_findPipeline(VKNVGcontext *vk, VKNVGCreatePipelineKey *pipelinekey);
VkResult vknvg_memory_type_from_properties(VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
int vknvg_convertPaint(
  VKNVGcontext *vk,
  VKNVGfragUniforms *frag,
  NVGpaint *paint,
  NVGscissor *scissor,
  float width,
  float fringe,
  float strokeThr
);

VKNVGBuffer vknvg_createBuffer(
  VkDevice device,
  VkPhysicalDeviceMemoryProperties memoryProperties,
  const VkAllocationCallbacks *allocator,
  VkBufferUsageFlags usage,
  VkMemoryPropertyFlagBits memory_type,
  void *data,
  uint32_t size
);

void vknvg_destroyBuffer(VkDevice device, const VkAllocationCallbacks *allocator, VKNVGBuffer *buffer);
void vknvg_UpdateBuffer(
  VkDevice device,
  const VkAllocationCallbacks *allocator,
  VKNVGBuffer *buffer,
  VkPhysicalDeviceMemoryProperties memoryProperties,
  VkBufferUsageFlags usage,
  VkMemoryPropertyFlagBits memory_type,
  void *data,
  uint32_t size
);

VkShaderModule vknvg_createShaderModule(VkDevice device, const void *code, size_t size, const VkAllocationCallbacks *allocator);
VkBlendFactor vknvg_NVGblendFactorToVkBlendFactor(enum NVGblendFactor factor);
VkPipelineColorBlendAttachmentState vknvg_compositOperationToColorBlendAttachmentState(NVGcompositeOperationState compositeOperation);
VkDescriptorSetLayout vknvg_createDescriptorSetLayout(VkDevice device, const VkAllocationCallbacks *allocator);
VkDescriptorPool vknvg_createDescriptorPool(VkDevice device, uint32_t count, const VkAllocationCallbacks *allocator);
VkPipelineLayout vknvg_createPipelineLayout(VkDevice device, VkDescriptorSetLayout descLayout, const VkAllocationCallbacks *allocator);
VkPipelineDepthStencilStateCreateInfo initializeDepthStencilCreateInfo(VKNVGCreatePipelineKey *pipelinekey);
VKNVGPipeline *vknvg_createPipeline(VKNVGcontext *vk, VKNVGCreatePipelineKey *pipelinekey);
VkPipeline vknvg_bindPipeline(VKNVGcontext *vk, VkCommandBuffer cmdBuffer, VKNVGCreatePipelineKey *pipelinekey);
int vknvg_UpdateTexture(VkDevice device, VKNVGtexture *tex, int dx, int dy, int w, int h, const unsigned char *data);
int vknvg_maxVertCount(const NVGpath *paths, int npaths);
VKNVGcall *vknvg_allocCall(VKNVGcontext *vk);
int vknvg_allocPaths(VKNVGcontext *vk, int n);
int vknvg_allocVerts(VKNVGcontext *vk, int n);
int vknvg_allocFragUniforms(VKNVGcontext *vk, int n);
VKNVGfragUniforms *vknvg_fragUniformPtr(VKNVGcontext *vk, int i);
void vknvg_vset(NVGvertex *vtx, float x, float y, float u, float v);
void vknvg_setUniforms(VKNVGcontext *vk, VkDescriptorSet descSet, int uniformOffset, VKNVGcall *call);
void vknvg_fill(VKNVGcontext *vk, VKNVGcall *call);
void vknvg_convexFill(VKNVGcontext *vk, VKNVGcall *call);
void vknvg_stroke(VKNVGcontext *vk, VKNVGcall *call);
void vknvg_triangles(VKNVGcontext *vk, VKNVGcall *call);
int vknvg_renderCreate(void *uptr);
int vknvg_renderCreateTexture(void *uptr, int type, int w, int h, int imageFlags, const unsigned char *data);
int vknvg_renderDeleteTexture(void *uptr, int image);
int vknvg_renderUpdateTexture(void *uptr, int image, int x, int y, int w, int h, const unsigned char *data);
int vknvg_renderGetTextureSize(void *uptr, int image, int *w, int *h);
void vknvg_renderViewport(void *uptr, int width, int height, float devicePixelRatio);
void vknvg_renderCancel(void *uptr);
void vknvg_renderFlush(void *uptr);
void vknvg_renderFill(
  void *uptr,
  NVGpaint *paint,
  NVGcompositeOperationState compositeOperation,
  NVGscissor *scissor,
  float fringe,
  const float *bounds,
  const NVGpath *paths,
  int npaths
);

void vknvg_renderStroke(
  void *uptr,
  NVGpaint *paint,
  NVGcompositeOperationState compositeOperation,
  NVGscissor *scissor,
  float fringe,
  float strokeWidth,
  const NVGpath *paths,
  int npaths
);

void vknvg_renderTriangles(
  void *uptr,
  NVGpaint *paint,
  NVGcompositeOperationState compositeOperation,
  NVGscissor *scissor,
  const NVGvertex *verts,
  int nverts,
  float fringe
);

void vknvg_renderDelete(void *uptr);
NVGcontext *nvgCreateVk(VKNVGCreateInfo createInfo, int flags);
void nvgDeleteVk(NVGcontext *ctx);

#ifdef __cplusplus
}
#endif
