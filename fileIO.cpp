#include "main.hpp"
#include "fileIO.hpp"
/* @TODO LATER */

/* open files for writing */
void open_files(ofstream& file0, ofstream& file1, ofstream& file2){
    file0.open("Nonlinear6_Cov_Corr_Mats.txt");
    file1.open("Nonlinear6_Cov_Corr_Mats_t2.txt"); 
    file2.open("Nonlinear6_Cov_Corr_Mats_t3.txt");
}

/* close files */
void close_files(ofstream& file0, ofstream& file1, ofstream& file2){
    file0.close();
    file1.close();
    file2.close();
}


void write_particle_data( const VectorXd& k , const VectorXd &initCon, const VectorXd& mom, const VectorXd& mu, double cost){
    ofstream file;
    file.open("First_Particle.txt");
    file <<"k const:" << k.transpose() << endl 
    << "cond:" << initCon.transpose() << endl
    << "mu:" << mu.transpose() << endl
    << "moments:" << mom.transpose() << endl
    << "<c1-u1>:" << mom(0) - mu(1) << endl
    << "cost:" << cost << endl;
    file.close();
}