void main( in float2 pos : POSITION, out float4 p: SV_POSITION, out float2 t: TEX_COORD ) {
	t = pos;
	p.x = pos.x * 2 - 1;
	p.y = 1 - pos.y * 2;
	p.z = 0;
	p.w = 1;
}