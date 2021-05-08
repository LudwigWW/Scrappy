#ifndef IGL_COPYLEFT_CGAL_IS_VALID_NESTING_H
#define IGL_COPYLEFT_CGAL_IS_VALID_NESTING_H
#include <igl/igl_inline.h>
#include <Eigen/Core>

namespace igl
{
  namespace copyleft
  {
    namespace cgal
    {
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
      IGL_INLINE bool is_valid_nesting(
        const Eigen::MatrixBase<DerivedVA> & VA,
        const Eigen::MatrixBase<DerivedFA> & FA,
        const Eigen::MatrixBase<DerivedVB> & VB,
        const Eigen::MatrixBase<DerivedFB> & FB,
        const Eigen::MatrixBase<Derivedp> & p,
        const Eigen::MatrixBase<Derivednp> & np,
        const Eigen::MatrixBase<Deriveda1> & a1,
        const Eigen::MatrixBase<Deriveda2> & a2);
    }
  }
}

#ifndef IGL_STATIC_LIBRARY
#  include "is_valid_nesting.cpp"
#endif

#endif



