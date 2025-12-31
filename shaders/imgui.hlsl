SamplerState sLinearWrap : register(s0, space1);
SamplerState sLinearClamp : register(s1, space1);
SamplerState sLinearBorder : register(s2, space1);
SamplerState sPointWrap : register(s3, space1);

float4 Sample2D(int index, SamplerState s, float2 uv, uint2 offset = 0)
{
    Texture2D tex = ResourceDescriptorHeap[index];
    return tex.Sample(s, uv, offset);
}

struct VertexInput
{
	float2 Position : POSITION;
	float2 UV : TEXCOORD;
	float4 Color : COLOR;
};

struct InterpolantsVSToPS
{
	float4 Position : SV_Position;
	float2 UV : TEXCOORD;
	float4 Color : COLOR;
};

struct Params
{
	float4 ScaleBias;
	uint TextureIndex;
};

ConstantBuffer<Params> cPass : register(b0);

InterpolantsVSToPS VSMain(VertexInput input)
{
	InterpolantsVSToPS output = (InterpolantsVSToPS)0;
    output.Position = float4(input.Position * cPass.ScaleBias.xy + cPass.ScaleBias.zw, 0.0f, 1.0f);
	output.UV = input.UV;
	output.Color = input.Color;
	return output;
}

float4 PSMain(InterpolantsVSToPS input) : SV_Target
{       
    return input.Color * Sample2D(cPass.TextureIndex, sPointWrap, input.UV);
}
