
#include "igl/copyleft/cgal/matryoshka.h"
#include <igl/matlab/parse_rhs.h>
#include <igl/matlab/prepare_lhs.h>
#include <igl/matlab/validate_arg.h>
#include <igl/matlab/MexStream.h>
#include <igl/matlab/mexErrMsgTxt.h>

#include <mex.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  using namespace std;
  using namespace igl;
  using namespace igl::matlab;
  using namespace Eigen;
  igl::matlab::MexStream mout;        
  std::streambuf *outbuf = cout.rdbuf(&mout);

  Eigen::MatrixXd VA,VB,VL,VR;
  Eigen::MatrixXi FA,FB,FL,FR;
  Eigen::VectorXi JL,JR;
  Eigen::RowVector3d p,np;

  mexErrMsgTxt(nrhs >= 6, "The number of input arguments must be >=6.");
  parse_rhs_double(prhs+0,VA);
  parse_rhs_index( prhs+1,FA);
  parse_rhs_double(prhs+2,VB);
  parse_rhs_index( prhs+3,FB);
  parse_rhs_double(prhs+4,p);
  parse_rhs_double(prhs+5,np);

  Eigen::RowVector3d a1(0,0,0),a2(0,0,0);
  igl::copyleft::cgal::matryoshka(VA,FA,VB,FB,p,np,a1,a2,VL,FL,JL,VR,FR,JR);

  switch(nlhs)
  {
    default:
    {
      mexErrMsgTxt(false,"Too many output parameters.");
    }
    case 6:
    {
      prepare_lhs_index(JR,plhs+5);
      // Fall through
    }
    case 5:
    {
      prepare_lhs_index(FR,plhs+4);
      // Fall through
    }
    case 4:
    {
      prepare_lhs_double(VR,plhs+3);
      // Fall through
    }
    case 3:
    {
      prepare_lhs_index(JL,plhs+2);
      // Fall through
    }
    case 2:
    {
      prepare_lhs_index(FL,plhs+1);
      // Fall through
    }
    case 1:
    {
      prepare_lhs_double(VL,plhs+0);
      // Fall through
    }

    case 0: break;
  }

  // Restore the std stream buffer Important!
  std::cout.rdbuf(outbuf);
}
