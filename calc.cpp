#include "ODE.hpp"
#include "calc.hpp"

/* abbreviation: cf1 = cost function 1 */
double calculate_cf1(const VectorXd& trueVec, const VectorXd& estVec){
    double cost = 0;
    VectorXd diff(trueVec.size());
    diff = trueVec - estVec;
    cost = diff.transpose() * diff.transpose().transpose();
    // for(int i = 0; i < n; i++){
    //     cost += (estVec(i) - trueVec(i)) * (estVec(i) - trueVec(i));
    // }
    return cost;
}

double calculate_cf2(const VectorXd& trueVec, const  VectorXd& estVec, const MatrixXd& w){
    double cost = 0;
    VectorXd diff(trueVec.size());
    /*for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
           cost += (estVec(i) - trueVec(i)) * w(i,j) *(estVec(j) - trueVec(j));
        }
    }*/
    diff = trueVec - estVec;
    cost = diff.transpose() * w * (diff.transpose()).transpose();
    return cost;
}

/* mVec = first moment vector (may or may not have other vector components
   m2 = second moment vector
   cov = value returned after calculating cov matrix
*/
MatrixXd calculate_covariance_matrix(const MatrixXd& m2, const VectorXd& mVec, int nProt){
    MatrixXd cov(nProt, nProt);
     /* calculate covariance matrix */
    for(int row = 0; row < nProt; row++){
        for(int col = 0; col < nProt; col++){
            cov(row, col) = m2(row,col) - mVec(row)*mVec(col);
        }
    }
    return cov;
}
MatrixXd calculate_omega_weight_matrix(const MatrixXd &sample, const VectorXd &mu, int nMom, int n){
    MatrixXd inv = MatrixXd::Zero(nMom, nMom);
    VectorXd X = VectorXd::Zero(nMom);
    for(int s = 0; s < n; s++){
        for(int row = 0; row < N_SPECIES; row++){
            X(row) = sample(s, row); 
            for(int col = row; col < N_SPECIES; col++){
                if( row == col){
                    X(N_SPECIES + row) = sample(s, row) * sample(s, col);
                }else{
                    X(2*N_SPECIES + (row + col - 1)) = sample(s,row) * sample(s,col);
                }
            }
        }
        for(int i = 0; i < nMom; i++){
            for(int j = 0; j < nMom; j++){
                inv(i,j) += (X(i) - mu(i)) * (X(j) - mu(j));
            }
        }
    }
    inv /= n;
    return inv.inverse();
}

/* Generation Functions */
/* Creation Functions for testing of ODEs and multinormal distributions! */
MatrixXd generate_covariance_matrix(const MatrixXd& sampleSpace, const VectorXd& mu, int nProt){
    MatrixXd cov = MatrixXd::Zero(nProt, nProt);
    /* Calculate covar matrix labeled sigma */
    for (int i = 0; i < N_SPECIES; i++) {
        for (int j = 0; j < N_SPECIES; j++) {
            for (int a = 0; a < N; a++) {
                cov(i, j) += (sampleSpace(a, i) - mu(i)) * (sampleSpace(a, j) - mu(j));
            }
        }
    }
    cov /= N;
    return cov;
}

/* generated a random sample space of n x nprot matrix that can be used to randomly compute a covariance matrix */
MatrixXd generate_sample_space(int nProt, int n){
     /* Random Number Generator */
    random_device ranDev;
    mt19937 generator(ranDev());
    MatrixXd sampleSpace(n, nProt);
    double mu_x = 1.47, mu_y = 1.74, mu_z = 1.99; // true means for MVN(theta)
	double var_x = 0.77, var_y = 0.99, var_z = 1.11; // true variances for MVN(theta);
	double sigma_x = sqrt(var_x), sigma_y = sqrt(var_y), sigma_z = sqrt(var_z);
    normal_distribution<double> xNorm(mu_x, sigma_x);
    normal_distribution<double> yNorm(mu_y, sigma_y);
    normal_distribution<double> zNorm(mu_z, sigma_z);
    /* Generate NPRotein mu vector and also NProtein Cov Matrix using the three MVN values */
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N_SPECIES; j++) {
            if (i % 3 == 0 ) {
                sampleSpace(i, j) = xNorm(generator);
            }else if (i % 3 == 1) {
                sampleSpace(i, j) = yNorm(generator);
            }else {
                sampleSpace(i,j) = zNorm(generator);
            }
        }
    }
    return sampleSpace;
}

/* Generates initial conditions for ODEs */
State_N gen_multi_lognorm_iSub(void){
    State_N c0;
    VectorXd mu(3);
    mu << 4.78334234137469844730960782, 
    5.52142091946216110500584912965, 
    4.3815581042632114978686130;
    MatrixXd sigma (3, 3);
    sigma << 0.008298802814695093876186221, 0, 0,
    0, 0.0000799968001706564273219830, 0,
    0, 0, 0.000937060821340228802149700;
    Multi_Normal_Random_Variable gen(mu, sigma);
    VectorXd c0Vec = gen();
    int j = 0;
    for(int i = 0; i < N_SPECIES; i++){
        if( i == 0 || i == 1 || i == 4 ){ // Syk, Vav, SHP1
            c0[i] = exp(c0Vec(j));
            j++;
        }else{
            c0[i] = 0;
        }
    }

    return c0;
}

/* Generates initial conditions for ODEs */
State_N gen_multi_norm_iSub(void){
    State_N c0;
    VectorXd mu(3);
    mu << 4.78334234137469844730960782, 
    5.52142091946216110500584912965, 
    4.3815581042632114978686130;
    MatrixXd sigma (3, 3);
    sigma << 0.008298802814695093876186221, 0, 0,
    0, 0.0000799968001706564273219830, 0,
    0, 0, 0.000937060821340228802149700;
    Multi_Normal_Random_Variable gen(mu, sigma);
    VectorXd c0Vec = gen();
    int j = 0;
    for(int i = 0; i < N_SPECIES; i++){
        if( i == 0 || i == 1 || i == 4 ){ // Syk, Vav, SHP1
            c0[i] = c0Vec(j);
            j++;
        }else{
            c0[i] = 0;
        }
    }

    return c0;
}
// finds all non-zero elements in momVec and returns them in vector form
VectorXd gen_sub_mom_vec(const VectorXd &momVec){
    VectorXd sub = VectorXd::Zero(0);
    for(int i = 0; i < momVec.size(); i++){
        if(momVec(i) != 0){
            sub.conservativeResize(sub.size() + 1);
            sub(sub.size() - 1) = momVec(i);
        }
    }
    return sub;
}

VectorXd comp_vel_vec(const VectorXd &posK){
    VectorXd rPoint;
    rPoint = posK;
    std::random_device rand_dev;
	std::mt19937 generator(rand_dev());
    vector<int> rand;
    for (int i = 0; i < N_DIM; i++) {
        rand.push_back(i);
    }
    shuffle(rand.begin(), rand.end(), generator); // shuffle indices as well as possible. 
    int ncomp = rand.at(0);
    VectorXd wcomp(ncomp);
    shuffle(rand.begin(), rand.end(), generator);
    for (int i = 0; i < ncomp; i++) {
        wcomp(i) = rand.at(i);
    }
    for (int smart = 0; smart < ncomp; smart++) {
        int px = wcomp(smart);
        double pos = rPoint(px);
        double alpha = 4 * pos;
        double beta = 4 - alpha;

        std::gamma_distribution<double> aDist(alpha, 1);
        std::gamma_distribution<double> bDist(beta, 1);
        
        double x = aDist(generator);
        double y = bDist(generator);
        
        rPoint(px) = (x/(x+y));
    }
    return rPoint;
}