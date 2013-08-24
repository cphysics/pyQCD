#define BOOST_TEST_MODULE Lattice test
#include <lattice.hpp>
#include <boost/test/unit_test.hpp>
#include <Eigen/Dense>
#include <complex>
#include <cstdlib>

using namespace Eigen;
using namespace std;

class exposedLattice: public Lattice
{
public:
  exposedLattice(const int spatialExtent = 4,
		 const int temporalExtent = 8,
		 const double beta = 5.5,
		 const double u0 = 1.0,
		 const int action = 0,
		 const int nCorrelations = 50,
		 const int updateMethod = 0,
		 const int parallelFlag = 1,
		 const int chunkSize = 4) :
    Lattice::Lattice(spatialExtent, temporalExtent, beta, u0, action,
		   nCorrelations, updateMethod, parallelFlag, chunkSize)
  {
    
  }

  ~exposedLattice()
  {
    
  }

  
  double computeLocalWilsonAction(const int link[5])
  {
    return Lattice::computeLocalWilsonAction(link);
  }

  double computeLocalRectangleAction(const int link[5])
  {
    return Lattice::computeLocalRectangleAction(link);
  }

  double computeLocalTwistedRectangleAction(const int link[5])
  {
    return Lattice::computeLocalTwistedRectangleAction(link);
  }

  Matrix3cd computeWilsonStaples(const int link[5])
  {
    return Lattice::computeWilsonStaples(link);
  }

  Matrix3cd computeRectangleStaples(const int link[5])
  {
    return Lattice::computeRectangleStaples(link);
  }
};

bool areEqual(const double x, const double y, const double precision)
{
  if (fabs(x - y) < precision)
    return true;
  else
    return false;
}

bool areEqual(const complex<double> x, const complex<double> y,
	      const double precision)
{
  if (!areEqual(x.real(), y.real(), precision))
    return false;
  else if (!areEqual(x.imag(), y.imag(), precision))
    return false;
  else
    return true;
}

bool areEqual(const Matrix3cd& A, const Matrix3cd& B, const double precision)
{
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (!areEqual(A(i, j), B(i, j), precision))
	return false;

  return true;
}

complex<double> randomComplexNumber()
{
  srand(time(0));

  double x = double(rand()) / double(RAND_MAX);
  double y = double(rand()) / double(RAND_MAX);

  return complex<double>(x, y);
}

BOOST_AUTO_TEST_CASE( gluonic_measurements_test )
{
  Lattice lattice;
  complex<double> randC = randomComplexNumber();
  Matrix3cd randomDiagonalMatrix = randC * Matrix3cd::Identity();
  
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 4; ++j) {
      for (int k = 0; k < 4; ++k) {
	for (int l = 0; l < 4; ++l) {
	  for (int m = 0; m < 4; ++m) {
	    int tempLink[5] = {i, j, k, l, m};
	    lattice.setLink(tempLink, randomDiagonalMatrix);
	  }
	}
      }
    }
  }

  double plaquetteVal = pow(abs(randC), 4);
  double rectangleVal = pow(abs(randC), 6);
  double twistedRectangleVal = pow(abs(randC), 8);

  int site[4] = {0, 0, 0, 0};

  // Checking all plaquettes, rectangles and twisted rectangles
  for (int i = 1; i < 4; ++i) {
    for (int j = 0; j < i; ++j) {
      BOOST_CHECK(areEqual(lattice.computePlaquette(site, i, j),
			   plaquetteVal, 100 * DBL_EPSILON));
      BOOST_CHECK(areEqual(lattice.computeRectangle(site, i, j),
			   rectangleVal, 100 * DBL_EPSILON));
      BOOST_CHECK(areEqual(lattice.computeTwistedRectangle(site, i, j),
			   twistedRectangleVal, 100 * DBL_EPSILON));
    }
  }
  // Checking average plaquette and rectangle
  BOOST_CHECK(areEqual(lattice.computeAveragePlaquette(),
		       plaquetteVal, 100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(lattice.computeAverageRectangle(),
		       rectangleVal, 100 * DBL_EPSILON));
  // Checking average Wilson loops
  BOOST_CHECK(areEqual(lattice.computeAverageWilsonLoop(1, 1),
		       plaquetteVal, 100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(lattice.computeAverageWilsonLoop(1, 1, 1, 0.5),
		       plaquetteVal, 100 * DBL_EPSILON));
}

BOOST_AUTO_TEST_CASE( action_test )
{
  exposedLattice lattice;
  // Initialise the lattice with a randome complex matrix
  complex<double> randC = randomComplexNumber();
  Matrix3cd randomDiagonalMatrix = randC * Matrix3cd::Identity();
  
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 4; ++j) {
      for (int k = 0; k < 4; ++k) {
	for (int l = 0; l < 4; ++l) {
	  for (int m = 0; m < 4; ++m) {
	    int tempLink[5] = {i, j, k, l, m};
	    lattice.setLink(tempLink, randomDiagonalMatrix);
	  }
	}
      }
    }
  }
  
  // Calculate plaquette, rectangle and twisted rectangle values
  double plaquetteVal = pow(abs(randC), 4);
  double rectangleVal = pow(abs(randC), 6);
  double twistedRectangleVal = pow(abs(randC), 8);
  // Use these values to calculate the actions
  double wilsonAction = -6 * plaquetteVal;
  double rectangleAction = 5.0 / 3.0 * wilsonAction + 18 / 12.0 * rectangleVal;
  double twistedRectangleAction = wilsonAction - 21 / 12.0 * twistedRectangleVal;

  // Compare the actions as calculated on the lattice with those calculated
  // above
  int link[5] = {0, 0, 0, 0, 0};
  BOOST_CHECK(areEqual(lattice.computeLocalWilsonAction(link),
		       5.5 * wilsonAction, 100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(lattice.computeLocalRectangleAction(link),
		       5.5 * rectangleAction, 100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(lattice.computeLocalTwistedRectangleAction(link),
		       5.5 * twistedRectangleAction, 100 * DBL_EPSILON));

  // Calculate the staples
  Matrix3cd wilsonStaples = lattice.computeWilsonStaples(link);
  Matrix3cd rectangleStaples = lattice.computeRectangleStaples(link);
  Matrix3cd linkMatrix = lattice.getLink(link);
  // Compare the lattice staples with the calculated action
  BOOST_CHECK(areEqual(1.0 / 3.0 * (linkMatrix * wilsonStaples).trace().real(),
		       -wilsonAction, 100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(1.0 / 3.0 * (linkMatrix * rectangleStaples)
		       .trace().real(), -rectangleAction,
		       100 * DBL_EPSILON));
}

BOOST_AUTO_TEST_CASE( update_test )
{
  int linkCoords[5] = {0, 0, 0, 0, 0};

  // Checking parallel heatbath updates
  Lattice lattice(8, 8, 5.5, 1.0, 0, 10, 0, 1, 4);
  lattice.thermalize();
  lattice.reunitarize();
  // Check basic observables
  BOOST_CHECK(lattice.computeAveragePlaquette() < 0.51 &&
	      lattice.computeAveragePlaquette() > 0.49);   
  BOOST_CHECK(lattice.computeAverageRectangle() < 0.27 &&
	      lattice.computeAverageRectangle() > 0.25);
  // Check unitarity
  Matrix3cd testLink = lattice.getLink(linkCoords);
  BOOST_CHECK(areEqual(testLink * testLink.adjoint(), Matrix3cd::Identity(),
		       100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(testLink.determinant(), 1.0, 100 * DBL_EPSILON));

  // Checking parallel Monte Carlo updates
  lattice = Lattice(8, 8, 5.5, 1.0, 0, 70, 1, 1, 4);
  lattice.thermalize();
  lattice.reunitarize();
  // Check basic observables
  BOOST_CHECK(areEqual(lattice.computeAveragePlaquette(), 0.5, 0.1));
  BOOST_CHECK(areEqual(lattice.computeAverageRectangle(), 0.26, 0.1));
  // Check unitarity
  testLink = lattice.getLink(linkCoords);
  BOOST_CHECK(areEqual(testLink * testLink.adjoint(), Matrix3cd::Identity(),
		       100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(testLink.determinant(), 1.0, 100 * DBL_EPSILON));

  // Checking serial heatbath
  lattice = Lattice(8, 8, 5.5, 1.0, 0, 10, 0, 0, 4);
  lattice.thermalize();
  lattice.reunitarize();
  // Check basic observables
  BOOST_CHECK(areEqual(lattice.computeAveragePlaquette(), 0.5, 0.1));
  BOOST_CHECK(areEqual(lattice.computeAverageRectangle(), 0.26, 0.1));
  // Check unitarity
  testLink = lattice.getLink(linkCoords);
  BOOST_CHECK(areEqual(testLink * testLink.adjoint(), Matrix3cd::Identity(),
		       100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(testLink.determinant(), 1.0, 100 * DBL_EPSILON));

  // Checking serial Monte Carlo
  lattice = Lattice(8, 8, 5.5, 1.0, 0, 70, 1, 0, 4);
  lattice.thermalize();
  lattice.reunitarize();
  // Check basic observables
  BOOST_CHECK(areEqual(lattice.computeAveragePlaquette(), 0.5, 0.1));
  BOOST_CHECK(areEqual(lattice.computeAverageRectangle(), 0.26, 0.1));
  // Check unitarity
  testLink = lattice.getLink(linkCoords);
  BOOST_CHECK(areEqual(testLink * testLink.adjoint(), Matrix3cd::Identity(),
		       100 * DBL_EPSILON));
  BOOST_CHECK(areEqual(testLink.determinant(), 1.0, 100 * DBL_EPSILON));
}