//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

cbuffer SceneConstantBuffer : register(b0)
{
	float2 offset;
	float aspect;
	float padding[62];
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD0)
{
	PSInput result;

	result.position = position;
	result.color = color;
	result.uv = uv;
	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float dist = 1 - smoothstep(0.1 * float2(aspect, 1), 0.12 * float2(aspect, 1), min(distance(offset, input.uv), 1));
	return float4(dist, dist, dist, 0);
}
