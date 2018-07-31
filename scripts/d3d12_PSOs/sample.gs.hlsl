struct DS_TESSELLATED_POINT_DATA_INPUT
{
    float4 position : TESS_POSITION;
	float3 normal : TESS_NORMAL;
};

struct GS_OUTPUT
{
	float4 raster_position : SV_Position;
    float4 position : TESS_POSITION;
	float3 normal : TESS_NORMAL;
};

[maxvertexcount(3)]
void GS_main(triangle DS_TESSELLATED_POINT_DATA_INPUT input[3], 
inout PointStream<GS_OUTPUT> point_stream0, 
inout PointStream<GS_OUTPUT> point_stream1)
{
    [unroll]
    for(int i = 0; i < 3; ++i)
    {
        GS_OUTPUT new_out;
        new_out.raster_position = input[i].position;
		new_out.position = input[i].position;
		new_out.normal = input[i].normal;
        point_stream0.Append(new_out);
		point_stream0.RestartStrip();
		
		new_out.position += .5f;
		point_stream1.Append(new_out);
		point_stream1.RestartStrip();
    }
}