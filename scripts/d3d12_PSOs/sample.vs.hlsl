struct View
{
	float4x4 ModelToWorld;
	float4x4 WorldToView;
	float4x4 ViewToTarget;
	
	float4x4 TargetToView;
	float4x4 ViewToWorld;
	float4x4 WorldToModel;
};

ConstantBuffer<View> ViewConstants(b0, space0);

struct VS_INPUT
{
    float4 position : SV_Position;
	float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
	float3 normal : NORMAL; 
};


VS_OUTPUT VS_main(VS_INPUT input)
{
    VS_OUTPUT output;
	
	float4x4 ModelToView = mul(ViewConstants.ModelToWorld, ViewConstants.WorldToView);
	float4x4 MVP = mul(ViewConstants.ViewToTarget, ModelToView);
	
    output.position = mul(MVP, input.position);
	output.normal = mul(float3x3(ModelToView), input.normal);

    return output;
}