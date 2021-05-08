#ifndef IGL_OPENGL_PREPARE_NESTING_H
#define IGL_OPENGL_PREPARE_NESTING_H
#include <igl/igl_inline.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <igl/opengl/gl.h>

namespace igl
{
  namespace opengl
  {
    IGL_INLINE void prepare_nesting(
      const Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> & VA,
      const Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> & FA,
      const Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> & VB,
      const Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> & FB,
      const GLsizei dbl_w,
      const GLsizei h,
      Eigen::RowVector3f & cenA,
      Eigen::RowVector3f & cenB,
      GLuint * tex_id,
      GLuint * fbo_id,
      GLuint * d_id,
      GLuint & ren_id,
      GLuint & A_va_id,
      GLsizei & FA_size,
      GLuint & B_va_id,
      GLsizei & FB_size,
      Eigen::Matrix4f & proj_view);
  }
}

#ifndef IGL_STATIC_LIBRARY
#  include "prepare_nesting.cpp"
#endif

#endif
