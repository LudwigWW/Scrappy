#define PSO_EARLY_EXIT_VERBOSE

#include "pso_earlyExit.h"
#include <cassert>
#include <Eigen/StdVector>
#include <vector>
#include <iostream>

template <
  typename Scalar, 
  typename DerivedX,
  typename DerivedLB, 
  typename DerivedUB>
IGL_INLINE Scalar igl::pso_earlyExit( //TODO: LWW: This should have a parameter at least for when the early exist should be
  const std::function< Scalar (DerivedX &) > f, // instead of just hard coded -1.0...
  const Eigen::MatrixBase<DerivedLB> & LB,
  const Eigen::MatrixBase<DerivedUB> & UB,
  const int max_iters,
  const int population,
  DerivedX & X)
{
  const Eigen::Array<bool,Eigen::Dynamic,1> P =
    Eigen::Array<bool,Eigen::Dynamic,1>::Zero(LB.size(),1);
  return igl::pso_earlyExit(f,LB,UB,P,max_iters,population,X); 
  
}

template <
  typename Scalar, 
  typename DerivedX,
  typename DerivedLB, 
  typename DerivedUB,
  typename DerivedP>
IGL_INLINE Scalar igl::pso_earlyExit(
  const std::function< Scalar (DerivedX &) > f,
  const Eigen::MatrixBase<DerivedLB> & LB,
  const Eigen::MatrixBase<DerivedUB> & UB,
  const Eigen::DenseBase<DerivedP> & P,
  const int max_iters,
  const int population,
  DerivedX & X)
{
  const float minimalAngle = 0.0; // 0.02
  const float cardinalRots[] = {0 + minimalAngle, M_PI - minimalAngle, -M_PI + minimalAngle, M_PI_2 - minimalAngle, -M_PI_2 + minimalAngle};
  const int cardinalPops = (population / 5);
  std::cout << "cardinalPops: " << cardinalPops << std::endl; // LWW
  const int dim = LB.size();
  assert(UB.size() == dim && "UB should match LB size");
  assert(P.size() == dim && "P should match LB size");
  typedef std::vector<DerivedX,Eigen::aligned_allocator<DerivedX> > VectorList;
  VectorList position(population);
  VectorList best_position(population);
  VectorList velocity(population);
  Eigen::Matrix<Scalar,Eigen::Dynamic,1> best_f(population);
  // https://en.wikipedia.org/wiki/Particle_swarm_optimization#Algorithm
  //
  // g → X
  // p_i → best[i]
  // v_i → velocity[i]
  // x_i → position[i]
  Scalar min_f = std::numeric_limits<Scalar>::max();
  for(int p=0;p<population;p++)
  {
    {
      const DerivedX R = DerivedX::Random(dim).array()*0.5+0.5;
      position[p] = LB.array() + R.array()*(UB-LB).array();
    }
    best_f[p] = f(position[p]);
    best_position[p] = position[p];
    if(best_f[p] < min_f)
    {
      min_f = best_f[p];
      X = best_position[p];
    }
    {
      const DerivedX R = DerivedX::Random(dim);
      velocity[p] = (UB-LB).array() * R.array();
    }
    //if (min_f <= -0.9985)
    if (false)
    {
      #ifdef PSO_EARLY_EXIT_VERBOSE
      std::cout << "PSO_population_iteration_min_f: " << min_f << std::endl; // LWW
      #endif
      return min_f;
    }
  }
  #ifdef PSO_EARLY_EXIT_VERBOSE
  std::cout << "Once_________________ " << std::endl; // LWW
  #endif
  int iter = 0;
  Scalar omega = 0.98;
  Scalar phi_p = 0.01;
  Scalar phi_g = 0.01;
  while(true)
  {
    //if(iter % 10 == 0)
    //{
    //  std::cout<<iter<<":"<<std::endl;
    //  for(int p=0;p<population;p++)
    //  {
    //    std::cout<<"  "<<best_f[p]<<", "<<best_position[p]<<std::endl;
    //  }
    //  std::cout<<std::endl;
    //}

    // LWW: Gratuitious code duplication to avoid two IF's in this huge loop
    for(int p=0;p<cardinalPops;p++)
    {
      const DerivedX R_p = DerivedX::Random(dim).array()*0.5+0.5;
      const DerivedX R_g = DerivedX::Random(dim).array()*0.5+0.5;
      velocity[p] = 
        omega * velocity[p].array() +
        phi_p * R_p.array() *(best_position[p] - position[p]).array() + 
        phi_g * R_g.array() *(               X - position[p]).array();
      position[p] += velocity[p];
      // Clamp to bounds
      for(int d = 0;d<3;d++)
      {
//#define IGL_PSO_earlyExit_REFLECTION
#ifdef IGL_PSO_earlyExit_REFLECTION
        assert(!P(d));
        // Reflect velocities if exceeding bounds
        if(position[p](d) < LB(d))
        {
          position[p](d) = LB(d);
          if(velocity[p](d) < 0.0) velocity[p](d) *= -1.0;
        }
        if(position[p](d) > UB(d))
        {
          position[p](d) = UB(d);
          if(velocity[p](d) > 0.0) velocity[p](d) *= -1.0;
        }
#else
        // 50% chance to act normally, otherwise choose randomly from the 4 cardinal directions (5 since flipping is included by rotating both left and right)
        position[p](d) = ((rand() % 2) == 0) ? (std::max(LB(d),std::min(UB(d),position[p](d)))) : (cardinalRots[rand() % 5]);
#endif
      }
      for(int d = 3;d<dim;d++)
      {
//#define IGL_PSO_earlyExit_REFLECTION
#ifdef IGL_PSO_earlyExit_REFLECTION
        assert(!P(d));
        // Reflect velocities if exceeding bounds
        if(position[p](d) < LB(d))
        {
          position[p](d) = LB(d);
          if(velocity[p](d) < 0.0) velocity[p](d) *= -1.0;
        }
        if(position[p](d) > UB(d))
        {
          position[p](d) = UB(d);
          if(velocity[p](d) > 0.0) velocity[p](d) *= -1.0;
        }
#else
        position[p](d) = std::max(LB(d),std::min(UB(d),position[p](d)));
#endif
      }
      //std::cout << "Calling F " << std::endl; // LWW
      const Scalar fp = f(position[p]);
      if(fp<best_f[p])
      {
        best_f[p] = fp;
        best_position[p] = position[p];
        if(best_f[p] < min_f)
        {
          min_f = best_f[p];
          X = best_position[p];
        }
      }
    }


    for(int p=cardinalPops;p<population;p++)
    {
      const DerivedX R_p = DerivedX::Random(dim).array()*0.5+0.5;
      const DerivedX R_g = DerivedX::Random(dim).array()*0.5+0.5;
      velocity[p] = 
        omega * velocity[p].array() +
        phi_p * R_p.array() *(best_position[p] - position[p]).array() + 
        phi_g * R_g.array() *(               X - position[p]).array();
      position[p] += velocity[p];
      // Clamp to bounds

      for(int d = 0;d<dim;d++)
      {
//#define IGL_PSO_earlyExit_REFLECTION
#ifdef IGL_PSO_earlyExit_REFLECTION
        assert(!P(d));
        // Reflect velocities if exceeding bounds
        if(position[p](d) < LB(d))
        {
          position[p](d) = LB(d);
          if(velocity[p](d) < 0.0) velocity[p](d) *= -1.0;
        }
        if(position[p](d) > UB(d))
        {
          position[p](d) = UB(d);
          if(velocity[p](d) > 0.0) velocity[p](d) *= -1.0;
        }
#else
//#warning "trying no bounds on periodic"
//        // TODO: I'm not sure this is the right thing to do/enough. The
//        // velocities could be weird. Suppose the current "best" value is ε and
//        // the value is -ε and the "periodic bounds" [0,2π]. Moding will send
//        // the value to 2π-ε but the "velocity" term will now be huge pointing
//        // all the way from 2π-ε to ε.
//        //
//        // Q: Would it be enough to try (all combinations) of ±(UB-LB) before
//        // computing velocities to "best"s? In the example above, instead of
//        //
//        //     v += best - p = ε - (2π-ε) = -2π+2ε
//        //
//        // you'd use
//        //
//        //     v +=  / argmin  |b - p|            \  - p = (ε+2π)-(2π-ε) = 2ε
//        //          |                              |
//        //           \ b∈{best, best+2π, best-2π} /
//        //
//        // Though, for multivariate b,p,v this would seem to explode
//        // combinatorially.
//        //
//        // Maybe periodic things just shouldn't be bounded and we hope that the
//        // forces toward the current minima "regularize" them away from insane
//        // values.
//        if(P(d))
//        {
//          position[p](d) = std::fmod(position[p](d)-LB(d),UB(d)-LB(d))+LB(d);
//        }else
//        {
//          position[p](d) = std::max(LB(d),std::min(UB(d),position[p](d)));
//        }
        position[p](d) = std::max(LB(d),std::min(UB(d),position[p](d)));
#endif
      }
      //std::cout << "Calling F " << std::endl; // LWW
      const Scalar fp = f(position[p]);
      if(fp<best_f[p])
      {
        best_f[p] = fp;
        best_position[p] = position[p];
        if(best_f[p] < min_f)
        {
          min_f = best_f[p];
          X = best_position[p];
        }
      }
    }
    // std::cout << "iter: " << iter << std::endl; // LWW
    iter++;
    if(iter>=max_iters)
    {
      #ifdef PSO_EARLY_EXIT_VERBOSE
      std::cout << "max_iters: " << min_f << std::endl; // LWW
      #endif
      // if (min_f <= -1.0) {
      //   min_f = -1.0;
      // }
      break;
    }
    if (min_f <= -1.0)
    {
      #ifdef PSO_EARLY_EXIT_VERBOSE
      std::cout << "PSO_end_min_f: " << min_f << std::endl; // LWW
      #endif
      return -1.0;
    }
  }
  return min_f;
}

#ifdef IGL_STATIC_LIBRARY
template float igl::pso_earlyExit<float, Eigen::Matrix<float, 1, -1, 1, 1, -1>, Eigen::Matrix<float, 1, -1, 1, 1, -1>, Eigen::Matrix<float, 1, -1, 1, 1, -1> >(std::function<float (Eigen::Matrix<float, 1, -1, 1, 1, -1>&)>, Eigen::MatrixBase<Eigen::Matrix<float, 1, -1, 1, 1, -1> > const&, Eigen::MatrixBase<Eigen::Matrix<float, 1, -1, 1, 1, -1> > const&, int, int, Eigen::Matrix<float, 1, -1, 1, 1, -1>&);
#endif
