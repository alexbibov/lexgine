struct VS_OUTPUT_HS_INPUT
{
    float4 pos : SV_Position;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float[4] outTessFactor : SV_TessFactor;
    float[2] inTessFactor : SV_InsideTessFactor;
};

struct HS_CONTROL_POINT_DATA_OUTPUT
{
    float4[4] position : SV_Position;
}


HS_CONSTANT_DATA_OUTPUT dummy(InputPatch<float3, 4> input)
{
    HS_CONSTANT_DATA_OUTPUT ouput;

    [unroll]
    for(int i = 0; i < 4; ++i)
    {
        output.outTessFactor[i] = 0;
    }

    [unroll]
    for(int i = 0; i < 2; ++i)
    {
        output.inTessFactor[i] = 0;
    }

    return output;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("dummy")]
HS_CONTROL_POINT_DATA_OUTPUT HS_main(InputPatch<VS_OUTPUT_HS_INPUT, 4> input,
uint controlPointID : SV_OutputControlPointID)
{
    HS_CONTROL_POINT_DATA_OUTPUT output;

    [unroll]
    for(int i = 0; i < 4; ++i)
    {
        output.position = input[i].position;
    }
}