// PSO.cpp : Replacing Dr. Stewarts linear 3 ODE's with the nonlinear3 ODE system provided way earlier
//

#include <iostream>
#include <fstream>
#include <boost/math/distributions.hpp>
#include <boost/array.hpp>
#include <boost/numeric/odeint.hpp>
#include <random>
#include <vector>
#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>
#include <cmath>
#include <chrono>

#define N_SPECIES 6
#define N 500 // # of samples to sample over
#define N_DIM 6 // dim of PSO hypercube
#define N_PARTICLES 20 


using Eigen::MatrixXd;
using Eigen::VectorXd;
using namespace std;
using namespace boost;
using namespace boost::math;
using namespace boost::numeric::odeint;

/* typedefs for boost ODE-ints */
typedef boost::array< double , N_SPECIES > State_N;
typedef runge_kutta_cash_karp54< State_N > Error_RK_Stepper_N;
typedef controlled_runge_kutta< Error_RK_Stepper_N > Controlled_RK_Stepper_N;

const double ke = 0.0001, kme = 20, kf = 0.01, kmf = 18, kd = 0.03, kmd = 1, 
ka2 = 0.01, ka3 = 0.01, C1T = 20, C2T = 5, C3T = 4;

struct Multi_Normal_Random_Variable
{
    Multi_Normal_Random_Variable(Eigen::MatrixXd const& covar)
        : Multi_Normal_Random_Variable(Eigen::VectorXd::Zero(covar.rows()), covar)
    {}

    Multi_Normal_Random_Variable(Eigen::VectorXd const& mean, Eigen::MatrixXd const& covar)
        : mean(mean)
    {
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigenSolver(covar);
        transform = eigenSolver.eigenvectors() * eigenSolver.eigenvalues().cwiseSqrt().asDiagonal();
    }

    Eigen::VectorXd mean;
    Eigen::MatrixXd transform;

    Eigen::VectorXd operator()() const
    {
        static std::mt19937 gen{ std::random_device{}() };
        static std::normal_distribution<> dist;

        return mean + transform * Eigen::VectorXd{ mean.size() }.unaryExpr([&](auto x) { return dist(gen); });
    }
};

struct K
{
    VectorXd k;
};

/* /* 3-var linear ODE system - need to rename! @TODO */
class Linear_ODE3
{
    struct K bill;

public:
    Linear_ODE3(struct K G) : bill(G) {}

    void operator() (  const State_N &c , State_N &dcdt , double t)
    {
        MatrixXd kr(3, 3); 
        kr << 0, bill.k(1), bill.k(3),
            bill.k(2), 0, bill.k(0),
            0, bill.k(4), 0;
        dcdt[0] = (kr(0,0) * c[0] - kr(0,0) * c[0]) +
              (kr(0,1) * c[1] - kr(1,0) * c[0]) + 
              (kr(0,2) * c[2] - kr(2,0) * c[0]);

        dcdt[1] = (kr(1,0) * c[0] - kr(0,1) * c[1]) +
                (kr(1,1) * c[1] - kr(1,1) * c[1]) + 
                (kr(1,2) * c[2] - kr(2,1) * c[1]);

        dcdt[2] = (kr(2,0) * c[0] - kr(0,2) * c[2]) + 
                (kr(2,1) * c[1] - kr(1,2) * c[2]) + 
                (kr(2,2) * c[2] - kr(2,2) * c[2]);
    }
};


class Nonlinear_ODE6
{
    struct K jay;

public:
    Nonlinear_ODE6(struct K G) : jay(G) {}

    void operator() (  const State_N &c , State_N &dcdt , double t)
    {   
        dcdt[0] = - (jay.k(0) * c[0] * c[1])  // Syk
                  + jay.k(1) * c[2] 
                  + jay.k(2) * c[2];
           
        dcdt[1] = - (jay.k(0) * c[0] * c[1]) // Vav
                + jay.k(1) * c[2] 
                + jay.k(5) * c[5];

        dcdt[2] = jay.k(0) * c[0] * c[1] // Syk-Vav
                - jay.k(1) * c[2]
                - jay.k(2) * c[2];

        dcdt[3] = jay.k(2) * c[2] //pVav
                - jay.k(3) * c[3] * c[4] 
                + jay.k(4) * c[5];

        dcdt[4] = -(jay.k(3) * c[3] * c[4]) // SHP1 
                + jay.k(4) * c[5] 
                + jay.k(5) * c[5];

        dcdt[5] = jay.k(3) * c[3] * c[4]  // SHP1-pVav
                - jay.k(4) * c[5] 
                - jay.k(5) * c[5];
    }
};

struct Data_Components{
    int index;
    MatrixXd mat;
    double timeToRecord;
};
struct Protein_Moments{
    VectorXd mVec;
    MatrixXd sec;
    double timeToRecord;
    Protein_Moments(double tf, int mom){
        mVec = VectorXd::Zero(mom);
        sec = MatrixXd::Zero(N_SPECIES, N_SPECIES);
        timeToRecord = tf;
    }
};

struct Mom_ODE_Observer 
{
    struct Protein_Moments &pMome;
    Mom_ODE_Observer( struct Protein_Moments &pMom) : pMome( pMom ) {}
    void operator()( State_N const& c, const double t ) const 
    {
        if(t == pMome.timeToRecord){
            for(int i = 0; i < N_SPECIES; i++){
                pMome.mVec(i) += c[i];
                for(int j = i; j < N_SPECIES; j++){
                    if( i == j ){ // diagonal elements
                        pMome.mVec(N_SPECIES + i) += c[i] * c[j];
                    }else{
                        pMome.mVec(2*N_SPECIES + (i + j - 1)) += c[i] *c[j];
                    }
                    pMome.sec(i,j) += c[i] * c[j];
                    pMome.sec(j,i) = pMome.sec(i,j); 
                }
            }
        }
    }
};
struct Data_ODE_Observer 
{
    struct Data_Components &dComp;
    Data_ODE_Observer( struct Data_Components &dCom) : dComp( dCom ) {}
    void operator()( State_N const& c, const double t ) const 
    {
        if(t == dComp.timeToRecord){
            for(int i = 0; i < dComp.mat.cols(); i++){ dComp.mat(dComp.index, i) = c[i]; }
        }
    }
};
struct Data_Components6{
    int index;
    MatrixXd mat;
	VectorXd sub;
    double timeToRecord;
};
struct Data_ODE_Observer6 
{
    struct Data_Components6 &dComp;
    Data_ODE_Observer6( struct Data_Components6 &dCom) : dComp( dCom ) {}
    void operator()( State_N const& c, const double t ) const 
    {
        if(t == dComp.timeToRecord){
			int i = 0, j = 0;
			while( i < N_SPECIES && j < dComp.sub.size()){
				if(i == dComp.sub(j)){
					dComp.mat(dComp.index,j) = c[i];
					j++;
				}
				i++;
			}
        }
    }
};
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

VectorXd gen_multi_lognorm_vecSub(void){
    VectorXd initVec(N_SPECIES);
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
            initVec(i) = exp(c0Vec(j));
            j++;
        }else{
            initVec(i) = 0;
        }
    }
    return initVec;
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
double calculate_cf1(const VectorXd& trueVec, const VectorXd& estVec, int n){
    double cost = 0;
    VectorXd diff(n);
    diff = trueVec - estVec;
    cost = diff.transpose() * diff.transpose().transpose();
    // for(int i = 0; i < n; i++){
    //     cost += (estVec(i) - trueVec(i)) * (estVec(i) - trueVec(i));
    // }
    return cost;
}
double calculate_cf2(const VectorXd& trueVec, const  VectorXd& estVec, const MatrixXd& w, int n){
    double cost = 0;
    VectorXd diff(n);
    /*for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
           cost += (estVec(i) - trueVec(i)) * w(i,j) *(estVec(j) - trueVec(j));
        }
    }*/
    diff = trueVec - estVec;
    cost = diff.transpose() * w * (diff.transpose()).transpose();
    return cost;
}

int main() {
	
	auto t1 = std::chrono::high_resolution_clock::now();
    random_device ran;
    mt19937 gen(ran());
    uniform_real_distribution<double> unifDist(0.0, 1.0);
	/*---------------------- Setup ------------------------ */
	int bsi = 1, Nterms = 9, useEqual = 0, Niter = 1, Biter = 1, psoIter = 2; 

	/* Variables (global) */
	double t0 = 0, tf = 5.0 * (9.69), dt = 0.1 * (9.69);
	int wasflipped = 0, Nprots = 3, Npars = 6;
	double squeeze = 0.96, sdbeta = 0.05;

	/* SETUP */
	int useDiag = 0;
	int sf1 = 1;
	int sf2 = 1;
	
	int Nparts_1 = 500;
	int Nparts = Nparts_1;
	int Nsteps_1 = 5;

	int Nparts_2 = 5;
	int Nsteps_2 = 500;
	int Nsteps = Nsteps_2;

    /* moments */
	int nMoments = (N_SPECIES * (N_SPECIES + 3)) / 2;
	MatrixXd Y_t = MatrixXd::Zero(N, N_SPECIES); // Values we are comparing towards - oMoments is derived from this.
	VectorXd pMoments(nMoments);
	MatrixXd X_t = MatrixXd::Zero(N, N_SPECIES);
	
	/* PSO weights */
    double sfp = 3.0, sfg = 1.0, sfe = 6.0; // initial particle historical weight, global weight social, inertial
    double sfi = sfe, sfc = sfp, sfs = sfg; // below are the variables being used to reiterate weights
	double w1 = 6, w2 = 1, w3 = 1, boundary = 0.001; 
	MatrixXd wt = MatrixXd::Identity(nMoments, nMoments); // wt matrix
	MatrixXd GBMAT(0,0);
	MatrixXd PBMAT;
    VectorXd PBVEC = VectorXd::Zero(Npars);
    MatrixXd GBVEC = VectorXd::Zero(Npars);
	VectorXd wmatup(4);
	wmatup << 0.15, 0.3, .45, .6;

    /* Solve for Y_t (mu) */
	struct K tru; 
	tru.k = VectorXd::Zero(Npars);
	tru.k << 5.0, 0.1, 1.0, 8.69, 0.05, 0.70;
	tru.k /= (9.69);
	struct K pos;
    Nonlinear_ODE6 trueSys(tru);
    Protein_Moments Yt(tf, nMoments);
    Mom_ODE_Observer YtObs(Yt);
    Controlled_RK_Stepper_N controlledStepper;
    
    for(int i = 0; i < N; i++){
        State_N c0 = gen_multi_lognorm_iSub(); // Y_0 is simulated using lognorm dist.
        integrate_adaptive(controlledStepper, trueSys, c0, t0, tf, dt, YtObs);
    }
    Yt.mVec /= N;
    Yt.sec /= N;
    
    /* PSO costs */
    double gCost;
    double pCost;
    
    /* Instantiate respective pos.k */
    for(int i = 0; i < Npars; i++){ pos.k(i) = unifDist(gen);}
    Protein_Moments Xt(tf, nMoments);
    Mom_ODE_Observer XtObs(Xt);
    Nonlinear_ODE6 sys(pos);
    for(int i = 0; i < N; i++){
        State_N c0 = gen_multi_lognorm_iSub();
        integrate_adaptive(controlledStepper, sys, c0, t0, tf, dt, XtObs);
    }
    Xt.mVec/=N;
    Xt.sec/=N;
    double costSeedk = calculate_cf2(Yt.mVec, Xt.mVec, wt, nMoments);
    gCost = costSeedk;
    pCost = costSeedk;

    /* PSO Starts here!*/
    for(int particle = 0; particle < Nparts; particle++){
        /* instantiate PBMAT for specific particle */
        for(int i = 0; i < Npars; i++){
            pos.k(i) = unifDist(gen);
        }
        Xt.mVec = VectorXd::Zero(nMoments);
        Xt.sec = MatrixXd(nMoments, nMoments);
        Mom_ODE_Observer XtObsInit(Xt);
        Nonlinear_ODE6 initSys(pos);
        for(int i = 0; i < N; i++){
            State_N c0 = gen_multi_lognorm_iSub();
            integrate_adaptive(controlledStepper, initSys, c0, t0, tf, dt, XtObsInit);
        }
        Xt.mVec /=N;
        Xt.sec /=N;
        PBMAT.conservativeResize(1, Npars + 1);
        for(int i = 0; i < Npars; i++){
            PBMAT(0, i) = pos.k(i);
        }
        PBVEC = pos.k;
        pCost = calculate_cf2(Yt.mVec, Xt.mVec, wt, nMoments); // initial particle best

		for(int step = 0; step < Nsteps; step++){
            w1 = sfi* unifDist(gen) / sf2; w2 = sfc * unifDist(gen) /sf2; w3 = sfs * unifDist(gen)/sf2; 
            double sumw = w1 + w2 + w3; //w1 = inertial, w2 = pbest, w3 = gbest
            w1 = w1 / sumw; w2 = w2/ sumw; w3 = w3/sumw;

            VectorXd rpoint = comp_vel_vec(pos.k);
            pos.k = w1 *rpoint + w2*PBVEC + w3 * GBVEC; // GBVEC = gbest in stewart's PSO

            Xt.mVec = VectorXd::Zero(nMoments);
            Xt.sec = MatrixXd(nMoments, nMoments);
            Mom_ODE_Observer XtObsPSO(Xt);
            Nonlinear_ODE6 psoSys(pos); 
            for(int i = 0; i < N; i++){
                State_N c0 = gen_multi_lognorm_iSub();
                integrate_adaptive(controlledStepper, psoSys, c0, t0, tf, dt, XtObsPSO);
            }
            Xt.mVec/=N;
            Xt.sec/=N;
            double pCurrCost = calculate_cf2(Yt.mVec, Xt.mVec, wt, nMoments);
            if(pCurrCost < pCost){
                PBVEC = pos.k;
                pCost = pCurrCost;
                if(pCurrCost < gCost){
                    gCost = pCurrCost;
                    GBVEC = pos.k;
                    GBMAT.conservativeResize(GBMAT.rows() + 1, Npars + 1);
                    for(int i = 0; i < Npars; i++){
                        GBMAT(GBMAT.rows() -1 , i) = GBVEC(i);
                    }
                    GBMAT(GBMAT.rows() - 1, Npars) = gCost;
                }
            }
            sfi = sfi - (sfe - sfg)/Nsteps;   // reduce the inertial weight after each step 
            sfs = sfs + (sfe - sfg)/Nsteps;   
		}
	}

    

    cout << GBMAT << endl;
	auto t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();
	cout << "CODE FINISHED RUNNING IN "<< duration<< " s TIME!" << endl;

	

	return 0; // just to close the program at the end.
}







/* generate random individual values */

/*double uniformRandDouble(double range_from, double range_to) {
	std::random_device rand_dev;
	std::mt19937 generator(rand_dev());
	std::uniform_int_distribution<double>    distr(range_from, range_to);
	return distr(generator);
}*/



