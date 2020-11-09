#version 450
#include "nested.glsl"

out float outF;

void main() {
  float outF = nested(1.23456);
}