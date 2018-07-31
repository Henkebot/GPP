#include "My3D.h"


glm::mat4 MY3D::SetViewport(VIEWPORT _vp)
{
	float w = _vp.Width;
	float h = _vp.Height;
	float n = _vp.Near;
	float f = _vp.Far;
	float ww = w * 0.5f;
	float hh = h * 0.5f;
	
	glm::mat4 vp(1);
	vp[0].x = ww;		/*0					0*/							vp[0].w= ww + _vp.X;
	/*0*/				vp[1].y = hh;		/*0*/						vp[1].w= hh + _vp.Y;
	/*0					0*/					vp[2].z = (n + f) * 0.5f;	vp[2].w = ((n - f) * 0.5f);

	return vp;
}
