// shared data structures between GPU and CPU

#ifdef CPU_HOST
    #include <rawkit/rawkit.h>
    #include <glm/glm.hpp>
    using namespace glm;
#else
    #extension GL_EXT_shader_explicit_arithmetic_types: required
    typedef uint32_t u32;
    typedef uint8_t u8;
#endif

enum NodeFlags {
    LEAF = 1<<0,
};

struct InnerNode {
    u32 children[8];
};

struct LeafNode {
    u8 octants[8];
};

