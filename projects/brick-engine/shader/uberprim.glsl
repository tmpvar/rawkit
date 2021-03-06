// sdUberprim with precomputed constants
float sdUnterprim(vec3 p, vec4 s, vec3 r, vec2 ba, float sz2) {
    vec3 d = abs(p) - s.xyz;
    float q = length(max(d.xy, 0.0)) + min(0.0,max(d.x,d.y)) - r.x;
    // hole support: without this line, all results are convex
#ifndef CONVEX
    q = abs(q) - s.w;
#endif

    vec2 pa = vec2(q, p.z - s.z);
    vec2 diag = pa - vec2(r.z,sz2) * clamp(dot(pa,ba), 0.0, 1.0);
    vec2 h0 = vec2(max(q - r.z,0.0),p.z + s.z);
    vec2 h1 = vec2(max(q,0.0),p.z - s.z);

    return sqrt(min(dot(diag,diag),min(dot(h0,h0),dot(h1,h1))))
        * sign(max(dot(pa,vec2(-ba.y, ba.x)), d.z)) - r.y;
}

// s: width, height, depth, thickness
// r: xy corner radius, z corner radius, bottom radius offset
float sdUberprim(vec3 p, vec4 s, vec3 r) {
    // these operations can be precomputed
    s.xy -= r.x;
#ifdef CONVEX
    r.x -= r.y;
#else
    r.x -= s.w;
    s.w -= r.y;
#endif
    s.z -= r.y;
    vec2 ba = vec2(r.z, -2.0*s.z);
    return sdUnterprim(p, s, r, ba/dot(ba,ba), ba.y);
}

// example parameters
#define SHAPE_COUNT 9.0
void getfactor (int i, out vec4 s, out vec3 r) {
    //i = 4;
    if (i == 0) { // cube
        s = vec4(1.0);
        r = vec3(0.0);
    } else if (i == 1) { // cylinder
        s = vec4(1.0);
        r = vec3(1.0,0.0,0.0);
    } else if (i == 2) { // cone
        s = vec4(0.0,0.0,1.0,1.0);
        r = vec3(0.0,0.0,1.0);
	} else if (i == 3) { // pill
        s = vec4(1.0,1.0,2.0,1.0);
        r = vec3(1.0,1.0,0.0);
    } else if (i == 4) { // sphere
        s = vec4(1.0);
        r = vec3(1.0,1.0,0.0);
    } else if (i == 5) { // pellet
        s = vec4(1.0,1.0,0.25,1.0);
        r = vec3(1.0,0.25,0.0);
    } else if (i == 6) { // torus
        s = vec4(1.0,1.0,0.25,0.25);
        r = vec3(1.0,0.25,0.0);
    } else if (i == 7) { // pipe
        s = vec4(vec3(1.0),0.25);
        r = vec3(1.0,0.1,0.0);
    } else if (i == 8) { // corridor
        s = vec4(vec3(1.0),0.25);
        r = vec3(0.1,0.1,0.0);
	}
}

float doModel( vec3 p, float time ) {
    float k = time;
    float u = smoothstep(0.0,1.0,smoothstep(0.0,1.0,fract(k)));
    int s1 = int(mod(k,SHAPE_COUNT));
    int s2 = int(mod(k+1.0,SHAPE_COUNT));

    vec4 sa,sb;
    vec3 ra,rb;
    getfactor(s1,sa,ra);
    getfactor(s2,sb,rb);
    return sdUberprim(p.zyx, mix(sa,sb,u), mix(ra,rb,u));
}