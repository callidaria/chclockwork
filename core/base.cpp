#include "base.h"


// ----------------------------------------------------------------------------------------------------
// Utility

/**
 *	check if a file can be found at given location
 *	\param path: path to check
 *	\returns true if file could be accessed
 *	NOTE for use in debug cases only, in release checking for data that is guaranteed is not good practice
 */
bool check_file_exists(const char* path)
{
	struct stat tf;
	return stat(path,&tf)==0;
}

/**
 *	split line into words
 *	\param words: output vector for words
 *	\param line: raw input line
 */
void split_words(vector<string>& words,string& line)
{
	std::stringstream stream(line);
	string word;
	while (stream>>word) words.push_back(word);
}

/**
 *	calculate halfway vector in-between the two given vectors
 *	\param a: first vector
 *	\param b: second vector
 *	\returns vector pointing to the location halfway in-between the given vectors
 */
vec3 halfway(vec3 a,vec3 b)
{
	return (a+b)*.5f;
}


// ----------------------------------------------------------------------------------------------------
// Low-Level Data

/**
 *	memory allocation for requested amount of bits with subsequent neutralization
 *	\param size: size of bit sequence. this is the exact amount of needed bits, not in fact a bytelength
 */
BitwiseWords::BitwiseWords(size_t size)
{
	m_Size = (size+MEM_WIDTH-1)>>MEM_SHIFT;
	m_Data = (__system_word*)malloc(m_Size*sizeof(__system_word));
	reset();
}

/**
 *	automatically release the allocated data bits on destruction
 */
BitwiseWords::~BitwiseWords()
{
	free(m_Data);
}


// ----------------------------------------------------------------------------------------------------
// Signals Between Threads

/**
 *	start to wait for a continue signal
 */
void ThreadSignal::wait()
{
	std::unique_lock<std::mutex> lock(mutex);
	cv.wait(lock,[this]{ return !semaphore; });
}

/**
 *	signal all threads listening to continue with procedure
 *	\param force: (default=false) force proceeding regardless of semaphore state
 */
void ThreadSignal::proceed(bool force)
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		semaphore--;
		semaphore *= !force;
	}
	cv.notify_one();
}

/**
 *	signal subprocess to vanish
 */
void ThreadSignal::exit()
{
	running = false;
	proceed(true);
}


// ----------------------------------------------------------------------------------------------------
// Geometry

/**
 *	determines if the given point is intersecting with the rect
 *	\param point: point in 2D space to check relationship with rect
 *	\returns true if point is inside rect bounds
 */
bool Rect::intersect(vec2 point)
{
	return (point.x<extent.x&&point.x>position.x)&&(point.y<extent.y&&point.y>position.y);
}


/**
 *	transform object
 *	\param p: object position
 *	\param s: object scale
 *	\param r: object rotation
 */
void Transform3D::transform(vec3 p,f32 s,vec3 r)
{
	translate(p);
	scale(s);
	rotate(r);
}

/**
 *	transform object
 *	\param p: object position
 *	\param s: object scale
 *	\param r: object rotation
 */
void Transform3D::transform(vec3 p,vec3 s,vec3 r)
{
	translate(p);
	scale(s);
	rotate(r);
}

/**
 *	transform object around arbitrary origin
 *	\param p: object position
 *	\param s: object scale
 *	\param r: object rotation
 *	\param a: arbitrary point of origin
 */
void Transform3D::transform(vec3 p,f32 s,vec3 r,vec3 a)
{
	model = mat4(1.f);
	translate(a-position);
	transform(position+p-a,s,r);
}

/**
 *	transform object around arbitrary origin
 *	\param p: object position
 *	\param s: object scale
 *	\param r: object rotation
 *	\param a: arbitrary point of origin
 */
void Transform3D::transform(vec3 p,vec3 s,vec3 r,vec3 a)
{
	model = mat4(1.f);
	translate(a-position);
	transform(position+p-a,s,r);
}

/**
 *	translate object position
 *	\param p: object position
 */
void Transform3D::translate(vec3 p)
{
	model[3][0] = p.x;
	model[3][1] = p.y;
	model[3][2] = p.z;
}

/**
 *	set model scaling
 *	\param s: scaling, 1.f is default size
 */
void Transform3D::scale(f32 s)
{
	model[0][0] = s;
	model[1][1] = s;
	model[2][2] = s;
}

/**
 *	set model scaling by axis
 *	\param s: scaling by axis
 */
void Transform3D::scale(vec3 s)
{
	model[0][0] = s.x;
	model[1][1] = s.y;
	model[2][2] = s.z;
}

/**
 *	scale object around arbitrary origin
 *	\param s: scaling, 1.f is default size
 *	\param a: arbitrary point of origin
 */
void Transform3D::scale(f32 s,vec3 a)
{
	model = glm::mat4(1.f);
	translate(a-position);
	transform(position-a,s,rotation);
}

/**
 *	scale object by axis around arbitrary origin
 *	\param s: scaling, 1.f is default size
 *	\param a: arbitrary point of origin
 */
void Transform3D::scale(vec3 s,vec3 a)
{
	model = glm::mat4(1.f);
	translate(a-position);
	transform(position-a,s,rotation);
}

/**
 *	rotate model around the x axis
 *	\param x: rotation around x in degrees
 */
void Transform3D::rotate_x(f32 x)
{
	model = glm::rotate(model,glm::radians(x),vec3(1,0,0));
	rotation.x = (abs(x)>360.f) ? fmodf(x,360.f) : x;
}

/**
 *	rotate model around the y axis
 *	\param y: rotation around y in degrees
 */
void Transform3D::rotate_y(f32 y)
{
	model = glm::rotate(model,glm::radians(y),vec3(0,1,0));
	rotation.y = (abs(y)>360.f) ? fmodf(y,360.f) : y;
}

/**
 *	rotate model around the z axis
 *	\param z: rotation around z in degrees
 */
void Transform3D::rotate_z(f32 z)
{
	model = glm::rotate(model,glm::radians(z),vec3(0,0,1));
	rotation.z = (abs(z)>360.f) ? fmodf(z,360.f) : z;
}

/**
 *	set model rotation
 *	\param r: object rotation
 */
void Transform3D::rotate(vec3 r)
{
	rotate_x(r.x);
	rotate_y(r.y);
	rotate_z(r.z);
}

/**
 *	rotate model around an arbitrary origin
 *	\param r: object rotation
 *	\param a: arbitrary point of origin
 */
void Transform3D::rotate(vec3 r,vec3 a)
{
	f32 __ScaleFactor = model[0][0];  // FIXME !!this is only true for simple transformation
	model = mat4(1.f);
	translate(a-position);
	rotate(r);
	transform(position-a,__ScaleFactor,r);
}


// ----------------------------------------------------------------------------------------------------
// Coordinate System

/**
 *	coordinate system setup for 2D objects
 *	\param xaxis: maximum range for visible geometry on x-axis
 *	\param yaxis: maximum range for visible geometry on y-axis
 */
CoordinateSystem2D::CoordinateSystem2D(f32 xaxis,f32 yaxis)
{
	view = glm::lookAt(vec3(0,-.0001f,1),vec3(.0f),vec3(0,0,1));
	proj = glm::ortho(.0f,xaxis,.0f,yaxis,.1f,10.f);
}


// ----------------------------------------------------------------------------------------------------
// Camera

/**
 *	create 3D camera
 *	\param tgt: starting camera view focus target
 *	\param dist: distance from target to camera
 *	\param p: camera rotation pitch
 *	\param y: camera rotation yaw
 *	\param w: screen resolution width, can also be downscaled to ratio
 *	\param h: screen resolution height, can also be downscaled to ratio
 *	\param ifov: field of view in degrees
 */
Camera3D::Camera3D(vec3 tgt,f32 dist,f32 p,f32 y,f32 w,f32 h,f32 ifov)
	: target(tgt),distance(dist),pitch(p),yaw(y),fov(ifov),m_Ratio(w/h)
{
	update();
	project();
}

/**
 *	create orthographic 3D camera
 *	\param tgt: starting camera view focus target
 *	\param dist: distance from target to camera
 *	\param p: camera rotation pitch
 *	\param y: camera rotation yaw
 *	\param w: screen resolution width, can also be downscaled to ratio
 *	\param h: screen resolution height, can also be downscaled to ratio
 *	\param n: near plane distance
 *	\param f: far plane distance
 */
Camera3D::Camera3D(vec3 tgt,f32 dist,f32 p,f32 y,u32 w,u32 h,f32 n,f32 f)
	: target(tgt),distance(dist),pitch(p),yaw(y),width(w),height(h),near(n),far(f)
{
	update();
	orthographics();
}

/**
 *	create orthographic 3D camera
 *	\param tgt: starting camera view focus target
 *	\param p: camera position
 *	\param w: screen resolution width, can also be downscaled to ratio
 *	\param h: screen resolution height, can also be downscaled to ratio
 *	\param n: near plane distance
 *	\param f: far plane distance
 */
Camera3D::Camera3D(vec3 tgt,vec3 p,u32 w,u32 h,f32 n,f32 f)
	: target(tgt),position(p),width(w),height(h),near(n),far(f)
{
	force_position();
	orthographics();
}

/**
 *	update camera view matrix based on position and target parameters
 */
void Camera3D::update()
{
	position = vec3(-cos(pitch)*sin(yaw),-cos(pitch)*cos(yaw),sin(pitch))*distance+target;
	view = glm::lookAt(position,target,up);
}

/**
 *	update camera view matrix with forced position vector
 */
void Camera3D::force_position()
{
	view = glm::lookAt(position,target,up);
}

/**
 *	update camera projection
 */
void Camera3D::project()
{
	proj = glm::perspective(glm::radians(fov),m_Ratio,near,far);
}

/**
 *	calculate camera projection orthographically
 */
void Camera3D::orthographics()
{
	f32 hwidth = width>>1;
	f32 hheight = height>>1;
	proj = glm::ortho(-hwidth,hwidth,-hheight,hheight,near,far);
}

/**
 *	set camera roll
 *	\param r: camera roll in degrees
 */
void Camera3D::roll(f32 r)
{
	up = glm::rotate(glm::mat4(1.f),glm::radians(r),target-position)*vec4(COORDINATE_SYSTEM_ORIENTATION,.0f);
}


// ----------------------------------------------------------------------------------------------------
// Physics Utility Section

/**
 *	create intertia effected target position that updates a linear applied vectorial position
 *	\param ffactor: amount of time in seconds the momentum takes to snap to target position
 */
TargetMomentumSnap::TargetMomentumSnap(f32 ffactor)
{
	f32 __Omega = 2.f/ffactor;
	m_Stiff = __Omega*__Omega;
	m_Damp = 2.f*__Omega;
}

/**
 *	update momentum based position to snap to given target position
 *	\param pos: reference to current position, that should be transformed towards target
 *	\param dt: delta time to apply to integration steps
 */
void TargetMomentumSnap::update(vec3& pos,f32 dt)
{
	m_Momentum += ((target-pos)*m_Stiff-m_Momentum*m_Damp)*dt;
	pos += m_Momentum*dt;
}
