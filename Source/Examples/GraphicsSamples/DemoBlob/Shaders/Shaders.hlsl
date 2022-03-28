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

PSInput VSMain(float2 position : POSITION, float4 color : COLOR, float2 uv : TEXCOORD0)
{
	PSInput result;

	result.position = float4(position, 0, 1);
	result.color = color;
	result.uv = uv;
	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float2 uvStretch = float2(aspect, 1);
	float dist = min(distance(offset * uvStretch, input.uv * uvStretch), 1);
	float col = 1 - smoothstep(0.1, 0.12, dist);
	return float4(col, col, col, 0);
}
