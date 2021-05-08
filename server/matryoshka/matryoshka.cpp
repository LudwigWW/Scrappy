#define MATRYOSHKA_DEBUG

#include "include/igl/parse_params.h"
#include "include/igl/copyleft/cgal/matryoshka.h"
#include "include/igl/opengl/maximize_nesting_scale.h"
//#include "include/igl/pca.h"
#include <igl/LinSpaced.h>
#include <igl/copyleft/offset_surface.h>
#include <igl/read_triangle_mesh.h>
#include <igl/write_triangle_mesh.h>
#include <igl/pathinfo.h>
#include <igl/STR.h>
#include <igl/opengl/glfw/background_window.h>
//#include <igl/bounding_box.h>
#include <igl/centroid.h>
#include <igl/combine.h>
#include <boost/geometry/algorithms/centroid.hpp> 
#include "BinPack2D/binpack2d.hpp"
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
// LWW
//#include <igl/copyleft/cgal/complex_to_mesh.h>
#include <igl/copyleft/cgal/convex_hull.h>
#include <igl/bounding_box.h>

//#include <Eigen/IO>
#include <chrono> 

#include <igl/matlab_format.h>


using namespace std::chrono; 


int main(int argc, const char * argv[])
{
  typedef Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> MatrixfX3R;
  typedef Eigen::Matrix<double,Eigen::Dynamic,3,Eigen::RowMajor> MatrixdX3R;
  typedef Eigen::Matrix<  int,Eigen::Dynamic,3,Eigen::RowMajor> MatrixiX3R;

  // Create a background window 
  GLFWwindow * window;
  if(!igl::opengl::glfw::background_window(window))
  {
    std::cerr<<"Failed to create opengl context."<<std::endl;
    return EXIT_FAILURE;
  }

  std::string USAGE = R"(USAGE:

./matryoshka -l 5 input.obj 

All meshes coordinates are interpretted as millimeters

-b  "scale to fit" bounding box (e.g., max out print volume) <=0 means do not
    scale {0}
-e  Minimum wall thickness in mm {2}
-g  Contouring grid resolution {100}
-i  PSO iterations {500}
-k  pack for printing {true}
-l  number of levels (including outer layer) {2}
-p  PSO population {300}
-s  Printer "accuracy" in mm {0.2}
)";
  // Number of levels (including outer shell)
  int num_levels = 0;
  int argi = 1;
  std::string flags = "";
  // Particle Swarm Optimization parameters
  int pso_iters = 500;
  int pso_population = 300;
  // printer accuracy in mm
  double sigma = 0.2;
  // Minimum wall thickness in mm
  double epsilon = 2;
  // Contouring grid resolution
  int grid_resolution = 100;
  // scale the input to fit in a box: <=0 means do not scale
  double box = 0;
  bool pack_for_printing = true;
  // LWW
  float clearance_height = 5.0;
  //Eigen::RowVector3d in_p(0,0,0),in_np(1,0,0); // LWW
  Eigen::RowVector3d in_p(0,0,0),in_np(0,0,1);
  if(!igl::parse_params(argc,argv,argi,flags,
    'b',&box,
    'e',&epsilon,
    'g',&grid_resolution,
    'h',&clearance_height,
    'i',&pso_iters,
    'k',&pack_for_printing,
    'l',&num_levels,
    'N',&in_np,
    'P',&in_p,
    'p',&pso_population,
    's',&sigma))
  {
    std::cout<<"Error: failed to parse params"<<std::endl;
    std::cerr<<USAGE<<std::endl;
    return EXIT_FAILURE;
  }
#ifdef MATRYOSHKA_DEBUG
  std::cout<<"h: "<<clearance_height<<std::endl;
  std::cout<<"i: "<<pso_iters<<std::endl;
  std::cout<<"p: "<<pso_population<<std::endl;
  std::cout<<"g: "<<grid_resolution<<std::endl;
  std::cout<<"s: "<<sigma<<std::endl;
  std::cout<<"b: "<<box<<std::endl;
  std::cout<<"e: "<<epsilon<<std::endl;
  std::cout<<"l: "<<num_levels<<std::endl;
  std::cout<<"p: "<<in_p<<std::endl;
  std::cout<<"np: "<<in_np<<std::endl;

  auto start = high_resolution_clock::now();
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
#endif
  if(argi>=argc)
  {
    std::cout<<"Error: no input file found"<<std::endl;
    std::cerr<<USAGE<<std::endl;
    return EXIT_FAILURE;
  }

  std::vector<std::string> input_filenames;
  for(;argi<argc;argi++)
  {
    input_filenames.emplace_back(argv[argi]);
  }

  if(
    num_levels!=0 && 
    input_filenames.size()>1 && 
    num_levels != input_filenames.size())
  {
    std::cerr<<"overriding '-l "<<num_levels<<"' with "<<
      input_filenames.size()<<" input meshes..."<<std::endl;
  }
  if(input_filenames.size() > 1)
  {
    num_levels = input_filenames.size();
  }
  if(num_levels == 0)
  {
    // Default
    num_levels = 2;
  }
  assert(num_levels>1 && "Number of levels must be greater than two");

#ifdef MATRYOSHKA_DEBUG
  std::cout<<"num_levels: "<<num_levels<<std::endl;
  for(auto filename : input_filenames)
  {
    std::cout<<"input: "<<filename<<std::endl;
  }
#endif


  // scales
  Eigen::RowVectorXd scales(num_levels);
  // Transformed and cut models
  std::vector<MatrixdX3R> VT(num_levels),VL(num_levels),VR(num_levels);
  std::vector<MatrixiX3R>  F(num_levels),FL(num_levels),FR(num_levels);
  std::vector<Eigen::RowVector3d> a1(num_levels),a2(num_levels);
  std::vector<Eigen::Affine3d,Eigen::aligned_allocator<Eigen::Affine3d> > model(num_levels);
  // Center translations for pre-centering hack
  Eigen::RowVector3d centerTranslation[num_levels];
  double iniVolumes[num_levels];
  double distanceToBase;
  //std::vector<Eigen::VectorXi> JL(num_levels),JR(num_levels);
  std::string prefix,ext;
  for(int i = 0;i<num_levels;i++)
  {
    std::cout<<i<<":"<<std::endl;
    MatrixdX3R V;
    std::string input_filename = 
      input_filenames[std::min((int)input_filenames.size()-1,i)];
    igl::read_triangle_mesh(
      input_filename,V,F[i]);
    // Outer shell always has scale = 1;

    // LWW - pre-centering hack
    Eigen::RowVector3d centroidHack(3);
    igl::centroid(V,F[i],centroidHack, iniVolumes[i]);
    V.rowwise() -= centroidHack;
    centerTranslation[i] = centroidHack;

    if (i == 0){
      distanceToBase = V.colwise().minCoeff()[2];
    }  


    // TODO: Make work with more than two levels
    if (i == 1)
    {
      if (iniVolumes[1] >= iniVolumes[0] && box == 0) // model might be scaled up later by box parameter
      {
        std::cout<<"Break earliest, volume too big to ever fit" <<std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
      }
      std::cout<<"<!..>translationOuter<..>" << centerTranslation[0] << "<..>translationInner<..>" << centerTranslation[1] << "<..>distanceToBase<..>" << distanceToBase << "<..><!..>" <<std::endl; // Ending with delimiter since there is more to follow 
    }

    if(i == 0)
    {
      // build the output filename prefix from first input
      {
        std::string d,b,f;
        igl::pathinfo(input_filename,d,b,ext,f);
        prefix = d + "/" + f;
      }

      // Identity transformation
      if(box > 0)
      {
        scales(i) = 
          box/(V.colwise().maxCoeff()-V.colwise().minCoeff()).maxCoeff();
      }else
      {
        scales(i) = 1;
      }
      model[i] = Eigen::Affine3d::Identity();
      model[i].scale(scales(i));
      VT[i] = 
        (V*model[i].matrix().topLeftCorner(3,3).transpose()).rowwise() + 
        model[i].translation().transpose();


      continue;
    }
    // Contract _scaled_ previous layer scales(i-1)*V[i-1],F[i-1] --> CV,CF
    MatrixdX3R CV;
    MatrixiX3R CF;
#ifdef MATRYOSHKA_DEBUG
    igl::write_triangle_mesh(
      STR(prefix<<"-"<<std::setfill('0')<<std::setw(2)<<i<<"-T."<<ext),VT[i-1],F[i-1]);
#endif
    if(epsilon > 0 || sigma > 0)
    {
      Eigen::MatrixXd GV;
      Eigen::RowVector3i side;
      Eigen::VectorXd S;
      std::cout<<"    offsetting..."<<std::endl;
      igl::copyleft::offset_surface(
        VT[i-1],
        F[i-1],
        -(sigma+epsilon),
        grid_resolution,
        igl::SIGNED_DISTANCE_TYPE_WINDING_NUMBER,
        CV,
        CF,
        GV,
        side,
        S);
    }else
    {
#ifdef MATRYOSHKA_DEBUG
      std::cout<<"just using input..."<<std::endl;
#endif
      CV = VT[i-1];
      CF = F[i-1];
    }
    if(CF.size() == 0)
    {
      std::cerr<<"Model is too small. Rescale or use -b option."<<std::endl;
      break;
    }
    // Find nesting orientation, placement and scale of next model within
    // contracted model
    Eigen::RowVector3d th,cen;
    Eigen::RowVector3d p,np;
#ifdef MATRYOSHKA_DEBUG
    std::cout<<"CV: "<<CV.rows()<<","<<CV.cols()<<std::endl;
    std::cout<<"nan police: "<<(CV.array() != CV.array()).any()<<std::endl;
    std::cout<<"CF: "<<CF.rows()<<","<<CF.cols()<<std::endl;
    std::cout<<"V: "<<V.rows()<<","<<V.cols()<<std::endl;
    std::cout<<"F: "<<F[i].rows()<<","<<F[i].cols()<<std::endl;
#endif
#ifdef MATRYOSHKA_DEBUG
    igl::write_triangle_mesh(
      STR(prefix<<"-"<<std::setfill('0')<<std::setw(2)<<i<<"-C."<<ext),CV,CF);
    igl::write_triangle_mesh(
      STR(prefix<<"-"<<std::setfill('0')<<std::setw(2)<<i<<"-V."<<ext),V,F[i]);
#endif
    np = (in_np*model[i-1].matrix().topLeftCorner(3,3).transpose()).normalized();
    p =   in_p*model[i-1].matrix().topLeftCorner(3,3).transpose() + 
      model[i-1].translation().transpose();

    stop = high_resolution_clock::now(); 
    duration = duration_cast<milliseconds>(stop - start); 
    std::cout << "Time taken by setup is : " << std::fixed 
    << duration.count();  
    std::cout << " ms. " << std::endl; 
    start = high_resolution_clock::now();
    std::cout<<"    sanity checking..."<<std::endl;


    // Possibility check

    Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");

    //boost::geometry::centroid(CV, centroidOuter);

    // Load and transform current model
    MatrixiX3R precheckF;
    Eigen::Affine3d precheckModel;
    MatrixdX3R precheckV;
    MatrixdX3R CVprecheck;
    std::string precheckInput_filename = 
      input_filenames[std::min((int)input_filenames.size()-1,i)];
    igl::read_triangle_mesh(
      precheckInput_filename,precheckV,precheckF);

    precheckModel = Eigen::Affine3d::Identity();
    CVprecheck = 
      (precheckV*precheckModel.matrix().topLeftCorner(3,3).transpose()).rowwise() + 
      precheckModel.translation().transpose();
    
    
    #ifdef MATRYOSHKA_DEBUG
        stop = high_resolution_clock::now(); 
        duration = duration_cast<milliseconds>(stop - start); 
        std::cout << "Time taken by Loading is : " << std::fixed 
        << duration.count();  
        std::cout << " ms. " << std::endl; 
        start = high_resolution_clock::now(); 
    #endif


    // First sanity check: mesh volume inner obj. vs outer obj.
    Eigen::VectorXd centroidInner(3);
    double volumeInner;
    igl::centroid(CVprecheck, precheckF, centroidInner, volumeInner);
    // std::cout<<"centroidInner:" << centroidInner << " volumeInner:" << volumeInner <<std::endl;
    
    Eigen::VectorXd centroidOuter(3);
    double volumeOuter;
    igl::centroid(CV, CF, centroidOuter, volumeOuter);
    // std::cout<<"centroidOuter:" << centroidOuter << " volumeOuter:" << volumeOuter <<std::endl;

    if (volumeInner >= volumeOuter)
    {
      std::cout<<"Break early, include volume too big to ever fit" <<std::endl;
      // Breakdown background window
      glfwDestroyWindow(window);
      glfwTerminate();
      return EXIT_FAILURE;
    }

    #ifdef MATRYOSHKA_DEBUG
        stop = high_resolution_clock::now(); 
        duration = duration_cast<milliseconds>(stop - start); 
        std::cout << "Time taken by getting volume is : " << std::fixed 
        << duration.count();  
        std::cout << " ms. " << std::endl; 
        start = high_resolution_clock::now(); 
    #endif

    Eigen::JacobiSVD< Eigen::Matrix<MatrixdX3R::Scalar,Eigen::Dynamic,3> > 
      svd_in( 
        CVprecheck.leftCols(3).rowwise()-CVprecheck.leftCols(3).colwise().mean(),
        Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::Matrix3d C_in = svd_in.matrixV();
    CVprecheck.leftCols(3) *= C_in;
    // handle reflections
    if(C_in.determinant() < 0)
    {
      CVprecheck.col(0).array() *= -1;
    }

    // Transform outer mesh copy
    MatrixdX3R CVcopy = CV;

    // Before determining 2D box, orient so that major principal component is
    // along axis direction (cheap-o oriented bounding box)
    //
    // Should use "rotating calipers" instead, O(n log n) exact solution
    // https://www.geometrictools.com/Documentation/MinimumAreaRectangle.pdf
    Eigen::JacobiSVD< Eigen::Matrix<MatrixdX3R::Scalar,Eigen::Dynamic,3> > 
      svd( 
        CVcopy.leftCols(3).rowwise()-CVcopy.leftCols(3).colwise().mean(),
        Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::Matrix3d C = svd.matrixV();
    CVcopy.leftCols(3) *= C;
    if(C.determinant() < 0)
    {
      CVcopy.col(0).array() *= -1;
    }

    #ifdef MATRYOSHKA_DEBUG
        stop = high_resolution_clock::now(); 
        duration = duration_cast<milliseconds>(stop - start); 
        std::cout << "Time taken by PCA is : " << std::fixed 
        << duration.count();  
        std::cout << " ms. " << std::endl; 
        start = high_resolution_clock::now(); 
    #endif

    // std::cout<<"CV size:" << CV.size() << " CVcopy size:" << CVcopy.size() <<" precheckV size:" << precheckV.size() << " CVprecheck size:" << CVprecheck.size() <<std::endl;
    // std::cout<< C_in.format(CleanFmt) <<std::endl;

    MatrixdX3R BIV;//, BOV;
    MatrixiX3R BIF;//, BOF;
    MatrixdX3R BOV;//, BOV;
    MatrixiX3R BOF;//, BOF;
    igl::bounding_box(CVprecheck, BIV, BIF);
    igl::bounding_box(CVcopy, BOV, BOF);

    // Common debug output meshes
    // igl::write_triangle_mesh( "Out.stl",CVprecheck,precheckF);
    // igl::write_triangle_mesh( "Out2.stl",CVcopy,CF);
    // igl::write_triangle_mesh( "Out3.stl",BIV,BIF);
    // igl::write_triangle_mesh( "Out4.stl",BOV,BOF);

    // double widthInner2 = CVprecheck.col(0).maxCoeff() - CVprecheck.col(0).minCoeff();
    // double heightInner2 = CVprecheck.col(1).maxCoeff() - CVprecheck.col(1).minCoeff();
    // double thickInner2 = CVprecheck.col(2).maxCoeff() - CVprecheck.col(2).minCoeff();

    // double widthInner3 = precheckV.col(0).maxCoeff() - precheckV.col(0).minCoeff();
    // double heightInner3 = precheckV.col(1).maxCoeff() - precheckV.col(1).minCoeff();
    // double thickInner3 = precheckV.col(2).maxCoeff() - precheckV.col(2).minCoeff();

    double widthOuter = CVcopy.col(0).maxCoeff() - CVcopy.col(0).minCoeff();
    double heightOuter = CVcopy.col(1).maxCoeff() - CVcopy.col(1).minCoeff();
    double thickOuter = CVcopy.col(2).maxCoeff() - CVcopy.col(2).minCoeff();

    // std::cout<<"HeightO:" << heightInner3 << " WidthO:" << widthInner3 << " TicknessO:" << thickInner3 <<std::endl;
    // std::cout<<"Height:" << heightInner2 << " Width:" << widthInner2 << " Tickness:" << thickInner2 <<std::endl;
    
    // std::cout<<"HeightOut:" << heightOuter << " WidthOut:" << widthOuter << " TicknessOut:" << thickOuter <<std::endl;


    // Convex hull tests
    //MatrixdX3R CVconvex;
    //MatrixiX3R Fconvex;
    //igl::copyleft::cgal::convex_hull(CV, CVconvex, Fconvex);
    //std::cout << CVprecheck.format(CleanFmt) <<std::endl;
    //std::cout << "-----------------" <<std::endl;
    //std::cout << CVconvex.format(CleanFmt) <<std::endl;

    //igl::write_triangle_mesh("convexInner.stl", CVconvex,Fconvex);





    // First sanity check: Random rotations for oriented BB fit approximation
    srand(time(0));
    //double volInner, maxInner, minInner, minOuter, maxOuter, volOuter;
    //volInner = maxInner = minInner = std::numeric_limits<double>::infinity();
    //volOuter = maxOuter = minOuter = std::numeric_limits<double>::infinity();

    //minOuter = std::min(std::min(widthOuter, heightOuter), thickOuter);
    //maxOuter = std::max(std::max(widthOuter, heightOuter), thickOuter);
    //volOuter = widthOuter * heightOuter * thickOuter;
    double BBOsize[3] = {widthOuter, heightOuter, thickOuter};
    std::sort(std::begin(BBOsize), std::end(BBOsize));
    bool BBfit = false;
    double infinit = INFINITY;
    double BBIbest[3] = {widthOuter*2, heightOuter*2, thickOuter*2};

    // Repeat
    for (int i = 0; i < 10000; i++) {
      // Make new random rotation
      // Eigen::AngleAxisd rollAngle(rand() / (RAND_MAX / M_PI), Eigen::Vector3d::UnitZ());
      // Eigen::AngleAxisd yawAngle(rand() / (RAND_MAX / M_PI), Eigen::Vector3d::UnitY());
      // Eigen::AngleAxisd pitchAngle(rand() / (RAND_MAX / M_PI), Eigen::Vector3d::UnitX());

      Eigen::AngleAxisd rollAngle(0, Eigen::Vector3d::UnitZ());
      Eigen::AngleAxisd yawAngle(0, Eigen::Vector3d::UnitY());
      Eigen::AngleAxisd pitchAngle(0, Eigen::Vector3d::UnitX());
      Eigen::Quaternion<double> rotQuat = rollAngle * yawAngle * pitchAngle;
      Eigen::Matrix3d rotationMatrix = rotQuat.matrix();
      
      // Rotate inner BB
      //MatrixdX3R rotatedBBInner = CVprecheck * rotationMatrix;
      //MatrixdX3R rotatedBBOuter = CVcopy * rotationMatrix;
      MatrixdX3R rotatedBBInner = BIV * rotationMatrix;
      //MatrixdX3R rotatedBBOuter = BOV * rotationMatrix;

      // Get size of principal components
      double widthInner = rotatedBBInner.col(0).maxCoeff() - rotatedBBInner.col(0).minCoeff();
      double heightInner = rotatedBBInner.col(1).maxCoeff() - rotatedBBInner.col(1).minCoeff();
      double thickInner = rotatedBBInner.col(2).maxCoeff() - rotatedBBInner.col(2).minCoeff();

      double BBIsize[3] = {widthInner, heightInner, thickInner};
      std::sort(std::begin(BBIsize), std::end(BBIsize));
      if (BBIsize[2] <= BBIbest[2] && BBIsize[1] * 0.9 <= BBIbest[1] && BBIsize[0] * 0.8 <= BBIbest[0]) {
        BBIbest[0] = BBIsize[0] * 0.8;
        BBIbest[1] = BBIsize[1] * 0.9;
        BBIbest[2] = BBIsize[2];
      }
      if (BBIsize[2] <= BBOsize[2] && BBIsize[1] * 0.9 <= BBOsize[1] && BBIsize[0] * 0.8 <= BBOsize[0]) {
        BBfit = true;
        break;
      }

      

      //std::cout<< BBsize[0] << "," << BBsize[1] << "," << BBsize[2] <<std::endl;
      //double widthOuter = rotatedBBOuter.col(0).maxCoeff() - rotatedBBOuter.col(0).minCoeff();
      //double heightOuter = rotatedBBOuter.col(1).maxCoeff() - rotatedBBOuter.col(1).minCoeff();
      //double thickOuter = rotatedBBOuter.col(2).maxCoeff() - rotatedBBOuter.col(2).minCoeff();

      //minInner = std::min(std::min(std::min(widthInner, heightInner), thickInner), minInner);
      //minOuter = std::min(std::min(std::min(widthOuter, heightOuter), thickOuter), minOuter);
      //maxInner = std::min(std::max(std::max(widthInner, heightInner), thickInner), maxInner);
      //maxOuter = std::min(std::max(std::max(widthOuter, heightOuter), thickOuter), maxOuter);
      //volInner = std::min(volInner, widthInner * heightInner * thickInner);
      //volOuter = std::min(volOuter, widthOuter * heightOuter * thickOuter);
    }
    std::cout<< "Inner: " << BBIbest[0] << "," << BBIbest[1] << "," << BBIbest[2] <<std::endl;
    std::cout<<"Outer: " << BBOsize[0] << "," << BBOsize[1] << "," << BBOsize[2] <<std::endl;
    
    // Perform sanity check 
    //bool tooBigMax = (maxInner > maxOuter); // Inner object has a side that is too long
    //bool tooBigMin = (minInner > minOuter); // Shortest side of inner is bigger than shortest side of outer
    //bool tooBigVol = (volInner > volOuter); // Inner bb is larger than outer bb by volume
    
    
    /*float max1D, max1D_in, max3D;
    max1D = std::max(float(std::max(width, height)), thick);
    max1D_in = std::max(float(std::max(width_in, height_in)), thick_in);
    max3D = sqrt(pow(width, 2) + pow(height, 2) + pow(thick, 2));
    bool tooBig1D = (max1D <= max1D_in);
    bool tooBig3D = (max1D_in >= max3D);
    */

    //std::cout<< i <<": MinInner:" << minInner << " MaxInner:" << maxInner << " volInner: " << volInner <<std::endl;
    //std::cout<< i <<": MinOuter:" << minOuter << " MaxOuter:" << maxOuter << " volOuter: " << volOuter <<std::endl;

    #ifdef MATRYOSHKA_DEBUG
        stop = high_resolution_clock::now(); 
        duration = duration_cast<milliseconds>(stop - start); 
        std::cout << "Time taken by sanity check is : " << std::fixed 
        << duration.count();  
        std::cout << " ms. " << std::endl; 
        start = high_resolution_clock::now(); 
    #endif

    //if (tooBigMax == true || tooBigMin == true || tooBigVol == true)
    if (BBfit == false)
    {
      std::cout<<"Break early, bounding box too big to ever fit" <<std::endl;
      //std::cout<<"b.b. Max dimension too big: " << tooBigMax << " b.b. Min dimension too big: " << tooBigMin << " b.b. Volume too big: " << tooBigVol <<std::endl;
      // Breakdown background window
      glfwDestroyWindow(window);
      glfwTerminate();
      return EXIT_FAILURE;
    }


    std::cout<<"    optimizing..."<<std::endl;
    const double max_s = igl::opengl::maximize_nesting_scale(
      CV,CF,
      V ,F[i],
      pso_iters, pso_population, clearance_height,
      th, igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT, // rotation
      cen,igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT, // position
      p,  flags.find('P')==std::string::npos ? igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT :  igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED,
      np, flags.find('N')==std::string::npos ? igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT :  igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_FIXED,
      a1[i-1], igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      a2[i-1], igl::opengl::MAXIMIZE_NESTING_SCALE_CONSTRAINT_TYPE_DEFAULT,
      model[i]);
#ifdef MATRYOSHKA_DEBUG
    std::cout<<i<<": max_s = "<<max_s<<std::endl;

    stop = high_resolution_clock::now(); 
    duration = duration_cast<milliseconds>(stop - start); 
    std::cout << "Time taken by optimization is : " << std::fixed 
    << duration.count();  
    std::cout << " ms. " << std::endl; 
    start = high_resolution_clock::now(); 
#endif

    // No packing required if the model did not fit with 100% size
    if (max_s < 0.999) 
    {
      std::cout<<"Break early, no fitting alignment found" <<std::endl;
      glfwDestroyWindow(window);
      glfwTerminate();
      return EXIT_FAILURE;
    }
    else
    {
      scales(i) = 1.0;
    }
    // std::string modelMatrixOut = "<!..>";
    Eigen::IOFormat CommaInitFmt(Eigen::StreamPrecision, Eigen::DontAlignCols, " ", "", "<..>", "", "<!..>", "<!..>");
    // std::cout << igl::matlab_format(model[i].matrix(), "model") << std::endl;
    // for (int matrixCols = 0; matrixCols < 4; matrixCols++)
    // {
    //   // std::cout << igl::matlab_format(model[i].matrix(), "model") << std::endl;
    //   modelMatrixOut += "<..>";
    //   // std::cout << model[i].matrix() << std::endl;
    //   modelMatrixOut += "<..>";
    // }
    std::cout << model[i].matrix().format(CommaInitFmt) << std::endl;
    // std::cout << model[i].matrix().transpose().format(CommaInitFmt) << std::endl;
    // std::cout << modelMatrixOut << "<!..>" << std::endl;
    // std::cout << model[i].translation().transpose() << std::endl;

    // scales(i) = max_s;
    std::cout<<"  "<< std::round((scales(i)/scales(i-1))*100) << "%" <<std::endl;

    VT[i] = 
      (V*model[i].matrix().topLeftCorner(3,3).transpose()).rowwise() + 
      model[i].translation().transpose();
    const auto & outname = [](
      const std::string prefix, 
      const int level, 
      const int side, 
      const std::string ext) -> std::string
    {
      return STR(
        prefix<<"-"<<
          std::setfill('0')<<std::setw(2)<<level<<"-"<<side<<"."<<ext);
    };
#ifdef MATRYOSHKA_DEBUG
    igl::write_triangle_mesh(outname(prefix,i-1,2,ext), VT[i-1],F[i-1]);
#endif
    // Create an offset surface of the transformed model
    MatrixdX3R OV;
    MatrixiX3R OF;
    if(epsilon > 0 || sigma > 0)
    {
      Eigen::MatrixXd GV;
      Eigen::RowVector3i side;
      Eigen::VectorXd S;
      std::cout<<"    offsetting..."<<std::endl;
      igl::copyleft::offset_surface(
        VT[i],
        F[i],
        sigma,
        grid_resolution,
        igl::SIGNED_DISTANCE_TYPE_WINDING_NUMBER,
        OV,
        OF,
        GV,
        side,
        S);
    }else
    {
#ifdef MATRYOSHKA_DEBUG
      std::cout<<"just using transformed input..."<<std::endl;
#endif
      OV = VT[i];
      OF = F[i];
    }
#ifdef MATRYOSHKA_DEBUG
    igl::write_triangle_mesh(STR(prefix<<"-"<<std::setfill('0')<<std::setw(2)<<i<<"-O."<<ext),OV,OF);
#endif
    // Carve matryoshka halves
    Eigen::VectorXi JL,JR;
    //Eigen::MatrixXES VSL,VSR;
    //Eigen::MatrixXi FSL,FSR;
#ifdef MATRYOSHKA_DEBUG
    // std::cout<<"np: "<<np<<std::endl;
    // std::cout<<"a1: "<<a1[i-1]<<std::endl;
    // std::cout<<"a2: "<<a2[i-1]<<std::endl;
#endif
    igl::copyleft::cgal::matryoshka(
      VT[i-1],F[i-1],OV,OF,p,np,a1[i-1],a2[i-1],
      VL[i-1],FL[i-1],JL,VR[i-1],FR[i-1],JR);
    igl::write_triangle_mesh(outname(prefix,i-1,0,ext), VL[i-1],FL[i-1]);
    igl::write_triangle_mesh(outname(prefix,i-1,1,ext), VR[i-1],FR[i-1]);

    // Gratuitous copying of left and right (top and bottom) side
    std::vector<MatrixdX3R> VVLandR(2);
    std::vector<MatrixiX3R> FFLandR(2);
    // Combined left+right mesh
    MatrixdX3R VLR;
    MatrixiX3R FLR;

    VVLandR[0] = VL[i-1];
    VVLandR[1] = VR[i-1];
    FFLandR[0] = FL[i-1];
    FFLandR[1] = FR[i-1];

    igl::combine(VVLandR, FFLandR, VLR, FLR);
    igl::write_triangle_mesh(outname(prefix,i-1,99,ext), VLR, FLR);

    if(i == num_levels-1)
    {
      // last item
      //VL[i] = VT[i];
      //FL[i] = F[i];
      //JL[i] = igl::LinSpaced(F[i].size(),0,F[i].size()-1);
      igl::write_triangle_mesh( outname(prefix,i,0,ext),VT[i],F[i],false);
    }
  }




  std::cout<<"    packing..."<<std::endl;
  stop = high_resolution_clock::now(); 
  duration = duration_cast<milliseconds>(stop - start); 
  std::cout << "Time taken by Minkowski is : " << std::fixed 
  << duration.count();  
  std::cout << " ms. " << std::endl; 

  // Gratuitous copying. Hope you have memory.
  std::vector<MatrixdX3R> VV(num_levels*2 - 1);
  std::vector<MatrixiX3R> FF(num_levels*2 - 1);
  for(int i = 0;i<num_levels;i++)
  {
    if(i == num_levels-1)
    {
      VV[2*i] = VT[i];
      FF[2*i] = F[i];
    }
    else
    {
      // TODO: Rotate so that removal direction points along z-axis
      if(pack_for_printing)
      {
        Eigen::Matrix3d R1(Eigen::Quaterniond().setFromTwoVectors(Eigen::RowVector3d::UnitZ(),a1[i]));
        VV[2*i + 0] = VL[i]*R1;
        Eigen::Matrix3d R2(Eigen::Quaterniond().setFromTwoVectors(Eigen::RowVector3d::UnitZ(),a2[i]));
        VV[2*i + 1] = VR[i]*R2;
      }else
      {
        VV[2*i + 0] = VL[i];
        VV[2*i + 1] = VR[i];
      }
      FF[2*i + 0] = FL[i];
      FF[2*i + 1] = FR[i];
    }
  }

  // Pack into 4x3 rectangle
  if(pack_for_printing)
  {
    // should use 2D _oriented_ bounding box...
    BinPack2D::ContentAccumulator<int> inputContent;
    int max_width = 0;
    int max_height = 0;
    for(int i = 0;i<VV.size();i++)
    {
      // Before determining 2D box, orient so that major principal component is
      // along axis direction (cheap-o oriented bounding box)
      //
      // Should use "rotating calipers" instead, O(n log n) exact solution
      // https://www.geometrictools.com/Documentation/MinimumAreaRectangle.pdf
      Eigen::JacobiSVD< Eigen::Matrix<MatrixdX3R::Scalar,Eigen::Dynamic,2> > 
        svd( 
          VV[i].leftCols(2).rowwise()-VV[i].leftCols(2).colwise().mean(),
          Eigen::ComputeThinU | Eigen::ComputeThinV);
      Eigen::Matrix2d C = svd.matrixV();
      VV[i].leftCols(2) *= C;
      // handle reflections
      if(C.determinant() < 0)
      {
#ifdef MATRYOSHKA_DEBUG
        //std::cout<<"C = ["<<C<<"];"<<std::endl;
        //std::cout<<"det(C): "<<C.determinant()<<std::endl;
#endif
        VV[i].col(0).array() *= -1;
      }

      int width = std::ceil(VV[i].col(0).maxCoeff() - VV[i].col(0).minCoeff());
      int height = std::ceil(VV[i].col(1).maxCoeff() - VV[i].col(1).minCoeff());
      max_width = std::max(width,max_width);
      max_height = std::max(height,max_height);
      //std::cout<<"VV size:" << VV[i].size() << " CVcopy size:" << VT[i].size() <<" precheckV size:" <<std::endl;
      inputContent += BinPack2D::Content<int>(
          i, BinPack2D::Coord(), BinPack2D::Size(width, height), false );
    }

    inputContent.Sort();
    int num_bins = 1;
    // hard coded size of stratasys build plate or max_width so BinPack2D
    // doesn't trivially return false
    int bin_w = std::max(254,std::max(max_width,max_height)), bin_h = bin_w;
    BinPack2D::CanvasArray<int> canvasArray = 
      BinPack2D::UniformCanvasArrayBuilder<int>(bin_w,bin_h,num_bins).Build();
    BinPack2D::ContentAccumulator<int> remainder;
    for(int run = 0;;run++)
    {
      if(canvasArray.Place( inputContent, remainder ))
      {
        break;
      }
      num_bins++;
      canvasArray = 
       BinPack2D::UniformCanvasArrayBuilder<int>(bin_w,bin_h,num_bins).Build();
      if(run == 100)
      {
        std::cerr<<"ERROR: bin packing failed. Models too big?"<<std::endl;
        break;
      }
    }
    BinPack2D::ContentAccumulator<int> outputContent;
    canvasArray.CollectContent( outputContent );
    typedef BinPack2D::Content<int>::Vector::iterator binpack2d_iterator;
    for(
      binpack2d_iterator itor = outputContent.Get().begin(); 
      itor != outputContent.Get().end(); 
      itor++ ) 
    {
      const BinPack2D::Content<int> &content = *itor;
      const int &i = content.content;
      if(content.rotated)
      {
        Eigen::Matrix2d R;R<<0,1,-1,0;
        VV[i].leftCols(2) *= R;
      }
      VV[i].rowwise() -= VV[i].colwise().minCoeff();
      VV[i].col(0).array() += content.coord.x + bin_w*content.coord.z;
      VV[i].col(1).array() += content.coord.y;
    }
  }


  // Concatenate meshes
  MatrixdX3R Vall;
  MatrixiX3R Fall;
  {
    // number of meshes
    int k = VV.size();
    assert(k == FF.size() && "#VV should match #FF");
    // total number of vertices
    int M = 0;
    // total number of faces
    int N = 0;
    for(int i = 0;i<k;i++)
    {
      N += VV[i].rows();
      M += FF[i].rows();
    }
    Vall.resize(N,3);
    Fall.resize(M,3);
    {
      int kn = 0;
      int km = 0;
      for(int i = 0;i<k;i++)
      {
        Vall.block(kn,0,VV[i].rows(),VV[i].cols()) = VV[i];
        Fall.block(km,0,FF[i].rows(),FF[i].cols()) = FF[i].array() + kn;
        // update running sums _after_ adding to faces
        kn += VV[i].rows();
        km += FF[i].rows();
      }
    }
  }
  igl::write_triangle_mesh(STR(prefix << "-all." << "obj"),Vall,Fall);

  // Breakdown background window
  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
