#version 330 core

in vec3 Position;

layout(location = 0) out vec4 pixelColour;

uniform vec3 camera_position;
uniform vec3 light_position;

// math constants
const float PI = 3.141592653;

// rayleigh
const vec3 rB = vec3(5.8e-6,13.5e-6,33.1e-6);
const float rPIi = 3./(16.*PI);
const float rScl = 2e5;

// mie
const float mG_air = .76f;
const float mGSq = mG_air*mG_air;
const vec3 mB = vec3(.002);
const float mPIi = 3./(8.*PI);
const float mScl = 1e1;
const float exposure = 4.f;

// geometric constants
const float Scale = 1.1;
const float PScale = .98725;  // FIXME floating point bullshit in .obj file parser
const float GrazeDistanceInv = 1./(2*sqrt(Scale*Scale-PScale*PScale));


void main()
{
	// precalculation
	vec3 camera_direction = normalize(Position-camera_position);
	vec3 norm_position = normalize(Position);

	// pseudo path-tracing through atmosphere utilizing linear algebra
	// intersection with atmosphere
	float a = 1.;
	float b = 2.*dot(camera_direction,camera_position);
	float c = dot(camera_position,camera_position)-Scale*Scale;
	float outer_discr = b*b-4.*a*c;
	float sq_discr = sqrt(outer_discr);
	float t0 = (-b-sq_discr)*.5;
	float t1 = (-b+sq_discr)*.5;

	// intersection with planet
	float c_planet = dot(camera_position,camera_position)-PScale*PScale;
	float inner_discr = b*b-4.*a*c_planet;
	if (inner_discr>.0)
	{
		sq_discr = sqrt(inner_discr);
		float t0i = (-b-sq_discr)*.5;
		float t1i = (-b+sq_discr)*.5;
		float tsurface = min(t0i,t1i);
		t0 = max(t0,.0);
		t1 = (tsurface>t0&&tsurface<t1) ? tsurface : t1;
	}
	float ray_distance = t1-t0;

	// phase calculations
	float cos_theta = dot(camera_direction,light_position);
	float ctsq = cos_theta*cos_theta;
	float rayleigh = rPIi*(1.+ctsq);
	float mie = mPIi*((1.-mGSq)*(1.+ctsq))/pow(1.+mGSq-2.*mG_air*cos_theta,1.5);

	// optical depth
	vec3 rTau = rB*rayleigh*ray_distance;
	vec3 mTau = mB*mie*ray_distance;
	vec3 tau = rTau*rScl+mTau*mScl;

	// light scattering
	vec3 beer_lambert = vec3(1.)-exp(-tau*exposure);

	// atmosphere fade
	float geom_factor = clamp(pow(dot(norm_position,light_position),.25),.2,1.);
	float fade = pow(clamp(ray_distance*GrazeDistanceInv,.4,1.),2.)*geom_factor;
	fade = max(pow(fade,1),.25);

	// combine scattering & fade
	pixelColour = vec4(beer_lambert,fade);
}
// FIXME this fade actually overwrites the alpha of the lower hull layers! how do i fix this?!??
