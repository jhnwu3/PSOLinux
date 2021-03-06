#ifndef _FILEIO_HPP_
#define _FILEIO_HPP_
#include "main.hpp"

/* FILE IO */
void open_files(ofstream& file0, ofstream& file1, ofstream& file2);
void close_files(ofstream& file0, ofstream& file1, ofstream& file2);
void write_particle_data( const VectorXd& k , const VectorXd &initCon, const VectorXd& mom, const VectorXd& mu, double cost);

struct Write_File_CSV 
{
    ostream& fOut;
    Write_File_CSV (ostream& out) : fOut( out ) {} 
    void operator()(const State_N &c, const double t){
        fOut << t << endl;
        for(int i = 0; i < N_SPECIES; i++){
        fOut << "," << c[i];
        }
    }
};

struct Write_File_Plot 
{
    ostream& fOut;
    Write_File_Plot (ostream& out) : fOut( out ) {} 
    void operator()(const State_6 &c, const double t){ // write all solved ODE values in GNU plot vals
        fOut << t;
        for(int i = 0; i < 6; i++){
        fOut << " " << c[i];
        }
        fOut << endl;
    }
}; 

struct Data_Components_IO{
    VectorXd subset;
    VectorXd moments;
    MatrixXd secondMoments;
    double tf;
    ofstream out;
};
struct Data_ODE_Observer_IO 
{
    struct Data_Components_IO &dComp;
    Data_ODE_Observer_IO( struct Data_Components_IO &dCom) : dComp( dCom ) {}
    void operator()( State_N const& c, const double t ) const 
    {   
        dComp.out << t;
        for(int i = 0; i < N_SPECIES; i++){
            dComp.out << " " << c[i];
        }
        dComp.out << endl;

        if(t == dComp.tf){
            for(int row = 0; row < dComp.subset.size(); row++){ // first moments of specified subset
                int i = dComp.subset(row) - 1; // i.e subset = {1,2,3} = index = {0,1,2}
                if(i >= 0){ dComp.moments(i) +=  c[i]; }
                for(int col = row; col < dComp.subset.size(); col++){
                    int j = dComp.subset(col) - 1;
                    if (j >= 0){
                        if( i == j ){
                            dComp.moments(N_SPECIES + i) += c[i] * c[j];
                        }else{
                            dComp.moments(2*N_SPECIES + (i + j - 1)) += c[i] *c[j];
                        }
                        dComp.secondMoments(i,j) += (c[i] * c[j]);   // store in a 2nd moment matrix
                        dComp.secondMoments(j,i) = dComp.secondMoments(i,j);   // store in a 2nd moment matrix
                    }
                }
            }
        }
    }
};

#endif
/* more FILE IO @TODO write classes for ease of file IO */
/* Example Streaming Observer Format 
struct streaming_observer
{
    std::ostream& m_out;

    streaming_observer( std::ostream &out ) : m_out( out ) { }

    template< class State >
    void operator()( const State &x , double t ) const
    {
        container_type &q = x.first;
        m_out << t;
        for( size_t i=0 ; i<q.size() ; ++i ) m_out << "\t" << q[i];
        m_out << "\n";
    }
}; */