#ifndef IGL_OPENGL_IS_VALID_NESTING_SCALE_H
#define IGL_OPENGL_IS_VALID_NESTING_SCALE_H

#include <igl/opengl/../igl_inline.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <igl/opengl/gl.h>

namespace igl
{
  namespace opengl
  {
    // Given two meshes A and B in their respective reference frames (with
    // centroids at the origin) and a list of parameters describing a possible
    // matryoshka "nesting" of B in A, determine the maximum scale of B so that
    // nesting is feasible (i.e., no intersections during linear removal).
    //
    // Inputs:
    //    dbl_w  two times viewport (and texture/framebuffer) width
    //    h  viewport (and texture/framebuffer) height
    //    tex_id  pointer to array containing 2 render-buffer color textures
    //      (see `init_render_to_texture`)
    //    fbo_id  pointer to array containing 2 framebuffers
    //    d_id  pointer to array containing 2 render-buffer depth textures
    //    ren_id  a pure-render framebuffer 
    //    A_va_id  vertex array id for mesh A
    //    FA_size  size of faces matrix for A (`FA.size()`)
    //    B_va_id  vertex array id for mesh B
    //    FB_size  size of faces matrix for B (`FB.size()`)
    //    proj_view  pre-multiplied projection and view matrix
    //    model  affine (usually rigid) transformation of B inside A
    //    p  point on clipping plane (often p:= transformed centroid of B in A)
    //    np  normal of clippling plane
    //    a1  angle of attack of top "half" 
    //    a2  angle of attack of bottom "half" 
    //    s  scale 
    // Returns true only if nesting is valid
    //
    IGL_INLINE bool is_valid_nesting(
      const GLsizei dbl_w,
      const GLsizei h,
      const GLuint * tex_id,
      const GLuint * fbo_id,
      const GLuint * d_id,
      const GLuint ren_id,
      const GLuint & A_va_id,
      const GLsizei FA_size,
      const GLuint & B_va_id,
      const GLsizei FB_size,
      const Eigen::Matrix4f & proj_view,
      const Eigen::Affine3f & model,
      // const Eigen::RowVector3f & p, // LWW
      const float & pZ,
      const Eigen::RowVector3f & np,
      const Eigen::RowVector3f & a1,
      const Eigen::RowVector3f & a2,
      const float s);
  }
}

#ifndef IGL_STATIC_LIBRARY
#  include "is_valid_nesting.cpp"
#endif

#endif

