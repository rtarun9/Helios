:: Note : This script is added since (currently) the SM 6.6 option doesnt show up on VS.
:: But, by upgrading the Windows SDK / or by using the -T flag, it should work.
:: Will use script now but can switch any time.

dxc -T vs_6_6 -E VsMain LightVS.hlsl -Fo LightVS.cso
dxc -T ps_6_6 -E PsMain LightPS.hlsl -Fo LightPS.cso

dxc -T vs_6_6 -E VsMain OffscreenRTVS.hlsl -Fo OffscreenRTVS.cso
dxc -T ps_6_6 -E PsMain OffscreenRTPS.hlsl -Fo OffscreenRTPS.cso

dxc -T vs_6_6 -E VsMain PBRVS.hlsl -Fo PBRVS.cso
dxc -T ps_6_6 -E PsMain PBRPS.hlsl -Fo PBRPS.cso

dxc -T vs_6_6 -E VsMain SkyBoxVS.hlsl -Fo SkyBoxVS.cso
dxc -T ps_6_6 -E PsMain SkyBoxPS.hlsl -Fo SkyBoxPS.cso

dxc -T vs_6_6 -E VsMain GPassVS.hlsl -Fo GPassVS.cso
dxc -T ps_6_6 -E PsMain GPassPS.hlsl -Fo GPassPS.cso

dxc -T cs_6_6 -E CsMain CubeFromEquirectTextureCS.hlsl -Fo CubeFromEquirectTextureCS.cso

dxc -T cs_6_6 -E CsMain CubeMapConvolutionCS.hlsl -Fo CubeMapConvolutionCS.cso

dxc -T cs_6_6 -E CsMain PreFilterCubeMapCS.hlsl -Fo PreFilterCubeMapCS.cso

dxc -T cs_6_6 -E CsMain BRDFConvolutionCS.hlsl -Fo BRDFConvolutionCS.cso

:: Note : This can use any shader it wants to. LightVS is a particular small shader, hence why I am using it here.
dxc -T vs_6_6 -E VsMain LightVS.hlsl -extractrootsignature -Fo BindlessRS.cso
