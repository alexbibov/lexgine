struct VS_INPUT
{
    float4 position : SV_Position;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
};


VS_OUTPUT VS_main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = input.position;

    return output;
}