#include "is_valid_nesting.h"
#include <igl/slice_mask.h>
#include <igl/copyleft/cgal/intersect_with_half_space.h>
#include <igl/copyleft/cgal/intersect_other.h>
#include <igl/copyleft/cgal/minkowski_sum.h>
#include <igl/copyleft/cgal/mesh_boolean.h>

template <
  typename DerivedVA,
  typename DerivedFA,
  typename DerivedVB,
  typename DerivedFB,
  typename Derivedp,
  typename Derivednp,
  typename Deriveda1,
  typename Deriveda2
  >
IGL_INLINE bool igl::copyleft::cgal::is_valid_nesting(
  const Eigen::MatrixBase<DerivedVA> & VA,
  const Eigen::MatrixBase<DerivedFA> & FA,
  const Eigen::MatrixBase<DerivedVB> & VB,
  const Eigen::MatrixBase<DerivedFB> & FB,
  const Eigen::MatrixBase<Derivedp> & p,
  const Eigen::MatrixBase<Derivednp> & np,
  const Eigen::MatrixBase<Deriveda1> & a1,
  const Eigen::MatrixBase<Deriveda2> & a2)
{
  typedef CGAL::Epeck Kernel;
  typedef Kernel::FT ExactScalar;
  typedef Eigen::Matrix<
    ExactScalar,
    Eigen::Dynamic,
    Eigen::Dynamic,
    DerivedVA::IsRowMajor> MatrixXES;

  // Intersect with half spaces above and below plane
  MatrixXES VAL,VAR;
  Eigen::MatrixXi FAL,FAR;
  Eigen::VectorXi JAL,JAR;
  intersect_with_half_space(VA,FA,p, np,VAL,FAL,JAL);
  intersect_with_half_space(VA,FA,p,-np,VAR,FAR,JAR);
  {
    // Only keep parts above (below) plane
    igl::slice_mask(Eigen::MatrixXi(FAL),(JAL.array()<FA.rows()),1,FAL);
    igl::slice_mask(Eigen::MatrixXi(FAR),(JAR.array()<FA.rows()),1,FAR);
  }

  // Linear sweep of B along np
  assert( (a1.array()==0).all() && "Not supported yet.");
  assert( (a2.array()==0).all() && "Not supported yet.");
  MatrixXES VSL,VSR;
  Eigen::MatrixXi FSL,FSR;
  {
    const double scale = 
      2.0*(VA.colwise().maxCoeff()-VA.colwise().minCoeff()).norm();
    Eigen::VectorXi _;
    minkowski_sum(VB,FB,Eigen::RowVector3d(0,0,0),( scale*np).eval(),true,VSL,FSL,_);
    minkowski_sum(VB,FB,Eigen::RowVector3d(0,0,0),(-scale*np).eval(),true,VSR,FSR,_);
  }

  Eigen::MatrixXi IF;
  MatrixXES VV;
  Eigen::MatrixXi FF;
  Eigen::VectorXi J,IM;
  return
    !intersect_other(VAL,FAL,VSL,FSL,{true,true},IF,VV,FF,J,IM) &&
    !intersect_other(VAR,FAR,VSR,FSR,{true,true},IF,VV,FF,J,IM);
}

#ifdef IGL_STATIC_LIBRARY
// Explicit template instantiation
template bool igl::copyleft::cgal::is_valid_nesting<Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<float, -1, 3, 1, -1, 3>, Eigen::Matrix<int, -1, 3, 1, -1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3>, Eigen::Matrix<float, 1, 3, 1, 1, 3> >(Eigen::MatrixBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<int, -1, 3, 1, -1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, 3, 1, 1, 3> > const&);
#endif
