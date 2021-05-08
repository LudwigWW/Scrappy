#include "is_valid_nesting.h"
#include "draw_nesting_depth_layer.h"
#include "vertex_array.h"
#include <igl/opengl/create_shader_program.h>
#include <igl/png/render_to_png.h>
#include <igl/opengl/report_gl_error.h>
#include <stb_image_write.h>
#include <Eigen/Geometry>
#include <iostream>
#include <igl/C_STR.h>
#include <vector>
IGL_INLINE bool igl::opengl::is_valid_nesting(
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
  //const Eigen::RowVector3f & p, // LWW
  const float & p,
  const Eigen::RowVector3f & np,
  const Eigen::RowVector3f & a1,
  const Eigen::RowVector3f & a2,
  const float mid_bounds)
{

// For rendering a full-viewport quad, set tex-coord from position
const std::string tex_v_shader = R"(
#version 330 core
in vec3 position;
out vec2 tex_coord;
void main()
{
  gl_Position = vec4(position,1.);
  tex_coord = vec2(0.5*(position.x+1), 0.5*(position.y+1));
}
)";
// Render directly from color or depth texture
const std::string tex_f_shader = R"(
#version 330 core
in vec2 tex_coord;
out vec4 color;
uniform sampler2D cur_color_tex;
uniform sampler2D cur_depth_tex;
uniform sampler2D prev_color_tex;
uniform sampler2D prev_depth_tex;
void main()
{
  vec4 cur_color = texture(cur_color_tex,tex_coord);
  vec4 cur_depth = texture(cur_depth_tex,tex_coord);
  vec4 prev_color = texture(prev_color_tex,tex_coord);
  vec4 prev_depth = texture(prev_depth_tex,tex_coord);
  //bool cur_from_A = cur_depth.r<1 && (cur_color.r>0 || cur_color.g >0);
  bool cur_from_A_front = cur_depth.r<1 && cur_color.r>0;
  bool cur_from_B = cur_depth.r<1 && cur_color.b>0;
  bool prev_from_A_front = prev_depth.r<1 && prev_color.r>0;
  bool prev_from_A_back = prev_depth.r<1 && prev_color.g>0;
  bool prev_from_B = prev_depth.r<1 && prev_color.b>0;
  bool cur_empty = cur_depth.r>=1;
  bool bad = 
    ((prev_from_A_front || prev_from_A_back) && cur_from_B) || 
    (cur_from_A_front && prev_from_B) ||
    (prev_from_B && cur_empty);
  if(bad)
  {
    // draw something so that counted
    color = vec4(1,1,1,1);
  }else
  {
    // if good, discard! (don't count)
    discard;
    //color = vec4(0,0,0,1);
  }
}
)";

  // Done once the first time that this method is called
  static bool first_call = true;
  static GLuint prog_id = 0, tex_prog_id = 0, Q_va_id = 0;
  if(first_call)
  {
    // Create vertex array for a square
    GLuint Q_ab_id,Q_eab_id;
    {
      typedef Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> MatrixfX3R;
      typedef Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> MatrixiX3R;
      // square
      igl::opengl::vertex_array(
        (MatrixfX3R(4,3)<<-1,-1,0,1,-1,0,1,1,0,-1,1,0).finished(),
        (MatrixiX3R(2,3)<< 0,1,2, 0,2,3).finished(),
        Q_va_id,Q_ab_id,Q_eab_id);
    }
    // Create a shader for drawing a textured square
    tex_prog_id = 
      igl::opengl::create_shader_program(tex_v_shader,tex_f_shader,{});
    igl::opengl::report_gl_error("create_shader_program: ");
    // Note: I suppose this isn't very nice because Q_va_id, etc. are never
    // destroyed... But there are already **a lot** of input parameters
    first_call = false;
  }

  // Generate a counter "object"
  static GLuint occ_count_id = 0;
  if(!glIsQuery(occ_count_id))
  {
    glGenQueries(1,&occ_count_id);
  }
  bool valid = true;

  const int max_passes  = 100;
  for(int pass = 0;pass<max_passes;pass++)
  {
    //num_draws++;
    glBeginQuery(GL_ANY_SAMPLES_PASSED,occ_count_id);
    const bool first_pass = pass == 0;
    // we're going to count if any fragments actually get drawn. If none,
    // then we've done enough passes
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id[pass%2]);
    draw_nesting_depth_layer(
      pass,dbl_w,h,d_id,
      A_va_id,FA_size,B_va_id,FB_size,proj_view,
      model,p,np,a1,a2,mid_bounds);
    // Get "count" of fragments drawn: either 1 (some) or 0 (none)
    glEndQuery(GL_ANY_SAMPLES_PASSED);
    //glFinish();
    GLuint draw_count;
    glGetQueryObjectuiv(occ_count_id,GL_QUERY_RESULT,&draw_count);
    //{
    //  // DEBUG render images to file
    //  glBindTexture(GL_TEXTURE_2D,tex_id[pass%2]);
    //  std::vector<unsigned char> pixels(dbl_w*h*4);
    //  glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,&pixels[0]);
    //  bool ret = stbi_write_png(
    //    //C_STR("iter-"<<iter<<"-sig-"<<sign_pass<<"-pass-"<<pass<<".png"),
    //    //C_STR("iter-"<<iter<<"-pass-"<<pass<<".png"),
    //    C_STR("scale-"<<mid_bounds<<"-pass-"<<pass<<".png"),
    //    dbl_w, h, 4, &pixels[0], 4*dbl_w*sizeof(unsigned char));
    //}
    //
    // For each new draw (except first) check whether:
    //   current frag is from B AND prev frag is from A
    //      OR
    //   current frag is empty AND prev frag is from B
    // If true for any frag then B cannot escape along orthogonal view
    // direction.
    if(!first_pass)
    {
      glDisable(GL_CULL_FACE);
      // Full viewport
      glViewport(0,0,dbl_w,h);
      // Counterintuitively we're counting the number of **bad** pixels:
      // we're going to discard all **good** pixels. So if anything is
      // drawn then the state is not valid.
      glBeginQuery(GL_ANY_SAMPLES_PASSED,occ_count_id);
      glBindFramebuffer(GL_FRAMEBUFFER,ren_id);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // draw square
      glUseProgram(tex_prog_id);
      {
        GLint color_tex_loc = glGetUniformLocation(tex_prog_id,"cur_color_tex");
        glUniform1i(color_tex_loc, 0);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, tex_id[pass%2]);
        GLint depth_tex_loc = glGetUniformLocation(tex_prog_id,"cur_depth_tex");
        glUniform1i(depth_tex_loc, 1);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, d_id[pass%2]);
      }
      {
        GLint color_tex_loc = glGetUniformLocation(tex_prog_id,"prev_color_tex");
        glUniform1i(color_tex_loc, 2);
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, tex_id[(pass-1)%2]);
        GLint depth_tex_loc = glGetUniformLocation(tex_prog_id,"prev_depth_tex");
        glUniform1i(depth_tex_loc, 3);
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, d_id[(pass-1)%2]);
      }
      glDisable(GL_DEPTH_TEST);
      glBindVertexArray(Q_va_id);
      glDrawElements(GL_TRIANGLES, 2*3, GL_UNSIGNED_INT, 0);
      glEndQuery(GL_ANY_SAMPLES_PASSED);
      //glFinish();
      // Count bad pixels
      GLuint bad_count;
      glGetQueryObjectuiv(occ_count_id,GL_QUERY_RESULT,&bad_count);
      //igl::png::render_to_png(C_STR("scale-"<<mid_bounds<<"-pass-"<<pass<<"-bad.png"),dbl_w,h,false,false);
      glBindFramebuffer(GL_FRAMEBUFFER,0);
      // If anything bad was drawn then state is invalid
      if(bad_count)
      {
        valid = false;
        break;
      }
      // If we're peeled a pure background layer then nothing new can
      // happen. Note: it's important that this comes at the end and not
      // before the bad_count. Even after peeling an empty layer there
      // can still be a bad fragment if the previous pixel is from B.
      if(!draw_count)
      {
        valid = true;
        break;
      }
    }
    if(pass == max_passes-1)
    {
      //return false;
    }
    if(!valid)
    {
      break;
    }
  }
  return valid;
}
