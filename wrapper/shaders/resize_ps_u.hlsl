Texture2D<uint4> T : register(t0);

uint4 main(in float4 p: SV_POSITION, in float2 t : TEX_COORD) : SV_TARGET {
	int W, H, L;
	T.GetDimensions(0, W, H, L);
	return T.Load(int3(t.x*(W - 1), t.y*(H - 1), 0), 0);
}