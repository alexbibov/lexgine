struct GS_INPUT
{
    float4 position : RasterizedPosition;
};

struct PS_OUTPUT
{
    float4 output : SV_Target0;
};

PS_OUTPUT PS_main(GS_INPUT input)
{
    PS_OUTPUT rv;
    rv.output = input.position;

    return rv;
}