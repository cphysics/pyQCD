#include <boost/python.hpp>
#include <boost/python/list.hpp>
#include "lattice.hpp"

namespace py = boost::python;

class pyLattice: public Lattice
{
  friend struct lattice_pickle_suite;
public:
  pyLattice(const int n = 8,
	  const double beta = 5.5,
	  const int Ncor = 50,
	  const int Ncf = 1000,
	  const double eps = 0.24,
	  const double a = 0.25,
	  const double rho = 0.3,
	  const double u0 = 1,
	  const int action = 0);
  ~pyLattice();

  double calcTwistRect_p(const py::list site2,const int mu, const int nu);
  double calcRectangle_p(const py::list site2,const int mu, const int nu);
  double calcPlaquette_p(const py::list site2,const int mu, const int nu);
  double calcWilsonLoop_p(const py::list cnr, const int r, const int t, const int dim, const int n_smears = 0);
  double calcAverageWilson_p(const int r, const int t, const int n_smears = 0);
  void runThreads(const int size, const int n_updates, const int remainder);
  py::list getLink_p(const int i, const int j, const int k, const int l, const int m) const;
  py::list getRandSU3(const int i) const;
};
