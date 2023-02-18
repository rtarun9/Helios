// clang-format off
#pragma once

// Set the matrix packing to row major by default. Prevents needing to transpose matrices on the C++ side.
#pragma pack_matrix(row_major)

#define BindlessRootSignature                                                                                          \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | "                              \
    "SAMPLER_HEAP_DIRECTLY_INDEXED),"                                                                                  \
    "RootConstants(b0, num32BitConstants=64, visibility = SHADER_VISIBILITY_ALL),"                                     \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = "               \
    "TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, mipLODBias = 0.0f, minLOD = 0.0f, maxLOD = 100.0f),"     \
    "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = "                \
    "TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  mipLODBias = 0.0f, minLOD = 0.0f, maxLOD = 100.0f),"      \
    "StaticSampler(s2, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_WRAP, addressV = "               \
    "TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP,  mipLODBias = 0.0f, minLOD = 0.0f, maxLOD = 100.0f), "     \
    "StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = "              \
    "TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_WRAP,  mipLODBias = 0.0f, minLOD = 0.0f, maxLOD = 100.0f), "    \
    "StaticSampler(s4, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = "        \
    "TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP),"                                                        \
    "StaticSampler(s5, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_WRAP, addressV = "         \
    "TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP),"                                                          \
    "StaticSampler(s6, filter = FILTER_MIN_MAG_POINT_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = "        \
    "TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP), "                                                       \
    "StaticSampler(s7, filter = FILTER_MIN_MAG_POINT_MIP_LINEAR, addressU = TEXTURE_ADDRESS_WRAP, addressV = "         \
    "TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP), "                                                         \
    "StaticSampler(s8, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16), "                                             \
    "StaticSampler(s9, filter = FILTER_MIN_MAG_MIP_LINEAR,  addressU = TEXTURE_ADDRESS_BORDER, addressV = "            \
    "TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK)"
  
// Samplers
SamplerState pointClampSampler : register(s0);
SamplerState pointWrapSampler : register(s1);
SamplerState linearWrapSampler : register(s2);
SamplerState linearClampSampler : register(s3);
SamplerState minMapLinearMipPointClampSampler : register(s4);
SamplerState minMapLinearMipPointWrapSampler : register(s5);
SamplerState minMapPointMipLinearClampSampler : register(s6);
SamplerState minMapPointMipLinearWrapSampler : register(s7);
SamplerState anisotropicSampler : register(s8);
SamplerState linearClampToBorder : register(s9);

