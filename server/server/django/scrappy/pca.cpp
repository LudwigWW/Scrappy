	#include <iostream>
	#include <fstream>
	#include <iomanip>
	#include <cstdio>
	#include <sstream>
	#include <eigen/Eigen/Sparse>
	#include <eigen/Eigen/Eigenvalues>
	#include <vector>
	#include <stdlib.h>
	#include <stdio.h>
	#include <chrono>
	using namespace std;
	using namespace Eigen;
	using namespace std::chrono;
	IGL_INLINE igl::int pca()
	{
		cout << "PCA in C++ by Chi Pui Jeremy, Wong" << endl;
		char filename[50];
		cout << "Please input the filename: ";
		cin.getline(filename, 50); 
		ifstream infile(filename);
		int spec_no;
		int ion_no;
		cout << "Please input the number of spectra: ";
		cin >> spec_no;
		cout << "Please input the number of ions: ";
		cin >> ion_no;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		VectorXd mass(ion_no);
		MatrixXd var1(ion_no,spec_no);
		for(int j=0; j<ion_no;j++){
			for(int i = 0; i < spec_no; i++){
				infile >> var1(j,i);
			}
		}
		infile.close();
		VectorXd x=VectorXd::Zero(ion_no);
		MatrixXd d1(spec_no,ion_no);
		for(int j = 0; j < spec_no; j++){
			for(int i = 0; i < ion_no; i++){
				x(i) = x(i) + var1(i,j)/((double)spec_no);
			}
		}
		for(int i = 0; i < ion_no; i++){
			for(int j=0; j < spec_no; j++){
	  			d1(j,i) = var1(i,j)-x(i);
			}
		}
		MatrixXd m=MatrixXd::Zero(ion_no,ion_no);
		for(int i2 = 0;i2< ion_no; i2++){
			for(int i = 0; i < ion_no; i++){
				for(int j=0; j < spec_no; j++){
	  				m(i2,i) = m(i2,i) + d1(j,i)*d1(j,i2)/((double)spec_no-1);
				}
			}
		}
		EigenSolver<MatrixXd> es(m);
		MatrixXd eig_v(ion_no,6);
		MatrixXd score=MatrixXd::Zero(spec_no,6);
		ofstream myfile;
		myfile.open("eigenvectors.txt");
		for(int k=0;k<ion_no;k++){
			myfile << k << "\t";
		for(int p=0;p<6;p++){
			myfile << es.eigenvectors().col(p)[k].real() << "\t";
			eig_v(k,p)=es.eigenvectors().col(p)[k].real();
		}
			myfile << endl;
		}
		myfile.close();
		for(int p=0;p<6;p++){
		for(int i=0;i<spec_no;i++){
		for(int j=0;j<ion_no;j++){
			score(i,p)=score(i,p)+eig_v(j,p)*d1(i,j);
		}
		}
		}
		myfile.open("scores.txt");
		for(int i=0;i<spec_no;i++){
			myfile << i << "\t";
		for(int p=0;p<6;p++){
			myfile << score(i,p) << "\t";
		}
			myfile << endl;
		}
		myfile.close();
		myfile.open("eigenvalues.txt");
		for(int p=0;p<6;p++){
			myfile << es.eigenvalues()[p].real() << "\t";
		}
		myfile << endl;
		myfile.close();
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>( t2 - t1 ).count();
		cout << "run time= " << duration << " microseconds" << endl;
	  	return 0;
	}