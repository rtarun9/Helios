#define LightRootSignature \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
    "SRV(t0, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "SRV(t1, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "SRV(t2, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "CBV(b0, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)"