struct DS_TESSELLATED_POINT_DATA_OUTPUT
{
    float4 position : TESS_POSITION;
	float3 normal : TESS_NORMAL;
};

struct HS_CONSTANT_DATA
{
    float outTessFactor[4] : SV_TessFactor;
    float inTessFactor[2] : SV_InsideTessFactor;
};

struct HS_CONTROL_POINT_DATA
{
    float4 position : TESS_CP_POSITION;
	float3 normal : TESS_CP_NORMAL;
};

[domain("quad")]
DS_TESSELLATED_POINT_DATA_OUTPUT DS_main(HS_CONSTANT_DATA PerPatchData, 
    float2 domainLocation : SV_DomainLocation, 
    const OutputPatch<HS_CONTROL_POINT_DATA, 4> input)
{
    DS_TESSELLATED_POINT_DATA_OUTPUT output;
    float4 intermediatePosition1 = lerp(input[0].position, input[1].position, domainLocation.x);
    float4 intermediatePosition2 = lerp(input[2].position, input[3].position, domainLocation.x);

    output.position = lerp(intermediatePosition1, intermediatePosition2, domainLocation.y);
	output.normal = normalize(input[0].normal + input[1].normal + 
	    input[2].normal + input[3].normal);
    return output;
}