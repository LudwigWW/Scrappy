#include "include/igl/opengl/prepare_nesting.h"
#include "include/igl/opengl/draw_nesting_depth_layer.h"
#include "include/igl/opengl/is_valid_nesting.h"
#include "include/igl/opengl/vertex_array.h"
#include "include/igl/opengl/maximize_nesting_scale.h"
#include "include/igl/opengl/draw_nesting_depth_layer.h"
#include "include/igl/copyleft/cgal/matryoshka.h"
#include "include/igl/copyleft/cgal/is_valid_nesting.h"
#include <igl/read_triangle_mesh.h>
#include <igl/matlab_format.h>
#include <igl/get_seconds.h>
#include <igl/copyleft/cgal/intersect_with_half_space.h>
#include <igl/ortho.h>
#include <igl/pathinfo.h>
#include <igl/opengl/init_render_to_texture.h>
#include <igl/C_STR.h>
#include <igl/centroid.h>
#include <igl/png/render_to_png.h>
#include <igl/opengl/gl.h>
#include "include/igl/parse_params.h"
#include <igl/opengl/report_gl_error.h>
#include <igl/opengl/glfw/background_window.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <Eigen/Core>
#include <stb_image_write.h>

#include <iostream>
#include <thread>

int main(int argc, const char * argv[])
{
  // Load input mesh(es)
  typedef Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> MatrixfX3R;
  typedef Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> MatrixiX3R;
  int argi = 1;
  int pso_iters = 500;
  int pso_population = 300;
  float clearance_height = 5.0;
  Eigen::RowVector3f th,cen;
  Eigen::RowVector3f p,np,a1,a2;
  std::string flags = "";
  igl::parse_params(argc,argv,argi,flags,
    'N',&np,
    'P',&p,
    'h',&clearance_height,
    'i',&pso_iters,
    'p',&pso_population);

  MatrixfX3R VA,VB;
  MatrixiX3R FA,FB;
  igl::read_triangle_mesh(argv[argi],VA,FA);
  std::string prefix;
  {
    std::string _1,_2,_3;
    igl::pathinfo(argv[argi],_1,_2,_3,prefix);
  }
  if(argc>argi+1)
  {
    // B inside A
    igl::read_triangle_mesh(argv[argi+1],VB,FB);
  }else
  {
    // Self-Matryoshka
    VB = VA;
    FB = FA;
  }

  /*
matryoshka
 -m (random|grid|pso) // method
 -b (0|1)             // binary search
 -p #                 // population size
 -i #                 // iterations
 -o output.txt        // output model, p, np, s
 -q (0|1)             // use background window
 -x initial-guess.txt  // initial guess
  */

  // Create an opengl context (it's a bit strange that this needs to be done
  // _before_ setting up buffers for meshes, but the window instatiates the
  // necessary opengl context)
  GLFWwindow * window;
  igl::opengl::glfw::background_window(window);
  Eigen::Affine3f model;
  const float max_s = igl::opengl::maximize_nesting_scale(
    VA,FA,
    VB,FB,
    pso_iters, pso_population, clearance_height,
    th, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
    cen,igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
    p,  flags.find('P')==std::string::npos ? igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT :  igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED,
    np, flags.find('N')==std::string::npos ? igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT :  igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED,
    a1, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
    a2, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
    model);

  {
    const int dbl_w = 2*512, h = 512;
    GLuint tex_id[2], fbo_id[2], d_id[2];
    GLuint ren_id;
    GLuint A_va_id;
    GLuint B_va_id;
    GLsizei FA_size,FB_size;
    Eigen::Matrix4f proj_view;
    Eigen::RowVector3f cenA,cenB;
    igl::opengl::prepare_nesting(
      VA,FA,VB,FB,dbl_w,h,
      cenA,cenB,tex_id,fbo_id,d_id,ren_id,A_va_id,FA_size,B_va_id,FB_size,proj_view);
      glBindFramebuffer(GL_FRAMEBUFFER,ren_id);
    Eigen::Affine3f cen_model = model;
    cen_model.matrix().col(3).head(3) +=
      (cen_model.matrix().block(0,0,3,3)*cenB.transpose())-cenA.transpose();
    cen_model.matrix().block(0,0,3,3) /= max_s;

  //std::cout<<igl::matlab_format(cen_model.matrix(),"model")<<std::endl;
  //std::cout<<igl::matlab_format(Eigen::RowVector3f(p-cenA),"p")<<std::endl;
  //std::cout<<igl::matlab_format(np,"np")<<std::endl;
  //std::cout<<igl::matlab_format(a1,"a1")<<std::endl;
  //std::cout<<igl::matlab_format(a2,"a2")<<std::endl;

    igl::opengl::draw_nesting_depth_layer(
      0,dbl_w,h,d_id,
      A_va_id,FA_size,B_va_id,FB_size,proj_view,
      cen_model,(p-cenA)(2),np,a1,a2,max_s); // LWW restrict p to Z value
    igl::png::render_to_png(C_STR(max_s<<"-"<<prefix<<".png"),dbl_w,h,false,false);
    glDeleteVertexArrays(1,&A_va_id);
    glDeleteVertexArrays(1,&B_va_id);
  }

  std::cout<<igl::matlab_format(model.matrix(),"model")<<std::endl;
  std::cout<<igl::matlab_format(p,"p")<<std::endl;
  std::cout<<igl::matlab_format(np,"np")<<std::endl;
  std::cout<<igl::matlab_format(max_s,"s")<<std::endl;
  std::cout<<igl::matlab_format(a1,"a1")<<std::endl;
  std::cout<<igl::matlab_format(a2,"a2")<<std::endl;
  std::cout<<prefix<<" "<<max_s<<std::endl;

  glfwDestroyWindow(window);
  glfwTerminate();
}
