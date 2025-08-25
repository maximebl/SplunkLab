#pragma once

#ifdef __cplusplus
#define matrix DirectX::XMMATRIX
#define float4 DirectX::XMFLOAT4
#define float3 DirectX::XMFLOAT3
#define float2 DirectX::XMFLOAT2
#define float4x4 DirectX::XMFLOAT4X4
#define uint uint32_t
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

#ifdef __cplusplus
_declspec(align(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
#endif
struct Constants
{
	float3 TestColor;
	float _padding0;
	float3 CameraPosition;
	float _padding1;
	float4x4 Model;
	float4x4 View;
	float4x4 ViewProjection;
};
