#include "draw_nesting_depth_layer.h"
#include <igl/opengl/create_shader_program.h>
#include <igl/matlab_format.h>
#include <igl/opengl/report_gl_error.h>
#include <iostream>

IGL_INLINE void igl::opengl::draw_nesting_depth_layer(
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
  // const Eigen::RowVector3f & p, // LWW
  const float & pZ,
  const Eigen::RowVector3f & np,
  const Eigen::RowVector3f & a1,
  const Eigen::RowVector3f & a2,
  const float mid_bounds)
{
  Eigen::RowVector3f p;
  p(0) = 0;
  p(1) = 0;
  p(2) = pZ;
  const std::string vertex_shader = R"(
#version 330 core
uniform mat4 proj_view;
uniform mat4 model;
uniform vec4 plane;
in vec3 position;
void main()
{
  vec4 mp = model * vec4(position,1.);
  gl_Position = proj_view * mp;
  gl_ClipDistance[0] = -dot(mp,plane);
}
)";
  const std::string fragment_shader = R"(
#version 330 core
uniform int id;
out vec3 color;
uniform bool first_pass;
uniform float width;
uniform float height;
uniform sampler2D depth_texture;
void main()
{
  color = vec3(
    float(id==0)*float(gl_FrontFacing),
    float(id==0)*float(!gl_FrontFacing),
    float(id!=0));
  color.rgb *= (1.-gl_FragCoord.z);
  if(!first_pass)
  {
    vec2 tex_coord = vec2(float(gl_FragCoord.x)/width,float(gl_FragCoord.y)/height);
    float max_depth = texture(depth_texture,tex_coord).r;
    if(gl_FragCoord.z <= max_depth)
    {
      discard;
    }
  }
}
)";

  // Done once the first time that this method is called
  static bool first_call = true;
  static GLuint prog_id = 0;
  if(first_call)
  {
    // Create a shader program for rendering meshes
    prog_id = 
      igl::opengl::create_shader_program(vertex_shader,fragment_shader,{});
    igl::opengl::report_gl_error("create_shader_program: ");
    // Note: I suppose this isn't very nice because Q_va_id, etc. are never
    // destroyed... But there are already **a lot** of input parameters
    first_call = false;
  }

  // Set up state 
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  //glEnable(GL_CULL_FACE);
  //glCullFace(GL_FRONT);

  glEnable(GL_CLIP_DISTANCE0);
  glClearColor(0.0,0.4,0.7,0.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // select program and attach uniforms
  glUseProgram(prog_id);
  //GLint proj_loc = glGetUniformLocation(prog_id,"proj");
  //glUniformMatrix4fv(proj_loc,1,GL_FALSE,proj.data());
  const bool first_pass = pass == 0;
  glUniform1i(glGetUniformLocation(prog_id,"first_pass"),first_pass);
  for(int sign_pass = 0;sign_pass<2;sign_pass++)
  {
    float sig = sign_pass==0?1.:-1.;
    glViewport(sign_pass*(dbl_w/2),0,(dbl_w/2),h);
    // Set up view based on angle of attack
    // View direction
#warning "should be using a1/a2"
    Eigen::Quaternionf quat =
      Eigen::Quaternionf::FromTwoVectors(sig*np,Eigen::RowVector3f::UnitZ());
    // TODO this should be further rotated to meet angle of attacks
    Eigen::Matrix4f proj_view_sig = proj_view * Eigen::Affine3f(quat).matrix();

    GLint proj_view_loc = glGetUniformLocation(prog_id,"proj_view");
    glUniformMatrix4fv(proj_view_loc,1,GL_FALSE,proj_view_sig.data());
    GLint plane_loc = glGetUniformLocation(prog_id,"plane");
    {
      // Clipping plane
      Eigen::RowVector4f plane_equ;
      plane_equ<<np,np.dot(-p);
      plane_equ *= sig;
      glUniform4fv(plane_loc,1,plane_equ.data());
    }
    glUniform1f(glGetUniformLocation(prog_id,"width"),dbl_w);
    glUniform1f(glGetUniformLocation(prog_id,"height"),h);
    // Draw meshes
    if(!first_pass)
    {
      glUniform1i(glGetUniformLocation(prog_id,"depth_texture"),0);
      glActiveTexture(GL_TEXTURE0 + 0);
      glBindTexture(GL_TEXTURE_2D, d_id[(pass-1)%2]);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLint id_loc = glGetUniformLocation(prog_id,"id");
    GLint model_loc = glGetUniformLocation(prog_id,"model");
    // Draw A
    {
      Eigen::Affine3f model = Eigen::Affine3f::Identity();
      glUniformMatrix4fv(model_loc,1,GL_FALSE,model.matrix().data());
      glUniform1i(id_loc,0);
      glBindVertexArray(A_va_id);
      glDrawElements(GL_TRIANGLES, FA_size, GL_UNSIGNED_INT, 0);
    }
    // Draw B
    {
      Eigen::Affine3f model_scaled = model;
      model_scaled.scale(mid_bounds);
      glUniformMatrix4fv(model_loc,1,GL_FALSE,model_scaled.matrix().data());
      glUniform1i(id_loc,1);
      glBindVertexArray(B_va_id);
      glDrawElements(GL_TRIANGLES, FB_size, GL_UNSIGNED_INT, 0);
    }
  }
  // Clean up state
  glBindVertexArray(0);
}
