struct DS_TESSELLATED_POINT_DATA_INPUT
{
    float4 position : TessellatedPointPosition;
};

struct GS_OUTPUT
{
    float4 position : RasterizedPosition;
};

[maxvertexcount(3)]
void GS_main(triangle DS_TESSELLATED_POINT_DATA_INPUT input[3], inout TriangleStream<GS_OUTPUT> tri_stream)
{
    [unroll]
    for(int i = 0; i < 3; ++i)
    {
        GS_OUTPUT new_out;
        new_out.position = input[i].position;
        tri_stream.Append(new_out);
    }
    tri_stream.RestartStrip();
}