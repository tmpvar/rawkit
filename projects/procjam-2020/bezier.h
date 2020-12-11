#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"

using namespace glm;

static vec3 bezier(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float t) {
  vec3 a = glm::lerp(p1, p2, t);
  vec3 b = glm::lerp(p2, p3, t);
  vec3 c = glm::lerp(p3, p4, t);

  return glm::lerp(
    glm::lerp(a, b, t),
    glm::lerp(b, c, t),
    t
  );
}
