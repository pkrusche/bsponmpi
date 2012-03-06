#include "mpiedupack.h"

/* These functions can be used to allocate and deallocate vectors and matrices.
   If not enough memory available, one processor halts them all.
*/

double *vecallocd(int n){
    /* This function allocates a vector of doubles of length n */
    double *pd;

    if (n==0){
        pd= NULL;
    } else {
        pd= (double *)malloc(n*SZDBL);
        if (pd==NULL)
            MPI_Abort(MPI_COMM_WORLD,-2);
    }
    return pd;

} /* end vecallocd */

int *vecalloci(int n){
    /* This function allocates a vector of integers of length n */
    int *pi;

    if (n==0){
        pi= NULL; 
    } else { 
        pi= (int *)malloc(n*SZINT);
        if (pi==NULL)
            MPI_Abort(MPI_COMM_WORLD,-3);
    }
    return pi;

} /* end vecalloci */

double **matallocd(int m, int n){
    /* This function allocates an m x n matrix of doubles */
    int i;
    double *pd, **ppd;

    if (m==0){
        ppd= NULL;  
    } else { 
        ppd= (double **)malloc(m*sizeof(double *));
        if (ppd==NULL)
            MPI_Abort(MPI_COMM_WORLD,-4);
        if (n==0){
            for (i=0; i<m; i++)
                ppd[i]= NULL;
        } else {  
            pd= (double *)malloc(m*n*SZDBL); 
            if (pd==NULL)
                MPI_Abort(MPI_COMM_WORLD,-4);
            ppd[0]=pd;
            for (i=1; i<m; i++)
                ppd[i]= ppd[i-1]+n;
        }
    }
    return ppd;

} /* end matallocd */

void vecfreed(double *pd){
    /* This function frees a vector of doubles */

    if (pd!=NULL)
        free(pd);

} /* end vecfreed */

void vecfreei(int *pi){
    /* This function frees a vector of integers */

    if (pi!=NULL)
        free(pi);

} /* end vecfreei */

void matfreed(double **ppd){
    /* This function frees a matrix of doubles */

    if (ppd!=NULL){
        if (ppd[0]!=NULL)
            free(ppd[0]);
        free(ppd);
    }

} /* end matfreed */
