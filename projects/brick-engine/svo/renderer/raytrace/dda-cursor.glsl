struct DDACursor {
  vec3 mask;
  vec3 mapPos;
  vec3 rayStep;
  vec3 sideDist;
  vec3 deltaDist;
};

DDACursor dda_cursor_create(
  in const vec3 pos,
  in const vec3 rayDir
) {
  DDACursor cursor;
  cursor.mapPos = floor(pos);
  cursor.deltaDist = abs(vec3(length(rayDir)) / rayDir);
  cursor.rayStep = sign(rayDir);
  vec3 p = (cursor.mapPos - pos);
  cursor.sideDist = (
    cursor.rayStep * p + (cursor.rayStep * 0.5) + 0.5
  ) * cursor.deltaDist;
  cursor.mask = step(cursor.sideDist.xyz, cursor.sideDist.yzx) *
                step(cursor.sideDist.xyz, cursor.sideDist.zxy);
  return cursor;
}

void dda_cursor_step(in out DDACursor cursor, out vec3 normal) {
  vec3 sideDist = cursor.sideDist;
  cursor.mask = step(sideDist.xyz, sideDist.yzx) *
                step(sideDist.xyz, sideDist.zxy);
  cursor.sideDist += cursor.mask * cursor.deltaDist;
  cursor.mapPos += cursor.mask * cursor.rayStep;
}