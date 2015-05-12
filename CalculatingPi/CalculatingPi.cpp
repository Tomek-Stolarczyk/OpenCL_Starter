// CalculatingPi.cpp : Defines the entry point for the console application.
//
// We will be calculating pi using the Gregory-Leibniz method
// which converges much more slowly than Nilakantha series, we want to see
// the benefit of OpenCL clearly

//scratch that, going the other way. theres inherited errors in those values of pi

// Initially, we'll calculate using CPU alone, and then implement openCl
// and compare the times to tend to the same epsilon

#include "stdafx.h"
#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "OpenCLBasic.h"

char * Source =
#include "PracticeKernel.cl"
;

#define SUCCESS 0
#define FAILURE 1

void CPUCalculatePi(double epsilon)
{
	time_t timer, timer2;
	time(&timer);

	double pi = 3.0;
	long long n = 2;
	int step = 0;
	while(fabs(M_PI - pi) > epsilon){
		if(++step % 2 == 1)
			pi += 4.0/(n*(n+1)*(n+2));
		else{
			pi -= 4.0/(n*(n+1)*(n+2));
			step = 0;
		}
		n+=2;
	}
	time(&timer2);

	std::cout << "calculated pi in " << n << " steps in " << difftime(timer2, timer) << " seconds\n";
	printf("%f\n", pi);
}

// Gregory-Leibniz using arrays instead of data structures
void compute(int len)
	//https://helloacm.com/faster-pi-computation/
{
	time_t timer, timer2;
	time(&timer);

	int* x = new int[len];
	memset(x, 0, len*sizeof(int));
	int* z = new int[len];
	memset(z, 0, len*sizeof(int));
    x[1] = z[1] = 2;

    int a = 1;
	int b = 3;
	int d, j, c, run;

    while(1)
	{
		d = 0;
        for(j=len-1; j > 0; j--)
		{
            c = z[j] * a + d;
            z[j] = c % 10;
            d = c / 10;
		}
        d = 0;
        for (j=0; j < len; j++)
		{
            c = z[j] + d * 10; 
            z[j] = c / b;
            d = c % b;
		}
        run = 0;
        for(j=len-1; j > 0; j--)
		{
            c = x[j] + z[j];
            x[j] = c % 10;
            x[j - 1] += c / 10;
            run |= z[j];
		}
        if (!run)
            break;
        a += 1;
        b += 2;
	}
	std::cout << x[1] << ".";
	for (int i=2; i<len; i++)
		std::cout << x[i];
	std::cout << "\n Done!\n";
	
	time(&timer2);
	std::cout << "calculated pi in " << difftime(timer2, timer) << " seconds\n";

    return;
}


int main(int argc, char* argv[])
{
	OpenCLBasic* MyOpenCL = new OpenCLBasic("PracticeKernel_source.cl");
	
	double sig = 10;
	if(argc == 2)
		sig = atoi(argv[1]);
	std::cout << "calculating pi to " << sig << " significant digits\n";
	compute((int)sig);
	MyOpenCL->AllocateBuffers((int)sig);
	MyOpenCL->ExecuteKernel((int)sig);
	return 0;
}

