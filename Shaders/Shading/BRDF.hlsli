#ifndef __BRDF_HLSLI__
#define __BRDF_HLSLI__


static const float3 BASE_DIELECTRIC_REFLECTIVITY = float3(0.04f, 0.04f, 0.04f);

// Compute the ratio of reflected light vs how much it refracts.
// As the viewing angle decreases, this ratio increases as well (quickly approaching one when angle becomes more and more oblique).
// f0 is the base reflectivity : the surface reflection at zero incidence. For non metals, it will just be a singular value in a float3 (v, v, v), but this is tinted for metals.
// Most dielectrics have a value of 0.04 as f0, but depending on how metallic a surface is it will be between 0.04 (metalness = 0) and the surface color (metalness = 1).
// cosTheta here is the angle between the halfway vector and the view direction. If the angle is 0.0, then said ratio is 1, and the light will be brightest here.
// Also acts as the kS term (where kS + kD = 1, due to energy conservation).
float3 FresnelSchlickApproximation(float3 f0, float vDotN, float roughnessFactor)
{
    return f0 + (max(float3(1.0 - roughnessFactor, 1.0 - roughnessFactor, 1.0 - roughnessFactor), f0) - f0) * pow(1.0 - vDotN, 5.0);
}

// Approximates the number of microfacts on the surface whose local normals are aligned with the half way vector. For light to reflect from the surface (diffuse or specular)
// and reach our camera, the normal and halfway vector have to be aligned. More rough a surface is, more chaotically aligned the surface normals will be, producing large and dim highlights, while very smooth surfaces
// will produce very sharp and bright highlights since majority of microfacet normals are aligned to half way vector.
// This is the GGX TrowBridge Reitx model.
float NormalDistribution(float3 normal, float3 halfWayVector, float roughnessFactor)
{
    float alpha = roughnessFactor * roughnessFactor;
    float alphaSquare = alpha * alpha;

    float nDotH = saturate(dot(normal, halfWayVector));
    
    return alphaSquare / (max(PI * pow((nDotH * nDotH * (alphaSquare - 1.0f) + 1.0f), 2.0f), MIN_FLOAT_VALUE));
}

// Geometry function : approximates the number / relative surface area of the surface which is actually visible to us.
// If the surface is rough, several microfacets could overshadow and block others, because of which the light reaching us will be occluded.
// Using Smith's method, by changing the angle, we can approximate both self shadowing and geometry obstruction.
// Source :https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// if x is viewDirection, then we are calculating geometric obstruction, and if light direction, we are calculating self shadowing.
float SchlickBeckmannGS(float3 normal, float3 x, float roughnessFactor)
{
    float k = roughnessFactor / 2.0f;
    float nDotX = saturate(dot(normal, x));
    
    return nDotX / (max((nDotX * (1.0f - k) + k), MIN_FLOAT_VALUE));
}

// Smiths method is used for approximation of geometry (both self shadowing and geometry obstruction). (ShlickGGX model).
// Uses SchlickBeckman formula to calculate both geometry obstruction, where the camera cannot see a point as some other microfacet is blocking it, or
// Self shadowing, where the light ray from a point is not able to reach the camera.
float GeometryShadowingFunction(float3 normal, float3 viewDirection, float3 lightDirection, float roughnessFactor)
{
    return SchlickBeckmannGS(normal, viewDirection, roughnessFactor) * SchlickBeckmannGS(normal, lightDirection, roughnessFactor);    
}

// BRDF = kD * diffuseBRDF + kS * specularBRDF. (Note : kS + kD = 1).
float3 BRDF(float3 normal, float3 viewDirection, float3 pixelToLightDirection, float3 albedo, float roughnessFactor, float metallicFactor)
{
    float3 halfWayVector = normalize(viewDirection + pixelToLightDirection);
    
    float3 f0 = lerp(BASE_DIELECTRIC_REFLECTIVITY, albedo.xyz, metallicFactor);

    // Using cook torrance BRDF for specular lighting.
    float3 fresnel = FresnelSchlickApproximation(f0, saturate(dot(viewDirection, halfWayVector)), roughnessFactor);
    float normalDistribution = NormalDistribution(normal, halfWayVector, roughnessFactor);
    float geometryShadowing = GeometryShadowingFunction(normal, viewDirection, pixelToLightDirection, roughnessFactor);
        
    float3 specularBRDF = (normalDistribution * geometryShadowing * fresnel) / max(4.0f * saturate(dot(viewDirection, normal)) * saturate(dot(pixelToLightDirection, normal)), MIN_FLOAT_VALUE);

    float3 kS = fresnel;

    // Metals have kD as 0.0f, so more metallic a surface is, closes kS ~ 1 and kD ~ 0.
    // Using lambertian model for diffuse light now.
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - fresnel, float3(0.0f, 0.0f, 0.0f), metallicFactor);
        
    float3 diffuseBRDF = albedo / PI;

    return (kD * diffuseBRDF + specularBRDF);
}


#endif
