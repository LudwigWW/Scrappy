#ifndef DRAW_NESTING_DEPTH_LAYER_H
#define DRAW_NESTING_DEPTH_LAYER_H

#include <igl/igl_inline.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <igl/opengl/gl.h>

namespace igl
{
  namespace opengl
  {
    // Inputs:
    //   pass  number of depth peels conducted already
    //
    //   proj  projection matrix
    //   view  view matrix
    //
    //   mid_bounds  scale
    IGL_INLINE void draw_nesting_depth_layer(
      const int pass,
      const GLsizei dbl_w,
      const GLsizei h,
      const GLuint * d_id,
      const GLuint & A_va_id,
      const GLsizei FA_size,
      const GLuint & B_va_id,
      const GLsizei FB_size,
      const Eigen::Matrix4f & proj_view,
      const Eigen::Affine3f & model,
      //const Eigen::RowVector3f & p,
      const float & pZ,
      const Eigen::RowVector3f & np,
      const Eigen::RowVector3f & a1,
      const Eigen::RowVector3f & a2,
      const float mid_bounds);
  }
}

#ifndef IGL_STATIC_LIBRARY
#include "draw_nesting_depth_layer.h"
#endif

#endif

