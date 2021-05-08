#include "prepare_nesting.h"
#include "vertex_array.h"
#include <igl/ortho.h>
#include <igl/opengl/init_render_to_texture.h>
#include <igl/centroid.h>

IGL_INLINE void igl::opengl::prepare_nesting(
  const Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> & _VA,
  const Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> & FA,
  const Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> & _VB,
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
  Eigen::Matrix4f & proj_view)
{
  // I truly regret doing this. Now model, p and np are in local reference
  // frames rather than the absolute coordinate system
  typedef Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> MatrixfX3R;
  MatrixfX3R VA=_VA,VB=_VB;
  igl::centroid(VA,FA,cenA);
  VA.rowwise() -= cenA;
  igl::centroid(VB,FB,cenB);
  VB.rowwise() -= cenB;
  // Create  pair of texture-framebuffers for depth peeling
  for(int i = 0;i<2;i++)
  {
    igl::opengl::init_render_to_texture(dbl_w,h,true,tex_id[i],fbo_id[i],d_id[i]);
  }
  // Create render buffer for layer comparison test
  {
    glGenFramebuffers(1,&ren_id);
    glBindFramebuffer(GL_FRAMEBUFFER,ren_id);
    GLuint col_id, d_id;
    glGenRenderbuffers(1,&col_id);
    glBindRenderbuffer(GL_RENDERBUFFER,col_id);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_RGB,dbl_w,h);
    glFramebufferRenderbuffer(
      GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,col_id);
    glGenRenderbuffers(1,&d_id);
    glBindRenderbuffer(GL_RENDERBUFFER,d_id);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,dbl_w,h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,d_id);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  // Create vertex arrays for each mesh
  GLuint A_ab_id,A_eab_id;
  GLuint B_ab_id,B_eab_id;
  igl::opengl::vertex_array(VA,FA,A_va_id,A_ab_id,A_eab_id);
  igl::opengl::vertex_array(VB,FB,B_va_id,B_ab_id,B_eab_id);
  FA_size = FA.size();
  FB_size = FB.size();
  // Center the camera around A's bounding sphere (i.e., tightest sphere
  // centered at center of bounding box)
  const Eigen::RowVector3f A_center = 
    0.5*(VA.colwise().maxCoeff() + VA.colwise().minCoeff());
  const float A_scale = 1.0/(VA.rowwise()-A_center).rowwise().norm().maxCoeff();
  // Set up constant orthographic projection matrix that fits A and enough
  // surrounding area in view
  Eigen::Matrix4f proj = Eigen::Matrix4f::Identity();
  // A little bigger than 1.0 just in case
  float hh = 1.5;
  float near = 0.01;
  float far = 2.*hh;
  float ratio = float(dbl_w*0.5)/float(h);
  igl::ortho(-hh*ratio,hh*ratio,-hh,hh,near,far,proj);
  // Construct view transformation
  Eigen::Affine3f view = Eigen::Affine3f::Identity();
  view.translate(Eigen::Vector3f(0,0,-hh));
  view.scale(A_scale);
  view.translate(-A_center.transpose());
  proj_view = proj * view.matrix();
}
