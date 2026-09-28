#version 450 core


// datastructure
// direction lights
struct light_sun
{
	vec3 position;
	vec3 colour;
};

// point lights
struct light_point
{
	vec3 position;
	vec3 colour;
	float constant;
	float linear;
	float quadratic;
};


// endpoint
layout(location = 0) in vec2 EdgeCoordinates;

layout(location = 0) out vec4 pixelColour;

// buffers
layout(set = 0,binding = 4) uniform sampler2D fpass_colour;
layout(set = 0,binding = 5) uniform sampler2D fpass_depth;
layout(set = 0,binding = 6) uniform sampler2D gpass_colour;
layout(set = 0,binding = 7) uniform sampler2D gpass_position;
layout(set = 0,binding = 8) uniform sampler2D gpass_normal;
layout(set = 0,binding = 9) uniform sampler2D gpass_material;
layout(set = 0,binding = 10) uniform sampler2D gpass_emission;
layout(set = 0,binding = 11) uniform sampler2D gpass_depth;

// camera parameters
layout(set = 0,binding = 40) uniform CameraAttributes
{
	vec3 position;
	float exposure;
	float gamma;
} cam;

// lights
layout(set = 0,binding = 41) uniform Lighting
{
	light_sun sunlights[8];
	light_point pointlights[64];
	uint sunlights_active;
	uint pointlights_active;
} lgt;


// constants
const float PI = 3.141592653;


// utility
vec3 lumen_sun(vec3 position,vec3 colour,vec3 normal,float metallic,float roughness,light_sun light);
vec3 lumen_point(vec3 position,vec3 colour,vec3 normal,float metallic,float roughness,light_point light);
vec3 pbs(vec3 colour,vec3 direction,vec3 influence,vec3 normal,vec3 halfway,float metallic,float roughness);
float schlick_beckmann_approx(float rel,float roughness);


// data
vec3 CameraDir;
vec3 Fresnel;
float ASquared;
float LightOut;
float SchlickOut;


void main()
{
	// map buffer components
	vec4 cmp_fcolour = texture(fpass_colour,EdgeCoordinates);
	vec4 cmp_gcolour = texture(gpass_colour,EdgeCoordinates);
	vec4 cmp_position = texture(gpass_position,EdgeCoordinates);
	vec4 cmp_normal = texture(gpass_normal,EdgeCoordinates);
	vec4 cmp_material = texture(gpass_material,EdgeCoordinates);
	vec4 cmp_emission = texture(gpass_emission,EdgeCoordinates);
	/*
	float cmp_fdepth = texture(fpass_depth,EdgeCoordinates).r;
	float cmp_gdepth = texture(gpass_depth,EdgeCoordinates).r;
	*/
	
	// translating buffer information
	vec3 colour = cmp_gcolour.rgb;
	vec3 position = cmp_position.rgb;
	vec3 normal = cmp_normal.rgb;
	float metalness = cmp_material.r;
	float roughness = cmp_material.g;
	float occlusion = cmp_material.b;
	vec3 emission = cmp_emission.rgb;

	// precalculate pixel parameters
	CameraDir = normalize(cam.position-position);
	Fresnel = mix(vec3(.04),colour,metalness);
	ASquared = pow(roughness,4.0);
	LightOut = max(dot(normal,CameraDir),.0);
	SchlickOut = schlick_beckmann_approx(LightOut,roughness);

	// processing simulated light sources
	vec3 sdw_component = vec3(0);
	vec3 lgt_component = vec3(0);
	for (int i=0;i<lgt.sunlights_active;i++)
		sdw_component += lumen_sun(position,colour,normal,metalness,roughness,lgt.sunlights[i]);
	for (int i=0;i<lgt.pointlights_active;i++)
		lgt_component += lumen_point(position,colour,normal,metalness,roughness,lgt.pointlights[i]);

	// TODO shadow processing

	// combination
	vec3 final = lgt_component;

	// process sub-geometric occlusion & emission
	final = final*occlusion;
	final = max(final,emission);

	// TODO light processing (for now until upgrade)

	// colour corrections
	final = vec3(1.)-exp(-final*cam.exposure);
	final = pow(final,vec3(cam.gamma));

	//final = mix(final,cmp_fcolour.rgb,cmp_fcolour.a*int(cmp_fdepth<cmp_gdepth));
	pixelColour = vec4(final,1.);
}

// sun light simulation processing
vec3 lumen_sun(vec3 position,vec3 colour,vec3 normal,float metallic,float roughness,light_sun light)
{
	vec3 direction = normalize(light.position-position);
	vec3 halfway = normalize(CameraDir+direction);
	return pbs(colour,direction,light.colour,normal,halfway,metallic,roughness);
}

// point light simulation processing
vec3 lumen_point(vec3 position,vec3 colour,vec3 normal,float metallic,float roughness,light_point light)
{
	// precalculations
	vec3 relation = light.position-position;
	vec3 direction = normalize(relation);
	vec3 halfway = normalize(CameraDir+direction);

	// calculate influence
	float dist = length(relation);
	float attenuation = 1./(light.constant+light.linear*dist+light.quadratic*pow(dist,2.));
	vec3 influence = light.colour*attenuation;

	// shade & return
	return pbs(colour,direction,influence,normal,halfway,metallic,roughness);
}

// shade function for physically defined objects
vec3 pbs(vec3 colour,vec3 direction,vec3 influence,vec3 normal,vec3 halfway,float metallic,float roughness)
{
	// distribution & fresnel component
	float throwbridge_reitz = ASquared/(PI*pow(pow(max(dot(normal,halfway),.0),2.)*(ASquared-1.)+1.,2.));
	vec3 fresnel = Fresnel+(1.-Fresnel)*pow(clamp(1.-max(dot(halfway,CameraDir),.0),.0,1.),5.);

	// geometry component
	float light_in = max(dot(normal,direction),.0);
	float smith = schlick_beckmann_approx(light_in,roughness)*SchlickOut;

	// specular brdf
	vec3 cook_torrance = (throwbridge_reitz*fresnel*smith)/(4.*light_in*LightOut+.0001);
	return ((vec3(1.)-fresnel)*(1.-metallic)*colour/PI+cook_torrance)*influence*light_in;
}
// TODO check the raytracer implementation for possible optimizations i have made in the past

// schlick beckmann approximation for physical based lighting components
float schlick_beckmann_approx(float rel,float roughness)
{
	float direct = pow(roughness+1.,2)/8.;
	return rel/(rel*(1-direct)+direct);
}
