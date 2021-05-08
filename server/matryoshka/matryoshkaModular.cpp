//#define MATRYOSHKA_DEBUG
#define MATRYOSHKA_VERBOSE

#include "include/igl/parse_params.h"
#include "include/igl/copyleft/cgal/matryoshka.h"
#include "include/igl/opengl/maximize_nesting_scale.h"
#include <igl/copyleft/cgal/intersect_with_half_space.h>
#include <igl/copyleft/cgal/mesh_boolean.h>
//#include "include/igl/pca.h"
#include <igl/LinSpaced.h>
#include <igl/copyleft/offset_surface.h>
#include <igl/read_triangle_mesh.h>
#include <igl/write_triangle_mesh.h>
#include <igl/pathinfo.h>
#include <igl/STR.h>
#include <igl/opengl/glfw/background_window.h>
#include <igl/MeshBooleanType.h>
//#include <igl/bounding_box.h>
#include <igl/centroid.h>
#include <igl/combine.h>
//#include <igl/decimate.h>
#include <igl/qslim.h>
#include <boost/geometry/algorithms/centroid.hpp> 
#include <boost/lexical_cast.hpp>
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

#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
    -h  Clearance_height in mm {0.2}
    -x  Continuation file for the mesh only mode
    -z  Matryoshka mode {0} 0=Original, 1=Only Position, 2=Mesh only, position is given
    -c  Path to high quality outer file
    -d  Path to high quality scrap file
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
  float clearance_height = 0.2;
  std::string contFilePath = "";
  int mMode = 0;
  std::string hqmodelPath = "";
  std::string hqscrapPath = "";
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
    's',&sigma,
    'x',&contFilePath,
    'c',&hqmodelPath,
    'd',&hqscrapPath,
    'z',&mMode))
  {
    std::cout<<"Error: failed to parse params"<<std::endl;
    std::cerr<<USAGE<<std::endl;
    return EXIT_FAILURE;
  }
  #ifdef MATRYOSHKA_DEBUG
  std::cout<<"Matryoshka mode: "<<mMode<<std::endl;
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
  std::cout<<"continuation file: "<<contFilePath<<std::endl;
  #endif

  // Not debug since it is required for contfile
  auto start = high_resolution_clock::now();
  milliseconds startMs = duration_cast<milliseconds>(start.time_since_epoch());
  unsigned int startInt = startMs.count();
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  



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

  // JSON variables
  namespace pt = boost::property_tree;
  pt::ptree matryoshkaOutRoot;

  std::string modelFilePath = input_filenames[0];
  std::string scrapFilePath = input_filenames[1];

  // Make matryoshka ignore the type of stls (Full STLs, simplified STLs, blob STLs)
  std::string::size_type blob_i = modelFilePath.find("_Blob");
  std::string::size_type full_i = modelFilePath.find("_Full");
  std::string::size_type simp_i = modelFilePath.find("_Simp");

  if (blob_i != std::string::npos)
    modelFilePath.erase(blob_i, 5);
  else if (full_i != std::string::npos)
    modelFilePath.erase(full_i, 5);
  else if (simp_i != std::string::npos)
    modelFilePath.erase(simp_i, 5);

  blob_i = scrapFilePath.find("_Blob");
  full_i = scrapFilePath.find("_Full");
  simp_i = scrapFilePath.find("_Simp");

  if (blob_i != std::string::npos)
    scrapFilePath.erase(blob_i, 5);
  else if (full_i != std::string::npos)
    scrapFilePath.erase(full_i, 5);
  else if (simp_i != std::string::npos)
    scrapFilePath.erase(simp_i, 5);

  // JSON proof the paths
  char chars[] = "./"; // Remove these chars to make sure JSON filename is valid

  for (unsigned int stringplace = 0; stringplace < strlen(chars); ++stringplace)
  {
      modelFilePath.erase(std::remove(modelFilePath.begin(), modelFilePath.end(), chars[stringplace]), modelFilePath.end());
      scrapFilePath.erase(std::remove(scrapFilePath.begin(), scrapFilePath.end(), chars[stringplace]), scrapFilePath.end());
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

  if (mMode == 1){ // Positioning mode: Save info in positioning mode for mesh-gen later
    contFilePath = "/path/to/scrappyLibrary/contFiles/" + std::to_string(startInt) + modelFilePath + scrapFilePath + ".json"; // Adjust path to library // TODO: Make a config for this

    matryoshkaOutRoot.put("contFilePath", contFilePath); // Self reference to file path, for test access purposes
    matryoshkaOutRoot.put("modelPath", modelFilePath);
    matryoshkaOutRoot.put("scrapPath", scrapFilePath);
    matryoshkaOutRoot.put("sigma", sigma);
    matryoshkaOutRoot.put("epsilon", epsilon);

    #ifdef MATRYOSHKA_VERBOSE
    std::cout<<"contFilePath: " << contFilePath << std::endl;
    #endif
    // contFile.close();

  }
  else if (mMode == 2){ // Load previous info for mesh-gen mode
    // Create a root
    pt::ptree matryoshkaInRoot;

    // Load the json file in this ptree
    pt::read_json(contFilePath, matryoshkaInRoot);

    double sigmaPrev = matryoshkaInRoot.get<double>("sigma");
    double epsilonPrev = matryoshkaInRoot.get<double>("epsilon");
    std::string modelPathPrev = matryoshkaInRoot.get<std::string>("modelPath");
    std::string scrapPathPrev = matryoshkaInRoot.get<std::string>("scrapPath");

    if (sigma != sigmaPrev || epsilon != epsilonPrev || modelPathPrev != modelFilePath || scrapPathPrev != scrapFilePath){
      std::cout<<"Continuation was called with the wrong parameters:" <<std::endl;
      std::cout<<"Prev: " << sigmaPrev << ", " << epsilonPrev << ", " << modelFilePath << ", " << scrapFilePath << std::endl;
      std::cout<<"New: " << sigma << ", " << epsilon << ", " << modelPathPrev << ", " << scrapPathPrev << std::endl;
      glfwDestroyWindow(window);
      glfwTerminate();
      return EXIT_FAILURE;
    }


    // int matrix[3][3];
    // int x = 0;
    // for (pt::ptree::value_type &row : matryoshkaInRoot.get_child("modelMatrix"))
    // {
    //     int y = 0;
    //     for (pt::ptree::value_type &cell : row.second)
    //     {
    //         matrix[x][y] = cell.second.get_value<int>();
    //         y++;
    //     }
    //     x++;
    // }

    //   std::cout << "Matrix :" << std::endl;
    //   for (int i = 0; i < 3; i++)
    //   {
    //       for (int j = 0; j < 3; j++)
    //           std::cout << " " << matrix[i][j];
    //       std::cout << std::endl;
    //   }
  }

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
  std::string prefix,ext,scrapFile,scrapExt;
  for(int i = 0;i<num_levels;i++)
  {
    #ifdef MATRYOSHKA_DEBUG
    std::cout<<i<<":"<<std::endl;
    #endif
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
        #ifdef MATRYOSHKA_VERBOSE
        std::cout<<"Break earliest, volume too big to ever fit" <<std::endl;
        #endif
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
      }
      // Communication with client output  // TODO: Make a better communication channel than a stream of strings
      std::cout<<"<!..>translationOuter<..>" << centerTranslation[0] << "<..>translationInner<..>" << centerTranslation[1] << "<..>distanceToBase<..>" << distanceToBase << "<..>scrapVolume<..>" << iniVolumes[1] << "<..>contFile<..>" << contFilePath << "<..><!..>" <<std::endl; // Ending with delimiter since there is more to follow 
    }

    if(i == 0)
    {
      // build the output filename prefix from first input
      {
        std::string d,b,f;
        igl::pathinfo(input_filename,d,b,ext,f);
        // -5 removes the "_FULL" or "_SIMP" or "_BLOB" part
        prefix = d + "/exportedDesigns/" + f.substr(0, f.size()-5); // TODO: Remove hard coded path
        std::string d2,b2,f2;
        igl::pathinfo(input_filenames[1],d2,b2,scrapExt,f2);
        scrapFile = f2.substr(0, f2.size()-5);
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


      continue; // This continue was so much trouble
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
      #ifdef MATRYOSHKA_DEBUG
      std::cout<<"    offsetting..."<<std::endl;
      #endif
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

    #ifdef MATRYOSHKA_DEBUG
    stop = high_resolution_clock::now(); 
    duration = duration_cast<milliseconds>(stop - start); 
    std::cout << "Time taken by setup is : " << std::fixed 
    << duration.count();  
    std::cout << " ms. " << std::endl; 
    start = high_resolution_clock::now();
    #endif

    #ifdef MATRYOSHKA_VERBOSE
    std::cout<<"    sanity checking..."<<std::endl;
    #endif


    if (mMode != 2){ // Positioning not necessary in mesh-gen-only mode

    
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
      // if (num_levels == 2) {
        
      // TODO: Could this cause problems for extra levels, since inner ones get offset by a previous run?
      volumeInner = iniVolumes[i];
      centroidInner = centerTranslation[1];


      // } else {
        // igl::centroid(CVprecheck, precheckF, centroidInner, volumeInner);
      // }
      
      // std::cout<<"centroidInner:" << centroidInner << " volumeInner:" << volumeInner <<std::endl;
      // std::cout<<"centroidInner:" << centerTranslation[1] << " volumeInner:" << iniVolumes[1] <<std::endl;
      
      Eigen::VectorXd centroidOuter(3);
      double volumeOuter;
      igl::centroid(CV, CF, centroidOuter, volumeOuter);
      // std::cout<<"centroidOuter:" << centroidOuter << " volumeOuter:" << volumeOuter <<std::endl;
      // std::cout<<"centroidOuter:" << centerTranslation[0] << " volumeOuter:" << iniVolumes[0] <<std::endl;



      if (volumeInner >= volumeOuter)
      {
        #ifdef MATRYOSHKA_VERBOSE
        std::cout<<"Break early, include volume too big to ever fit" <<std::endl;
        #endif
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
      #ifdef MATRYOSHKA_DEBUG
        // igl::write_triangle_mesh( "Out.stl",CVprecheck,precheckF);
        // igl::write_triangle_mesh( "Out2.stl",CVcopy,CF);
        // igl::write_triangle_mesh( "Out3.stl",BIV,BIF);
        // igl::write_triangle_mesh( "Out4.stl",BOV,BOF);
      #endif

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
      for (int rotNum = 0; rotNum < 10000; rotNum++) {
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
      #ifdef MATRYOSHKA_DEBUG
        std::cout<<"Inner: " << BBIbest[0] << "," << BBIbest[1] << "," << BBIbest[2] <<std::endl;
        std::cout<<"Outer: " << BBOsize[0] << "," << BBOsize[1] << "," << BBOsize[2] <<std::endl;
      #endif
      
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



      // th = Eigen::RowVector3f(X(k), X(k + 1), X(k + 2));

      // igl::centroid(VA,FA,cenA);
      // VA.rowwise() -= cenA;

      // Model affine matrix generation helper now here also to adjust model[0] // TODO: Or maybe I can just translate it directly
      const auto model_matrix = [](
                                    const Eigen::RowVector3f &cen,
                                    const Eigen::RowVector3f &th) -> Eigen::Affine3f {
        Eigen::Affine3f model = Eigen::Affine3f::Identity();
        model.translate(cen.transpose());
        model.rotate(
            Eigen::AngleAxisf(th(0), Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(th(1), Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(th(2), Eigen::Vector3f::UnitZ()));
        return model;
      };

      #ifdef MATRYOSHKA_VERBOSE
      std::cout<<"    optimizing..."<<std::endl;
      #endif
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
      
      #ifdef MATRYOSHKA_VERBOSE
      std::cout<<i<<": max_s = "<<max_s<<std::endl;
      #endif
      #ifdef MATRYOSHKA_DEBUG
      stop = high_resolution_clock::now(); 
      duration = duration_cast<milliseconds>(stop - start); 
      std::cout << "Time taken by optimization is : " << std::fixed 
      << duration.count();  
      std::cout << " ms. " << std::endl; 
      start = high_resolution_clock::now(); 
      #endif

      // Eigen::RowVector3f previousCenter; // Additionally return center of i=0 to correct model[0] druing maximize_nesting_scale

      // Saving model[0], which is only the translation to center // TODO: Prossibly skip this and rely on using centerTranslation 
      // when translating the high quality models instead
      Eigen::Affine3d doubleModel;
      if (i == 1){
        Eigen::RowVector3f nullRot;
        Eigen::Affine3f floatModel;
        Eigen::RowVector3f floatCenter = -1 * centerTranslation[0].cast<float>();
        nullRot = Eigen::RowVector3f(0, 0, 0);
        // model[0].translate(centerTranslation[0]);
        floatModel = model_matrix(floatCenter, nullRot);
        doubleModel = floatModel.cast<double>();
        model[0] = doubleModel;
      }

      // No packing required if the model did not fit with 100% size
      if (max_s < 0.999) 
      {
        #ifdef MATRYOSHKA_VERBOSE
        std::cout<<"Break early, no fitting alignment found" <<std::endl;
        #endif
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
      
      // Matrix output for communication
      std::cout << model[i].matrix().format(CommaInitFmt) << std::endl;
      // std::cout << model[i].matrix().transpose().format(CommaInitFmt) << std::endl;
      // std::cout << modelMatrixOut << "<!..>" << std::endl;
      // std::cout << model[i].translation().transpose() << std::endl;

      // scales(i) = max_s;
      #ifdef MATRYOSHKA_DEBUG
      std::cout<<"  "<< std::round((scales(i)/scales(i-1))*100) << "%" <<std::endl;
      #endif

      // Matrix outputs
      // std::cout << std::to_string(i) <<std::endl;
      // std::cout << "\n\nmodel0: " <<std::endl;

      // Eigen::IOFormat CommaInitFmt3(Eigen::StreamPrecision, Eigen::DontAlignCols, " ", "", "<..>", "", "<!..>", "<!..>");
      // std::cout << model[0].matrix().format(CommaInitFmt3) << std::endl;
      // std::cout << model[0].translation().transpose() << std::endl;

      // std::cout << "\n\nmodel1: " <<std::endl;
      // std::cout << model[1].matrix().format(CommaInitFmt3) << std::endl;
      // std::cout << model[1].translation().transpose() << std::endl;

      // Working around the i==0 continue: saving 0 as well during 1
      int iStart = i;
      int iEnd = i+1;
      if (i == 0){
        iEnd = 0; // Skip 0 here as well, if it ever comes to that
      }
      else if (i == 1){ // 0 should always get skipped, so we do it at i=1
        iStart = 0;
      }
      if (i >= 0 && mMode == 1) { //Positioning mode: Save all models for safety
        for(iStart;iStart<iEnd;iStart++) {


          // Add model.matrix
          // double matrixJSON[4][4];
          pt::ptree model_matrix_node;
          for (int rowIndex = 0; rowIndex < 4; rowIndex++)
          {
            pt::ptree row;
            for (int cellIndex = 0; cellIndex < 4; cellIndex++)
            {
              // Create an unnamed value
              pt::ptree cell;
              // cell.put_value(matrixJSON[i][j]);
              cell.put_value(model[iStart].matrix()(rowIndex, cellIndex));
              // Add the value to our row
              row.push_back(std::make_pair("", cell));
            }
            // Add the row to our matrix
            model_matrix_node.push_back(std::make_pair("", row));
          }
          // Add the node to the root
          matryoshkaOutRoot.add_child("modelMatrix" + std::to_string(iStart), model_matrix_node);

          // Add model.translation
          // std::vector<double> coordinates; // Actually model.translation

          pt::ptree model_translation_node;
          // for (auto &fruit : coordinates)
          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++)
          {
            // Create an unnamed node containing the value
            pt::ptree coordinate_node;
            coordinate_node.put("", model[iStart].translation().transpose()[coordinateIndex]); // TODO: make to_string?

            // Add this node to the list.
            model_translation_node.push_back(std::make_pair("", coordinate_node));
          }
          matryoshkaOutRoot.add_child("modelTranslation" + std::to_string(iStart), model_translation_node);

          // Add center translation (from centroidHack)
          pt::ptree center_translation_node;
          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++)
          {
            pt::ptree coordinate_node;
            coordinate_node.put("", centerTranslation[iStart][coordinateIndex]);

            center_translation_node.push_back(std::make_pair("", coordinate_node));
          }
          matryoshkaOutRoot.add_child("centerTranslation" + std::to_string(iStart), center_translation_node);

          // Add insertion direction 1
          pt::ptree a1_node;
          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++)
          {
            pt::ptree coordinate_node;
            coordinate_node.put("", a1[iStart][coordinateIndex]); // TODO: make to_string?

            a1_node.push_back(std::make_pair("", coordinate_node));
          }
          matryoshkaOutRoot.add_child("a1" + std::to_string(iStart), a1_node);

          // Add insertion direction 2
          pt::ptree a2_node;
          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++)
          {
            pt::ptree coordinate_node;
            coordinate_node.put("", a2[iStart][coordinateIndex]); // TODO: make to_string?

            a2_node.push_back(std::make_pair("", coordinate_node));
          }
          matryoshkaOutRoot.add_child("a2" + std::to_string(iStart), a2_node);
        }


        if (i == 1) {
          // Add p
          pt::ptree p_node;
          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++)
          {
            pt::ptree coordinate_node;
            coordinate_node.put("", p[coordinateIndex]); // TODO: make to_string?

            p_node.push_back(std::make_pair("", coordinate_node));
          }
          matryoshkaOutRoot.add_child("p", p_node);
        }

        pt::write_json(contFilePath, matryoshkaOutRoot);
      }

      if (i == 1 && mMode == 1) { //Positioning mode: Reposition and save inner mesh
        VT[i] = 
          (V*model[i].matrix().topLeftCorner(3,3).transpose()).rowwise() + 
          model[i].translation().transpose();
        {
          std::string scrapOutFile = prefix + "_" + scrapFile + ".stl"; // For preview
          igl::write_triangle_mesh(scrapOutFile,VT[i],F[i]);        
        }      
      }
    } // End of positioning


    if (mMode != 1) { // Adjusted mesh generation not necessary in position-only mode

      // Working around the i==0 continue: loading 0 as well during 1
      int iStart = i;
      int iEnd = i+1;
      if (i == 0){
        iEnd = 0; // Skip 0 here as well
      }
      else if (i == 1){ // 0 should always get skipped, so we do it at i=1
        iStart = 0;
      }

      if (i >= 0 && mMode == 2) { //Mesh generation mode: Load models for all levels for safety
        // Create a root
        pt::ptree matryoshkaInRoot;

        // Load the json file in this ptree
        pt::read_json(contFilePath, matryoshkaInRoot);
        
        for(iStart;iStart<iEnd;iStart++) {
          int x = 0;
          for (pt::ptree::value_type &row : matryoshkaInRoot.get_child("modelMatrix" + std::to_string(iStart)))
          {
              int y = 0;
              for (pt::ptree::value_type &cell : row.second)
              {
                  model[iStart].matrix()(x, y) = cell.second.get_value<double>();
                  y++;
              }
              x++;
          }

          // std::cout << "Matrix :" << std::endl;
          // for (int xIndex = 0; xIndex < 4; xIndex++)
          // {
          //     for (int yIndex = 0; yIndex < 4; yIndex++)
          //         std::cout << " " << model[iStart].matrix()(xIndex, yIndex);
          //     std::cout << std::endl;
          // }

          std::vector<double> coordinates;
          for (pt::ptree::value_type &coordinate : matryoshkaInRoot.get_child("modelTranslation" + std::to_string(iStart)))
          {
              // coordinate.first contain the string ""
              coordinates.push_back(boost::lexical_cast<double>(coordinate.second.data()));
          }

          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++){
            model[iStart].translation().transpose()[coordinateIndex] = coordinates[coordinateIndex]; // TODO: Figure out how to do this while reading the JSON
          }

          // Read center translations (from centroidHack)
          std::vector<double> coordinates_centerTranslation;
          for (pt::ptree::value_type &coordinate : matryoshkaInRoot.get_child("centerTranslation" + std::to_string(iStart)))
          {
              // coordinate.first contain the string ""
              coordinates_centerTranslation.push_back(boost::lexical_cast<double>(coordinate.second.data()));
          }

          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++){
            // std::cout << "centerTranslation: " << coordinates_centerTranslation[coordinateIndex] << "\n" << std::endl;
            centerTranslation[iStart][coordinateIndex] = coordinates_centerTranslation[coordinateIndex];
          }

          std::vector<double> coordinates_a1;
          for (pt::ptree::value_type &coordinate : matryoshkaInRoot.get_child("a1" + std::to_string(iStart)))
          {
              // coordinate.first contain the string ""
              coordinates_a1.push_back(boost::lexical_cast<double>(coordinate.second.data()));
          }

          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++){
            #ifdef MATRYOSHKA_DEBUG
            std::cout << "coordinates_a1: " << coordinates_a1[coordinateIndex] << "\n" << std::endl;
            #endif
            a1[iStart][coordinateIndex] = coordinates_a1[coordinateIndex]; // TODO: Figure out how to do this while reading the JSON
          }

          std::vector<double> coordinates_a2;
          for (pt::ptree::value_type &coordinate : matryoshkaInRoot.get_child("a2" + std::to_string(iStart)))
          {
              // coordinate.first contain the string ""
              coordinates_a2.push_back(boost::lexical_cast<double>(coordinate.second.data()));
          }

          for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++){
            #ifdef MATRYOSHKA_DEBUG
            std::cout << "coordinates_a2: " << coordinates_a2[coordinateIndex] << "\n" << std::endl;
            #endif
            a2[iStart][coordinateIndex] = coordinates_a2[coordinateIndex]; // TODO: Figure out how to do this while reading the JSON
          }
        }

        std::vector<double> coordinates_p;
        for (pt::ptree::value_type &coordinate : matryoshkaInRoot.get_child("p"))
        {
            coordinates_p.push_back(boost::lexical_cast<double>(coordinate.second.data()));
        }

        for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++){
          p[coordinateIndex] = coordinates_p[coordinateIndex]; // TODO: Figure out how to do this while reading the JSON
        }
        
      }

      
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
        #ifdef MATRYOSHKA_VERBOSE
        std::cout<<"    offsetting..."<<std::endl;
        #endif
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
      #ifdef MATRYOSHKA_VERBOSE
        std::cout<<"just using transformed input..."<<std::endl;
      #endif
        OV = VT[i];
        OF = F[i];
      }
      #ifdef MATRYOSHKA_DEBUG
      igl::write_triangle_mesh(STR(prefix<<"-"<<std::setfill('0')<<std::setw(2)<<i<<"-O."<<ext),OV,OF);
      #endif

      // Decimate offset mesh first

      // igl::write_triangle_mesh(STR(prefix<<"-"<<std::setfill('0')<<std::setw(2)<<i<<"-Offset."<<ext),OV,OF);

      // MatrixdX3R DV;
      // MatrixiX3R DF;
      // const MatrixdX3R OinV = OV;
      // const MatrixiX3R OinF = OF;
      // Eigen::VectorXi vectorJ;
      // Eigen::VectorXi vectorI;
      // const size_t decimateTargetNumber = 3000;

      Eigen::MatrixXd VA,VB;
      Eigen::MatrixXi FA,FB;
      Eigen::VectorXi vectorJ;
      Eigen::VectorXi vectorI;
      const size_t decimateTargetNumber = 2000;
      igl::qslim(OV, OF, decimateTargetNumber, VB, FB, vectorJ, vectorI);

      MatrixdX3R DV = VB;
      MatrixiX3R DF = FB;
  
      // OV = VB;
      // OF = FB;
      
      // if(epsilon > 0 || sigma > 0)
      // {
      //   igl::decimate(OinV, OinF, decimateTargetNumber, DV, DF, vectorJ);
      // }
      // igl::qslim(OV, OF, decimateTargetNumber, OV, OF, vectorJ, vectorI);

      // igl::write_triangle_mesh(STR(prefix<<"-"<<std::setfill('0')<<std::setw(2)<<i<<"-Decimated."<<ext),OV,OF);

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
        VT[i-1],F[i-1],DV,DF,p,np,a1[i-1],a2[i-1], // was OV, OF
        VL[i-1],FL[i-1],JL,VR[i-1],FR[i-1],JR);
      #ifdef MATRYOSHKA_DEBUG
      igl::write_triangle_mesh(outname(prefix,i-1,0,ext), VL[i-1],FL[i-1]);
      igl::write_triangle_mesh(outname(prefix,i-1,1,ext), VR[i-1],FR[i-1]);
      #endif

      



      // Gratuitous copying of left and right side
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

      #ifdef MATRYOSHKA_DEBUG
      std::string modelOutFile = prefix + "_" + scrapFile + "_EmptyModel.stl";
      std::cout << "Writing file: " << modelOutFile <<std::endl;
      igl::write_triangle_mesh(modelOutFile,VLR,FLR);  
      #endif
      // igl::write_triangle_mesh(outname(prefix,i-1,99,ext), VLR, FLR);
      
      // typedef typename DerivedVL::Scalar Scalar;
      // typedef CGAL::Epeck Kernel;
      // typedef Kernel::FT ExactScalar;
      // typedef Eigen::Matrix<Scalar,Eigen::Dynamic,3> MatrixX3S;
      // typedef Eigen::Matrix<typename DerivedJL::Scalar,Eigen::Dynamic,1> VectorXJ;
      // typedef Eigen::Matrix<
      //   ExactScalar,
      //   Eigen::Dynamic,
      //   Eigen::Dynamic,
      //   DerivedVL::IsRowMajor> MatrixXES;

      // MatrixXES VAL,VAR;
      MatrixdX3R VAL,VAR;
      Eigen::MatrixXi FAL,FAR;
      Eigen::VectorXi JAL,JAR;

      igl::copyleft::cgal::intersect_with_half_space(VT[i-1],F[i-1],p, np,VAL,FAL,JAL);

      igl::copyleft::cgal::intersect_with_half_space(VT[i-1],F[i-1],p,-np,VAR,FAR,JAR);

      // Gratuitous copying of left and right side
      std::vector<MatrixdX3R> VVLandRFULL(2);
      std::vector<MatrixiX3R> FFLandRFULL(2);
      // Combined left+right original (not emptied) mesh
      MatrixdX3R VLRFULL;
      MatrixiX3R FLRFULL;

      VVLandRFULL[0] = VAL;
      VVLandRFULL[1] = VAR;
      FFLandRFULL[0] = FAL;
      FFLandRFULL[1] = FAR;

      igl::combine(VVLandRFULL, FFLandRFULL, VLRFULL, FLRFULL);

      #ifdef MATRYOSHKA_DEBUG
      std::string cutOutFile = prefix + "_" + scrapFile + "_CutModel.stl";
      std::cout << "Writing file: " << cutOutFile <<std::endl;
      igl::write_triangle_mesh(cutOutFile,VLRFULL,FLRFULL);
      #endif

      MatrixdX3R VINNER;
      MatrixiX3R FINNER;
      Eigen::VectorXi JINNER;
      const bool ret = igl::copyleft::cgal::mesh_boolean(VLRFULL,FLRFULL,VLR,FLR,igl::MESH_BOOLEAN_TYPE_MINUS,VINNER,FINNER,JINNER);
      std::string scrappedOutFile = prefix + "_" + scrapFile + "_ScrappedVolume.stl";
      igl::write_triangle_mesh(scrappedOutFile,VINNER,FINNER);




      if(i == num_levels-1)
      {
        // last item
        //VL[i] = VT[i];
        //FL[i] = F[i];
        //JL[i] = igl::LinSpaced(F[i].size(),0,F[i].size()-1);
        
        #ifdef MATRYOSHKA_DEBUG
        std::cout << "Writing file: " << outname(prefix,i,0,ext) <<std::endl;
        igl::write_triangle_mesh( outname(prefix,i,0,ext),VT[i],F[i],false);
        #endif
        
        MatrixdX3R hqmodelV, hqscrapV, hqmodelOutV, hqscrapOutV;
        MatrixiX3R hqmodelF, hqscrapF;

        if(hqmodelPath != "") {
          igl::read_triangle_mesh(
            hqmodelPath,hqmodelV,hqmodelF);

          hqmodelOutV = 
            (hqmodelV*model[0].matrix().topLeftCorner(3,3).transpose()).rowwise() + 
            model[0].translation().transpose();

          std::string hqmodelOut = prefix + "_" + scrapFile + "_hqModel.stl";
          igl::write_triangle_mesh(hqmodelOut,hqmodelOutV,hqmodelF);
        }

        // STL and model outputs
        // Eigen::IOFormat CleanFmt2(4, 0, ", ", "\n", "[", "]");
        // std::cout << "Original vertices: " <<std::endl;
        // std::cout << hqmodelV.format(CleanFmt2) <<std::endl;
        // std::cout << "\n\nModel transformed vertices: " <<std::endl;
        // std::cout << hqmodelOutV.format(CleanFmt2) <<std::endl;
        // std::cout << "\n\nmodel0: " <<std::endl;

        // Eigen::IOFormat CommaInitFmt2(Eigen::StreamPrecision, Eigen::DontAlignCols, " ", "", "<..>", "", "<!..>", "<!..>");
        // std::cout << model[0].matrix().format(CommaInitFmt2) << std::endl;
        // std::cout << model[0].translation().transpose() << std::endl;

        // std::cout << "\n\nmodel1: " <<std::endl;
        // std::cout << model[1].matrix().format(CommaInitFmt2) << std::endl;
        // std::cout << model[1].translation().transpose() << std::endl;

        if(hqscrapPath != "") {
          igl::read_triangle_mesh(
            hqscrapPath,hqscrapV,hqscrapF);

          hqscrapV.rowwise() -= centerTranslation[1];

          hqscrapOutV = 
            (hqscrapV*model[1].matrix().topLeftCorner(3,3).transpose()).rowwise() + 
            model[1].translation().transpose();

          std::string hqscrapOut = prefix + "_" + scrapFile + "_hqScrap.stl";
          igl::write_triangle_mesh(hqscrapOut,hqscrapOutV,hqscrapF);
        }
        


      }
    } // End of adjusted mesh generation


  } // End of mega for-loop


  if (mMode == 0) { // Full mode: Packing only necessary in original mode

    #ifdef MATRYOSHKA_VERBOSE
    std::cout<<"    packing..."<<std::endl;
    #endif

    #ifdef MATRYOSHKA_DEBUG
    stop = high_resolution_clock::now(); 
    duration = duration_cast<milliseconds>(stop - start); 
    std::cout << "Time taken by Minkowski is : " << std::fixed 
    << duration.count();  
    std::cout << " ms. " << std::endl; 
    #endif

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
  }

  // Breakdown background window
  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
