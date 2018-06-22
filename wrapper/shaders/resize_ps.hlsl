Texture2D<float4> T : register(t0);
SamplerState S : register(s0) {
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

float4 main(in float4 p: SV_POSITION, in float2 t: TEX_COORD) : SV_TARGET {
	//return float4(t.x, t.y, 1, 1);
	return T.Sample(S, t);
}