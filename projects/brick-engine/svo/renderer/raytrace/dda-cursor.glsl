struct DDACursor {
  vec3 mapPos;
  vec3 rayStep;
  vec3 sideDist;
  vec3 deltaDist;
};

DDACursor dda_cursor_create(
  in const vec3 pos,
  in const vec3 mapPos,
  in const vec3 rayDir
) {
  DDACursor cursor;
  cursor.mapPos = mapPos;
  cursor.deltaDist = abs(vec3(length(rayDir)) / rayDir);
  cursor.rayStep = sign(rayDir);
  vec3 p = (cursor.mapPos - pos);
  cursor.sideDist = (
    cursor.rayStep * p + (cursor.rayStep * 0.5) + 0.5
  ) * cursor.deltaDist;
  return cursor;
}

void dda_cursor_step(in out DDACursor cursor) {
  vec3 sideDist = cursor.sideDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) *
              step(sideDist.xyz, sideDist.zxy);
  cursor.sideDist += mask * cursor.deltaDist;
  cursor.mapPos += mask * cursor.rayStep;
}

int dda_cursor_octant(const in DDACursor cursor) {
  bvec3 r = greaterThanEqual(cursor.mapPos, vec3(1.0));

  return (
    int(r.x) << 0 |
    int(r.y) << 1 |
    int(r.z) << 2
  );
}