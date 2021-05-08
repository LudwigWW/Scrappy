#include "maximize_nesting_scale.h"
#include "draw_nesting_depth_layer.h"
#include "is_valid_nesting.h"
#include "prepare_nesting.h"
#include <igl/random_search.h>
#include <igl/grid_search.h>
#include <igl/pso_earlyExit.h>
//#include <igl/pso.h>
#include <igl/png/render_to_png.h>
#include <igl/opengl/glfw/background_window.h>
#include <igl/matlab_format.h>
#include <igl/C_STR.h>
#include <igl/write_triangle_mesh.h> // LWW
#include <Eigen/Core>
#include <stb_image_write.h>
#include <iostream>
#include <fstream>

IGL_INLINE float igl::opengl::maximize_nesting_scale(
    const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> &VA,
    const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> &FA,
    const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> &VB,
    const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> &FB,
    const int pso_iters,
    const int pso_population,
    const float clearance_height,
    Eigen::RowVector3f &th, const MaximizeNestingScaleConstraintType const_th,
    Eigen::RowVector3f &cen, const MaximizeNestingScaleConstraintType const_cen,
    Eigen::RowVector3f &_p, const MaximizeNestingScaleConstraintType const_p,
    Eigen::RowVector3f &np, const MaximizeNestingScaleConstraintType const_np,
    Eigen::RowVector3f &a1, const MaximizeNestingScaleConstraintType const_a1,
    Eigen::RowVector3f &a2, const MaximizeNestingScaleConstraintType const_a2,
    Eigen::Affine3f &model)
{
  // More innaccous Parameters:
  const int dbl_w = 2 * 512, h = 512;
  const bool early_exit = true;
  const float binary_search_tol = 1e-3;

  // Prepare opengl buffers, precompute centroids and fitted viewports
  GLuint tex_id[2], fbo_id[2], d_id[2];
  GLuint ren_id;
  GLuint A_va_id, A_ab_id, A_eab_id;
  GLuint B_va_id, B_ab_id, B_eab_id;
  GLsizei FA_size, FB_size;
  Eigen::Matrix4f proj_view;
  Eigen::RowVector3f cenA, cenB;
  prepare_nesting(
      VA, FA, VB, FB, dbl_w, h,
      cenA, cenB, tex_id, fbo_id, d_id, ren_id, A_va_id, FA_size, B_va_id, FB_size, proj_view);

  // Helper function that converts translation of B's centroid and rotation
  // about centroid into an Affine transform
  const auto model_matrix = [](
                                const Eigen::RowVector3f &cen,
                                const Eigen::RowVector3f &th) -> Eigen::Affine3f {
    Eigen::Affine3f model = Eigen::Affine3f::Identity();
    model.translate(cen.transpose());
    model.rotate(
        Eigen::AngleAxisf(th(0), Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(th(1), Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(th(2), Eigen::Vector3f::UnitZ()));
    return model;
  };

  Eigen::RowVector3f min_A = VA.colwise().minCoeff() - cenA;
  Eigen::RowVector3f max_A = VA.colwise().maxCoeff() - cenA;
  // p is given in world coordinates but we're conducting everything as if we've
  // moved to the center of A. We'll move the optimized p back at the end.
  Eigen::RowVector3f p = _p - cenA;



  Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> VBCopy = VB; // PCA for ~smallest dimension

  // First move working copy to center of coordinate system (as we do with A)
  //VBCopy = VBCopy.rowwise() - cenB;

  Eigen::JacobiSVD< Eigen::Matrix<Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor>::Scalar,Eigen::Dynamic,3> > 
    svdVB( 
      VBCopy.leftCols(3).rowwise()-VBCopy.leftCols(3).colwise().mean(),
      Eigen::ComputeThinU | Eigen::ComputeThinV);
  Eigen::Matrix3f Cvb = svdVB.matrixV();
  VBCopy.leftCols(3) *= Cvb;
  if(Cvb.determinant() < 0)
  {
    VBCopy.col(0).array() *= -1;
  }

  

  Eigen::RowVector3f min_B = VBCopy.colwise().minCoeff();
  Eigen::RowVector3f max_B = VBCopy.colwise().maxCoeff();
  Eigen::RowVector3f size_B = max_B - min_B;
  Eigen::RowVector3f size_A = max_A - min_A;
  float min_size_B = std::min(std::min(size_B(0), size_B(1)), size_B(2)); // smallest size of all three dimensions
  float height_A = size_A(2); // height search space
  VBCopy.resize(0,0); // free memory
  

  // std::fstream debugFile; // clear debug file
  // debugFile.open("debug.txt",std::fstream::out);
  // debugFile << "";
  // debugFile.close();

  // debugFile.open("debug.txt",std::ios_base::app);
  // debugFile << "Beginning";
  // debugFile.close();

  // std::cout << "cenA: " << cenA << std::endl; // LWW
  // std::cout << "_p: " << _p << std::endl; // LWW
  // std::cout << "p start: " << p << std::endl; // LWW

  // Count number of free variables
  const int dim =
      (const_th == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE || const_th == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT ? 3 : 0) +
      (const_cen == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE || const_cen == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT ? 3 : 0) +
      (const_p == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE || const_p == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT ? 1 : 0) +
      (const_np == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE || const_np == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT ? 2 : 0) +
      (const_a1 == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE ? 2 : 0) +
      (const_a2 == MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE ? 2 : 0);
  Eigen::RowVectorXf X(dim), LB(dim), UB(dim);
  {
    // Step through variables in order
    int k = 0;

    // 3 rotation angles
    switch (const_th)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      // TODO: Why was this LB(k) set to 0 instead of -M_PI?
      // LB(k) = 0;
      LB(k) = -M_PI;
      LB(k + 1) = -M_PI;
      LB(k + 2) = -M_PI;
      UB(k) = M_PI;
      UB(k + 1) = M_PI;
      UB(k + 2) = M_PI;
      X(k) = 0;
      X(k + 1) = 0;
      X(k + 2) = 0;
      k += 3;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      std::cout << "Rotation has been fixed" << std::endl;
      break;
    }

    // 3 translation coordinates
    switch (const_cen)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      for (int c = 0; c < 3; c++)
      {
        // TODO: Could possibly be restricted by half min size of a side of inner object
        LB(k + c) = min_A(c);
        UB(k + c) = max_A(c);
        X(k + c) = 0;
      }
      k += 3;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      break;
    }

    // 3 point on plane coordinates // LWW: Just 1 now 
    switch (const_p)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      // Default behavior will be handled each iteration
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      for (int c = 2; c < 3; c++)
      // int c = 2;
      {
        // std::cout << "min_A:" << min_A(c) << std::endl;
        // std::cout << "max_A:" << max_A(c) << std::endl;
        //LB(k) = min_A(c) + ((1/sqrt(2)) * min_size_B);
        
        // LB(k) = min_A(c) + (0.70710678 * min_size_B) - clearance_height;
        LB(k) = min_A(c);
        //LB(k) = min_A(c);
        UB(k) = max_A(c);
        // X(k) = 0.5 * (min_A(c) + (0.70710678 * min_size_B) - clearance_height + max_A(c)); // new middle after search space reduction
        X(k) = 0;
        std::cout << "Lower_Bound:" << min_A(c) << " Upper_Bound:" << max_A(c) << std::endl;
      }
      k += 1;
      std::cout << "Point on cut plane has _NOT_ been fixed" << std::endl;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      std::cout << "Point on cut plane has been fixed" << std::endl;
      break;
    }

    // 2 coordinates to represent cut plane normal
    switch (const_np)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      // We'll represent a normal as the cosine of the azimuth angle φ and the
      // altitude/inclination angle θ.
      //
      // n = [ sin(θ)*sin(φ)            cos(θ)*sin(φ)            cos(φ) ]
      // n = [ sin(θ)*sin(acos(cos(φ))) cos(θ)*sin(acos(cos(φ))) cos(φ) ]
      // n = [ sin(θ)*sin(acos(c))      cos(θ)*sin(acos(c))      c      ]
      //
      // Q: Is this biased near the [0,0,1] vector? I must have had a reason
      // not to use φ directly... Maybe I thought that this bias was a
      // feature?
      //
      LB(k) = -1;
      UB(k) = 1;
      LB(k + 1) = -M_PI;
      UB(k + 1) = M_PI;
      X(k) = 0;
      X(k + 1) = 0;
      k += 2;
      std::cout << "Cut plane normal has _NOT_ been fixed" << std::endl;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      std::cout << "Cut plane normal has been fixed" << std::endl;
      break;
    }

    // 2 coordinates to represent removal direction
    switch (const_a1)
    {
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      // Removal direction will be represented around the x then y axes in the
      // coordinate frame where the cut plane normal is placed along the
      // z-axes
      LB(k) = 0;
      UB(k) = M_PI * 0.5;
      LB(k + 1) = -M_PI;
      UB(k + 1) = M_PI;
      X(k) = 0;
      X(k + 1) = 0;
      k += 2;
      break;
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      // Default will be handled each iteration
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      assert(false && "fixed removal directions not implemented");
      break;
    }

    // 2 coordinates to represent removal direction
    switch (const_a2)
    {
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      // Removal direction will be represented around the x then y axes in the
      // coordinate frame where the cut plane normal is placed along the
      // z-axes
      LB(k) = 0;
      UB(k) = M_PI * 0.5;
      LB(k + 1) = -M_PI;
      UB(k + 1) = M_PI;
      X(k) = 0;
      X(k + 1) = 0;
      k += 2;
      break;
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      // Default will be handled each iteration
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      assert(false && "fixed removal directions not implemented");
      break;
    }

    assert(k == dim);
  }

  // Inputs:
  //    X  #X vector of free parameters
  // Side-effect Outputs:
  //    th,cen,p,np,a1,a2
  const auto unzip = [&dim,
                      &th, &const_th,
                      &cen, &const_cen,
                      &p, &const_p,
                      &np, &const_np,
                      &a1, &const_a1,
                      &a2, &const_a2](const Eigen::RowVectorXf &X) {
    // Enforce constraints/defaults
    int k = 0;

    switch (const_th)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      th = Eigen::RowVector3f(X(k), X(k + 1), X(k + 2));
      k += 3;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      // Do nothing, continue using `th` as given
      break;
    }

    switch (const_cen)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      cen = Eigen::RowVector3f(X(k), X(k + 1), X(k + 2));
      k += 3;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      // Do nothing, continue using `cen` as given
      break;
    }

    switch (const_p)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      // use center of mass as point on plane
      //p = cen; //LWW
      //break; //LWW
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      //p = Eigen::RowVector3f(X(k), X(k + 1), X(k + 2)); // LWW
      p = Eigen::RowVector3f(0, 0, X(k));
      k += 1;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      // Do nothing, continue using `p` as given
      break;
    }

    switch (const_np)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      np = Eigen::RowVector3f(
          sin(X(k + 1)) * sin(acos(X(k))), cos(X(k + 1)) * sin(acos(X(k))), X(k));
      k += 2;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      // Do nothing, continue using `np` as given
      break;
    }

    // Rotation from normal to z-axis
    Eigen::Quaternionf Z;
    Z.setFromTwoVectors(np, Eigen::RowVector3f::UnitZ());
    switch (const_a1)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      a1 = np;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      a1 =
          Z.conjugate() *
          Eigen::AngleAxisf(X(k + 1), Eigen::Vector3f::UnitZ()) *
          Eigen::AngleAxisf(X(k + 0), Eigen::Vector3f::UnitX()) *
          Z * np;
      k += 2;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      // Do nothing, continue using `a1` as given
      break;
    }

    switch (const_a2)
    {
    default:
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT:
      a2 = -np;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FREE:
      a2 =
          -(Z.conjugate() *
            Eigen::AngleAxisf(X(k + 1), Eigen::Vector3f::UnitZ()) *
            Eigen::AngleAxisf(X(k + 0), Eigen::Vector3f::UnitX()) *
            Z * np);
      k += 2;
      break;
    case MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED:
      // Do nothing, continue using `a2` as given
      break;
    }

    assert(k == dim);
  };

  // conservative upper bound on scale
  float s_upper_bound =
      (VA.colwise().maxCoeff() - VA.colwise().minCoeff()).norm() /
      (VB.colwise().maxCoeff() - VB.colwise().minCoeff()).norm();
  // const float s_upper_bound = 1.1; // LWW: stop searching after position with size 1 was found
  if (s_upper_bound < 1.0) { // Fail state, can't get to full size anyway
    std::cout << "upper bound issue: " << s_upper_bound << "_________________________________" << std::endl; // LWW
    return 0.5;
  }// else if (s_upper_bound_max > 1.2) { // Prevent searching for too long by setting maximum to close to 1.0 (but above 1.0 to ensure larger step size)
  //   s_upper_bound_max = 1.2;
  // }
  // const float s_upper_bound = s_upper_bound_max;

  std::cout << "s_upper_bound: " << s_upper_bound << std::endl; // LWW
  // Persistent variables
  // Maximum feasible value ever scene, used for early exit.
  float max_s = 0;
  // Number of function evals
  int iter = 0;
  // Cost function to be _minimized_
  const std::function<float(Eigen::RowVectorXf &)> f =
      [&unzip,
       &dim,
       &th, &const_th,
       &cen, &const_cen,
       &p, &const_p,
       &np, &const_np,
       &a1, &const_a1,
       &a2, &const_a2,
       &dbl_w, &h, &tex_id, &fbo_id, &d_id, &ren_id, &model_matrix,
       &A_va_id, &FA_size, &B_va_id, &FB_size, &proj_view, &binary_search_tol, &max_s,
       &iter, &early_exit,
       &s_upper_bound, VB, FB, clearance_height, height_A, cenB](Eigen::RowVectorXf &X) -> float {
    unzip(X);

    Eigen::Affine3f model = model_matrix(cen, th);
    float lower_bound, upper_bound;

    //std::cout << "cen: " << cen << std::endl; // LWW
    //std::cout << "p: " << p << std::endl; // LWW


    typedef Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> MatrixfX3R;
    typedef Eigen::Matrix<double,Eigen::Dynamic,3,Eigen::RowMajor> MatrixdX3R;
    typedef Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> MatrixiX3R;

    // Eigen::AngleAxisd rollAngle(rand() / (RAND_MAX / M_PI), Eigen::Vector3d::UnitZ());
    // Eigen::AngleAxisd yawAngle(rand() / (RAND_MAX / M_PI), Eigen::Vector3d::UnitY());
    // Eigen::AngleAxisd pitchAngle(rand() / (RAND_MAX / M_PI), Eigen::Vector3d::UnitX());
    // Eigen::Quaternion<double> rotQuat = rollAngle * yawAngle * pitchAngle;
    // Eigen::Matrix3d rotationMatrix = rotQuat.matrix();
    
    // Rotate inner BB
    //MatrixdX3R rotatedBBInner = CVprecheck * rotationMatrix;
    //MatrixdX3R rotatedBBOuter = CVcopy * rotationMatrix;
    // MatrixdX3R rotatedTranslatedInner = model * VB;
    // //rotatedBBInner.translate(cen.transpose())
    // //MatrixdX3R rotatedBBOuter = BOV * rotationMatrix;

    // // Get size of principal components
    // double widthInner = rotatedTranslatedInner.col(0).maxCoeff() - rotatedTranslatedInner.col(0).minCoeff();
    // double heightInner = rotatedTranslatedInner.col(1).maxCoeff() - rotatedTranslatedInner.col(1).minCoeff();
    // double thickInner = rotatedTranslatedInner.col(2).maxCoeff() - rotatedTranslatedInner.col(2).minCoeff();

    // double BBIsize[3] = {widthInner, heightInner, thickInner};

    //auto rotatedTranslatedInner = model * VB;

    Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");
    //std::cout<< VB.format(CleanFmt) <<std::endl;
    //Eigen::Affine3f rotatedTranslatedInner = VB;
    //Eigen::Affine3f model = Eigen::Affine3f::Identity();

    // std::cout << th[0] << std::endl;
    // std::cout << th[1] << std::endl;
    // std::cout << th[2] << std::endl;

    // float rotX, rotY, rotZ;
    // if (th[0] < 0) {
    //   rotX = 1*M_PI + th[0];
    // } else {
    //   rotX = th[0];
    // }
    // if (th[1] < 0) {
    //   rotY = 1*M_PI + th[1];
    // } else {
    //   rotY = th[1];
    // }
    // if (th[2] < 0) {
    //   rotZ = 1*M_PI + th[2];
    // } else {
    //   rotZ = th[2];
    // }


    // VB.rotate(
    //     Eigen::AngleAxisf(th(0), Eigen::Vector3f::UnitX())
    //     * Eigen::AngleAxisf(th(1), Eigen::Vector3f::UnitY())
    //     * Eigen::AngleAxisf(th(2), Eigen::Vector3f::UnitZ())
    //     );
    // auto rotationM = Eigen::AngleAxisf(th(0), Eigen::Vector3f::UnitX())
    //     * Eigen::AngleAxisf(th(1), Eigen::Vector3f::UnitY())
    //     * Eigen::AngleAxisf(th(2), Eigen::Vector3f::UnitZ());
    
    

    // Eigen::AngleAxis<float> xAngle(rotX, Eigen::Vector3f::UnitX());
    // Eigen::AngleAxis<float> yAngle(rotY, Eigen::Vector3f::UnitY());
    // Eigen::AngleAxis<float> zAngle(rotZ, Eigen::Vector3f::UnitZ());
    // Eigen::Quaternion<float> rotQuat = xAngle * yAngle * zAngle;

    // rotQuat.vec() = th;
    // rotQuat.w() = 1;
    // rotQuat.normalize();
    // Eigen::Matrix3f rotationMatrix = rotQuat.matrix();

    // std::cout<< rotationMatrix.format(CleanFmt) <<std::endl;
    // std::cout << igl::matlab_format(rotationMatrix, "rotationMatrix") << std::endl;
    // std::cout << igl::matlab_format(th, "th") << std::endl;




    // MatrixfX3R rotatedTranslatedInner = VB;
    // Eigen::JacobiSVD< Eigen::Matrix<MatrixdX3R::Scalar,Eigen::Dynamic,3> > 
    //   svd_B( 
    //     rotatedTranslatedInner.leftCols(3).rowwise()-rotatedTranslatedInner.leftCols(3).colwise().mean(),
    //     Eigen::ComputeThinU | Eigen::ComputeThinV);
    // Eigen::Matrix3d CB = svd_B.matrixV();
    // rotatedTranslatedInner.leftCols(3) *= CB;
    // // handle reflections
    // if(CB.determinant() < 0)
    // {
    //   rotatedTranslatedInner.col(0).array() *= -1;
    // }
    
    // MatrixfX3R rotatedTranslatedInner = VB * rotationMatrix;
    // MatrixfX3R rotatedTranslatedInner =  rotationM * VB;

    // Eigen::MatrixXd 
    MatrixfX3R rotatedTranslatedInner = VB;
    // Move towards center first, since all happens around center of A / 0,0,0
    //rotatedTranslatedInner = rotatedTranslatedInner.rowwise() - cenB;
    rotatedTranslatedInner = (rotatedTranslatedInner*model.matrix().topLeftCorner(3,3).transpose()).rowwise() + model.translation().transpose();
    
    // MatrixfX3R VBi = VB.row(i).transpose();

    // MatrixfX3R VBt = VB;
    // for (int i = 0; i <3; i++) {
    //   VBt.row(i) = (model * VB.row(i).transpose()).transpose().eval();
    // }

    // MatrixfX3R rotatedTranslatedInner = VBt;
    

    // float heightInner = rotatedTranslatedInner.col(2).maxCoeff() - rotatedTranslatedInner.col(2).minCoeff();
    // // float heightInner = 100;
    // float heightInnerPre = VB.col(2).maxCoeff() - VB.col(2).minCoeff();
    // std::cout << "Max:" << rotatedTranslatedInner.col(2).maxCoeff() << std::endl; 
    // std::cout << "Min:" << rotatedTranslatedInner.col(2).minCoeff() << std::endl; 

    // float topHeight = 100;
    // float topHeight = (rotatedTranslatedInner.col(2).maxCoeff()) + cen[2];
    float topHeight = (rotatedTranslatedInner.col(2).maxCoeff());
    // float botHeight = (rotatedTranslatedInner.col(2).minCoeff());
    // float topHeight2 = rotatedTranslatedInner.col(2).lpNorm<Eigen::Infinity>();
    // if (topHeight - topHeight2 > 0.1 || topHeight - topHeight2 < 0.1) {
    //   std::cout << "maxCoeff:" << topHeight << " lpNorm:" << topHeight2 << std::endl;
    //   std::cout << "maxCoeffDiff:" << topHeight - p[2] << " lpNormDiff:" << topHeight2 - p[2] << std::endl;
    // }
    

    float distance;
    // std::cout << "clearance_height:" << clearance_height << std::endl;
    // std::cout << "upper:" << (topHeight) << std::endl;
    // std::cout << "lower:" << (topHeight - clearance_height) << std::endl;
    
    std::fstream debugFile;
    // debugFile.open("debug.txt",std::ios_base::app);
    // debugFile << "Findme\n";
    // debugFile << "Top: "<< topHeight << " Bot: " << botHeight << " Size: " << topHeight - botHeight <<"\n";

    // TODO REMOVEEEEE
    topHeight = topHeight;

    bool exit = false;
    if (p[2] < (topHeight - clearance_height)) {

      distance = (topHeight - clearance_height) - p[2];
      //distance = 0;
      // std::cout << "miss under:" << distance << std::endl;
      // debugFile << "0\n";
      //return 0;
      exit = true;
    }
    else if (p[2] > topHeight) {
      distance = p[2] - topHeight;
      //distance = 0;
      // std::cout << "miss over:" << distance << std::endl;
      // debugFile << "0\n";
      //return 0;
      exit = true;
    }
    else {
      // debugFile << "-----------\n";
      // std::cout << "hit" << std::endl;
    }
    // debugFile.close();


    // if (p[2] > 11.5) {
    //   exit = false;
    //   // std::cout << igl::matlab_format(p, "p") << std::endl;
    // }
    // else {
    //   exit = true;
    // }


    // Common debug output meshes 
    if (false) {//(heightInner - heightInnerPre > 10 || heightInnerPre - heightInner > 10) {
      // std::cout << heightInner - heightInnerPre << std::endl;
      igl::write_triangle_mesh("Out5.stl",VB,FB);
      igl::write_triangle_mesh("Out6.stl",rotatedTranslatedInner,FB);
    }



    if (exit == true) {
      if (-distance > max_s)
      {
        std::cout<< "Warning: Recovery procedure for a case that shouldn't happen executed." <<std::endl;
        max_s = -distance;
      }
      // std::cout << "out_iter: " << iter << std::endl; // LWW
      // iter++;
      // float gradientValue = (-distance / height_A) * 0.01;
      float normalizedDistance = (distance / height_A);
      // float biggerDistance = 5 * distance;
      return normalizedDistance;
      // return biggerDistance;
    }
    // Eigen::Quaternion<double> rotQuat = th.col(0) * th[1] * th[2];
    // Eigen::Matrix3d rotationMatrix = rotQuat.matrix();
    // MatrixdX3R rotatedTranslatedInner = VB * rotationMatrix;


    // rotatedTranslatedInner.translate(cen.transpose());
    // rotatedTranslatedInner.rotate(
    //     Eigen::AngleAxisf(th(0), Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(th(1), Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(th(2), Eigen::Vector3f::UnitZ()));
    

    //std::cout<< igl::matlab_format(VB, "VB") <<std::endl;


    // std::cout<< igl::matlab_format(model.matrix(), "model") <<std::endl;
    // std::cout << igl::matlab_format(th, "th") << std::endl;
    // std::cout << igl::matlab_format(cen, "cen") << std::endl;
    // std::cout << igl::matlab_format(p, "p") << std::endl;
    // std::cout << igl::matlab_format(np, "np") << std::endl;
    // std::cout << igl::matlab_format(a1, "a1") << std::endl;
    // std::cout << igl::matlab_format(a2, "a2") << std::endl;


    //float testing;
    //testing = igl::opengl::maximize_nesting_scale(
    igl::opengl::maximize_nesting_scale(
        dbl_w, h, tex_id, fbo_id, d_id, ren_id,
        A_va_id, FA_size, B_va_id, FB_size, proj_view,
        model, p, np, a1, a2, binary_search_tol, early_exit ? max_s : 0.,
        s_upper_bound, clearance_height, 
        lower_bound, upper_bound);

    //std::cout << "OuterCall mid_bounds: " << testing << " lower bound: " << lower_bound << " max_s: " << max_s << std::endl; // LWW


    if (lower_bound > max_s)
    {
      max_s = lower_bound;

      //#define RENDER_NEW_MAX
      #ifdef RENDER_NEW_MAX
            glBindFramebuffer(GL_FRAMEBUFFER, ren_id);

            std::cout << igl::matlab_format(model.matrix(), "model") << std::endl;
            std::cout << igl::matlab_format(p, "p") << std::endl;
            std::cout << igl::matlab_format(np, "np") << std::endl;
            std::cout << igl::matlab_format(a1, "a1") << std::endl;
            std::cout << igl::matlab_format(a2, "a2") << std::endl;

            draw_nesting_depth_layer(
                0, dbl_w, h, d_id,
                A_va_id, FA_size, B_va_id, FB_size, proj_view,
                model, p, np, a1, a2, max_s);
            igl::png::render_to_png(C_STR("max_s-" << max_s << ".png"), dbl_w, h, false, false);
            std::cout << max_s << std::endl;
            std::cout << "+++ X: " << X << std::endl;
      #endif
    }
    // std::cout << "original_iter: " << iter << std::endl; // LWW
    // iter++;
    // Return minus lower bound because we want to maximize
    //std::cout << "Done F " << -lower_bound << std::endl; // LWW
    return -lower_bound;
  };

  enum GlobalOptimizationMethod
  {
    GLOBAL_OPTIMIZATION_METHOD_GRID_SEARCH = 0,
    GLOBAL_OPTIMIZATION_METHOD_RANDOM_SEARCH = 1,
    GLOBAL_OPTIMIZATION_METHOD_PSO = 2,
    NUM_GLOBAL_OPTIMIZATION_METHOS = 3
  } method = GLOBAL_OPTIMIZATION_METHOD_PSO;

  float fval;
  switch (method)
  {
  case GLOBAL_OPTIMIZATION_METHOD_GRID_SEARCH:
    fval = igl::random_search(f, LB, UB, pso_iters, X);
    break;
  case GLOBAL_OPTIMIZATION_METHOD_RANDOM_SEARCH:
  {
    Eigen::RowVectorXi I(dim);
    I.setConstant(10);
    fval = igl::grid_search(f, LB, UB, I, X);
    break;
  }
  default:
  case GLOBAL_OPTIMIZATION_METHOD_PSO:
    fval = igl::pso_earlyExit(f, LB, UB, pso_iters, pso_population, X);
    break;
  }

  // Final unzip so that outputs are set according to winning X
  max_s = -fval;
  // LWW TODO: set to exactly 1.0 if result was good
  // std::cout << "p before unzip: " << p << std::endl; // LWW
  unzip(X);
  {
    model = model_matrix(cen, th);
    // Karl Sjuniors
    //np = Eigen::RowVector3f(0.03,-0.09,-1);
    //np = Eigen::RowVector3f(0.0,0.0,1.0);
    //p = Eigen::RowVector3f(115,111,128);
    // std::cout << "p after unzip: " << p << std::endl; // LWW
  }

  model.matrix().block(0, 0, 3, 3) *= max_s;
  model.matrix().col(3).head(3) +=
      (model.matrix().block(0, 0, 3, 3) * -cenB.transpose()) + cenA.transpose();
  _p = p + cenA;
  // std::cout << "_p plus cenA: " << _p << std::endl; // LWW
  // Output for communication with client // TODO: Make a better communicaton channel than streams of strings.
  std::cout << "<!..>cen<..>" << cen << "<..>th<..>" << th << "<..>p<..>" << p << "<..>_p<..>" << _p << "<!..>" << std::endl; // LWW

  // Clean up
  glDeleteVertexArrays(1, &A_va_id);
  glDeleteBuffers(1, &A_ab_id);
  glDeleteBuffers(1, &A_eab_id);
  glDeleteVertexArrays(1, &B_va_id);
  glDeleteBuffers(1, &B_ab_id);
  glDeleteBuffers(1, &B_eab_id);
  glUseProgram(0);
  //std::cout << "max_s: " << max_s << std::endl; // LWW
  return max_s;
}

IGL_INLINE double igl::opengl::maximize_nesting_scale(
    const Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor> &VA,
    const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> &FA,
    const Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor> &VB,
    const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> &FB,
    const int pso_iters,
    const int pso_population,
    const float clearance_height,
    Eigen::RowVector3d &th, const MaximizeNestingScaleConstraintType const_th,
    Eigen::RowVector3d &cen, const MaximizeNestingScaleConstraintType const_cen,
    Eigen::RowVector3d &_p, const MaximizeNestingScaleConstraintType const_p,
    Eigen::RowVector3d &np, const MaximizeNestingScaleConstraintType const_np,
    Eigen::RowVector3d &a1, const MaximizeNestingScaleConstraintType const_a1,
    Eigen::RowVector3d &a2, const MaximizeNestingScaleConstraintType const_a2,
    Eigen::Affine3d &model)
{
  const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> f_VA = VA.cast<float>();
  const Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> f_VB = VB.cast<float>();

  Eigen::RowVector3f f_th = th.cast<float>();
  Eigen::RowVector3f f_cen = cen.cast<float>();
  Eigen::RowVector3f f_p = _p.cast<float>();
  Eigen::RowVector3f f_np = np.cast<float>();
  Eigen::RowVector3f f_a1 = a1.cast<float>();
  Eigen::RowVector3f f_a2 = a2.cast<float>();
  Eigen::Affine3f f_model = model.cast<float>();
  double max_s = maximize_nesting_scale(
      f_VA, FA, f_VB, FB, pso_iters, pso_population, clearance_height,
      f_th, const_th,
      f_cen, const_cen,
      f_p, const_p,
      f_np, const_np,
      f_a1, const_a1,
      f_a2, const_a2,
      f_model);
  th = f_th.cast<double>();
  cen = f_cen.cast<double>();
  _p = f_p.cast<double>();
  np = f_np.cast<double>();
  a1 = f_a1.cast<double>();
  a2 = f_a2.cast<double>();
  model = f_model.cast<double>();
  //std::cout << "max_s2: " << max_s << std::endl; // LWW
  return max_s;
}

IGL_INLINE bool igl::opengl::maximize_nesting_scale(
    const GLsizei dbl_w,
    const GLsizei h,
    const GLuint *tex_id,
    const GLuint *fbo_id,
    const GLuint *d_id,
    const GLuint ren_id,
    const GLuint &A_va_id,
    const GLsizei FA_size,
    const GLuint &B_va_id,
    const GLsizei FB_size,
    const Eigen::Matrix4f &proj_view,
    const Eigen::Affine3f &model,
    const Eigen::RowVector3f &p,
    const Eigen::RowVector3f &np,
    const Eigen::RowVector3f &a1,
    const Eigen::RowVector3f &a2,
    const float tol,
    const float max_s,
    const float in_upper_bound,
    const float clearance_height, 
    float &lower_bound,
    float &upper_bound)
{
  // This has really just become a glorified binary search with an early
  // exit...

  upper_bound = in_upper_bound;
  lower_bound = 0.0;

  // LWW
  //upper_bound = 1.0;
  //lower_bound = 1.0;

  int iter = 0;
  //int num_draws = 0;

  float mid_bounds = max_s > 0 ? max_s : 0.5 * (upper_bound + lower_bound);
  float last_mid_bounds = mid_bounds;
  while (true)
  {
    if (mid_bounds > 1.0) { // We may jump past 1.0 due to higher upper bound, but only test up until 100% size
      mid_bounds = 1.0;
    }
    if (last_mid_bounds != mid_bounds)
    {
      // std::cout << "mid_bounds: " << mid_bounds << std::endl; // LWW: cout only if it changed
    }
    last_mid_bounds = mid_bounds;
    assert(mid_bounds <= upper_bound);
    assert(mid_bounds >= lower_bound);
    bool valid = is_valid_nesting(
        dbl_w,
        h,
        tex_id,
        fbo_id,
        d_id,
        ren_id,
        A_va_id,
        FA_size,
        B_va_id,
        FB_size,
        proj_view,
        model,
        p(2),
        np,
        a1,
        a2,
        mid_bounds);

    if (valid)
    {
      lower_bound = mid_bounds;
      if (lower_bound >= 1.0) 
        {
          std::cout<<"lower bound exit reached: "<<iter<<std::endl;
          return true; // LWW: return mid_bounds;
        }
    }
    else
    {
      upper_bound = mid_bounds;
    }
    if (upper_bound <= max_s)
    {
      upper_bound = max_s - 1e-16;
      lower_bound = upper_bound;
      //std::cout<<"iter: "<<iter<<std::endl;
      break;
    }

    if ((upper_bound - lower_bound) < tol)
    {
      std::cout << "iter:" << iter << ": (tolerance reached) lower bound: " << lower_bound << std::endl;
      break;
    }
    iter++;
    mid_bounds = 0.5 * (upper_bound + lower_bound);
  }
  //std::cout<<"num_draws: "<<num_draws<<std::endl;
  // Strictly less than
  return lower_bound < upper_bound;
  //return mid_bounds;
}
