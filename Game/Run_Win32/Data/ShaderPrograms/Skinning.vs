#version 420 core												

layout(binding=1, std140) uniform cameraUBO
{
	mat4 VIEW;
	mat4 PROJECTION;

	mat4 CAMERA_MATRIX;

	vec3	CAMERA_RIGHT;
	float	PADDING_0;
	vec3	CAMERA_UP;
	float	PADDING_1;
	vec3	CAMERA_FORWARD;
	float	PADDING_2;
	vec3	CAMERA_POSITION;
	float	PADDING_3;
};

layout(binding=2, std140) uniform modelUBO
{
	mat4 MODEL;
};


const int MAX_BONES = 150;
layout(binding=12, std140) uniform boneUBO
{
    mat4 BONES[MAX_BONES];
};	

in vec3 POSITION;												
in vec4 COLOR;													
in vec2 UV;	
in vec3 NORMAL;	
in vec4 TANGENT;												
in uvec4 BONE_IDS;
in vec4 BONE_WEIGHTS;

out vec2 passUV;												
out vec4 passColor;
out vec3 passWorldPosition; // For determining light direction
out mat4 passTBNTransform;					
out vec3 passEyePosition;

																									
void main( void )												
{	
    vec4 localPosition = vec4(0.f);

	localPosition += ((BONES[BONE_IDS.x] * vec4(POSITION, 1.0f)) * BONE_WEIGHTS.x);
	localPosition += ((BONES[BONE_IDS.y] * vec4(POSITION, 1.0f)) * BONE_WEIGHTS.y);
	localPosition += ((BONES[BONE_IDS.z] * vec4(POSITION, 1.0f)) * BONE_WEIGHTS.z);
	localPosition += ((BONES[BONE_IDS.w] * vec4(POSITION, 1.0f)) * BONE_WEIGHTS.w);
	
	localPosition = vec4(localPosition.xyz, 1.0f); 

	//vec4 localPosition = vec4(POSITION, 1.0f);
	
	vec4 worldPosition = MODEL * localPosition;					
	vec4 clipPosition = PROJECTION * VIEW * worldPosition; 				
																
	passUV = UV;											
	passColor = COLOR;

	passWorldPosition = localPosition.xyz;

	// Calculate the TBN transform
	vec3 worldNormal = normalize((MODEL * vec4(NORMAL, 0.f)).xyz);
	vec3 worldTangent = normalize((MODEL * vec4(TANGENT.xyz, 0.0f)).xyz);
	vec3 worldBitangent = cross(worldTangent, worldNormal) * TANGENT.w;

	passTBNTransform = mat4(vec4(worldTangent, 0.f), vec4(worldBitangent, 0.f), vec4(worldNormal, 0.f), vec4(passWorldPosition, 1.0f));
	passEyePosition = CAMERA_POSITION;

	gl_Position = clipPosition; 
									
};