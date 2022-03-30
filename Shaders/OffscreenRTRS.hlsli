#define OffscreenRTRootSignature           \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
    "SRV(t0, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "SRV(t1, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "DescriptorTable(SRV(t0, space = 1, flags = DATA_STATIC, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0, space = 1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)" 
