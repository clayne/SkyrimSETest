#include "../../../rendering/common.h"
#include "../../../../common.h"
#include "../../NiMain/NiSourceTexture.h"
#include "../../Setting.h"
#include "../../NiMain/NiDirectionalLight.h"
#include "../BSShaderManager.h"
#include "../BSShaderUtil.h"
#include "../BSLight.h"
#include "../BSShadowLight.h"
#include "BSLightingShader.h"
#include "BSLightingShaderProperty.h"

DEFINE_SHADER_DESCRIPTOR(
	"Lighting",

	// Vertex
	CONFIG_ENTRY(VS, PER_GEO, 0, row_major float3x4, World)
	CONFIG_ENTRY(VS, PER_GEO, 1, row_major float3x4, PreviousWorld)
	CONFIG_ENTRY(VS, PER_GEO, 2, float3, EyePosition)
	CONFIG_ENTRY(VS, PER_GEO, 3, float4, LandBlendParams)
	CONFIG_ENTRY(VS, PER_GEO, 4, float4, TreeParams)
	CONFIG_ENTRY(VS, PER_GEO, 5, float2, WindTimers)
	CONFIG_ENTRY(VS, PER_GEO, 6, row_major float3x4, TextureProj)
	CONFIG_ENTRY(VS, PER_GEO, 7, float, IndexScale)
	CONFIG_ENTRY(VS, PER_GEO, 8, float4, WorldMapOverlayParameters)
	CONFIG_ENTRY(VS, PER_MAT, 9, float3, LeftEyeCenter)
	CONFIG_ENTRY(VS, PER_MAT, 10, float3, RightEyeCenter)
	CONFIG_ENTRY(VS, PER_MAT, 11, float4, TexcoordOffset)
	CONFIG_ENTRY(VS, PER_TEC, 12, float4, HighDetailRange)
	CONFIG_ENTRY(VS, PER_TEC, 13, float4, FogParam)
	CONFIG_ENTRY(VS, PER_TEC, 14, float4, FogNearColor)
	CONFIG_ENTRY(VS, PER_TEC, 15, float4, FogFarColor)
	CONFIG_ENTRY(VS, PER_GEO, 16, unknown, Bones)

	/*
	// Pixel TODO
	CONFIG_ENTRY(PS, PER_GEO, 0, unknown, NumLightNumShadowLight)
	CONFIG_ENTRY(PS, PER_GEO, 1, unknown, PointLightPosition)
	CONFIG_ENTRY(PS, PER_GEO, 2, unknown, PointLightColor)
	CONFIG_ENTRY(PS, PER_GEO, 3, unknown, DirLightDirection)
	CONFIG_ENTRY(PS, PER_GEO, 4, unknown, DirLightColor)
	CONFIG_ENTRY(PS, PER_GEO, 5, unknown, DirectionalAmbient)
	CONFIG_ENTRY(PS, PER_GEO, 6, unknown, AmbientSpecularTintAndFresnelPower)
	CONFIG_ENTRY(PS, PER_GEO, 7, unknown, MaterialData)
	CONFIG_ENTRY(PS, PER_GEO, 8, unknown, EmitColor)
	CONFIG_ENTRY(PS, PER_GEO, 9, unknown, AlphaTestRef)
	CONFIG_ENTRY(PS, PER_GEO, 10, unknown, ShadowLightMaskSelect)
	CONFIG_ENTRY(PS, PER_GEO, 11, unknown, VPOSOffset)
	CONFIG_ENTRY(PS, PER_GEO, 12, unknown, ProjectedUVParams)
	CONFIG_ENTRY(PS, PER_GEO, 13, unknown, ProjectedUVParams2)
	CONFIG_ENTRY(PS, PER_GEO, 14, unknown, ProjectedUVParams3)
	CONFIG_ENTRY(PS, PER_GEO, 15, unknown, SplitDistance)
	CONFIG_ENTRY(PS, PER_GEO, 16, unknown, SSRParams)
	CONFIG_ENTRY(PS, PER_GEO, 17, unknown, WorldMapOverlayParametersPS)
	CONFIG_ENTRY(PS, PER_GEO, 18, unknown, AmbientColor)
	CONFIG_ENTRY(PS, PER_GEO, 19, unknown, FogColor)
	CONFIG_ENTRY(PS, PER_GEO, 20, unknown, ColourOutputClamp)
	CONFIG_ENTRY(PS, PER_GEO, 21, unknown, EnvmapData)
	CONFIG_ENTRY(PS, PER_GEO, 22, unknown, ParallaxOccData)
	CONFIG_ENTRY(PS, PER_GEO, 23, unknown, TintColor)
	CONFIG_ENTRY(PS, PER_GEO, 24, unknown, LODTexParams)
	CONFIG_ENTRY(PS, PER_GEO, 25, unknown, SpecularColor)
	CONFIG_ENTRY(PS, PER_GEO, 26, unknown, SparkleParams)
	CONFIG_ENTRY(PS, PER_GEO, 27, unknown, MultiLayerParallaxData)
	CONFIG_ENTRY(PS, PER_GEO, 28, unknown, LightingEffectParams)
	CONFIG_ENTRY(PS, PER_GEO, 29, unknown, IBLParams)
	CONFIG_ENTRY(PS, PER_GEO, 30, unknown, LandscapeTexture1to4IsSnow)
	CONFIG_ENTRY(PS, PER_GEO, 31, unknown, LandscapeTexture5to6IsSnow)
	CONFIG_ENTRY(PS, PER_GEO, 32, unknown, LandscapeTexture1to4IsSpecPower)
	CONFIG_ENTRY(PS, PER_GEO, 33, unknown, LandscapeTexture5to6IsSpecPower)
	CONFIG_ENTRY(PS, PER_GEO, 34, unknown, SnowRimLightParameters)
	CONFIG_ENTRY(PS, PER_GEO, 35, unknown, CharacterLightParams)
	*/
);

//
// Shader notes:
//
// - Constructor is not implemented
// - Destructor is not implemented
// - Vanilla bug fix for SetupMaterial() case RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW
// - Global variables eliminated in each Setup/Restore function
// - A lock is held in GeometrySetupMTLandExtraConstants()
// - "Bones" and "IndexScale" vertex constants are not used in the game (undefined types)
//
using namespace DirectX;
using namespace BSGraphics;

AutoPtr(NiSourceTexture *, BSShader_DefNormalMap, 0x3052920);
AutoPtr(NiSourceTexture *, BSShader_DefHeightMap, 0x3052900);
AutoPtr(NiSourceTexture *, WorldMapOverlayNormalTexture, 0x1E32F90);
AutoPtr(NiSourceTexture *, WorldMapOverlayNormalSnowTexture, 0x1E32F98);
AutoPtr(NiSourceTexture *, ProjectedNormalTexture, 0x30528A0);
AutoPtr(NiSourceTexture *, ProjectedNoiseTexture, 0x3052890);
AutoPtr(NiSourceTexture *, ProjectedDiffuseTexture, 0x3052898);
AutoPtr(NiSourceTexture *, ProjectedNormalDetailTexture, 0x30528A8);
AutoPtr(int, dword_143051B3C, 0x3051B3C);
AutoPtr(int, dword_143051B40, 0x3051B40);
AutoPtr(uintptr_t, qword_14304F260, 0x304F260);
AutoPtr(float, flt_143257C50, 0x3257C50);// fLightingOutputColourClampPostLit_General
AutoPtr(float, flt_143257C54, 0x3257C54);// fLightingOutputColourClampPostEnv_General
AutoPtr(float, flt_143257C58, 0x3257C58);// fLightingOutputColourClampPostSpec_General
AutoPtr(float, flt_141E32F54, 0x1E32F54);
AutoPtr(float, flt_141E32F58, 0x1E32F58);
AutoPtr(float, flt_141E32F5C, 0x1E32F5C);
AutoPtr(float, flt_141E32F60, 0x1E32F60);
AutoPtr(BYTE, byte_141E32FE0, 0x1E32FE0);
AutoPtr(int, dword_141E33BA0, 0x1E33BA0);
AutoPtr(BYTE, byte_141E32F66, 0x1E32F66);// bEnableCharacterLighting
AutoPtr(float, xmmword_141E3302C, 0x1E3302C);// CharacterLightingStrength { Primary, Secondary, Luminance, Max Luminance } 
AutoPtr(XMVECTORF32, xmmword_141E3301C, 0x1E3301C);
AutoPtr(int, dword_141E33040, 0x1E33040);
AutoPtr(uintptr_t, qword_1431F5810, 0x31F5810);// ImagespaceShaderManager
AutoPtr(float, flt_141E32FBC, 0x1E32FBC);
AutoPtr(XMVECTORF32, xmmword_14187D940, 0x187D940);
AutoPtr(BYTE, byte_141E32E88, 0x1E32E88);
AutoPtr(uintptr_t, qword_14304EF00, 0x304EF00);
AutoPtr(float, flt_143257C40, 0x3257C40);
AutoPtr(uint32_t, dword_141E3527C, 0x1E3527C);// Replaced by TLS
AutoPtr(uint32_t, dword_141E35280, 0x1E35280);// Replaced by TLS
AutoPtr(NiColorA, dword_1431F5540, 0x31F5540);// Unknown setting from SceneGraph
AutoPtr(NiColorA, dword_1431F5550, 0x31F5550);// 4x fMapMenuOverlayScale settings
AutoPtr(float, xmmword_141880020, 0x1880020);
AutoPtr(float, flt_141E32F40, 0x1E32F40);// Part of BSShaderManager timer array
AutoPtr(float, flt_141E32FD8, 0x1E32FD8);
AutoPtr(float, flt_141E32FB8, 0x1E32FB8);
AutoPtr(BYTE, byte_1431F547C, 0x31F547C);
AutoPtr(XMFLOAT4, xmmword_141E32FC8, 0x1E32FC8);

DefineIniSetting(bEnableSnowMask, Display);
DefineIniSetting(iLandscapeMultiNormalTilingFactor, Display);
DefineIniSetting(fSnowRimLightIntensity, Display);
DefineIniSetting(fSnowGeometrySpecPower, Display);
DefineIniSetting(fSnowNormalSpecPower, Display);
DefineIniSetting(bEnableSnowRimLighting, Display);
DefineIniSetting(fSpecMaskBegin, Display);
DefineIniSetting(fSpecMaskSpan, Display);
DefineIniSetting(bEnableProjecteUVDiffuseNormals, Display);
DefineIniSetting(bEnableProjecteUVDiffuseNormalsOnCubemap, Display);
DefineIniSetting(fProjectedUVDiffuseNormalTilingScale, Display);
DefineIniSetting(fProjectedUVNormalDetailTilingScale, Display);
DefineIniSetting(bEnableParallaxOcclusion, Display);
DefineIniSetting(iShadowMaskQuarter, Display);

thread_local uint32_t TLS_m_CurrentRawTechnique;
thread_local DepthStencilDepthMode TLS_dword_141E35280;
thread_local uint32_t TLS_dword_141E3527C;

char hookbuffer[50];

void TestHook5()
{
	memcpy(hookbuffer, (PBYTE)(g_ModuleBase + 0x1307BD0), 50);

	Detours::X64::DetourFunctionClass((PBYTE)(g_ModuleBase + 0x1307BD0), &BSLightingShader::__ctor__);
}

BSLightingShader::BSLightingShader() : BSShader(ShaderConfig.Type)
{
	uintptr_t v1 = *(uintptr_t *)((uintptr_t)this + 0x0);
	uintptr_t v2 = *(uintptr_t *)((uintptr_t)this + 0x10);
	uintptr_t v3 = *(uintptr_t *)((uintptr_t)this + 0x18);

	PatchMemory((g_ModuleBase + 0x1307BD0), (PBYTE)&hookbuffer, 50);

	auto sub_141307BD0 = (uintptr_t(__fastcall *)(BSLightingShader *))(g_ModuleBase + 0x1307BD0);
	sub_141307BD0(this);

	m_Type = BSShaderManager::BSSM_SHADER_LIGHTING;
	pInstance = this;

	*(uintptr_t *)((uintptr_t)this + 0x0) = v1;
	*(uintptr_t *)((uintptr_t)this + 0x10) = v2;
	*(uintptr_t *)((uintptr_t)this + 0x18) = v3;
}

BSLightingShader::~BSLightingShader()
{
	Assert(false);
}

bool BSLightingShader::SetupTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSLightingShader::SetupTechnique, Technique);

	// Check if shaders exist
	uint32_t rawTechnique = GetRawTechnique(Technique);
	uint32_t vertexShaderTechnique = GetVertexTechnique(rawTechnique);
	uint32_t pixelShaderTechnique = GetPixelTechnique(rawTechnique);

	if (!BeginTechnique(vertexShaderTechnique, pixelShaderTechnique, false))
		return false;

	//m_CurrentRawTechnique = rawTechnique;
	TLS_m_CurrentRawTechnique = rawTechnique;

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);

	renderer->SetTextureFilterMode(0, 3);
	renderer->SetTextureFilterMode(1, 3);

	switch ((rawTechnique >> 24) & 0x3F)
	{
	case RAW_TECHNIQUE_ENVMAP:
	case RAW_TECHNIQUE_EYE:
		renderer->SetTextureFilterMode(4, 3);
		renderer->SetTextureFilterMode(5, 3);
		break;

	case RAW_TECHNIQUE_GLOWMAP:
		renderer->SetTextureFilterMode(6, 3);
		break;

	case RAW_TECHNIQUE_PARALLAX:
	case RAW_TECHNIQUE_PARALLAXOCC:
		renderer->SetTextureFilterMode(3, 3);
		break;

	case RAW_TECHNIQUE_FACEGEN:
		renderer->SetTextureFilterMode(3, 3);
		renderer->SetTextureFilterMode(4, 3);
		renderer->SetTextureFilterMode(12, 3);
		break;

	case RAW_TECHNIQUE_MTLAND:
	case RAW_TECHNIQUE_MTLANDLODBLEND:
	{
		// Override all 16 samplers
		for (int i = 0; i < 16; i++)
		{
			renderer->SetTexture(i, BSShader_DefHeightMap->QRendererTexture());
			renderer->SetTextureMode(i, 3, 3);
		}

		renderer->SetTextureMode(15, 3, 1);
	}
	break;

	case RAW_TECHNIQUE_LODLAND:
	case RAW_TECHNIQUE_LODLANDNOISE:
		renderer->SetTextureMode(0, 3, 1);
		renderer->SetTextureMode(1, 3, 1);
		TechUpdateHighDetailRangeConstants(vertexCG);
		break;

	case RAW_TECHNIQUE_MULTILAYERPARALLAX:
		renderer->SetTextureFilterMode(4, 3);
		renderer->SetTextureFilterMode(5, 3);
		renderer->SetTextureFilterMode(8, 3);
		break;

	case RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW:
		renderer->SetTexture(10, ProjectedNormalTexture->QRendererTexture());
		renderer->SetTextureMode(10, 3, 0);
		break;
	}

	TechUpdateFogConstants(vertexCG, pixelCG);

	// PS: p20 float4 ColourOutputClamp
	{
		XMVECTORF32& colourOutputClamp = pixelCG.ParamPS<XMVECTORF32, 20>();

		colourOutputClamp.f[0] = flt_143257C50;// Initial value is fLightingOutputColourClampPostLit
		colourOutputClamp.f[1] = flt_143257C54;// Initial value is fLightingOutputColourClampPostEnv
		colourOutputClamp.f[2] = flt_143257C58;// Initial value is fLightingOutputColourClampPostSpec
		colourOutputClamp.f[3] = 0.0f;
	}

	bool shadowed = (rawTechnique & RAW_FLAG_SHADOW_DIR) || (rawTechnique & (RAW_FLAG_UNKNOWN6 | RAW_FLAG_UNKNOWN5 | RAW_FLAG_UNKNOWN4));
	bool defShadow = (rawTechnique & RAW_FLAG_DEFSHADOW);

	// NOTE: A use-after-free has been eliminated. Constants are flushed AFTER this code block now.
	if (shadowed && defShadow)
	{
		renderer->SetShaderResource(14, (ID3D11ShaderResourceView *)qword_14304F260);
		renderer->SetTextureMode(14, 0, (iShadowMaskQuarter->uValue.i != 4) ? 1 : 0);

		// PS: p11 float4 VPOSOffset
		XMVECTORF32& vposOffset = pixelCG.ParamPS<XMVECTORF32, 11>();

		vposOffset.f[0] = 1.0f / (float)dword_143051B3C;
		vposOffset.f[1] = 1.0f / (float)dword_143051B40;
		vposOffset.f[2] = 0.0f;
		vposOffset.f[3] = 0.0f;
	}

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_TECHNIQUE);
	return true;
}

void BSLightingShader::RestoreTechnique(uint32_t Technique)
{
	BSSHADER_FORWARD_CALL(TECHNIQUE, &BSLightingShader::RestoreTechnique, Technique);

	auto *renderer = BSGraphics::Renderer::GetGlobals();

	if (TLS_m_CurrentRawTechnique & RAW_FLAG_DEFSHADOW)
		renderer->SetShaderResource(14, nullptr);

	uintptr_t v2 = *(uintptr_t *)(qword_1431F5810 + 448);

	if (v2 && *(bool *)(v2 + 16) && *(bool *)(v2 + 17))
		renderer->SetShaderResource(15, nullptr);

	EndTechnique();
}

void BSLightingShader::SetupMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSLightingShader::SetupMaterial, Material);

	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);

	const uint32_t rawTechnique = TLS_m_CurrentRawTechnique;
	const uint32_t baseTechniqueID = (rawTechnique >> 24) & 0x3F;
	bool setDiffuseNormalSamplers = true;

	const uintptr_t v5 = (uintptr_t)this;
	const uintptr_t material = (uintptr_t)Material;

	switch (baseTechniqueID)
	{
	// BSLightingShaderMaterialEnvmap
	case RAW_TECHNIQUE_ENVMAP:
	{
		sub_14130C470(*(uintptr_t *)(material + 160), material);
		sub_14130C4D0(*(uintptr_t *)(material + 168), material);

		// PS: p21 float4 EnvmapData
		XMVECTORF32& envmapData = pixelCG.ParamPS<XMVECTORF32, 21>();

		envmapData.f[0] = *(float *)(material + 176);
		envmapData.f[1] = (*(uintptr_t *)(material + 168)) ? 1.0f : 0.0f;
	}
	break;

	// BSLightingShaderMaterialGlowmap
	case RAW_TECHNIQUE_GLOWMAP:
		sub_14130C220(6, *(uintptr_t *)(material + 160), material);
		break;

	// BSLightingShaderMaterialParallax
	case RAW_TECHNIQUE_PARALLAX:
		sub_14130C220(3, *(uintptr_t *)(material + 160), material);
		break;

	// BSLightingShaderMaterialFacegen
	case RAW_TECHNIQUE_FACEGEN:
		sub_14130C220(3, *(uintptr_t *)(material + 160), material);
		sub_14130C220(4, *(uintptr_t *)(material + 168), material);
		sub_14130C220(12, *(uintptr_t *)(material + 176), material);
		break;

	// BSLightingShaderMaterialFacegenTint
	// BSLightingShaderMaterialHairTint
	case RAW_TECHNIQUE_FACEGENRGBTINT:
	case RAW_TECHNIQUE_HAIR:
	{
		// PS: p23 float4 TintColor
		XMVECTORF32& tintColor = pixelCG.ParamPS<XMVECTORF32, 23>();

		tintColor.f[0] = *(float *)(material + 160);
		tintColor.f[1] = *(float *)(material + 164);
		tintColor.f[2] = *(float *)(material + 168);
	}
	break;

	// BSLightingShaderMaterialParallaxOcc
	case RAW_TECHNIQUE_PARALLAXOCC:
	{
		// PS: p22 float4 ParallaxOccData
		XMVECTORF32& parallaxOccData = pixelCG.ParamPS<XMVECTORF32, 22>();

		sub_14130C220(3, *(uintptr_t *)(material + 160), material);

		parallaxOccData.f[0] = *(float *)(material + 172);// Reversed on purpose?
		parallaxOccData.f[1] = *(float *)(material + 168);// Reversed on purpose?
	}
	break;

	case RAW_TECHNIQUE_MTLAND:
	case RAW_TECHNIQUE_MTLANDLODBLEND:
	{
		// PS: p24 float4 LODTexParams
		{
			XMVECTORF32& lodTexParams = pixelCG.ParamPS<XMVECTORF32, 24>();

			lodTexParams.f[0] = *(float *)(material + 328);
			lodTexParams.f[1] = *(float *)(material + 332);
			lodTexParams.f[2] = (byte_141E32E88) ? 1.0f : 0.0f;
			lodTexParams.f[3] = *(float *)(material + 336);
		}

		MatSetMultiTextureLandOverrides(material);

		if (*(uintptr_t *)(material + 0x100))
			sub_14130C220(15, *(uintptr_t *)(material + 0x100), material);

		setDiffuseNormalSamplers = false;

		// PS: p32 float4 LandscapeTexture1to4IsSpecPower
		{
			XMVECTORF32& landscapeTexture1to4IsSpecPower = pixelCG.ParamPS<XMVECTORF32, 32>();

			landscapeTexture1to4IsSpecPower.f[0] = *(float *)(material + 304);
			landscapeTexture1to4IsSpecPower.f[1] = *(float *)(material + 308);
			landscapeTexture1to4IsSpecPower.f[2] = *(float *)(material + 312);
			landscapeTexture1to4IsSpecPower.f[3] = *(float *)(material + 316);
		}

		// PS: p33 float4 LandscapeTexture5to6IsSpecPower
		{
			XMVECTORF32& landscapeTexture5to6IsSpecPower = pixelCG.ParamPS<XMVECTORF32, 33>();

			landscapeTexture5to6IsSpecPower.f[0] = *(float *)(material + 320);
			landscapeTexture5to6IsSpecPower.f[1] = *(float *)(material + 324);
			landscapeTexture5to6IsSpecPower.f[2] = 0.0f;
			landscapeTexture5to6IsSpecPower.f[3] = 0.0f;
		}

		if (rawTechnique & RAW_FLAG_SNOW)
		{
			// PS: p30 float4 LandscapeTexture1to4IsSnow
			XMVECTORF32& landscapeTexture1to4IsSnow = pixelCG.ParamPS<XMVECTORF32, 30>();

			landscapeTexture1to4IsSnow.f[0] = *(float *)(material + 280);
			landscapeTexture1to4IsSnow.f[1] = *(float *)(material + 284);
			landscapeTexture1to4IsSnow.f[2] = *(float *)(material + 288);
			landscapeTexture1to4IsSnow.f[3] = *(float *)(material + 292);

			// PS: p31 float4 LandscapeTexture5to6IsSnow
			XMVECTORF32& LandscapeTexture5to6IsSnow = pixelCG.ParamPS<XMVECTORF32, 31>();

			LandscapeTexture5to6IsSnow.f[0] = *(float *)(material + 296);
			LandscapeTexture5to6IsSnow.f[1] = *(float *)(material + 300);
			LandscapeTexture5to6IsSnow.f[2] = (bEnableSnowMask->uValue.b) ? 1.0f : 0.0f;
			LandscapeTexture5to6IsSnow.f[3] = 1.0f / iLandscapeMultiNormalTilingFactor->uValue.i;
		}
	}
	break;

	// BSLightingShaderMaterialLODLandscape
	case RAW_TECHNIQUE_LODLAND:
	case RAW_TECHNIQUE_LODLANDNOISE:
	{
		// PS: p24 float4 LODTexParams
		{
			XMVECTORF32& lodTexParams = pixelCG.ParamPS<XMVECTORF32, 24>();

			lodTexParams.f[0] = *(float *)(material + 184);
			lodTexParams.f[1] = *(float *)(material + 188);
			lodTexParams.f[2] = (byte_141E32E88) ? 1.0f : 0.0f;
			lodTexParams.f[3] = *(float *)(material + 192);
		}

		if (baseTechniqueID == RAW_TECHNIQUE_LODLANDNOISE)
		{
			if (*(uintptr_t *)(material + 176))
				sub_14130C220(15, *(uintptr_t *)(material + 176), material);

			renderer->SetTextureMode(15, 3, 1);
		}
	}
	break;

	// BSLightingShaderMaterialMultiLayerParallax
	case RAW_TECHNIQUE_MULTILAYERPARALLAX:
	{
		sub_14130C220(8, *(uintptr_t *)(material + 160), material);

		// PS: p27 float4 MultiLayerParallaxData
		{
			XMVECTORF32& multiLayerParallaxData = pixelCG.ParamPS<XMVECTORF32, 27>();

			multiLayerParallaxData.f[0] = *(float *)(material + 184);
			multiLayerParallaxData.f[1] = *(float *)(material + 188);
			multiLayerParallaxData.f[2] = *(float *)(material + 192);
			multiLayerParallaxData.f[3] = *(float *)(material + 196);
		}

		sub_14130C470(*(uintptr_t *)(material + 168), material);
		sub_14130C4D0(*(uintptr_t *)(material + 176), material);

		// PS: p21 float4 EnvmapData
		{
			XMVECTORF32& envmapData = pixelCG.ParamPS<XMVECTORF32, 21>();

			envmapData.f[0] = *(float *)(material + 200);
			envmapData.f[1] = (*(uintptr_t *)(material + 176)) ? 1.0f : 0.0f;
		}
	}
	break;

	case RAW_TECHNIQUE_MULTIINDEXTRISHAPESNOW:
	{
		// PS: p26 float4 SparkleParams
		XMVECTORF32& sparkleParams = pixelCG.ParamPS<XMVECTORF32, 26>();

		// if (is BSLightingShaderMaterialSnow RTTI instance)
		if (*(uintptr_t *)(*(uintptr_t *)(material) - 8) == (g_ModuleBase + 0x19AA838))
		{
			sparkleParams.f[0] = *(float *)(material + 160);
			sparkleParams.f[1] = *(float *)(material + 164);
			sparkleParams.f[2] = *(float *)(material + 168);
			sparkleParams.f[3] = *(float *)(material + 172);
		}
		else
		{
			// Bug fix: Without the RTTI check, this would read out-of-bounds data causing a crash or
			// simply invalid values
			sparkleParams.v = _mm_set1_ps(1.0f);
		}
	}
	break;

	// BSLightingShaderMaterialEye
	case RAW_TECHNIQUE_EYE:
	{
		sub_14130C470(*(uintptr_t *)(material + 160), material);
		sub_14130C4D0(*(uintptr_t *)(material + 168), material);

		// VS: p9 float4 LeftEyeCenter
		{
			XMVECTORF32& leftEyeCenter = vertexCG.ParamVS<XMVECTORF32, 9>();

			leftEyeCenter.f[0] = *(float *)(material + 180);
			leftEyeCenter.f[1] = *(float *)(material + 184);
			leftEyeCenter.f[2] = *(float *)(material + 188);
		}

		// VS: p10 float4 RightEyeCenter
		{
			XMVECTORF32& rightEyeCenter = vertexCG.ParamVS<XMVECTORF32, 10>();

			rightEyeCenter.f[0] = *(float *)(material + 192);
			rightEyeCenter.f[1] = *(float *)(material + 196);
			rightEyeCenter.f[2] = *(float *)(material + 200);
		}

		// PS: p21 float4 EnvmapData
		{
			XMVECTORF32& envmapData = pixelCG.ParamPS<XMVECTORF32, 21>();

			envmapData.f[0] = *(float *)(material + 176);
			envmapData.f[1] = (*(uintptr_t *)(material + 168)) ? 1.0f : 0.0f;
		}
	}
	break;
	}

	// VS: p11 float4 TexcoordOffset
	{
		XMVECTORF32& texcoordOffset = vertexCG.ParamVS<XMVECTORF32, 11>();

		texcoordOffset.f[0] = *(float *)(material + 8i64 * (unsigned int)dword_141E33040 + 12);
		texcoordOffset.f[1] = *(float *)(material + 8i64 * (unsigned int)dword_141E33040 + 16);
		texcoordOffset.f[2] = *(float *)(material + 8i64 * (unsigned int)dword_141E33040 + 28);
		texcoordOffset.f[3] = *(float *)(material + 8i64 * (unsigned int)dword_141E33040 + 32);
	}

	if (rawTechnique & RAW_FLAG_SPECULAR)
	{
		// PS: p25 float4 SpecularColor
		XMVECTORF32& specularColor = pixelCG.ParamPS<XMVECTORF32, 25>();

		specularColor.f[0] = *(float *)(material + 56) * *(float *)(material + 0x8C);
		specularColor.f[1] = *(float *)(material + 60) * *(float *)(material + 0x8C);
		specularColor.f[2] = *(float *)(material + 64) * *(float *)(material + 0x8C);
		specularColor.f[3] = *(float *)(material + 136);

		if (rawTechnique & RAW_FLAG_MODELSPACENORMALS)
		{
			sub_14130C220(2, *(uintptr_t *)(material + 104), material);

			uint32_t v68 = *(uint32_t *)(material + 112);
			NiSourceTexture *v69 = *(NiSourceTexture **)(material + 104);

			renderer->SetTexture(2, v69->QRendererTexture());
			renderer->SetTextureMode(2, v68, 3);
		}
	}

	if (rawTechnique & RAW_FLAG_AMBIENT_SPECULAR)
	{
		// PS: p6 float4 AmbientSpecularTintAndFresnelPower
		pixelCG.ParamPS<XMVECTORF32, 6>() = xmmword_141E3301C;
	}

	if (rawTechnique & RAW_FLAG_SOFT_LIGHTING)
	{
		uint32_t v71 = *(uint32_t *)(material + 112);
		NiSourceTexture *v72 = *(NiSourceTexture **)(material + 96);

		renderer->SetTexture(12, v72->QRendererTexture());
		renderer->SetTextureAddressMode(12, v71);

		// PS: p28 float4 LightingEffectParams
		XMVECTORF32& lightingEffectParams = pixelCG.ParamPS<XMVECTORF32, 28>();

		lightingEffectParams.f[0] = *(float *)(material + 144);
		lightingEffectParams.f[1] = *(float *)(material + 148);
	}

	if (rawTechnique & RAW_FLAG_RIM_LIGHTING)
	{
		// I guess this is identical to RAW_FLAG_SOFT_LIGHTING above
		uint32_t v76 = *(uint32_t *)(material + 112);
		NiSourceTexture *v77 = *(NiSourceTexture **)(material + 96);

		renderer->SetTexture(12, v77->QRendererTexture());
		renderer->SetTextureAddressMode(12, v76);

		// PS: p28 float4 LightingEffectParams
		XMVECTORF32& lightingEffectParams = pixelCG.ParamPS<XMVECTORF32, 28>();

		lightingEffectParams.f[0] = *(float *)(material + 144);
		lightingEffectParams.f[1] = *(float *)(material + 148);
	}

	if (rawTechnique & RAW_FLAG_BACK_LIGHTING)
	{
		uint32_t v81 = *(uint32_t *)(material + 112);
		NiSourceTexture *v82 = *(NiSourceTexture **)(material + 104);

		renderer->SetTexture(9, v82->QRendererTexture());
		renderer->SetTextureAddressMode(9, v81);
	}

	if (rawTechnique & RAW_FLAG_SNOW)
	{
		// PS: p34 float4 SnowRimLightParameters
		XMVECTORF32& snowRimLightParameters = pixelCG.ParamPS<XMVECTORF32, 34>();

		snowRimLightParameters.f[0] = fSnowRimLightIntensity->uValue.f;
		snowRimLightParameters.f[1] = fSnowGeometrySpecPower->uValue.f;
		snowRimLightParameters.f[2] = fSnowNormalSpecPower->uValue.f;
		snowRimLightParameters.f[3] = bEnableSnowRimLighting->uValue.b ? 1.0f : 0.0f;
	}

	if (setDiffuseNormalSamplers)
	{
		int v88 = *(int *)(material + 80);

		if (v88 == -1)
		{
			sub_14130C220(0, *(uintptr_t *)(material + 72), material);
		}
		else
		{
			uint32_t v89 = *(uint32_t *)(material + 112);
			auto v90 = (ID3D11ShaderResourceView *)*(&qword_14304EF00 + 6 * v88);

			renderer->SetShaderResource(0, v90);
			renderer->SetTextureAddressMode(0, v89);
		}

		uint32_t v91 = *(uint32_t *)(material + 112);
		NiSourceTexture *v92 = *(NiSourceTexture **)(material + 88);

		renderer->SetTexture(1, v92->QRendererTexture());
		renderer->SetTextureAddressMode(1, v91);
	}

	// PS: p29 float4 IBLParams
	{
		XMVECTORF32& iblParams = pixelCG.ParamPS<XMVECTORF32, 29>();

		XMVECTORF32 thing;

		if (*(bool *)(v5 + 240))
			thing = *(XMVECTORF32 *)(v5 + 208);
		else
			thing = *(XMVECTORF32 *)(v5 + 224);

		iblParams.f[0] = *(float *)(v5 + 204);
		iblParams.f[1] = thing.f[0];
		iblParams.f[2] = thing.f[1];
		iblParams.f[3] = thing.f[2];
	}

	if (rawTechnique & RAW_FLAG_CHARACTER_LIGHT)
	{
		if (dword_141E33BA0 >= 0)
		{
			auto v99 = (ID3D11ShaderResourceView *)*(&qword_14304EF00 + 6 * (signed int)sub_141314170(g_ModuleBase + 0x1E33B98));

			renderer->SetShaderResource(11, v99);
			renderer->SetTextureAddressMode(11, 0);
		}

		// PS: p35 float4 CharacterLightParams
		XMVECTORF32& characterLightParams = pixelCG.ParamPS<XMVECTORF32, 35>();

		// if (bEnableCharacterRimLighting)
		if (byte_141E32F66)
			characterLightParams.v = _mm_loadu_ps(&xmmword_141E3302C);
	}

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_MATERIAL);
}

void BSLightingShader::RestoreMaterial(BSShaderMaterial const *Material)
{
	BSSHADER_FORWARD_CALL(MATERIAL, &BSLightingShader::RestoreMaterial, Material);
}

// BSUtilities::GetInverseWorldMatrix(const NiTransform& Transform, bool UseInputTransform, D3DMATRIX& Matrix)
void GetInverseWorldMatrix(const NiTransform& Transform, bool UseWorldPosition, XMMATRIX& OutMatrix)
{
	if (UseWorldPosition)
	{
		const NiPoint3 posAdjust = BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust;

		// XMMatrixIdentity(), row[3] = { world.x, world.y, world.z, 1.0f }, XMMatrixInverse()
		OutMatrix = XMMatrixInverse(nullptr, XMMatrixTranslation(posAdjust.x, posAdjust.y, posAdjust.z));
	}
	else
	{
		NiTransform inverted;
		Transform.Invert(inverted);

		OutMatrix = BSShaderUtil::GetXMFromNiPosAdjust(inverted, NiPoint3::ZERO);
	}
}

void BSLightingShader::SetupGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSLightingShader::SetupGeometry, Pass, RenderFlags);

	auto property = static_cast<BSLightingShaderProperty *>(Pass->m_Property);
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	auto vertexCG = renderer->GetShaderConstantGroup(renderer->m_CurrentVertexShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
	auto pixelCG = renderer->GetShaderConstantGroup(renderer->m_CurrentPixelShader, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);

	const uint32_t rawTechnique = TLS_m_CurrentRawTechnique;
	const uint32_t baseTechniqueID = (rawTechnique >> 24) & 0x3F;

	bool drawInWorldSpace = (rawTechnique & RAW_FLAG_SKINNED) == 0;
	bool updateEyePosition = false;
	bool isLOD = false;

	uint8_t v12 = Pass->Byte1C;
	uint8_t v103 = (unsigned __int8)(v12 - 2) <= 1u;

	if (v12 == 3 && property->GetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_WRITE))
	{
		TLS_dword_141E3527C = renderer->AlphaBlendStateGetUnknown2();
		renderer->AlphaBlendStateSetUnknown2(1);
	}

	switch (baseTechniqueID)
	{
	case RAW_TECHNIQUE_ENVMAP:
	case RAW_TECHNIQUE_MULTILAYERPARALLAX:
	case RAW_TECHNIQUE_EYE:
	{
		// NOTE: The game updates view projection twice...? See the if() after this switch.
		if (drawInWorldSpace)
			GeometrySetupViewProjection(vertexCG, Pass->m_Geometry->GetWorldTransform(), false, nullptr);

		drawInWorldSpace = false;
		updateEyePosition = true;

		// PS: p7 float4 MaterialData (NOTE: This is written AGAIN)
		auto& var = pixelCG.ParamPS<XMVECTORF32, 7>();
		var.f[0] = property->fEnvmapLODFade;
	}
	break;

	case RAW_TECHNIQUE_MTLAND:
	case RAW_TECHNIQUE_MTLANDLODBLEND:
		GeometrySetupMTLandExtraConstants(
			vertexCG,
			Pass->m_Geometry->GetWorldTranslate(),
			*(float *)((uintptr_t)property->pMaterial + 264i64),
			*(float *)((uintptr_t)property->pMaterial + 268i64));
		break;

	case RAW_TECHNIQUE_LODLAND:
	case RAW_TECHNIQUE_LODOBJ:
	case RAW_TECHNIQUE_LODOBJHD:
		GeometrySetupViewProjection(vertexCG, Pass->m_Geometry->GetWorldTransform(), false, nullptr);
		GeometrySetupViewProjection(vertexCG, Pass->m_Geometry->GetWorldTransform(), true, &renderer->m_PreviousPosAdjust);
		isLOD = true;
		break;

	case RAW_TECHNIQUE_TREE:
		GeometrySetupTreeAnimConstants(vertexCG, property);
		break;
	}

	if (drawInWorldSpace && !isLOD)
	{
		// Unknown renderer flag: Determines if previous world projection is used
		const NiTransform& world = Pass->m_Geometry->GetWorldTransform();
		const NiTransform& temp = (RenderFlags & 0x10) ? world : Pass->m_Geometry->GetPreviousWorldTransform();

		GeometrySetupViewProjection(vertexCG, world, false, nullptr);
		GeometrySetupViewProjection(vertexCG, temp, true, &renderer->m_PreviousPosAdjust);
	}

	XMMATRIX inverseWorldMatrix;
	GetInverseWorldMatrix(Pass->m_Geometry->GetWorldTransform(), false, inverseWorldMatrix);

	GeometrySetupDirectionalLights(pixelCG, Pass, inverseWorldMatrix, drawInWorldSpace);
	GeometrySetupAmbientLights(pixelCG, Pass->m_Geometry->GetWorldTransform(), drawInWorldSpace);

	// PS: p7 float4 MaterialData (Write #1)
	{
		XMVECTORF32& materialData = pixelCG.ParamPS<XMVECTORF32, 7>();

		if (Pass->m_Lod.SingleLevel)
			materialData.f[2] = property->GetAlpha() * *(float *)((uintptr_t)property->pFadeNode + 332i64);
		else
			materialData.f[2] = property->GetAlpha();
	}

	GeometrySetupEmitColorConstants(pixelCG, property);

	uint32_t lightCount = (rawTechnique >> 3) & 0b111;		// 0 - 7
	uint32_t shadowLightCount = (rawTechnique >> 6) & 0b111;// 0 - 7

	// PS: p0 float2 NumLightNumShadowLight
	{
		XMFLOAT2& numLightNumShadowLight = pixelCG.ParamPS<XMFLOAT2, 0>();

		numLightNumShadowLight.x = (float)lightCount;
		numLightNumShadowLight.y = (float)shadowLightCount;
	}

	//
	// PS: p1 float4 PointLightPosition[7]
	// PS: p2 float4 PointLightColor[7]
	//
	// Original code just memset()'s these but it's already zeroed now. The values get
	// set up in the relevant function (GeometrySetupConstantPointLights).
	//
	//{
	//	auto& pointLightPosition = pixelCG.Param<XMVECTOR[7], 1>(ps);
	//	auto& pointLightColor = pixelCG.Param<XMVECTOR[7], 2>(ps);
	//
	//	memset(&pointLightPosition, 0, sizeof(XMVECTOR) * 7);
	//	memset(&pointLightColor, 0, sizeof(XMVECTOR) * 7);
	//}
	//

	if (lightCount > 0)
	{
		float specularScale = Pass->m_Geometry->GetWorldTransform().m_fScale;

		if (baseTechniqueID == RAW_TECHNIQUE_ENVMAP || baseTechniqueID == RAW_TECHNIQUE_EYE)
			specularScale = 1.0f;

		GeometrySetupConstantPointLights(pixelCG, Pass, inverseWorldMatrix, lightCount, shadowLightCount, specularScale, drawInWorldSpace);
	}

	if (rawTechnique & RAW_FLAG_SPECULAR)
	{
		// PS: p7 float4 MaterialData (Write #2)
		XMVECTORF32& materialData = pixelCG.ParamPS<XMVECTORF32, 7>();

		materialData.f[1] = property->fSpecularLODFade;
		updateEyePosition = true;
	}

	if (rawTechnique & (RAW_FLAG_SOFT_LIGHTING | RAW_FLAG_RIM_LIGHTING | RAW_FLAG_BACK_LIGHTING | RAW_FLAG_AMBIENT_SPECULAR))
		updateEyePosition = true;

	if ((rawTechnique & RAW_FLAG_PROJECTED_UV) && (baseTechniqueID != RAW_TECHNIQUE_HAIR))
	{
		bool enableProjectedUvNormals = bEnableProjecteUVDiffuseNormals->uValue.b && (!(RenderFlags & 0x8) || !bEnableProjecteUVDiffuseNormalsOnCubemap->uValue.b);

		renderer->SetTexture(11, ProjectedNoiseTexture->QRendererTexture());
		renderer->SetTextureMode(11, 3, 1);

		if (enableProjectedUvNormals && ProjectedDiffuseTexture)
		{
			renderer->SetTexture(3, ProjectedDiffuseTexture->QRendererTexture());
			renderer->SetTextureMode(3, 3, 1);

			renderer->SetTexture(8, ProjectedNormalTexture->QRendererTexture());
			renderer->SetTextureMode(8, 3, 1);

			renderer->SetTexture(10, ProjectedNormalDetailTexture->QRendererTexture());
			renderer->SetTextureMode(10, 3, 1);
		}

		// IDA says there are 2 args to this virtual function, it's probably wrong
		if ((*(__int64(__fastcall **)(BSGeometry *))(*(uintptr_t *)Pass->m_Geometry + 424i64))(Pass->m_Geometry))
		{
			float *v60 = (float *)Pass->m_Geometry;

			// VS: p6 float3x4 TextureProj
			float *textureProj = &vertexCG.ParamVS<float, 6>();

			textureProj[0] = v60[91];
			textureProj[1] = v60[95];
			textureProj[2] = v60[99];
			textureProj[3] = v60[103];

			textureProj[4] = v60[92];
			textureProj[5] = v60[96];
			textureProj[6] = v60[100];
			textureProj[7] = v60[104];

			textureProj[8] = v60[93];
			textureProj[9] = v60[97];
			textureProj[10] = v60[101];
			textureProj[11] = v60[105];

			GeometrySetupProjectedUv(pixelCG, Pass->m_Geometry, property, enableProjectedUvNormals);
		}
		else
		{
			XMMATRIX outputTemp;
			sub_14130C8A0(Pass->m_Geometry->GetWorldTransform(), outputTemp, baseTechniqueID == RAW_TECHNIQUE_ENVMAP);

			// VS: p6 float3x4 textureProj
			BSShaderUtil::TransposeStoreMatrix3x4(&vertexCG.ParamVS<float, 6>(), outputTemp);

			GeometrySetupProjectedUv(pixelCG, nullptr, property, enableProjectedUvNormals);
		}
	}

	if (rawTechnique & RAW_FLAG_WORLD_MAP)
	{
		renderer->SetTexture(12, WorldMapOverlayNormalTexture->QRendererTexture());
		renderer->SetTextureAddressMode(12, 3);

		renderer->SetTexture(13, WorldMapOverlayNormalSnowTexture->QRendererTexture());
		renderer->SetTextureAddressMode(13, 3);

		// VS: p8 float4 WorldMapOverlayParameters
		BSGraphics::Utility::CopyNiColorAToFloat(&vertexCG.ParamVS<XMVECTOR, 8>(), dword_1431F5540);

		// PS: p17 float4 WorldMapOverlayParametersPS
		BSGraphics::Utility::CopyNiColorAToFloat(&pixelCG.ParamPS<XMVECTOR, 17>(), dword_1431F5550);
	}

	// VS: p2 float3 EyePosition
	if (updateEyePosition)
	{
		XMFLOAT3& eyePosition = vertexCG.ParamVS<XMFLOAT3, 2>();

		if (drawInWorldSpace)
		{
			float *v83 = (float *)BSShaderManager::GetCurrentAccumulator();
			XMVECTOR coord = XMVectorSet(v83[91], v83[92], v83[93], 0.0f);

			XMStoreFloat3(&eyePosition, XMVector3TransformCoord(coord, inverseWorldMatrix));
		}
		else
		{
			float *v86 = (float *)BSShaderManager::GetCurrentAccumulator();

			eyePosition.x = v86[91] - renderer->m_CurrentPosAdjust.x;
			eyePosition.y = v86[92] - renderer->m_CurrentPosAdjust.y;
			eyePosition.z = v86[93] - renderer->m_CurrentPosAdjust.z;
		}
	}

	if (Pass->Byte1C == 10)
	{
		uintptr_t v89 = (uintptr_t)property->pFadeNode;
		float v90;
		
		if (Pass->m_Lod.SingleLevel)
			v90 = *(float *)(v89 + 332) * 31.0f;
		else
			v90 = *(float *)(v89 + 304) * 31.0f;

		renderer->DepthStencilStateSetStencilMode(11, (uint32_t)v90 & 0xFF);
	}

	if (!v103)
	{
		auto oldDepthMode = renderer->DepthStencilStateGetDepthMode();

		if (!property->GetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_WRITE))
		{
			TLS_dword_141E35280 = oldDepthMode;
			renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_TEST);
		}

		if (!property->GetFlag(BSShaderProperty::BSSP_FLAG_ZBUFFER_TEST))
		{
			TLS_dword_141E35280 = oldDepthMode;
			renderer->DepthStencilStateSetDepthMode(DEPTH_STENCIL_DEPTH_MODE_DISABLED);
		}
	}

	// PS: p16 float4 SSRParams
	{
		XMVECTORF32& ssrParams = pixelCG.ParamPS<XMVECTORF32, 16>();

		ssrParams.f[0] = fSpecMaskBegin->uValue.f;
		ssrParams.f[1] = fSpecMaskSpan->uValue.f + fSpecMaskBegin->uValue.f;
		ssrParams.f[2] = flt_143257C40;

		float v98 = 0.0f;
		float v99 = 0.0f;

		if (rawTechnique & RAW_FLAG_SPECULAR)
			v99 = property->fSpecularLODFade;

		if ((RenderFlags & 2) == 0)
			v98 = 1.0f;

		ssrParams.f[3] = v98 * v99;
	}

	renderer->FlushConstantGroupVSPS(&vertexCG, &pixelCG);
	renderer->ApplyConstantGroupVSPS(&vertexCG, &pixelCG, BSGraphics::CONSTANT_GROUP_LEVEL_GEOMETRY);
}

void BSLightingShader::RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags)
{
	BSSHADER_FORWARD_CALL(GEOMETRY, &BSLightingShader::RestoreGeometry, Pass, RenderFlags);

	auto *renderer = BSGraphics::Renderer::GetGlobals();

	if (Pass->Byte1C == 10)
		renderer->DepthStencilStateSetStencilMode(DEPTH_STENCIL_STENCIL_MODE_DEFAULT, 255);

	if (TLS_dword_141E35280 != DEPTH_STENCIL_DEPTH_MODE_TESTGREATER)
	{
		renderer->DepthStencilStateSetDepthMode(TLS_dword_141E35280);
		TLS_dword_141E35280 = DEPTH_STENCIL_DEPTH_MODE_TESTGREATER;
	}

	if (TLS_dword_141E3527C != 13)
	{
		renderer->AlphaBlendStateSetUnknown2(TLS_dword_141E3527C);
		TLS_dword_141E3527C = 13;
	}
}

uint32_t BSLightingShader::GetRawTechnique(uint32_t Technique)
{
	uint32_t outputTech = Technique - 0x4800002D;
	uint32_t subIndex = (outputTech >> 24) & 0x3F;

	if (subIndex == RAW_TECHNIQUE_LODLANDNOISE && !BSShaderManager::bLODLandscapeNoise)
	{
		outputTech = outputTech & 0xC9FFFFFF | 0x9000000;
	}
	else if (subIndex == RAW_TECHNIQUE_PARALLAXOCC && !bEnableParallaxOcclusion->uValue.b)
	{
		outputTech &= 0xC0FFFFFF;
	}

	return outputTech;
}

uint32_t BSLightingShader::GetVertexTechnique(uint32_t RawTechnique)
{
	uint32_t flags = RawTechnique & (
		RAW_FLAG_VC |
		RAW_FLAG_SKINNED |
		RAW_FLAG_MODELSPACENORMALS |
		RAW_FLAG_PROJECTED_UV |
		RAW_FLAG_WORLD_MAP);

	if (RawTechnique & (RAW_FLAG_SPECULAR | RAW_FLAG_RIM_LIGHTING | RAW_FLAG_AMBIENT_SPECULAR))
		flags |= RAW_FLAG_SPECULAR;

	return (RawTechnique & 0x3F000000) | flags;
}

uint32_t BSLightingShader::GetPixelTechnique(uint32_t RawTechnique)
{
	uint32_t flags = RawTechnique & ~(
		RAW_FLAG_UNKNOWN1 |
		RAW_FLAG_UNKNOWN2 |
		RAW_FLAG_UNKNOWN3 |
		RAW_FLAG_UNKNOWN4 |
		RAW_FLAG_UNKNOWN5 |
		RAW_FLAG_UNKNOWN6);

	if ((flags & RAW_FLAG_MODELSPACENORMALS) == 0)
		flags &= ~RAW_FLAG_SKINNED;

	return flags | RAW_FLAG_VC;
}

void BSLightingShader::TechUpdateHighDetailRangeConstants(BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();

	// VS: p12 float4 HighDetailRange
	BSGraphics::Utility::CopyNiColorAToFloat(&VertexCG.ParamVS<XMVECTOR, 12>(),
		NiColorA(
			flt_141E32F54 - renderer->m_CurrentPosAdjust.x,
			flt_141E32F58 - renderer->m_CurrentPosAdjust.y,
			flt_141E32F5C - 15.0f,
			flt_141E32F60 - 15.0f));
}

void BSLightingShader::TechUpdateFogConstants(BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG)
{
	uintptr_t fogParams = (uintptr_t)BSShaderManager::GetFogProperty(byte_141E32FE0);

	if (!fogParams)
		return;

	// Set both vertex & pixel here
	{
		XMVECTORF32 fogColor;
		fogColor.f[0] = *(float *)(fogParams + 56);
		fogColor.f[1] = *(float *)(fogParams + 60);
		fogColor.f[2] = *(float *)(fogParams + 64);
		fogColor.f[3] = flt_141E32FBC;

		// VS: p14 float4 FogNearColor
		VertexCG.ParamVS<XMVECTORF32, 14>() = fogColor;

		// PS: p19 float4 FogColor
		PixelCG.ParamPS<XMVECTORF32, 19>() = fogColor;
	}

	// VS: p15 float4 FogFarColor
	XMVECTORF32& fogFarColor = VertexCG.ParamVS<XMVECTORF32, 15>();
	fogFarColor.f[0] = *(float *)(fogParams + 68);
	fogFarColor.f[1] = *(float *)(fogParams + 72);
	fogFarColor.f[2] = *(float *)(fogParams + 76);
	fogFarColor.f[3] = 0.0f;

	// VS: p13 float4 FogParam
	XMVECTORF32& fogParam = VertexCG.ParamVS<XMVECTORF32, 13>();

	float v5 = *(float *)(fogParams + 84);
	float v6 = *(float *)(fogParams + 80);

	if (v5 != 0.0f || v6 != 0.0f)
	{
		float fogMultiplier = 1.0f / (v5 - v6);

		fogParam.f[0] = fogMultiplier * v6;
		fogParam.f[1] = fogMultiplier;
		fogParam.f[2] = *(float *)(fogParams + 132);
		fogParam.f[3] = *(float *)(fogParams + 136);
	}
	else
	{
		fogParam = xmmword_14187D940;
	}
}

void BSLightingShader::sub_14130C470(__int64 a1, __int64 a2)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	NiSourceTexture *v2 = (NiSourceTexture *)a1;
	uint32_t v3 = *(uint32_t *)(a2 + 112);

	renderer->SetTexture(4, v2->QRendererTexture());
	renderer->SetTextureAddressMode(4, v3);
}

void BSLightingShader::sub_14130C4D0(__int64 a1, __int64 a2)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	NiSourceTexture *v2 = nullptr;
	uint32_t v3 = *(uint32_t *)(a2 + 112);

	if (a1)
		v2 = (NiSourceTexture *)a1;
	else
		v2 = BSShader_DefNormalMap;

	renderer->SetTexture(5, v2->QRendererTexture());
	renderer->SetTextureAddressMode(5, v3);
}

void BSLightingShader::sub_14130C220(int a1, __int64 a2, __int64 a3)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();
	NiSourceTexture *v3 = (NiSourceTexture *)a2;
	uint32_t v4 = *(uint32_t *)(a3 + 112);

	renderer->SetTexture(a1, v3->QRendererTexture());
	renderer->SetTextureAddressMode(a1, v4);
}

void BSLightingShader::MatSetMultiTextureLandOverrides(__int64 a1)
{
	auto *renderer = BSGraphics::Renderer::GetGlobals();

	// This overrides all 16 samplers/input resources for land parameters if set in the property
	NiSourceTexture *v2 = *(NiSourceTexture **)(a1 + 72);
	NiSourceTexture *v3 = *(NiSourceTexture **)(a1 + 88);

	renderer->SetTexture(0, v2->QRendererTexture());
	renderer->SetTexture(7, v3->QRendererTexture());

	if (*(uint32_t *)(a1 + 160))
	{
		int v4 = 8;
		do
		{
			NiSourceTexture *v5 = (NiSourceTexture *)(*(uintptr_t *)(a1 + 8i64 * (unsigned int)(v4 - 8) + 0xA8));
			uint32_t v6 = (unsigned int)(v4 - 7);

			renderer->SetTexture(v6, v5->QRendererTexture());
			renderer->SetTextureAddressMode(v6, 3);

			NiSourceTexture *v8 = (NiSourceTexture *)(*(uintptr_t *)(a1 + 8i64 * (unsigned int)(v4 - 8) + 208));

			renderer->SetTexture(v4, v8->QRendererTexture());
			renderer->SetTextureAddressMode(v4, 3);

			++v4;
		} while ((unsigned int)(v4 - 8) < *(uint32_t *)(a1 + 160));
	}

	if (*(uintptr_t *)(a1 + 248))
	{
		NiSourceTexture *result = *(NiSourceTexture **)(a1 + 248);

		renderer->SetTexture(13, result->QRendererTexture());
		renderer->SetTextureAddressMode(13, 0);
	}
}

__int64 BSLightingShader::sub_141314170(__int64 a1)
{
	return *(unsigned int *)(a1 + 8);
}

void BSLightingShader::GeometrySetupViewProjection(BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, const NiTransform& Transform, bool IsPreviousWorld, const NiPoint3 *PosAdjust)
{
	//
	// Instead of using the typical 4x4 matrix like everywhere else, someone decided that the
	// lighting shader is going to use 3x4 matrices. The missing 4th column is assumed to be
	// { 0, 0, 0, 1 } in row-major form.
	//
	XMMATRIX projMatrix;

	if (PosAdjust)
		projMatrix = BSShaderUtil::GetXMFromNiPosAdjust(Transform, *PosAdjust);
	else
		projMatrix = BSShaderUtil::GetXMFromNi(Transform);

	//
	// VS: p0 float3x4 World
	// -- or --
	// VS: p1 float3x4 PreviousWorld
	//
	if (!IsPreviousWorld)
		BSShaderUtil::TransposeStoreMatrix3x4(&VertexCG.ParamVS<float, 0>(), projMatrix);
	else
		BSShaderUtil::TransposeStoreMatrix3x4(&VertexCG.ParamVS<float, 1>(), projMatrix);
}

SRWLOCK asdf = SRWLOCK_INIT;

void BSLightingShader::GeometrySetupMTLandExtraConstants(const BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, const NiPoint3& Translate, float a3, float a4)
{
	float v4 = 0.0f;
	float v6 = (flt_141E32F40 - flt_141E32FD8) / (flt_141E32FB8 * 5.0f);

	if (v6 >= 0.0f)
		v4 = fmin(1.0f, v6);

	/*
	FO4 code:

	v7 = NiPoint2::operator-(&unk_10194FD8, &v10, &unk_10194FD0, &v11, LODWORD(v6));
	v8 = NiPoint2::operator*(v7);
	NiPoint2::operator+(&unk_10194FD0, &v12, v8);
	*/

	// v11 might be wildly incorrect (swap?): v11 = _mm_unpacklo_ps(LODWORD(xmmword_141E32FC8[0]), LODWORD(xmmword_141E32FC8[1]));
	AcquireSRWLockExclusive(&asdf);
	XMFLOAT2 v11;
	v11.x = xmmword_141E32FC8.x;
	v11.y = xmmword_141E32FC8.y;

	float v9 = xmmword_141E32FC8.z - xmmword_141E32FC8.x;
	float v10 = xmmword_141E32FC8.w - xmmword_141E32FC8.y;

	*(XMFLOAT2 *)&xmmword_141E32FC8 = v11;
	ReleaseSRWLockExclusive(&asdf);

	v11.x += (v9 * v4);
	v11.y += (v10 * v4);

	if (v4 == 1.0f)
		byte_1431F547C = 0;

	// VS: p3 float4 LandBlendParams
	XMVECTORF32& landBlendParams = VertexCG.ParamVS<XMVECTORF32, 3>();

	landBlendParams.f[0] = a3;
	landBlendParams.f[1] = a4;
	landBlendParams.f[2] = v11.x - Translate.x;
	landBlendParams.f[3] = v11.y - Translate.y;
}

void BSLightingShader::GeometrySetupTreeAnimConstants(const BSGraphics::ConstantGroup<BSGraphics::VertexShader>& VertexCG, BSLightingShaderProperty *Property)
{
	// Tree leaf animations
	// __int64 __fastcall sub_14130BC60(__int64 a1, __int64 a2)

	struct tempbufdata
	{
		char _pad[8];
		void *ptr;
	} temp;

	temp.ptr = VertexCG.RawData();

	auto GeometrySetupTreeAnimConstants = (void(__fastcall *)(tempbufdata *, BSLightingShaderProperty *))(g_ModuleBase + 0x130BC60);
	GeometrySetupTreeAnimConstants(&temp, Property);
}

void BSLightingShader::GeometrySetupDirectionalLights(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, const BSRenderPass *Pass, XMMATRIX& World, bool WorldSpace)
{
	BSLight *bsLight = Pass->QLights()[0];
	NiDirectionalLight *sunDirectionalLight = static_cast<NiDirectionalLight *>(bsLight->GetLight());

	float v12 = *(float *)(qword_1431F5810 + 224) * sunDirectionalLight->GetDimmer();

	XMFLOAT3& dirLightColor = PixelCG.ParamPS<XMFLOAT3, 4>();		// PS: p4 float3 DirLightColor
	XMFLOAT3& dirLightDirection = PixelCG.ParamPS<XMFLOAT3, 3>();	// PS: p3 float3 DirLightDirection

	dirLightColor.x = v12 * sunDirectionalLight->GetDiffuseColor().r;
	dirLightColor.y = v12 * sunDirectionalLight->GetDiffuseColor().g;
	dirLightColor.z = v12 * sunDirectionalLight->GetDiffuseColor().b;

	XMVECTOR lightDir = XMVectorNegate(sunDirectionalLight->GetWorldDirection().AsXmm());

	if (WorldSpace)
		lightDir = XMVector3TransformNormal(lightDir, World);

	XMStoreFloat3(&dirLightDirection, XMVector3Normalize(lightDir));
}

void BSLightingShader::GeometrySetupAmbientLights(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, const NiTransform& Transform, bool WorldSpace)
{
	// PS: p5 float3x4 DirectionalAmbient
	auto directionalAmbient = PixelCG.ParamPS<float[3][4], 5>();

	NiTransform unkTransform = *(NiTransform *)(g_ModuleBase + 0x1E32FE8);

	if (WorldSpace)
	{
		unkTransform = unkTransform * Transform;
		unkTransform.m_Translate.x = *(float *)(g_ModuleBase + 0x1E3300C);
		unkTransform.m_Translate.y = *(float *)(g_ModuleBase + 0x1E33010);
		unkTransform.m_Translate.z = *(float *)(g_ModuleBase + 0x1E33014);
	}

	BSShaderUtil::StoreTransform3x4NoScale(directionalAmbient, unkTransform);
}

void BSLightingShader::GeometrySetupEmitColorConstants(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, BSLightingShaderProperty *Property)
{
	// PS: p8 float4 EmitColor
	XMVECTORF32& emitColor = PixelCG.ParamPS<XMVECTORF32, 8>();

	emitColor.f[0] = Property->pEmitColor->r * Property->fEmitColorScale;
	emitColor.f[1] = Property->pEmitColor->g * Property->fEmitColorScale;
	emitColor.f[2] = Property->pEmitColor->b * Property->fEmitColorScale;
}

void BSLightingShader::GeometrySetupConstantPointLights(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, BSRenderPass *Pass, XMMATRIX& Transform, uint32_t LightCount, uint32_t ShadowLightCount, float Scale, bool WorldSpace)
{
	AssertMsg(BSShaderManager::GetRenderMode() != 22 && BSShaderManager::GetRenderMode() != 17, "This code path was removed and should never be called!");
	AssertMsg(ShadowLightCount <= 4, "Shader only expects shadow selector data to fit in a FLOAT4");

	auto& pointLightPosition = PixelCG.ParamPS<XMVECTORF32[7], 1>();// PS: p1 float4[7] PointLightPosition
	auto& pointLightColor = PixelCG.ParamPS<XMVECTORF32[7], 2>();	// PS: p2 float4[7] PointLightColor
	auto& shadowLightMaskSelect = PixelCG.ParamPS<float[4], 10>();	// PS: p10 float4 ShadowLightMaskSelect

	for (uint32_t i = 0; i < LightCount; i++)
	{
		BSLight *screenSpaceLight = Pass->QLights()[i + 1];
		NiLight *niLight = screenSpaceLight->GetLight();

		AssertMsgDebug(niLight, "If the SSL is non-null, the NiLight should also be non-null.");

		NiPoint3 worldPos = niLight->GetWorldTranslate();
		float dimmer = niLight->GetDimmer() * screenSpaceLight->GetLODDimmer();

		// if (bLiteBrite->value.b)
		//	dimmer = 0.0f;

		pointLightColor[i].f[0] = dimmer * niLight->GetDiffuseColor().r;
		pointLightColor[i].f[1] = dimmer * niLight->GetDiffuseColor().g;
		pointLightColor[i].f[2] = dimmer * niLight->GetDiffuseColor().b;

		if (WorldSpace)
		{
			pointLightPosition[i].v = DirectX::XMVector3TransformCoord(worldPos.AsXmm(), Transform);
			pointLightPosition[i].f[3] = niLight->GetSpecularColor().r / Scale;
		}
		else
		{
			worldPos = worldPos - BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust;

			pointLightPosition[i].v = worldPos.AsXmm();
			pointLightPosition[i].f[3] = niLight->GetSpecularColor().r;
		}

		if (i < ShadowLightCount)
			shadowLightMaskSelect[i] = (float)static_cast<BSShadowLight *>(screenSpaceLight)->UnkDword520;
	}
}

void BSLightingShader::GeometrySetupProjectedUv(const BSGraphics::ConstantGroup<BSGraphics::PixelShader>& PixelCG, BSGeometry *Geometry, BSLightingShaderProperty *Property, bool EnableProjectedNormals)
{
	// PS: p12 float4 ProjectedUVParams
	{
		XMVECTORF32& projectedUVParams = PixelCG.ParamPS<XMVECTORF32, 12>();

		float *v6 = (float *)((uintptr_t)Geometry + 444);

		if (!Geometry)
			v6 = (float *)((uintptr_t)Property + 0x10C);

		float v8 = 1.0f - v6[3];

		projectedUVParams.f[0] = v8 * v6[0];
		//projectedUVParams.f[1] = ;
		projectedUVParams.f[2] = v6[2];
		projectedUVParams.f[3] = (v8 * v6[1]) + v6[3];
	}

	// PS: p13 float4 ProjectedUVParams2
	{
		XMVECTORF32& projectedUVParams2 = PixelCG.ParamPS<XMVECTORF32, 13>();

		if (Geometry)
		{
			projectedUVParams2.f[0] = *(float *)((uintptr_t)Geometry + 464);// Reversed on purpose?
			projectedUVParams2.f[1] = *(float *)((uintptr_t)Geometry + 460);// Reversed on purpose?
		}
		else
		{
			BSGraphics::Utility::CopyNiColorAToFloat((XMVECTOR *)&projectedUVParams2, Property->QProjectedUVColor());
		}
	}

	// PS: p14 float4 ProjectedUVParams3
	{
		XMVECTORF32& projectedUVParams3 = PixelCG.ParamPS<XMVECTORF32, 14>();

		projectedUVParams3.f[0] = fProjectedUVDiffuseNormalTilingScale->uValue.f;
		projectedUVParams3.f[1] = fProjectedUVNormalDetailTilingScale->uValue.f;
		projectedUVParams3.f[2] = 0.0f;
		projectedUVParams3.f[3] = (EnableProjectedNormals) ? 1.0f : 0.0f;
	}
}

void BSLightingShader::sub_14130C8A0(const NiTransform& Transform, XMMATRIX& OutMatrix, bool DontMultiply)
{
	NiTransform temp;

	temp.m_Rotate.m_pEntry[0][0] = 0.0f;
	temp.m_Rotate.m_pEntry[0][1] = 1.0f;
	temp.m_Rotate.m_pEntry[0][2] = 0.0f;

	temp.m_Rotate.m_pEntry[1][0] = -1.0f;
	temp.m_Rotate.m_pEntry[1][1] = 0.0f;
	temp.m_Rotate.m_pEntry[1][2] = 0.0f;

	temp.m_Rotate.m_pEntry[2][0] = 0.0f;
	temp.m_Rotate.m_pEntry[2][1] = 0.0f;
	temp.m_Rotate.m_pEntry[2][2] = 1.0f;

	temp.m_Translate = BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust;
	temp.m_fScale = 1.0f;

	if (DontMultiply)
	{
		OutMatrix = BSShaderUtil::GetXMFromNi(temp);
	}
	else
	{
		XMMATRIX m1 = BSShaderUtil::GetXMFromNi(temp);
		XMMATRIX m2 = BSShaderUtil::GetXMFromNi(Transform);

		// * Operator order DOES matter
		// OutMatrix = Mul(Translate(m2, renderer->m_CurrentPosAdjust), m1);
		m2.r[3] = XMVectorAdd(m2.r[3], XMVectorSet(
			temp.m_Translate.x,
			temp.m_Translate.y,
			temp.m_Translate.z,
			0.0f));

		OutMatrix = XMMatrixMultiply(m2, m1);
	}
}