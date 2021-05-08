#ifndef IGL_COPYLEFT_CGAL_MATRYOSHKA_H
#define IGL_COPYLEFT_CGAL_MATRYOSHKA_H
#include <igl/igl_inline.h>
#include <Eigen/Core>

namespace igl
{
  namespace copyleft
  {
    namespace cgal
    {
      // Cut an outer shape (VA,FA) in half according to a cut-plane (p,np) and
      // subtract the removal of an outer shape (VB,FB) along translational
      // removal directions (a1,a2). "Generalized Matryoshka: Computational
      // Design of Nesting Objects" [Jacobson 2017].
      //
      // Inputs:
      //  VA  #VA by 3 list of vertex positions of "outer" shape 
      //  FA  #FA by 3 list of triangle indices into VA
      //  VB  #VB by 3 list of vertex positions of "inner" shape 
      //  FB  #FB by 3 list of triangle indices into VB
      //  p  3d point on cut plane
      //  np  3d normal vector of cut plane
      //  a1  left removal direction
      //  a2  right removal direction
      // Outputs:
      //   VL  #VL by 3 list of vertex positions of "left" portion of "outer"
      //     shape with inner shape "removed"
      //   FL  #FL by 3 list of triangle indices into VL
      //   IL  #FL list of triangle indices into [FA;FB]
      //   VR  #VR by 3 list of vertex positions of "right" portion of "outer"
      //     shape with inner shape "removed"
      //   FR  #FR by 3 list of triangle indices into VR
      //   IR  #FR list of triangle indices into [FA;FB]
      //   
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
        typename DerivedIL,
        typename DerivedVR,
        typename DerivedFR,
        typename DerivedIR
        >
      IGL_INLINE void matryoshka(
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
        Eigen::PlainObjectBase<DerivedIL> & IL,
        Eigen::PlainObjectBase<DerivedVR> & VR,
        Eigen::PlainObjectBase<DerivedFR> & FR,
        Eigen::PlainObjectBase<DerivedIR> & IR);
    }
  }
}

#ifndef IGL_STATIC_LIBRARY
#  include "matryoshka.cpp"
#endif

#endif
