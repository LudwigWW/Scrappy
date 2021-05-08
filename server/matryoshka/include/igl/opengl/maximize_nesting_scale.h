#ifndef IGL_OPENGL_MAXIMIZE_NESTING_SCALE_H
#define IGL_OPENGL_MAXIMIZE_NESTING_SCALE_H

#include <igl/opengl/../igl_inline.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <igl/opengl/gl.h>

enum MaximizeNestingScaleMask
{
};

namespace igl
{
  namespace opengl
  {
    enum MaximizeNestingScaleConstraintType
    {
      MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE = 0,
      MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED = 1,
      MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT = 2,
      NUM_MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPES = 3
    };

    // Given two meshes A and B deteremine the maximum scale of B so that
    // nesting is feasible (i.e., no intersections during linear removal).
    // 
    // Inputs:
    //   VA  #VA by 3 list of A's vertex positions
    //   FA  #FA by 3 list of A's triangle indices into VA
    //   VB  #VB by 3 list of B's vertex positions
    //   FB  #FB by 3 list of B's triangle indices into VB
    //   th  Rotation of B as Euler angles 
    //   pso_iters  number of particle swarm optimization iterations {500}
    //   pso_population  size of particle swarm {300}
    //   const_XXX  constraint type of previous output. If
    //     const_XXX==MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED then the
    //     initial value of XXX will be used as the fixed value.
    // Outputs:
    //   p   point on cut plane
    //   np  normal of cut plane
    //   a1  angle of attack of top "half" 
    //   a2  angle of attack of bottom "half" 
    //   model  similarity transformation moving B into valid nesting within A
    // Returns the maximum nesting scale found.
    IGL_INLINE float maximize_nesting_scale(
      const Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> & VA,
      const Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> & FA,
      const Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> & VB,
      const Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> & FB,
      const int pso_iters,
      const int pso_population,
      const float clearance_height,
      Eigen::RowVector3f & th,  const MaximizeNestingScaleConstraintType const_th,
      Eigen::RowVector3f & cen, const MaximizeNestingScaleConstraintType const_cen,
      Eigen::RowVector3f & p,   const MaximizeNestingScaleConstraintType const_p,
      Eigen::RowVector3f & np,  const MaximizeNestingScaleConstraintType const_np,
      Eigen::RowVector3f & a1,  const MaximizeNestingScaleConstraintType const_a1,
      Eigen::RowVector3f & a2,  const MaximizeNestingScaleConstraintType const_a2,
      Eigen::Affine3f & model);
    // Double version that will create temporary float copies
    IGL_INLINE double maximize_nesting_scale(
      const Eigen::Matrix<double,Eigen::Dynamic,3,Eigen::RowMajor> & VA,
      const Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> & FA,
      const Eigen::Matrix<double,Eigen::Dynamic,3,Eigen::RowMajor> & VB,
      const Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> & FB,
      const int pso_iters,
      const int pso_population,
      const float clearance_height,
      Eigen::RowVector3d & th,  const MaximizeNestingScaleConstraintType const_th,
      Eigen::RowVector3d & cen, const MaximizeNestingScaleConstraintType const_cen,
      Eigen::RowVector3d & p,   const MaximizeNestingScaleConstraintType const_p,
      Eigen::RowVector3d & np,  const MaximizeNestingScaleConstraintType const_np,
      Eigen::RowVector3d & a1,  const MaximizeNestingScaleConstraintType const_a1,
      Eigen::RowVector3d & a2,  const MaximizeNestingScaleConstraintType const_a2,
      Eigen::Affine3d & model);

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
    ////    A_scale  scale so that A fits into unit sphere:
    ////      A_scale = 1.0/(VA.rowwise()-A_center).rowwise().norm().maxCoeff();
    ////    A_center center of A's bounding box:
    //      A_center = 0.5*(VA.colwise().maxCoeff() + VA.colwise().minCoeff())
    //    model  affine (usually rigid) transformation of B inside A
    //    p  point on clipping plane (often p:= transformed centroid of B in A)
    //    np  normal of clippling plane
    //    a1  angle of attack of top "half" 
    //    a2  angle of attack of bottom "half" 
    //    tol  binary search stopping criteria tolerance. Stop if u-l < tol
    //    max_s  maximum scale found so far (if upper bound is less than this
    //      then exit early)
    //    in_upper_bound  input upper bound
    // Outputs:
    //   lower_bound  scale for which nesting is possible
    //   upper_bound  scale larger than lower_bound for which nesting is
    //     **not** possible
    //
    IGL_INLINE bool maximize_nesting_scale(
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
      const Eigen::RowVector3f & p,
      const Eigen::RowVector3f & np,
      const Eigen::RowVector3f & a1,
      const Eigen::RowVector3f & a2,
      const float tol,
      const float max_s,
      const float in_upper_bound,
      const float clearance_height,
      float & lower_bound,
      float & upper_bound);
  }
}

#ifndef IGL_STATIC_LIBRARY
#  include "maximize_nesting_scale.cpp"
#endif

#endif
