//Fast Quadric Proj: "GPU-Based Ray-Casting of Quadratic Surfaces" http://dl.acm.org/citation.cfm?id=2386396
void quadricProj(
  in vec3 osPosition,
  in float voxelSize,
  in mat4 objectToScreenMatrix,
  in vec2 screenSize,
  out vec4 position,
  out float pointSize
) {
  const vec4 quadricMat = vec4(1.0, 1.0, 1.0, -1.0);
  float sphereRadius = voxelSize * 1.732051;
  vec4 sphereCenter = vec4(osPosition.xyz, 1.0);
  mat4 modelViewProj = transpose(objectToScreenMatrix);

  mat3x4 matT = mat3x4( mat3(modelViewProj[0].xyz, modelViewProj[1].xyz, modelViewProj[3].xyz) * sphereRadius);
  matT[0].w = dot(sphereCenter, modelViewProj[0]);
  matT[1].w = dot(sphereCenter, modelViewProj[1]);
  matT[2].w = dot(sphereCenter, modelViewProj[3]);

  mat3x4 matD = mat3x4(matT[0] * quadricMat, matT[1] * quadricMat, matT[2] * quadricMat);
  vec4 eqCoefs =
      vec4(dot(matD[0], matT[2]), dot(matD[1], matT[2]), dot(matD[0], matT[0]), dot(matD[1], matT[1]))
      / dot(matD[2], matT[2]);

  vec4 outPosition = vec4(eqCoefs.x, eqCoefs.y, 0.0, 1.0);
  vec2 AABB = sqrt(eqCoefs.xy*eqCoefs.xy - eqCoefs.zw);
  AABB *= screenSize;

  position.xy = outPosition.xy;
  pointSize = max(AABB.x, AABB.y);
}


float brick_mip(
  in vec3 osPosition,
  in float voxelSize,
  in mat4 objectToScreenMatrix,
  in vec2 screenSize
) {
  vec4 position;
  float pointSize;
  quadricProj(
    osPosition,
    voxelSize,
    objectToScreenMatrix,
    screenSize,
    position,
    pointSize
  );

  return log2(pointSize);
}