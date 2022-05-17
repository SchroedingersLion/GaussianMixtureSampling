#include <cmath>
#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <string>
#include <sstream>
#include <chrono>
#include <math.h> 
#include <iomanip>
#include <algorithm>
#include<limits>
#include <numeric>

using namespace std;


// define stuff, including the two force functions.

const double PI = 3.141592653589793;

mt19937 twister;

struct params {
double mu1,mu2,p1,p2;
};
struct forces{
double fmu1, fmu2;
};


double likelihood_GM(const params& theta, const double sig1, const double sig2, const double a1, const double a2, const double x);

double U_pot_GM( const double T, const params& theta, const double sig1, const double sig2, const double a1, const double a2, 
					  const vector <double>& Xdata, const double sig0);
vector <double> read_dataset(string datafile);
forces F_naive(const params& theta, const double sig1, const double sig2, const double a1, const double a2, 
					  			  vector <double>& Xdata, const size_t B, const double sig0);
forces F_fisher_yates(const params& theta, const double sig1, const double sig2, const double a1, const double a2, 
					  			  vector <double>& Xdata, const size_t B, vector <int>& idx_arr, const double sig0);



int main(int argc, char *argv[]){


double sig1 = 3;		// some params
double sig2 = 0.5;
double a1 = 0.8;
double a2 = 0.2;
double sig0 = 5;
params theta{1,1,1,1};

// read data set			
string datafile = "GM_data_500.csv";
vector <double> Xdata = read_dataset(datafile);

vector <int> idx_arr(Xdata.size());		// stores list of ints used by F_fisher_yates method
for (int i=0; i<Xdata.size(); ++i){
	idx_arr[i] = i;
}


int N_trials = atoi(argv[1]);			// how often should forces be evaluated?
int B =  atoi(argv[2]);					// how many entries of Xdata are to be used by forces?
int randomseed = atoi(argv[3]);



// seed rng
seed_seq seq{1,20,3200,403,5*randomseed+1,12000,73667,9474+randomseed,19151-randomseed};
vector<std::uint32_t> seeds(1);
seq.generate(seeds.begin(), seeds.end());
twister.seed(seeds.at(0)); 




// measure time for the naive forces

auto t1 = chrono::high_resolution_clock::now();
forces F1;
for (size_t k=0; k<N_trials; ++k){
	
	F1 = F_naive(theta, sig1, sig2, a1, a2, Xdata, B, sig0);

} 
auto t2 = chrono::high_resolution_clock::now();
auto ms_int = chrono::duration_cast<chrono::milliseconds>(t2 - t1);
double T_naive = ms_int.count();



// measure time for Fisher-Yates

auto t3 = chrono::high_resolution_clock::now();
forces F2;
for (size_t k=0; k<N_trials; ++k){
	
	F2 = F_fisher_yates(theta, sig1, sig2, a1, a2, Xdata, B, idx_arr, sig0);

} 

auto t4 = chrono::high_resolution_clock::now();
auto ms_int2 = chrono::duration_cast<chrono::milliseconds>(t4 - t3);
double T_fisher_yates = ms_int2.count();


cout << "N = " << N_trials << endl;
cout << "T_naive = " << T_naive << endl;
cout << "T_fisher_yates = " << T_fisher_yates << endl;


return 0;

}





double likelihood_GM(const params& theta, const double sig1, const double sig2, const double a1, const double a2, const double x){
	double e1 = exp( -1*(x-theta.mu1)*(x-theta.mu1)/(2*sig1*sig1) );
	double e2 = exp( -1*(x-theta.mu2)*(x-theta.mu2)/(2*sig2*sig2) );
	double p = a1/(sqrt(2*PI)*sig1) * e1 + a2/(sqrt(2*PI)*sig2) * e2;	
	return p;
}



forces F_naive (const params& theta, const double sig1, const double sig2, const double a1, const double a2, 
					  			  vector <double>& Xdata, const size_t B, const double sig0){
	forces F{0,0};
	double P, e1, e2, x, scale;
	scale = Xdata.size()/double(B);

	if(B<Xdata.size()) shuffle( Xdata.begin(), Xdata.end(), twister );   // for B==Xdata.size(), we use all elements anyway, 
																								// so no point in shuffling

	// these computations happen in the other force method as well	
	for(int i=0; i<B; ++i){
		
			x = Xdata[i];
			P = likelihood_GM(theta, sig1, sig2, a1, a2, x);		
			e1 = exp( -1*(x-theta.mu1)*(x-theta.mu1)/(2*sig1*sig1) );
			e2 = exp( -1*(x-theta.mu2)*(x-theta.mu2)/(2*sig2*sig2) );
			F.fmu1 += 1/P * e1 * (x-theta.mu1);
			F.fmu2 += 1/P * e2 * (x-theta.mu2);
	}


	F.fmu1 *= a1/(sqrt(2*PI)*sig1*sig1*sig1) * scale;
	F.fmu2 *= a2/(sqrt(2*PI)*sig2*sig2*sig2) * scale;
	
	F.fmu1 -= theta.mu1/(sig0*sig0);   // prior part
	F.fmu2 -= theta.mu2/(sig0*sig0);


	return F;
}


forces F_fisher_yates(const params& theta, const double sig1, const double sig2, const double a1, const double a2, 
					  			  vector <double>& Xdata, const size_t B, vector <int>& idx_arr, const double sig0){
	forces F{0,0};
	double P, e1, e2, x, scale;
	scale = Xdata.size()/double(B);
	int help_int, idx;
	int size_minus_B = Xdata.size()-B;


	// choose random indices that are to be used in force calculation

	if(Xdata.size() != B){
		for(int i=Xdata.size()-1; i>=size_minus_B; --i){ // start at last index

			uniform_int_distribution<> distrib(0, i);  // sample int lower than index
			idx = distrib(twister);
		
			help_int = idx_arr[i];			// swap the int at current index with the sampled one
			idx_arr[i] = idx_arr[idx];
			idx_arr[idx] = help_int; 
		
		}
	}
	// last B elements in idx_arr are now the random indices we evaluate Xdata on

	// same computations as in other force function (only indices chosen differently)
	for(int i=idx_arr.size()-1; i>=size_minus_B; --i){		
	
			x = Xdata[ idx_arr[i] ];
			P = likelihood_GM(theta, sig1, sig2, a1, a2, x);		
			e1 = exp( -1*(x-theta.mu1)*(x-theta.mu1)/(2*sig1*sig1) );
			e2 = exp( -1*(x-theta.mu2)*(x-theta.mu2)/(2*sig2*sig2) );
			F.fmu1 += 1/P * e1 * (x-theta.mu1);
			F.fmu2 += 1/P * e2 * (x-theta.mu2);
				
	}


	F.fmu1 *= a1/(sqrt(2*PI)*sig1*sig1*sig1) * scale;
	F.fmu2 *= a2/(sqrt(2*PI)*sig2*sig2*sig2) * scale;
	
	F.fmu1 -= theta.mu1/(sig0*sig0);   
	F.fmu2 -= theta.mu2/(sig0*sig0);

	return F;

}


// ignore
vector <double> read_dataset(string datafile){
	ifstream datasource(datafile);
	vector <double> Xdata;
	string row;
	while (getline(datasource, row)){
		Xdata.push_back(stod(row));
	}
return Xdata;
}