#include "mpiedupack.h"

/*  This is a test program which uses mpifft to perform 
    a Fast Fourier Transform and its inverse.

    The input vector is defined by x[j]=j+i, for 0 <= j < n.
    Here i= sqrt(-1).
 
    The output vector should equal the input vector,
    up to roundoff errors. Output is by triples (j, Re x[j], Im x[j]).
    Warning: don't rely on this test alone to check correctness. 
    (After all, deleting the main loop will give similar results ;) 

*/

#define NITERS 5   /* Perform NITERS forward and backward transforms.
                      A large NITERS helps to obtain accurate timings. */
#define NPRINT 10  /* Print NPRINT values per processor */
#define MEGA 1000000.0

int main(int argc, char **argv){

    void mpifft(double *x, int n, int p, int s, int sign, double *w0,
                double *w, double *tw, int *rho_np, int *rho_p);
    void mpifft_init(int n, int p, int s, double *w0, 
                double *w, double *tw, int *rho_np, int *rho_p);
    int k1_init(int n, int p);

    int p, s, n, np, k1, j, jglob, it, *rho_np, *rho_p;
    double time0, time1, time2, ffttime, nflops,
           max_error, max_error_glob, error_re, error_im, error,
           *x, *w0, *w, *tw;
  
    MPI_Init(&argc,&argv);

    MPI_Comm_size(MPI_COMM_WORLD,&p);
    MPI_Comm_rank(MPI_COMM_WORLD,&s);

    if (s==0){
        printf("Please enter length n: \n");
        scanf("%d",&n);
        if(n<2*p)
            MPI_Abort(MPI_COMM_WORLD,-7);
    }

    MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
 
    if (s==0){
        printf("FFT of vector of length %d using %d processors\n",n,p);
        printf("performing %d forward and %d backward transforms\n",
                NITERS,NITERS);
    }

    /* Allocate, register,  and initialize vectors */
    np= n/p;
    x= vecallocd(2*np);
    k1= k1_init(n,p);
    w0= vecallocd(k1);
    w=  vecallocd(np);
    tw= vecallocd(2*np+p);
    rho_np= vecalloci(np);
    rho_p=  vecalloci(p);

    for (j=0; j<np; j++){
        jglob= j*p+s;
        x[2*j]= (double)jglob;
        x[2*j+1]= 1.0;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    time0=MPI_Wtime();
 
    /* Initialize the weight and bit reversal tables */
    for(it=0; it<NITERS; it++)
        mpifft_init(n,p,s,w0,w,tw,rho_np,rho_p);
    MPI_Barrier(MPI_COMM_WORLD);
    time1=MPI_Wtime();
  
    /* Perform the FFTs */
    for(it=0; it<NITERS; it++){
        mpifft(x,n,p,s,1,w0,w,tw,rho_np,rho_p);
        mpifft(x,n,p,s,-1,w0,w,tw,rho_np,rho_p);
    }
    MPI_Barrier(MPI_COMM_WORLD); 
    time2=MPI_Wtime(); 

    /* Compute the accuracy */
    max_error= 0.0;
    for(j=0; j<np; j++){
        jglob= j*p+s;
        error_re= fabs(x[2*j]-(double)jglob);
        error_im= fabs(x[2*j+1]-1.0);
        error= sqrt(error_re*error_re + error_im*error_im);
        if (error>max_error)
            max_error= error;
    }
    MPI_Reduce(&max_error,&max_error_glob,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);

    for(j=0; j<NPRINT && j<np; j++){
        jglob= j*p+s;
        printf("proc=%d j=%d Re= %f Im= %f \n",s,jglob,x[2*j],x[2*j+1]);
    }
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);

    if (s==0){
        printf("Time per initialization = %lf sec \n",
                (time1-time0)/NITERS);
        ffttime= (time2-time1)/(2.0*NITERS);
        printf("Time per FFT = %lf sec \n", ffttime);
        nflops= 5*n*log((double)n)/log(2.0) + 2*n;
        printf("Computing rate in FFT = %lf Mflop/s \n",
                nflops/(MEGA*ffttime));
        printf("Absolute error= %e \n", max_error_glob);
        printf("Relative error= %e \n\n", max_error_glob/n);
    }

    vecfreei(rho_p);
    vecfreei(rho_np);
    vecfreed(tw);
    vecfreed(w);
    vecfreed(w0);
    vecfreed(x);
 
    MPI_Finalize();
    exit(0);
 
} /* end main */
