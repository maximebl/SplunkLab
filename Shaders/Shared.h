#pragma once

#if __cplusplus
#pragma once
#define matrix DirectX::XMMATRIX
#define float4 DirectX::XMFLOAT4
#define float3 DirectX::XMFLOAT3
#define float2 DirectX::XMFLOAT2
#define float16_t uint16_t
#define float16_t2 uint32_t
#define uint uint32_t
#define OUT_PARAMETER(X) X&
#else
#define OUT_PARAMETER(X) out X
#endif

struct MaterialData
{
	float3 BaseColor;
	int BaseColorTexId;

	float3 Emissive;
	int EmissiveTexId;

	float Metalness;
	float Roughness;
	float Opacity;
	int RoughnessMetalnessTexId;

	int AlphaMode; //< 0: Opaque, 1: Blend, 2: Masked
	float AlphaCutoff;
	int DoubleSided; //< 0: false, 1: true
	int NormalTexId; //< Tangent space XYZ
};
