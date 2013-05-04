#include "lattice.hpp"

namespace lattice
{
  const complex<double> i (0,1);
  const double pi = 3.1415926535897932384626433;
  
  int mod(int number, const int &divisor)
  {
    number += (abs(number / divisor) + 1) * divisor;
    return number % divisor;
  }
  


  void copyArray(int array1[], const int array2[], const int& length)
  {
    for (int i = 0; i < length; i++) {
      array1[i] = array2[i];
    }
  }



  int sgn(const int& x)
  {
    return (x < 0) ? -1 : 1;
  }



  bool isArrayEqual(const int array1[], const int array2[],
		    const int& length)
  {
    for (int i = 0; i < length; i++) {
      if (array1[i] != array2[i])
	return false;
    }
    return true;
  }



  Matrix4cd gamma1 = (MatrixXcd(4,4) << 0, 0, 0, -i,
		      0, 0, -i, 0,
		      0, i, 0, 0,
		      i, 0, 0, 0).finished();
  
  Matrix4cd gamma2 = (MatrixXcd(4,4) <<  0, 0, 0, -1,
		      0, 0, 1, 0,
		      0, 1, 0, 0,
		      -1, 0, 0, 0).finished();

  Matrix4cd gamma3 = (MatrixXcd(4,4) << 0, 0, -i, 0,
		      0, 0, 0, i,
		      i, 0, 0, 0,
		      0, -i, 0, 0).finished();

  Matrix4cd gamma4 = (MatrixXcd(4,4) << 0, 0, 1, 0,
		      0, 0, 0, 1,
		      1, 0, 0, 0,
		      0, 1, 0, 0).finished();

  Matrix4cd gamma5 = (MatrixXcd(4,4) << 1, 0, 0, 0,
		      0, 1, 0, 0,
		      0, 0, -1, 0,
		      0, 0, 0, -1).finished();
  
  Matrix4cd gammas[5] = {gamma1, gamma2, gamma3, gamma4, gamma5};


  
  Matrix4cd gamma(const int& index)
  {
    int prefactor = sgn(index);
    return prefactor * gammas[abs(index) - 1];
  }
}

Lattice::Lattice(const int nEdgePoints, const double beta,
		 const int nCorrelations, const int nConfigurations,
		 const double epsilson, const double a, const double rho,
		 const double u0, const int action)
{
  // Default constructor. Assigns function arguments to member variables
  // and initializes links.
  this->nEdgePoints = nEdgePoints;
  this->beta = beta;
  this->nCorrelations = nCorrelations;
  this->nConfigurations = nConfigurations;
  this->epsilon = epsilon;
  this->a = a;
  this->rho = rho;
  this->nUpdates = 0;
  this->u0 = u0;
  this->action = action;

  srand(time(0));
  // Resize the link vector and assign each link a random SU3 matrix
  this->links_.resize(this->nEdgePoints);
  for (int i = 0; i < this->nEdgePoints; i++) {
    this->links_[i].resize(this->nEdgePoints);
    for (int j = 0; j < this->nEdgePoints; j++) {
      this->links_[i][j].resize(this->nEdgePoints);
      for (int k = 0; k < this->nEdgePoints; k++) {
	this->links_[i][j][k].resize(this->nEdgePoints);
	for (int l = 0; l < this->nEdgePoints; l++) {
	  this->links_[i][j][k][l].resize(4);
	  for (int m = 0; m < 4; m++) {
	    this->links_[i][j][k][l][m] = this->makeRandomSU3();
	  }
	}
      }
    }
  }

  // Generate a set of random SU3 matrices for use in the updates
  for (int i = 0; i < 200; i++) {
    Matrix3cd randSU3 = this->makeRandomSU3();
    this->randSU3s.push_back(randSU3);
    this->randSU3s.push_back(randSU3.adjoint());
  }

  // Set the action to point to the correct function
  if (action == 0) {
    this->computeLocalAction = &Lattice::computeLocalWilson;
  }
  else if (action == 1) {
    this->computeLocalAction = &Lattice::computeLocalRectangle;
  }
  else if (action == 2) {
    this->computeLocalAction = &Lattice::computeLocalTwistedRectangle;
  }
  else {
    cout << "Warning! Specified action does not exist." << endl;
    this->computeLocalAction = &Lattice::computeLocalWilson;
  }
}



Lattice::Lattice(const Lattice& lattice)
{
  // Default constructor. Assigns function arguments to member variables
  // and initializes links.
  this->nEdgePoints = lattice.nEdgePoints;
  this->beta = lattice.beta;
  this->nCorrelations = lattice.nCorrelations;
  this->nConfigurations = lattice.nConfigurations;
  this->eps = lattice.eps;
  this->a = lattice.a;
  this->rho = lattice.rho;
  this->nUpdates = lattice.nUpdates;
  this->u0 = lattice.u0;
  this->links_ = lattice.links_;
  this->randSU3s = lattice.randSU3s;
  this->computeLocalAction = lattice.computeLocalAction;
  this->action = action;
}



Lattice::~Lattice()
{
  // Destructor
}



void Lattice::printLattice()
{
  /*Print the links out. A bit redundant due to the interfaces library,
   but here in case it's needed.*/
  for (int i = 0; i < this->nEdgePoints; i++) {
    for (int j = 0; j < this->nEdgePoints; j++) {
      for (int k = 0; k < this->nEdgePoints; k++) {
	for (int l = 0; l < this->nEdgePoints; l++) {
	  for (int m = 0; m < 4; m++) {
	    cout << this->links_[i][j][k][l][m] << endl;
	  }
	}
      }
    }
  }
}



Matrix3cd Lattice::getLink(const int link[5])
{
  // Return link specified by index (sanitizes link indices)
  int link2[5];
  for (int i = 0; i < 5; i++) {
    link2[i] = lattice::mod(link[i], this->nEdgePoints);
  }
  return this->links_[link2[0]][link2[1]][link2[2]][link2[3]][link2[4]];
}



void Lattice::runThreads(const int size, const int n_updates, const int remainder)
{
  int index = 0;

#pragma omp parallel for schedule(dynamic,1) collapse(4)
  for (int i = 0; i < this->nEdgePoints; i+=size) {
    for (int j = 0; j < this->nEdgePoints; j+=size) {
      for (int k = 0; k < this->nEdgePoints; k+=size) {
	for (int l = 0; l < this->nEdgePoints; l+=size) {
	  int site[4] = {i,j,k,l};
	  if (index%2 == remainder) {
	    this->updateSegment(i,j,k,l,size,n_updates);
	  }
	  index++;
	}
      }
    }
  }
}



void Lattice::schwarzUpdate(const int size, const int n_updates)
{
  //Update even and odd blocks using Schwarz
  this->runThreads(size,n_updates,0);
  this->runThreads(size,n_updates,1);
  this->nUpdates++;
}



void Lattice::update()
{
  /*Iterate through the lattice and update the links using Metropolis
    algorithm*/
  for (int i = 0; i < this->nEdgePoints; i++) {
    for (int j = 0; j < this->nEdgePoints; j++) {
      for (int k = 0; k < this->nEdgePoints; k++) {
	for (int l = 0; l < this->nEdgePoints; l++) {
	  for (int m = 0; m < 4; m++) {
	    //We'll need an array with the link indices
	    int link[5] = {i,j,k,l,m};
	    //Record the old action contribution
	    double Si_old = (this->*computeLocalAction)(link);
	    //Record the old link in case we need it
	    Matrix3cd linki_old = this->links_[i][j][k][l][m];
	    //Get ourselves a random SU3 matrix for the update
	    Matrix3cd randSU3 = this->randSU3s[rand() % this->randSU3s.size()];
	    //Multiply the site
	    this->links_[i][j][k][l][m] = randSU3 * this->links_[i][j][k][l][m];
	    //What's the change in the action?
	    double dS = (this->*computeLocalAction)(link) - Si_old;
	    //Was the change favourable? If not, revert the change
	    if ((dS > 0) && (exp(-dS) < double(rand()) / double(RAND_MAX))) {
	      this->links_[i][j][k][l][m] = linki_old;
	    }
	  }
	}
      }
    }
  }
  this->nUpdates++;
}



void Lattice::updateSegment(const int i, const int j, const int k, const int l, const int size, const int n_updates)
{
  //Updates a segment of the lattice - used for SAP
  for (int n = 0; n < n_updates; n++) {
    for (int m = i; m < i + size; m++) {
      for (int o = j; o < j + size; o++) {
	for (int p = k; p < k + size; p++) {
	  for (int q = l; q < l + size; q++) {
	    for (int r = 0; r < 4; r++) {
	      //We'll need an array with the link indices
	      int link[5] = {m,o,p,q,r};
	      //Record the old action contribution
	      double Si_old = (this->*computeLocalAction)(link);
	      //Record the old link in case we need it
	      Matrix3cd linki_old = this->links_[m][o][p][q][r];
	      //Get ourselves a random SU3 matrix for the update
	      Matrix3cd randSU3 = this->randSU3s[rand() % this->randSU3s.size()];
	      //Multiply the site
	      this->links_[m][o][p][q][r] = randSU3 * this->links_[m][o][p][q][r];
	      //What's the change in the action?
	      double dS = (this->*computeLocalAction)(link) - Si_old;
	      //Was the change favourable? If not, revert the change
	      if ((dS > 0) && (exp(-dS) < double(rand()) / double(RAND_MAX))) {
		this->links_[m][o][p][q][r] = linki_old;
	      }
	    }
	  }
	}
      }
    }
  }
}



void Lattice::thermalize()
{
  /*Update all links until we're at thermal equilibrium*/
  while(this->nUpdates < 5 * this->nCorrelations) {
    this->schwarzUpdate(4,10);
  }
}



void Lattice::getNextConfig()
{
  /*Run nCorrelations updates*/
  for (int i = 0; i < this->nCorrelations; i++) {
    this->schwarzUpdate(4,1);
  }
}



Matrix3cd Lattice::computePath(const vector<vector<int> > path)
{
  // Multiplies the matrices together specified by the indices in path
  Matrix3cd out = Matrix3cd::Identity();
  
  for (int i = 0; i < path.size() - 1; i++) {
    // Which dimension are we moving in?
    int dimension = path[i][4];
    int dimensionDifference = path[i + 1][dimension] - path[i][dimension];

    if (abs(dimensionDifference) != 1) {
      // Consecutive points don't match link direction, so throw an error
      cout << "Error! Path contains non-consecutive link variables." << endl;
    }
    else if (dimensionDifference == -1) {
      // We're going backwards, so the link must be the adjoint of the link 
      // matrix, which we get by using the next site on the lattice.
      int link[5] = {path[i + 1][0],
		     path[i + 1][1],
		     path[i + 1][2],
		     path[i + 1][3],
		     path[i][4]};

      out *= this->getLink(link).adjoint();
    }
    else {
      // We're going forwards, so it's just the normal matrix
      int link[5] = {path[i][0],
		     path[i][1],
		     path[i][2],
		     path[i][3],
		     path[i][4]};

      out *= this->getLink(link);
    }
  }
  return out;
}



Matrix3cd Lattice::computeLine(const int start[4], const int finish[4])
{
  // Multiplies all gauge links along line from start to finish
  // First check that's actually a straight path
  Matrix3cd out = Matrix3cd::Identity();
  int countDimensions = 0;
  int dimension = 0;
  for (int i = 0; i < 4; i++) {
    if (abs(start[i] - finish[i]) != 0) {
      // Keep track of the most recent dimension that differs between start
      // and finish, as if we're good to go then we'll need this when
      // defining the path.
      dimension = i;
      countDimensions++;
    }
  }

  if (countDimensions != 1) {
    cout << "Error! Start and end points do not form a straight line."
	 << endl;
  }
  else {
    // If the two points are on the same line, we're good to go
    vector<vector<int> >  line; //Stores the path

    // Now need to know if we're going backwards or forwards
    if (start[dimension] > finish[dimension]) {
      for (int i = start[dimension]; i >= finish[dimension]; i--) {
	// Create the link vector to append, and initialize it's elements
	vector<int> link;
	link.assign(start,start+4);
	// Update the index that's parallel to the line with the current
	// location
	link[dimension] = i;
	// The direction is going to be equal to the direction of the line
	link.push_back(dimension);
	// Push it onto the line
	line.push_back(link);
      }
      out = this->computePath(line);
    }
    else {
      // Same again, but this time we deal with the case of going backwards
      for (int i = start[dimension]; i <= finish[dimension]; i++) {
	vector<int> link;
	link.assign(start, start+4);
	link[dimension] = i;
	link.push_back(dimension);
	line.push_back(link);
      }
      out = this->computePath(line);
    }
  }
  return out;
}



double Lattice::computeWilsonLoop(const int corner1[4], const int corner2[4],
				  const int nSmears)
{
  // Calculates the loop specified by corners corner1 and corner2 (which must
  // lie in the same plane)
  Matrix3cd out = Matrix3cd::Identity();
  SubField linkStore1;
  SubField linkStore2;
  //Smear the links if specified, whilst storing the non-smeared ones.
  if (nSmears > 0) {
    linkStore1 = this->links_[lattice::mod(corner1[0],this->nEdgePoints)];
    linkStore2 = this->links_[lattice::mod(corner2[0],this->nEdgePoints)];
    this->smearLinks(corner1[0],nSmears);
    this->smearLinks(corner2[0],nSmears);
  }

  //Check that corner1 and corner2 are on the same plane
  int dimensionCount = 0;
  for (int i = 0; i < 4; i++) {
    if (corner1[i] != corner2[i]) {
      dimensionCount++;
    }
  }
  
  if (dimensionCount != 2 || corner1[0] == corner2[0]) {
    cout << "Error! The two corner points do not form a rectangle with"
	 << " two spatial and two temporal sides." << endl;
  }
  else {
    //Get the second corner (going round the loop)
    int corner3[4] = {corner1[0], corner1[1], corner1[2], corner1[3]};
    corner3[0] = corner2[0];
    //Calculate the line segments between the first three corners
    out *= this->computeLine(corner1, corner3);
    out *= this->computeLine(corner3, corner2);
    //And repeat for the second set of sides
    int corner4[4] = {corner2[0], corner2[1], corner2[2], corner2[3]};
    corner4[0] = corner1[0];
    out *= this->computeLine(corner2, corner4);
    out *= this->computeLine(corner4, corner1);
  }
  //Restore the old links
  if (nSmears > 0) {
    this->links_[lattice::mod(corner1[0],this->nEdgePoints)] = linkStore1;
    this->links_[lattice::mod(corner2[0],this->nEdgePoints)] = linkStore2;
  }
  
  return 1./3 * out.trace().real();
}



double Lattice::computeWilsonLoop(const int corner[4], const int r,
				  const int t, const int dimension,
				  const int nSmears)
{
  /*Calculates the loop specified by initial corner, width, height and 
   dimension*/
  int corner2[4];
  lattice::copyArray(corner2,c,4);
  corner2[dimension] += r;
  corner2[0] += t;
  return this->computeWilsonLoop(corner,corner2,nSmears);
}



double Lattice::computePlaquette(const int site[4],const int mu,
				 const int nu)
{
  /*Calculate the plaquette operator at the given site, on plaquette
    specified by directions mu and nu.*/

  //We define some variables to contain the offsets of the various
  //links around the lattice
  int mu_vec[4] = {0,0,0,0};
  mu_vec[mu] = 1;
  int nu_vec[4] = {0,0,0,0};
  nu_vec[nu] = 1;
  //The links also contain direction information, so we must create a new
  //set of variables
  int link1[5] = {0,0,0,0,mu};
  int link2[5] = {0,0,0,0,nu};
  int link3[5] = {0,0,0,0,mu};
  int link4[5] = {0,0,0,0,nu};

  //Do some assignment
  for (int i = 0; i < 4; i++) {
    link1[i] = site[i];
    link2[i] = site[i] + mu_vec[i];
    link3[i] = site[i] + nu_vec[i];
    link4[i] = site[i];
  }

  //Run through the links and multiply them together.
  Matrix3cd product = this->getLink(link1);
  product *= this->getLink(link2);
  product *= this->getLink(link3).adjoint();
  product *= this->getLink(link4).adjoint();
  return 1./3 * product.trace().real();
}



double Lattice::computeRectangle(const int site[4],const int mu,
				 const int nu)
{
  /*Calculate the rectangle operator at the given site, on the rectangle
    specified by directions mu and nu.*/
  
  //Define some variables to offset the given site
  int mu_vec[4] = {0,0,0,0};
  mu_vec[mu] = 1;
  int nu_vec[4] = {0,0,0,0};
  nu_vec[nu] = 1;
  //Links also contain direction information, so must create a new set of
  //variables
  int link1[5] = {0,0,0,0,mu};
  int link2[5] = {0,0,0,0,mu};
  int link3[5] = {0,0,0,0,nu};
  int link4[5] = {0,0,0,0,mu};
  int link5[5] = {0,0,0,0,mu};
  int link6[5] = {0,0,0,0,nu};

  for (int i = 0; i < 4; i++) {
    link1[i] = site[i];
    link2[i] = site[i] + mu_vec[i];
    link3[i] = site[i] + 2 * mu_vec[i];
    link4[i] = site[i] + mu_vec[i] + nu_vec[i];
    link5[i] = site[i] + nu_vec[i];
    link6[i] = site[i];
  }

  //Multiply all the links together to get the product.
  Matrix3cd product = this->getLink(link1);
  product *= this->getLink(link2);
  product *= this->getLink(link3);
  product *= this->getLink(link4).adjoint();
  product *= this->getLink(link5).adjoint();
  product *= this->getLink(link6).adjoint();

  return 1./3 * product.trace().real();  
}



double Lattice::computeTwistedRectangle(const int site[4],const int mu, const int nu)
{
  /*Calculate the twisted rectangle operator*/
  
  //Define some variables to offset the given site
  int mu_vec[4] = {0,0,0,0};
  mu_vec[mu] = 1;
  int nu_vec[4] = {0,0,0,0};
  nu_vec[nu] = 1;
  //Links also contain direction information, so must create a new set of
  //variables
  int link1[5] = {0,0,0,0,mu};
  int link2[5] = {0,0,0,0,nu};
  int link3[5] = {0,0,0,0,mu};
  int link4[5] = {0,0,0,0,nu};
  int link5[5] = {0,0,0,0,mu};
  int link6[5] = {0,0,0,0,nu};
  int link7[5] = {0,0,0,0,mu};
  int link8[5] = {0,0,0,0,nu};

  for (int i = 0; i < 4; i++) {
    link1[i] = site[i];
    link2[i] = site[i] + mu_vec[i];
    link3[i] = site[i] + mu_vec[i] + nu_vec[i];
    link4[i] = site[i] + 2 * mu_vec[i];
    link5[i] = site[i] + mu_vec[i];
    link6[i] = site[i] + mu_vec[i];
    link7[i] = site[i] + nu_vec[i];
    link8[i] = site[i];
  }

  //Multiply all the links together to get the product.
  Matrix3cd product = this->getLink(link1);
  product *= this->getLink(link2);
  product *= this->getLink(link3);
  product *= this->getLink(link4).adjoint();
  product *= this->getLink(link5).adjoint();
  product *= this->getLink(link6);
  product *= this->getLink(link7).adjoint();
  product *= this->getLink(link8).adjoint();

  return 1./3 * product.trace().real();
}



double Lattice::computeAveragePlaquette()
{
  /*Calculate average plaquette operator value*/
  //mu > nu, so there are six plaquettes at each site.
  int nus[6] = {0,0,0,1,1,2};
  int mus[6] = {1,2,3,2,3,3};
  double Ptot = 0;
  //Pretty simple: step through the matrix and add all plaquettes up
  for (int i = 0; i < this->nEdgePoints; i++) {
    for (int j = 0; j < this->nEdgePoints; j++) {
      for (int k = 0; k < this->nEdgePoints; k++) {
	for (int l = 0; l < this->nEdgePoints; l++) {
	  for (int m = 0; m < 6; m++) {
	    int site[4] = {i,j,k,l};
	    Ptot += this->computePlaquette(site,mus[m],nus[m]);
	  }
	}
      }
    }
  }
  //Divide through by number of plaquettes to get mean (simples!)
  return Ptot / (pow(this->nEdgePoints,4) * 6);
}



double Lattice::computeAverageRectangle()
{
  /*Calculate average plaquette operator value*/
  //mu > nu, so there are six plaquettes at each site.
  int nus[6] = {0,0,0,1,1,2};
  int mus[6] = {1,2,3,2,3,3};
  double Rtot = 0;
  //Pretty simple: step through the matrix and add all plaquettes up
  for (int i = 0; i < this->nEdgePoints; i++) {
    for (int j = 0; j < this->nEdgePoints; j++) {
      for (int k = 0; k < this->nEdgePoints; k++) {
	for (int l = 0; l < this->nEdgePoints; l++) {
	  for (int m = 0; m < 6; m++) {
	    int site[4] = {i,j,k,l};
	    Rtot += this->R(site,mus[m],nus[m]);
	  }
	}
      }
    }
  }
  //Divide through by number of plaquettes to get mean (simples!)
  return Rtot / (pow(this->nEdgePoints,4) * 6);
}



double Lattice::computeAverageWilsonLoop(const int r, const int t,
					 const int nSmears)
{
  /*Calculates the average of all possible Wilson loops of a given
    dimension*/
  //First off, save the current links and smear all time slices
  GaugeField templinks = this->links_;
  for (int time = 0; time < this->nEdgePoints; time++) {
    this->smearLinks(time,nSmears);
  }
  double Wtot = 0;
  for (int i = 0; i < this->nEdgePoints; i++) {
    for (int j = 0; j < this->nEdgePoints; j++) {
      for (int k = 0; k < this->nEdgePoints; k++) {
	for (int l = 0; l < this->nEdgePoints; l++) {
	  for (int m = 1; m < 4; m++) {
	    int site[4] = {i,j,k,l};
	    Wtot += this->computeWilsonLoop(site,r,t,m,0);
	  }
	}
      }
    }
  }
  this->links_ = templinks;
  return Wtot / (pow(this->nEdgePoints,4)*3);
}



Matrix3cd Lattice::makeRandomSU3()
{
  /*Generate a random SU3 matrix, weighted by eps*/
  Matrix3cd A;
  //First generate a random matrix whos elements all lie in/on unit circle  
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      A(i,j) = double(rand()) / double(RAND_MAX);
      A(i,j) *= exp(2 * lattice::pi * lattice::i * double(rand()) / double(RAND_MAX));
    }
  }
  //Weight the matrix with weighting eps
  A *= this->eps;
  //Make the matrix traceless and Hermitian
  A(2,2) = -(A(1,1) + A(0,0));
  Matrix3cd B = 0.5 * (A - A.adjoint());
  Matrix3cd U = B.exp();
  //Compute the matrix exponential to get a special unitary matrix
  return U;
}



Matrix3cd Lattice::computeQ(const int link[5])
{
  // Calculates Q matrix for analytic smearing according to paper on analytic
  // smearing
  Matrix3cd C = Matrix3cd::Zero();

  for (int nu = 1; nu < 4; nu++) {
    if (nu != link[4]) {
      int tempLink[5] = {0,0,0,0,0};
      lattice::copyArray(tempLink, link, 4);
      tempLink[4] = nu;
      Matrix3cd tempMatrix = this->getLink(tempLink);

      tempLink[4] = link[4];
      tempLink[nu] += 1;
      tempMatrix *= this->getLink(tempLink);

      tempLink[4] = nu;
      tempLink[nu] -= 1;
      tempLink[link[4]] += 1;
      tempMatrix *= this->getLink(tempLink).adjoint();

      C += tempMatrix;

      lattice::copyArray(tempLink, link, 4);
      tempLink[nu] -= 1;
      tempLink[4] = nu;
      tempMatrix = this->getLink(tempLink).adjoint();

      tempLink[4] = link[4];
      tempMatrix *= this->getLink(tempLink);

      tempLink[4] = nu;
      tempLink[link[4]] += 1;
      tempMatrix *= this->getLink(tempLink);

      C += tempMatrix;
    }
  }
  
  C *= this->rho;

  Matrix3cd Omega = C * this->getLink(link).adjoint();
  Matrix3cd OmegaAdjoint = Omega.adjoint() - Omega;
  Matrix3cd Q = 0.5 * lattice::i * OmegaAdjoint;
  Q -= lattice::i / 6.0 * OmegaAdjoint.trace() * Matrix3cd::Identity();

  return Q;
}



void Lattice::smearLinks(const int time, const int nSmears)
{
  // Smear the specified time slice by iterating calling this function
  for (int i = 0; i < nSmears; i++) {
    // Iterate through all the links and calculate the new ones from
    // the existing ones.    
    SubField newLinks(this->nEdgePoints, 
		      Sub2Field(this->nEdgePoints,
				Sub3Field(this->nEdgePoints, 
					  Sub4Field(4))));

    for (int i = 0; i < this->nEdgePoints; i++) {
      for (int j = 0; j < this->nEdgePoints; j++) {
	for (int k = 0; k < this->nEdgePoints; k++) {
	  // NB, spatial links only, so l > 0!
	  newLinks[i][j][k][0] = this->links_[time][i][j][k][0];
	  for (int l = 1; l < 4; l++) {
	    // Create a temporary matrix to store the new link
	    int link[5] = {time,i,j,k,l};
	    Matrix3cd tempMatrix = (lattice::i * this->computeQ(link)).exp()
	      * this->getLink(link);
	    newLinks[i][j][k][l] = tempMatrix;
	  }
	}
      }
    }
    // Apply the changes to the existing lattice.
    this->links_[lattice::mod(time, this->nEdgePoints)] = newLinks;
  }
}



SparseMatrix<complex<double> > Lattice::computeDiracMatrix(const double mass)
{
  //Calculates the Dirac matrix for the current field configuration
  //using Wilson fermions
  
  //Calculate some useful quantities
  int n_indices = int(12 * pow(this->nEdgePoints,4));
  int n_sites = int(pow(this->nEdgePoints,4));
  //Create the sparse matrix we're going to return
  SparseMatrix<complex<double> > out(n_indices,n_indices);

  vector<Tlet> tripletList;
  for (int i = 0; i < n_indices; i++) {
    tripletList.push_back(Tlet(i,i,mass + 4/this->a));
  }

  //Create and initialise a vector of the space, lorentz and colour indices
  vector<vector<int> > indices(pow(this->nEdgePoints,4) * 12,vector<int>(6));
  //int** indices = new int* [int(pow(this->nEdgePoints,4) * 12)];
  int index = 0;
  for (int i = 0; i < this->nEdgePoints; i++) {
    for (int j = 0; j < this->nEdgePoints; j++) {
      for (int k = 0; k < this->nEdgePoints; k++) {
	for (int l = 0; l < this->nEdgePoints; l++) {
	  for (int alpha = 0; alpha < 4; alpha++) {
	    for (int a = 0; a < 3; a++) {
	      //indices[index] = new int[6];
	      indices[index][0] = i;
	      indices[index][1] = j;
	      indices[index][2] = k;
	      indices[index][3] = l;
	      indices[index][4] = alpha;
	      indices[index][5] = a;
	      index++;
	    }
	  }
	}
      }
    }
  }

  int mus[8] = {-4,-3,-2,-1,1,2,3,4};
  
  //Now iterate through the matrix and add the various elements to the vector
  //of triplets
  #pragma omp parallel for
  for (int i = 0; i < n_indices; i++) {
    int site_i[4] = {indices[i][0],
		     indices[i][1],
		     indices[i][2],
		     indices[i][3]};
    
    for (int j = 0; j < n_indices; j++) {
      int m = i / 12;
      int n = j / 12;

      //We can determine whether the spatial indices are going
      //to trigger the delta function in advance, and hence
      //if that's not going to happen we can save ourself a lot
      //of hassle
      bool delta = false;
      for (int k = 0; k < 4; k++) {
	//Store this to save calculating it twice
	int n_off = pow(this->nEdgePoints,k);	
	if (m == lattice::mod(n + n_off,n_sites)
	   || m == lattice::mod(n - n_off,n_sites)) {
	  delta = true;
	  break;
	}
      }
      if (delta) {
	//First we'll need something to put the sum into
	complex<double> sum = complex<double>(0,0);	
	//First create an array for the site specified by the index i	
	int site_j[4] = {indices[j][0],
			 indices[j][1],
			 indices[j][2],
			 indices[j][3]};
	for (int k = 0; k < 8; k++) {
	  //First need to implement the kronecker delta in the sum of mus,
	  //which'll be horrendous, but hey...
	
	  //Add a minkowski lorentz index because that what the class deals in
	  int mu_mink = abs(mus[k]) % 4;
	  //Add (or subtract) the corresponding mu vector from the second
	  //lattice site
	  site_j[mu_mink] = lattice::mod(site_j[mu_mink] + 
					 lattice::sgn(mus[k]),
					 this->nEdgePoints);
	
	  //If they are, then we have ourselves a matrix element
	  //First test for when mu is positive, as then we'll need to deal
	  //with the +ve or -ve cases slightly differently
	  if (lattice::arrayEqual(site_i,site_j,4)) {
	    int link[5];
	    lattice::copyArray(link,site_i,4);
	    link[4] = mu_mink;
	    Matrix3cd U;
	    Matrix4cd lorentz = Matrix4cd::Identity() - lattice::gamma(mus[k]);
	    if (mus[k] > 0) U = this->getLink(link);
	    else {
	      link[mu_mink] -= 1;
	      U = this->getLink(link).adjoint();
	    }
	    sum += lorentz(indices[i][4],indices[j][4]) 
	      * U(indices[i][5],indices[j][5]);
	  }
	}
	sum /= -(2 * this->a);
	#pragma omp critical
	if (sum.imag() != 0 && sum.real() != 0)
	  tripletList.push_back(Tlet(i,j,sum));
      }
      else {
	j = (n + 1) * 12 - 1;
      }
    }
  }
  
  out.setFromTriplets(tripletList.begin(), tripletList.end());
  
  return out;
}

VectorXcd Lattice::computePropagator(const double mass, int site[4], const int alpha, const int a)
{
  SparseMatrix<complex<double> > D = this->computeDiracMatrix(mass);
  int n_indices = int(12 * pow(this->nEdgePoints,4));
  BiCGSTAB<SparseMatrix<complex<double> > > solver(D);
  
  VectorXcd S(n_indices);
  
  int m = site[3] + this->nEdgePoints * (site[2] + this->nEdgePoints * (site[1] + this->nEdgePoints * site[0]));
  int index = a + 3 * (alpha + 4 * m);
  S(index) = 1.;
  
  VectorXcd propagator = solver.solve(S);
  
  return propagator;
}



double Lattice::computeLocalWilsonAction(const int link[5])
{
  /*Calculate the contribution to the Wilson action from the given link*/
  int planes[3];
  double Psum = 0;

  //Work out which dimension the link is in, since it'll be irrelevant here
  int j = 0;
  for (int i = 0; i < 4; i++) {
    if (link[4] != i) {
      planes[j] = i;
      j++;
    }    
  }

  /*For each plane, calculate the two plaquettes that share the given link*/
  for (int i = 0; i < 3; i++) {
    int site[4] = {link[0],link[1],link[2],link[3]};
    Psum += this->computePlaquette(site,link[4],planes[i]);
    site[planes[i]] -= 1;
    Psum += this->computePlaquette(site,link[4],planes[i]);
  }

  return -this->beta * Psum / pow(this->u0,4);
}



double Lattice::computeLocalRectangleAction(const int link[5])
{
  /*Calculate contribution to improved action from given link*/

  //First contrbution is from standard Wilson action, so add that in
  double out = 5./3 * this->computeLocalWilson(link);
  double Rsum = 0;

  int planes[3];

  //Work out which dimension the link is in, since it'll be irrelevant here
  int j = 0;
  for (int i = 0; i < 4; i++) {
    if (link[4] != i) {
      planes[j] = i;
      j++;
    }
  }
  
  for (int i = 0; i < 3; i++) {
    int site[4] = {link[0],link[1],link[2],link[3]};
    Rsum += this->computeRectangle(site,link[4],planes[i]);
    site[link[4]] -= 1;
    Rsum += this->computeRectangle(site,link[4],planes[i]);

    site[link[4]] += 1;
    site[planes[i]] -= 1;
    Rsum += this->computeRectangle(site,link[4],planes[i]);
    site[link[4]] -= 1;
    Rsum += this->computeRectangle(site,link[4],planes[i]);
    
    site[link[4]] += 1;
    site[planes[i]] += 1;
    Rsum += this->computeRectangle(site,planes[i],link[4]);
    site[planes[i]] -= 2;
    Rsum += this->computeRectangle(site, planes[i],link[4]);
  }
  out += this->beta / (12 * pow(this->u0,6)) * Rsum;
  return out;
}



double Lattice::computeLocalTwistedRectangleAction(const int link[5])
{
  /*Calculate contribution to improved action from given link*/

  //First contrbution is from standard Wilson action, so add that in
  double out = this->computeLocalWilson(link);
  double Tsum = 0;

  int planes[3];

  //Work out which dimension the link is in, since it'll be irrelevant here
  int j = 0;
  for (int i = 0; i < 4; i++) {
    if (link[4] != i) {
      planes[j] = i;
      j++;
    }
  }
  
  for (int i = 0; i < 3; i++) {
    int site[4] = {link[0],link[1],link[2],link[3]};
    Tsum += this->computeTwistRect(site,link[4],planes[i]);
    site[link[4]] -= 1;
    Tsum += this->computeTwistRect(site,link[4],planes[i]);

    site[link[4]] += 1;
    site[planes[i]] -= 1;
    Tsum += this->computeTwistRect(site,link[4],planes[i]);
    site[link[4]] -= 1;
    Tsum += this->computeTwistRect(site,link[4],planes[i]);
    
    site[link[4]] += 1;
    site[planes[i]] += 1;
    Tsum += this->computeTwistRect(site,planes[i],link[4]);
    site[planes[i]] -= 1;
    Tsum += this->computeTwistRect(site, planes[i],link[4]);
    site[planes[i]] -= 1;
    Tsum += this->computeTwistRect(site, planes[i],link[4]);
  }
  out -= this->beta / (12 * pow(this->u0,8)) * Tsum;
  return out;
}
