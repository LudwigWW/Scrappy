#include "include/igl/opengl/prepare_nesting.h"
#include "include/igl/opengl/draw_nesting_depth_layer.h"
#include "include/igl/opengl/is_valid_nesting.h"
#include "include/igl/opengl/vertex_array.h"
#include "include/igl/opengl/maximize_nesting_scale.h"
#include "include/igl/opengl/draw_nesting_depth_layer.h"
#include "include/igl/copyleft/cgal/matryoshka.h"
#include "include/igl/copyleft/cgal/is_valid_nesting.h"
#include <igl/snap_to_canonical_view_quat.h>
#include <igl/pso.h>
#include <igl/read_triangle_mesh.h>
#include <igl/matlab_format.h>
#include <igl/get_seconds.h>
#include <igl/copyleft/cgal/intersect_with_half_space.h>
#include <igl/ortho.h>
#include <igl/pathinfo.h>
#include <igl/opengl/init_render_to_texture.h>
#include <igl/C_STR.h>
#include <igl/centroid.h>
#include <igl/opengl/create_shader_program.h>
#include <igl/opengl/report_gl_error.h>
#include <igl/png/render_to_png.h>
#include <igl/opengl/gl.h>
#include <igl/opengl/glfw/background_window.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <stb_image_write.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <thread>
#include <stack>

// Global variables so we can use lambdas with GLFW
// width and height of window
int g_width=900,g_height=600;
// highdpi ratio
int g_highdpi=1;
// vertex array 
GLuint g_A_va_id,g_B_va_id;
GLsizei g_FA_size,g_FB_size;
// Transformation parameters
// Projection and view matrix
Eigen::Matrix4f g_proj;
float g_camera_scale,g_camera_init_scale;
Eigen::Quaternionf g_camera_rotation;
Eigen::RowVector3f g_camera_translation,g_camera_init_translation;
// Program
GLuint g_prog_id;
// Key interactions
std::map<int,double> g_key_tics;
int g_modifiers;

struct State
{
  enum Mode
  {
    MODE_NORMAL = 0, // navigation and manipulation
    MODE_SELECT = 1, // selection mode
    NUM_MODES = 2
  } m_mode = MODE_NORMAL;
  float m_T_scale = 1.0;
  Eigen::Quaternionf m_T_rotation;
  Eigen::RowVector3f m_T_center;
  Eigen::RowVector3f m_T_translation;
} g_state;


// Undo Management
std::stack<State> undo_stack,redo_stack;
const auto push_undo = [](State & _s=g_state)
{
  undo_stack.push(_s);
  // clear
  redo_stack = std::stack<State>();
};
const auto undo = []()
{
  if(!undo_stack.empty())
  {
    redo_stack.push(g_state);
    g_state = undo_stack.top();
    undo_stack.pop();
  }
};
const auto redo = []()
{
  if(!redo_stack.empty())
  {
    undo_stack.push(g_state);
    g_state = redo_stack.top();
    redo_stack.pop();
  }
};

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
  if(argc>2)
  {
    // B inside A
    igl::read_triangle_mesh(argv[2],VB,FB);
  }else
  {
    // Self-Matryoshka
    VB = VA;
    FB = FA;
  }

  std::cout<<R"(
vim3d:

H/L        spin left/right about up axis
J/K        tumble toward/away from camera
Z          Snap to canonical view direction
⇧ J/K      Zoom in/out
⇧ Z        Reset Zoom
⌃ H/J/K/L  Pan left/down/up/right
⌃ Z        Reset panning

modeler:
⌃ V        Manipulate object
)";

  glfwInit();
  glfwSetErrorCallback(
    [](int,const char* description){std::cerr<<description<<std::endl;});
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(g_width,g_height,"matryoshka", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
  const auto reshape = 
    [](GLFWwindow* window,int w,int h){
      g_width=w,g_height=h;
      {
        // A little bigger than 1.0 just in case
        float hh = 1.0;
        float near = 0.01;
        float far = 2.*hh;
        float ratio = float(g_width)/float(g_height);
        igl::ortho(-hh*ratio,hh*ratio,-hh,hh,near,far,g_proj);
      }
    };
  glfwSetWindowSizeCallback(window,reshape);
  {
    int fbwidth, fbheight;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    int width_window, height_window;
    glfwGetWindowSize(window, &width_window, &height_window);
    g_highdpi = fbwidth/width_window;
    reshape(window,width_window,height_window);
  }
  glfwSetCharModsCallback(window,
    [](GLFWwindow* window, unsigned int unicode_key, int modifier)
    {
      std::cout<<"char(unicode_key): "<<
        (unsigned char)(unicode_key)<<
        " unicode_key: "<<unicode_key<<std::endl;
      switch(unicode_key)
      {
        case ' ':
          std::cout<<"space"<<std::endl;
          break;
        //case 'h':
        //  g_camera_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf( 0.05,Eigen::Vector3f::UnitY()));
        //  break;
        //case 'l':
        //  g_camera_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(-0.05,Eigen::Vector3f::UnitY()));
        //  break;
        //default:
        //std::cout<<"Unknown unicode_key command: "<<
        //  (unsigned char)(unicode_key)<<" "<<unicode_key<<std::endl;
      }
    });
  glfwSetKeyCallback(window, 
    [](GLFWwindow* window, int key, int scancode, int action, int mod)
    {
      g_modifiers = mod;
      std::cout<<"        "
      " key: "<<key<<
      " char(key): "<<(unsigned char)(key)<<
      " scancode: "<<scancode<<
      " action: "<<action<<
      " modifier: "<<mod<<std::endl;

      if(action==GLFW_PRESS || action== GLFW_REPEAT)
      {
        g_key_tics[key] = igl::get_seconds();
        switch(key)
        {
          // ESC
          case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
        case '[':
          if(mod & GLFW_MOD_CONTROL)
          {
            g_state.m_mode = State::Mode::MODE_NORMAL;
          }
          break;
        case 'V':
          if(mod & GLFW_MOD_CONTROL)
          {
            g_state.m_mode = State::Mode::MODE_SELECT;
          }
          break;
        case 'Z':
          if(mod & GLFW_MOD_SUPER)
          {
            std::cout<<"undo/redo? "<<undo_stack.size()<<std::endl;
            (mod & GLFW_MOD_SHIFT) ? redo() : undo();
          }else
          {
            const auto & snap_transform = [&](
              const float & init_scale,
              const Eigen::RowVector3f & init_translation,
              float & scale,
              Eigen::Quaternionf & rotation,
              Eigen::RowVector3f & center)
            {
              if(mod & GLFW_MOD_SHIFT)
              {
                scale = init_scale;
              }else if(mod & GLFW_MOD_CONTROL)
              {
                center = init_translation;
              }else if(mod == 0)
              {
                igl::snap_to_canonical_view_quat( rotation,1,rotation);
              }
            };
            switch(g_state.m_mode)
            {
              default:
              case State::Mode::MODE_NORMAL:
                snap_transform(
                  g_camera_init_scale,
                  g_camera_init_translation,
                  g_camera_scale,
                  g_camera_rotation,
                  g_camera_translation);
                break;
              case State::Mode::MODE_SELECT:
                snap_transform(
                  1.0,
                  Eigen::RowVector3f(0,0,0),
                  g_state.m_T_scale,
                  g_state.m_T_rotation,
                  g_state.m_T_translation);
                break;
            }
          }

          break;
          //default:
          //  std::cout<<"Unknown key press: "<<
          //    (unsigned char)(key)<<" "<<key<<std::endl;
        }
      }else if(action==GLFW_RELEASE)
      {
        //std::cout<<
        //  (unsigned char)(key)<<" "<<
        //  (igl::get_seconds() - g_key_tics[key])<<" secs"<<std::endl;
        g_key_tics[key] = 0;
      }
    });

  // Prepare vertex arrays
  GLuint A_ab_id,A_eab_id;
  GLuint B_ab_id,B_eab_id;
  igl::opengl::vertex_array(VA,FA,g_A_va_id,A_ab_id,A_eab_id);
  igl::opengl::vertex_array(VB,FB,g_B_va_id,B_ab_id,B_eab_id);
  g_FA_size = FA.size();
  g_FB_size = FB.size();
  // Center the camera around A's bounding sphere (i.e., tightest sphere
  const Eigen::RowVector3f A_translation = 
    0.5*(VA.colwise().maxCoeff() + VA.colwise().minCoeff());
  // Set up constant orthographic projection matrix that fits A and enough
  // surrounding area in view
  // Construct view transformation

  g_camera_init_scale= 1.0/(VA.rowwise()-A_translation).rowwise().norm().maxCoeff();
  g_camera_scale  = g_camera_init_scale;
  g_camera_rotation = Eigen::Quaternionf::Identity();
  g_camera_init_translation = -A_translation;
  g_camera_translation = g_camera_init_translation;
  // Initialize transformation
  g_state.m_T_scale = 1.0;
  g_state.m_T_rotation = Eigen::Quaternionf::Identity();
  igl::centroid(VB,FB,g_state.m_T_center);
  g_state.m_T_translation = Eigen::RowVector3f(0,0,0);


  {
  }



  // Prepare shaders
  const std::string vertex_shader = R"(
#version 330 core
uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
in vec3 position;
void main()
{
  gl_Position = proj * view * model * vec4(position,1.);
}
)";
  const std::string fragment_shader = R"(
#version 330 core
uniform int id;
out vec4 color;
void main()
{
  float alpha = 0.9;
  color = vec4(
    float(id==0)*float(gl_FrontFacing),
    float(id==0)*float(!gl_FrontFacing),
    float(id!=0),
    alpha);
  color.rgb *= (1.-gl_FragCoord.z);
}
)";
  // Create a shader program for rendering meshes
  g_prog_id = 
    igl::opengl::create_shader_program(vertex_shader,fragment_shader,{});

  // Draw loop
  while (!glfwWindowShouldClose(window))
  {
    double tic = igl::get_seconds();
    // Handle "animations"
    const auto & update_transform = [&](
      float & scale,
      Eigen::Quaternionf & rotation,
      Eigen::RowVector3f & center)->bool
    {
      bool something_changed = false;
      const float speed = 4.0;
      const std::map<int,Eigen::RowVector3f > key_trans = 
      {
        {'J',-Eigen::Vector3f::UnitY()},
        {'K', Eigen::Vector3f::UnitY()},
        {'H',-Eigen::Vector3f::UnitX()},
        {'L', Eigen::Vector3f::UnitX()},
      };
      for(const auto & pair : key_trans)
      {
        if(g_key_tics[pair.first] > 0)
        {
          if(g_modifiers == GLFW_MOD_CONTROL)
          {
            float t = speed*0.1*(tic-g_key_tics[pair.first]);
            center += t*pair.second;
            something_changed = true;
            g_key_tics[pair.first] = tic;
          }
        }
      }
      const std::map<int,float> key_scales = {{'J',speed},{'K',1.0/speed}};
      for(const auto & pair : key_scales)
      {
        if(g_key_tics[pair.first] > 0)
        {
          if(g_modifiers == GLFW_MOD_SHIFT)
          {
            float t = (tic-g_key_tics[pair.first]);
            scale *= std::pow(pair.second,t);
            something_changed = true;
            g_key_tics[pair.first] = tic;
          }
        }
      }
      const std::map<int,std::pair<bool,Eigen::RowVector3f> > key_rots = 
      {
        {'J',{false,Eigen::Vector3f::UnitX()}},
        {'K',{false,-Eigen::Vector3f::UnitX()}},
        {'H',{true,-Eigen::Vector3f::UnitY()}},
        {'L',{true,Eigen::Vector3f::UnitY()}},
      };
      for(const auto & pair : key_rots)
      {
        if(g_key_tics[pair.first] > 0)
        {
          if(g_modifiers == 0)
          {
            float t = speed*(tic-g_key_tics[pair.first]);
            const Eigen::RowVector3f & axis = pair.second.second;
            const bool pre_mult = pair.second.first;
            if(pre_mult)
            {
              rotation = rotation * 
                Eigen::Quaternionf(Eigen::AngleAxisf(t,axis));
            }else
            {
              rotation = 
                Eigen::Quaternionf(Eigen::AngleAxisf(t,axis)) 
                * rotation;
            }
            something_changed = true;
            g_key_tics[pair.first] = tic;
          }
        }
      }
      return something_changed;
    };
    switch(g_state.m_mode)
    {
      default:
      case State::Mode::MODE_NORMAL:
        update_transform(g_camera_scale,g_camera_rotation,g_camera_translation);
        break;
      case State::Mode::MODE_SELECT:
        State state_copy = g_state;
        if(update_transform(g_state.m_T_scale,g_state.m_T_rotation,g_state.m_T_translation))
        {
          push_undo(state_copy);
        }
        break;
    }


    // clear screen and set viewport
    {
      Eigen::MatrixXf background_colors(2,4);
      background_colors<<
        1,1,1,0,
        0.8,0.8,0.8,0;
      int c = 0;
      switch(g_state.m_mode)
      {
        default:
        case State::Mode::MODE_NORMAL:
          c = 0;
          break;
        case State::Mode::MODE_SELECT:
          c = 1;
          break;
      }
      glClearColor(
        background_colors(c,0),
        background_colors(c,1),
        background_colors(c,2),
        background_colors(c,3));
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,g_width,g_height);

    // set up program, state and uniforms
    glUseProgram(g_prog_id);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );

    GLint proj_loc = glGetUniformLocation(g_prog_id,"proj");
    glUniformMatrix4fv(proj_loc,1,GL_FALSE,g_proj.data());

    Eigen::Affine3f view = Eigen::Affine3f::Identity();
    view.translate(Eigen::Vector3f(0,0,-1));
    view.scale(g_camera_scale);
    view.rotate(g_camera_rotation);
    view.translate(g_camera_translation.transpose());

    GLint view_loc = glGetUniformLocation(g_prog_id,"view");
    glUniformMatrix4fv(view_loc,1,GL_FALSE,view.matrix().data());
    GLint id_loc = glGetUniformLocation(g_prog_id,"id");
    GLint model_loc = glGetUniformLocation(g_prog_id,"model");
    // Draw B
    {
      Eigen::Affine3f model = Eigen::Affine3f::Identity();

      model.translate(g_state.m_T_center.transpose());
      model.translate(g_state.m_T_translation.transpose());
      model.rotate(g_state.m_T_rotation);
      model.scale(g_state.m_T_scale);
      model.translate(-g_state.m_T_center.transpose());

      glUniformMatrix4fv(model_loc,1,GL_FALSE,model.matrix().data());
      glUniform1i(id_loc,1);
      glBindVertexArray(g_B_va_id);
      glDrawElements(GL_TRIANGLES, g_FB_size, GL_UNSIGNED_INT, 0);
    }
    // Draw A
    {
      Eigen::Affine3f model = Eigen::Affine3f::Identity();
      glUniformMatrix4fv(model_loc,1,GL_FALSE,model.matrix().data());
      glUniform1i(id_loc,0);
      glBindVertexArray(g_A_va_id);
      glDrawElements(GL_TRIANGLES, g_FA_size, GL_UNSIGNED_INT, 0);
    }
    // Clean up vertex array state
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    {
      glfwPollEvents();
      // In microseconds
      double duration = 1000000.*(igl::get_seconds()-tic);
      const double min_duration = 1000000./60.;
      if(duration<min_duration)
      {
        std::this_thread::sleep_for(
          std::chrono::microseconds((int)(min_duration-duration)));
      }
    }
  }
  //Clean up like a good little boy
  glfwDestroyWindow(window);
  glfwTerminate();
}
