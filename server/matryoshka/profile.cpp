#include "include/igl/opengl/prepare_nesting.h"
#include "include/igl/opengl/maximize_nesting_scale.h"
#include "include/igl/opengl/draw_nesting_depth_layer.h"
#include "include/igl/opengl/is_valid_nesting.h"
#include <igl/read_triangle_mesh.h>
#include <igl/opengl/glfw/background_window.h>
#include <igl/png/render_to_png.h>
#include <igl/pathinfo.h>
#include <igl/C_STR.h>
#include <igl/get_seconds.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>

int main(int argc, char * argv[])
{
  // Load input mesh(es)
  typedef Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> MatrixfX3R;
  typedef Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> MatrixiX3R;
  MatrixfX3R VA,VB;
  MatrixiX3R FA,FB;
  igl::read_triangle_mesh(argv[1],VA,FA);
  std::string prefix;
  {
    std::string _1,_2,_3;
    igl::pathinfo(argv[1],_1,_2,_3,prefix);
  }
  std::cout<<argv[1]<<" "<<FA.rows()<<std::endl;
  if(argc>2)
  {
    // B inside A
    igl::read_triangle_mesh(argv[2],VB,FB);
    std::cout<<argv[2]<<" "<<FB.rows()<<std::endl;
  }else
  {
    // Self-Matryoshka
    VB = VA;
    FB = FA;
    std::cout<<argv[1]<<" "<<FB.rows()<<std::endl;
  }

  GLFWwindow * window;
  igl::opengl::glfw::background_window(window);

  const int dbl_w = 2*512, h = 512;
  const float clearance_height = 5.0; // LWW !!!
  GLuint tex_id[2], fbo_id[2], d_id[2];
  GLuint ren_id;
  GLuint A_va_id,A_ab_id,A_eab_id;
  GLuint B_va_id,B_ab_id,B_eab_id;
  GLsizei FA_size,FB_size;
  Eigen::Matrix4f proj_view;
  Eigen::RowVector3f cenA,cenB;
  double tic;
  tic = igl::get_seconds();
  igl::opengl::prepare_nesting(
    VA,FA,VB,FB,dbl_w,h,
    cenA,cenB,tex_id,fbo_id,d_id,ren_id,A_va_id,FA_size,B_va_id,FB_size,proj_view);
  std::cout<<"prepare_nesting: "<<(igl::get_seconds()-tic)<<std::endl;

  Eigen::Affine3f model = Eigen::Affine3f::Identity();
  Eigen::RowVector3f p(0,0,0);
  Eigen::RowVector3f np(0,0,1);
  Eigen::RowVector3f a1(0,0,0),a2(0,0,0);
  float lower_bound,upper_bound;
  double MAX_RUNS = 20;

  bool valid = igl::opengl::is_valid_nesting(
    dbl_w, h, tex_id, fbo_id, d_id, ren_id, A_va_id, FA_size, B_va_id,
    FB_size, proj_view, model, p(2), np, a1, a2, 0.5); // LWW restrict p to Z value
  tic = igl::get_seconds();
  for(int r = 0;r<MAX_RUNS;r++)
  {
    bool valid = igl::opengl::is_valid_nesting(
      dbl_w, h, tex_id, fbo_id, d_id, ren_id, A_va_id, FA_size, B_va_id,
      FB_size, proj_view, model, p(2), np, a1, a2, 0.5); // LWW restrict p to Z value
  }
  std::cout<<"is_valid_nesting: "<<(igl::get_seconds()-tic)/MAX_RUNS<<std::endl;

  igl::opengl::maximize_nesting_scale(
    dbl_w,h,tex_id,fbo_id,d_id,ren_id,
    A_va_id,FA_size,B_va_id,FB_size,proj_view,
    model,p,np,a1,a2,1e-3,0.,1.,clearance_height,lower_bound,upper_bound);
  tic = igl::get_seconds();
  for(int r = 0;r<MAX_RUNS;r++)
  {
    igl::opengl::maximize_nesting_scale(
      dbl_w,h,tex_id,fbo_id,d_id,ren_id,
      A_va_id,FA_size,B_va_id,FB_size,proj_view,
      model,p,np,a1,a2,1e-3,0.,1.,clearance_height,lower_bound,upper_bound);
  }
  std::cout<<"binary search: "<<(igl::get_seconds()-tic)/MAX_RUNS<<std::endl;
  std::cout<<"lower_bound: "<<lower_bound<<std::endl;

  float max_s;
  Eigen::RowVector3f th,cen;
  int pso_iters = 500;
  int pso_population = 300;
  tic = igl::get_seconds();
  MAX_RUNS = 1;
  for(int r = 0;r<MAX_RUNS;r++)
  {
    max_s = igl::opengl::maximize_nesting_scale(
      VA,FA,
      VB,FB,
      pso_iters, pso_population, clearance_height,
      th, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      cen,igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      p,  igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      np, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      a1, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      a2, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      model);
  }
  std::cout<<"maximize_nesting_scale: "<<(igl::get_seconds()-tic)/MAX_RUNS<<std::endl;
  std::cout<<"max_s: "<<max_s<<std::endl;

  {
    glBindFramebuffer(GL_FRAMEBUFFER,ren_id);
    Eigen::Affine3f cen_model = model;
    cen_model.matrix().col(3).head(3) +=
      (cen_model.matrix().block(0,0,3,3)*cenB.transpose())-cenA.transpose();
    cen_model.matrix().block(0,0,3,3) /= max_s;
    igl::opengl::draw_nesting_depth_layer(
      0,dbl_w,h,d_id,
      A_va_id,FA_size,B_va_id,FB_size,proj_view,
      cen_model,(p-cenA)(2),np,a1,a2,max_s); // LWW restrict p to Z value
    igl::png::render_to_png(C_STR(max_s<<"-"<<prefix<<".png"),dbl_w,h,false,false);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}
