#define PBRRootSignature           \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
    "SRV(t0, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "SRV(t1, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "SRV(t2, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC)," \
    "CBV(b0, space = 0, visibility = SHADER_VISIBILITY_VERTEX, flags = DATA_STATIC),"\
    "CBV(b0, space = 1, visibility = SHADER_VISIBILITY_PIXEL, flags = DATA_STATIC),"\
    "RootConstants(b1, num32BitConstants=8, space = 1, visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t0, space = 1, flags = DATA_STATIC, numDescriptors = 2), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0, space = 1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP)," \
    "StaticSampler(s1, space = 1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP)" 
