#include "ODE.hpp"

/* Calculation Functions */
double CF1(const VectorXd& trueVec, const VectorXd& estVec, int n){
    double cost = 0;
    for(int i = 0; i < n; i++){
        cost += (estVec(i) - trueVec(i)) * (estVec(i) - trueVec(i));
    }
    return cost;
}

double CF2(const VectorXd& trueVec, const  VectorXd& estVec, const MatrixXd& w, int n){
    double cost = 0;
    VectorXd diff(n);
    /*for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
           cost += (estVec(i) - trueVec(i)) * w(i,j) *(estVec(j) - trueVec(j));
        }
    }*/
    for(int i = 0; i < n; i++){
        diff(i) = estVec(i) - trueVec(i);
    }
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


/* Creation Functions for testing of ODEs and multinormal distributions! */
MatrixXd create_covariance_matrix(const MatrixXd& sampleSpace, const VectorXd& mu, int nProt){
    MatrixXd cov(nProt, nProt);
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
MatrixXd generate_sample_space(int nProt, int n){
     /* Random Number Generator */
    random_device rand_dev;
    mt19937 generator(rand_dev());
    MatrixXd sampleSpace(n, nProt);
    std::normal_distribution<double> xNorm(mu_x, sigma_x);
    std::normal_distribution<double> yNorm(mu_y, sigma_y);
    std::normal_distribution<double> zNorm(mu_z, sigma_z);
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
