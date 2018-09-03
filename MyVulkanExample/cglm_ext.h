#ifndef VK_EXAMPLE_CGLM_EXT_H
#define VK_EXAMPLE_CGLM_EXT_H

#include "cglm/cglm.h"

/*!
 * @brief set up perspective projection matrix
 *
 * @param[in]  fovy    field of view angle
 * @param[in]  aspect  aspect ratio ( width / height )
 * @param[in]  nearVal near clipping plane
 * @param[in]  farVal  far clipping planes
 * @param[out] dest    result matrix
 */
CGLM_INLINE
void
glm_perspective_zto(float fovy,
                    float aspect,
                    float nearVal,
                    float farVal,
                    mat4  dest) {
  float f, fn;

  glm__memzero(float, dest, sizeof(mat4));

  f  = 1.0f / tanf(fovy * 0.5f);
  fn = 1.0f / (nearVal - farVal);

  dest[0][0] = f / aspect;
  dest[1][1] = f;
  dest[2][2] = farVal * fn;
  dest[2][3] =-1.0f;
  dest[3][2] = nearVal * farVal * fn;
}

#endif // VK_EXAMPLE_CGLM_EXT_H