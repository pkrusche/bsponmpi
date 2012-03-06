#include "mpiedupack.h"

/*  This program computes the sum of the first n squares, for n>=0,
        sum = 1*1 + 2*2 + ... + n*n
    by computing the inner product of x=(1,2,...,n)^T and itself.
    The output should equal n*(n+1)*(2n+1)/6.
    The distribution of x is cyclic.
*/

int nloc(int p, int s, int n){
    /* Compute number of local components of processor s for vector
       of length n distributed cyclically over p processors. */

    return  (n+p-s-1)/p ; 

} /* end nloc */

double mpiip(int p, int s, int n, double *x, double *y){
    /* Compute inner product of vectors x and y of length n>=0 */

    int nloc(int p, int s, int n);
    double inprod, alpha;
    int i;
  
    inprod= 0.0;
    for (i=0; i<nloc(p,s,n); i++){
        inprod += x[i]*y[i];
    }
    MPI_Allreduce(&inprod,&alpha,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);

    return alpha;

} /* end mpiip */

int main(int argc, char **argv){

    double mpiip(int p, int s, int n, double *x, double *y);
    int nloc(int p, int s, int n);
    double *x, alpha, time0, time1;
    int p, s, n, nl, i, iglob;
    
    /* sequential part */

    /* SPMD part */
    MPI_Init(&argc,&argv);

    MPI_Comm_size(MPI_COMM_WORLD,&p); /* p = number of processors */ 
    MPI_Comm_rank(MPI_COMM_WORLD,&s); /* s = processor number */ 

    if (s==0){
        printf("Please enter n:\n"); fflush(stdout);
        scanf("%d",&n);
        if(n<0)
            MPI_Abort(MPI_COMM_WORLD,-1);
    }

    MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);

    nl= nloc(p,s,n);
    x= vecallocd(nl);
    for (i=0; i<nl; i++){
        iglob= i*p+s;
        x[i]= iglob+1;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    time0=MPI_Wtime();

    alpha= mpiip(p,s,n,x,x);
    MPI_Barrier(MPI_COMM_WORLD); 
    time1=MPI_Wtime();

    printf("Processor %d: sum of squares up to %d*%d is %.lf\n",
            s,n,n,alpha); fflush(stdout);
    if (s==0){
        printf("This took only %.6lf seconds.\n", time1-time0);
        fflush(stdout);
    }

    vecfreed(x);
    MPI_Finalize();

    /* sequential part */
    exit(0);

} /* end main */
