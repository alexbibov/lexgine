struct VS_OUTPUT_HS_INPUT
{
    float4 pos : SV_Position;
};

struct HS_CONSTANT_DATA
{
    float outTessFactor[4] : SV_TessFactor;
    float inTessFactor[2] : SV_InsideTessFactor;
};

struct HS_CONTROL_POINT_DATA_OUTPUT
{
    float4 position : ControlPointPosition;
};


HS_CONSTANT_DATA dummy()
{
    HS_CONSTANT_DATA output;
	int i;
	
    [unroll]
    for(i = 0; i < 4; ++i)
    {
        output.outTessFactor[i] = 0;
    }

    [unroll]
    for(i = 0; i < 2; ++i)
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
    output.position = input[controlPointID].pos;
	return output;
}