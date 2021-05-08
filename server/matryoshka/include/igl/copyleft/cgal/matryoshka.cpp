#include "matryoshka.h"
#include <igl/copyleft/cgal/intersect_with_half_space.h>
#include <igl/copyleft/cgal/minkowski_sum.h>
#include <igl/copyleft/cgal/minkowski_sum_point.h>
#include <igl/copyleft/cgal/assign.h>
#include <igl/copyleft/cgal/mesh_boolean.h>
//LWW
#include <igl/writeOBJ.h>
#include <chrono> 
#include <ctime>

#define IGL_COPYLEFT_CGAL_MATRYOSHKA_DEBUG

template <
  typename DerivedVA,
  typename DerivedFA,
  typename DerivedVB,
  typename DerivedFB,
  typename Derivedp,
  typename Derivednp,
  typename Deriveda1,
  typename Deriveda2,
  typename DerivedVL,
  typename DerivedFL,
  typename DerivedJL,
  typename DerivedVR,
  typename DerivedFR,
  typename DerivedJR
  >
IGL_INLINE void igl::copyleft::cgal::matryoshka(
  const Eigen::MatrixBase<DerivedVA> & VA,
  const Eigen::MatrixBase<DerivedFA> & FA,
  const Eigen::MatrixBase<DerivedVB> & VB,
  const Eigen::MatrixBase<DerivedFB> & FB,
  const Eigen::MatrixBase<Derivedp> & p,
  const Eigen::MatrixBase<Derivednp> & np,
  const Eigen::MatrixBase<Deriveda1> & a1,
  const Eigen::MatrixBase<Deriveda2> & a2,
  Eigen::PlainObjectBase<DerivedVL> & VL,
  Eigen::PlainObjectBase<DerivedFL> & FL,
  Eigen::PlainObjectBase<DerivedJL> & JL,
  Eigen::PlainObjectBase<DerivedVR> & VR,
  Eigen::PlainObjectBase<DerivedFR> & FR,
  Eigen::PlainObjectBase<DerivedJR> & JR
  )
{
  // WOW! what a hack!
  // #warning "writeobj hack"
  // #ifndef WRITEOBJ
  // #define WRITEOBJ(name,V,F) {Eigen::MatrixXd Vd;igl::copyleft::cgal::assign(V,Vd);igl::writeOBJ(name,Vd,F);};
  // #endif

  //WRITEOBJ("A.obj",VA,FA);
  //WRITEOBJ("B.obj",VB,FB);
  typedef typename DerivedVL::Scalar Scalar;
  typedef CGAL::Epeck Kernel;
  typedef Kernel::FT ExactScalar;
  typedef Eigen::Matrix<Scalar,Eigen::Dynamic,3> MatrixX3S;
  typedef Eigen::Matrix<typename DerivedJL::Scalar,Eigen::Dynamic,1> VectorXJ;
  typedef Eigen::Matrix<
    ExactScalar,
    Eigen::Dynamic,
    Eigen::Dynamic,
    DerivedVL::IsRowMajor> MatrixXES;

  // Intersect with half spaces above and below plane
  MatrixXES VAL,VAR;
  Eigen::MatrixXi FAL,FAR;
  Eigen::VectorXi JAL,JAR;
#ifdef IGL_COPYLEFT_CGAL_MATRYOSHKA_DEBUG
  auto LWWTime = std::chrono::system_clock::now();
      
  std::time_t LWWTimeX = std::chrono::system_clock::to_time_t(LWWTime);
  std::cout << "Time: " << std::ctime(&LWWTimeX);
  std::cout << " " << std::endl;
  std::cout<<"    intersect_with_half_space..."<<std::endl;
#endif
  intersect_with_half_space(VA,FA,p, np,VAL,FAL,JAL);
  //WRITEOBJ("AL.obj",VAL,FAL);
#ifdef IGL_COPYLEFT_CGAL_MATRYOSHKA_DEBUG
  auto LWWTime2 = std::chrono::system_clock::now();
  
  std::time_t LWWTimeX2 = std::chrono::system_clock::to_time_t(LWWTime2);
  std::cout << "Time: " << std::ctime(&LWWTimeX2);
  std::cout << " " << std::endl;
  std::cout<<"    intersect_with_half_space..."<<std::endl;
#endif
  intersect_with_half_space(VA,FA,p,-np,VAR,FAR,JAR);
  //WRITEOBJ("AR.obj",VAR,FAR);

  // Q: Why not use the implicit based swept volume subtraction like in
  // "Computational Design of Reconfigurables" [Garg et al. 2016]?
  // 

  // Linear sweep of B along np
  assert( ( (a1.array()==0).all() || a1.isApprox(np))
      && "Not supported yet.");
  assert( ( (a2.array()==0).all() || a2.isApprox(-np))
      && "Not supported yet.");
  MatrixXES VSL,VSR;
  Eigen::MatrixXi FSL,FSR;
  {
    const double scale = 
      2.0*(VA.colwise().maxCoeff()-VA.colwise().minCoeff()).norm();
    Eigen::Matrix<CGAL::Epeck::FT,1,3> zero(0,0,0);
    Eigen::Matrix<CGAL::Epeck::FT,1,3> snp(np(0),np(1),np(2));

    Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]"); // LWW
    std::cout << "[" << np(0) << "," << np(1) << "," << np(2) << "]" <<std::endl;

    MatrixXES VBE;
    assign(VB,VBE);
    //MatrixXES VAE;
    //assign(VA,VAE);

    snp *= scale; 
    std::cout<<"    (Minkowski-)scale: "<<scale<<std::endl;
    Eigen::VectorXi _;
#ifdef IGL_COPYLEFT_CGAL_MATRYOSHKA_DEBUG
    auto LWWTime3 = std::chrono::system_clock::now();
    
    std::time_t LWWTimeX3 = std::chrono::system_clock::to_time_t(LWWTime3);
    std::cout << "Time: " << std::ctime(&LWWTimeX3);
    std::cout << " " << std::endl;
    std::cout<<"    minkowski_sum_point..."<<std::endl;
#endif
    minkowski_sum_point(VBE,FB,zero,( snp).eval(),true,VSL,FSL,_);
    //minkowski_sum_point(VBE,FB,VA,FA,true,VSL,FSL,_); LWW try (and fail) to call minkowsk with two meshes 
    //WRITEOBJ("SL.obj",VSL,FSL);
#ifdef IGL_COPYLEFT_CGAL_MATRYOSHKA_DEBUG
    auto LWWTime4 = std::chrono::system_clock::now();
    
    std::time_t LWWTimeX4 = std::chrono::system_clock::to_time_t(LWWTime4);
    std::cout << "Time: " << std::ctime(&LWWTimeX4);
    std::cout << " " << std::endl;
    std::cout<<"    minkowski_sum_point..."<<std::endl;
#endif
    minkowski_sum_point(VBE,FB,zero,(-snp).eval(),true,VSR,FSR,_);
    //WRITEOBJ("SR.obj",VSR,FSR);
  }

  // Subtract sweeps from top and bottom
  const auto subtract_sweep = [&FA](
    const MatrixXES & VAL,
    const Eigen::MatrixXi  & FAL,
    const Eigen::VectorXi & JAL,
    const MatrixXES & VSL,
    const Eigen::MatrixXi  & FSL,
    Eigen::PlainObjectBase<DerivedVL> & VL,
    Eigen::PlainObjectBase<DerivedFL> & FL,
    Eigen::PlainObjectBase<DerivedJL> & JL)
  {
    Eigen::VectorXi JL1;
#ifdef IGL_COPYLEFT_CGAL_MATRYOSHKA_DEBUG
    auto LWWTime5 = std::chrono::system_clock::now();
    
    std::time_t LWWTimeX5 = std::chrono::system_clock::to_time_t(LWWTime5);
    std::cout << "Time: " << std::ctime(&LWWTimeX5);
    std::cout << " " << std::endl;
    std::cout<<"    mesh_boolean..."<<std::endl;
#endif
    mesh_boolean(VAL,FAL,VSL,FSL,MESH_BOOLEAN_TYPE_MINUS,VL,FL,JL1);
    JL.resizeLike(JL1);
    for(int i = 0;i<JL1.size();i++)
    {
      if(JL1(i)<JAL.size())
      {
        JL(i) = JAL(JL1(i));
      }else
      {
        JL(i) = (JL1(i)-JAL.size())+FA.size();
      }
    }
  };

  subtract_sweep(VAL,FAL,JAL,VSL,FSL,VL,FL,JL);
  //WRITEOBJ("L.obj",VL,FL);
  subtract_sweep(VAR,FAR,JAR,VSR,FSR,VR,FR,JR);
  //WRITEOBJ("R.obj",VR,FR);
}

#ifdef IGL_STATIC_LIBRARY
// Explicit template instantiation
template void igl::copyleft::cgal::matryoshka<Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, 1, 0, -1, 1>, Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, 1, 0, -1, 1> >(Eigen::MatrixBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::PlainObjectBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, -1, 0, -1, -1> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&, Eigen::PlainObjectBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, -1, 0, -1, -1> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&);
template void igl::copyleft::cgal::matryoshka<Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 1, 0, -1, 1>, Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 1, 0, -1, 1> >(Eigen::MatrixBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::PlainObjectBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&, Eigen::PlainObjectBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&);
template void igl::copyleft::cgal::matryoshka<Eigen::Matrix<double, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<double, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 1, 0, -1, 1>, Eigen::Matrix<double, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 1, 0, -1, 1> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&);
//template void igl::copyleft::cgal::matryoshka<Eigen::Matrix<double, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<double, -1, -1, 0, -1, -1>, Eigen::Matrix<int, -1, -1, 0, -1, -1>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, 1, 3, 1, 1, 3>, Eigen::Matrix<double, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 1, 0, -1, 1>, Eigen::Matrix<double, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 1, 0, -1, 1> >(Eigen::MatrixBase<Eigen::Matrix<double, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, -1, 0, -1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<double, 1, 3, 1, 1, 3> > const&, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&, Eigen::PlainObjectBase<Eigen::Matrix<double, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> >&, Eigen::PlainObjectBase<Eigen::Matrix<int, -1, 1, 0, -1, 1> >&);
#endif
