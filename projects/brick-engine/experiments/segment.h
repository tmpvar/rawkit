#pragma once

#include <glm/glm.hpp>

struct Segment {
  glm::vec2 start;
  glm::vec2 end;
  glm::vec2 normal() {
    glm::vec2 n = glm::normalize(end - start);
    return glm::vec2(-n.y, n.x);
  }

  float orientation(glm::vec2 p) {
    return glm::sign(
      (this->end.y - this->start.y) *
      (p.x         - this->end.x)   -
      (this->end.x - this->start.x) *
      (p.y         - this->end.y)
    );
  }
};
