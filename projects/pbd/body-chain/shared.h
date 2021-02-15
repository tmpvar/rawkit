#ifdef CPU
  #include <glm/glm.hpp>
  using namespace glm;
#endif

struct Scene {
  mat4 worldToScreen;
  float time;
};

struct BodyProxy {
  vec4 pos;
  vec4 rot;
  vec4 size;
};