#include "mpiedupack.h"

#define EPS 1.0e-15

int nloc(int p, int s, int n){
    /* Compute number of local components of processor s for vector
       of length n distributed cyclically over p processors. */

    return  (n+p-s-1)/p ;

} /* end nloc */

void mpilu(int M, int N, int s, int t, int n, int *pi, double **a){
    /* Compute LU decomposition of n by n matrix A with partial pivoting.
       Processors are numbered in two-dimensional fashion.
       Program text for P(s,t) = processor s+t*M,
       with 0 <= s < M and 0 <= t < N.
       A is distributed according to the M by N cyclic distribution.
    */

    int nloc(int p, int s, int n);
    double *uk, *lk;
    int nlr, nlc, k, i, j, r;

    MPI_Comm row_comm_s, col_comm_t;
    MPI_Status status, status1;

    /* Create a new communicator for my processor row and column */
    MPI_Comm_split(MPI_COMM_WORLD,s,t,&row_comm_s);
    MPI_Comm_split(MPI_COMM_WORLD,t,s,&col_comm_t);

    nlr=  nloc(M,s,n); /* number of local rows */
    nlc=  nloc(N,t,n); /* number of local columns */

    uk= vecallocd(nlc);
    lk= vecallocd(nlr);
    
    /* Initialize permutation vector pi */
    if (t==0){
        for(i=0; i<nlr; i++)
            pi[i]= i*M+s; /* global row index */
    }

    for (k=0; k<n; k++){
        int kr, kr1, kc, kc1, imax, smax, tmp;
        double absmax, pivot, atmp;
        struct {
            double val;
            int idx;
        } max, max_glob;

        /****** Superstep 0 ******/
        kr=  nloc(M,s,k); /* first local row with global index >= k */ 
        kr1= nloc(M,s,k+1); 
        kc=  nloc(N,t,k); 
        kc1= nloc(N,t,k+1);
        
        if (k%N==t){   /* k=kc*N+t */
            /* Search for local absolute maximum in column k of A */
            absmax= 0.0; imax= -1;
            for (i=kr; i<nlr; i++){
                if (fabs(a[i][kc])>absmax){
                    absmax= fabs(a[i][kc]);
                    imax= i;
                }
            } 

            /* Determine value and global index of absolute maximum 
               and broadcast them to P(*,t) */
            max.val= absmax;
            if (absmax>0.0){
                max.idx= imax*M+s;
            } else {
                max.idx= n; /* represents infinity */
            }
            MPI_Allreduce(&max,&max_glob,1,MPI_DOUBLE_INT,MPI_MAXLOC,col_comm_t);
      
            /****** Superstep 1 ******/
  
            /* Determine global maximum */
            r= max_glob.idx;
            pivot= 0.0;
            if (max_glob.val > EPS){
                smax= r%M;
                if (s==smax)
                    pivot = a[imax][kc];
                /* Broadcast pivot value to P(*,t) */
                MPI_Bcast(&pivot,1,MPI_DOUBLE,smax,col_comm_t);
        
                for(i=kr; i<nlr; i++)
                    a[i][kc] /= pivot;
                if (s==smax)
                    a[imax][kc]= pivot; /* restore value of pivot */
            } else {
                MPI_Abort(MPI_COMM_WORLD,-6);
            }
        }
        
        /* Broadcast index of pivot row to P(*,*) */
        MPI_Bcast(&r,1,MPI_INT,k%N,row_comm_s);

        /****** Superstep 2 ******/
        if (t==0){
            /* Swap pi(k) and pi(r) */
            if (k%M != r%M){
                if (k%M==s){
                    /* Swap pi(k) and pi(r) */
                    MPI_Send(&pi[k/M],1,MPI_INT,r%M,0,MPI_COMM_WORLD);
                    MPI_Recv(&pi[k/M],1,MPI_INT,r%M,0,MPI_COMM_WORLD,&status);
                }
                if (r%M==s){
                    MPI_Recv(&tmp,1,MPI_INT,k%M,0,MPI_COMM_WORLD,&status);
                    MPI_Send(&pi[r/M],1,MPI_INT,k%M,0,MPI_COMM_WORLD);     
                    pi[r/M]= tmp;
                }
            } else if (k%M==s){
                tmp= pi[k/M];
                pi[k/M]= pi[r/M];
                pi[r/M]= tmp;
            }
        }
        /* Swap rows k and r */
        if (k%M != r%M){
            if (k%M==s){
                MPI_Send(a[k/M],nlc,MPI_DOUBLE,r%M+t*M,1,MPI_COMM_WORLD);
                MPI_Recv(a[k/M],nlc,MPI_DOUBLE,r%M+t*M,1,MPI_COMM_WORLD,&status1);
            }
            if (r%M==s){
                /* abuse uk as a temporary receive buffer */
                MPI_Recv(uk,nlc,MPI_DOUBLE,k%M+t*M,1,MPI_COMM_WORLD,&status1);
                MPI_Send(a[r/M],nlc,MPI_DOUBLE,k%M+t*M,1,MPI_COMM_WORLD);
                for(j=0; j<nlc; j++)
                    a[r/M][j]= uk[j];
            }
        } else if (k%M==s){
            for(j=0; j<nlc; j++){
                atmp= a[k/M][j];
                a[k/M][j]= a[r/M][j];
                a[r/M][j]= atmp;
            }
        }
        
        /****** Superstep 3 ******/
        if (k%N==t){ 
            /* Store new column k in lk */
            for(i=kr1; i<nlr; i++)     
                lk[i-kr1]= a[i][kc];
        }
        if (k%M==s){ 
            /* Store new row k in uk */
            for(j=kc1; j<nlc; j++)
                uk[j-kc1]= a[kr][j];
        }
        MPI_Bcast(lk,nlr-kr1,MPI_DOUBLE,k%N,row_comm_s);
        
        /****** Superstep 4 ******/
        MPI_Bcast(uk,nlc-kc1,MPI_DOUBLE,k%M,col_comm_t);
      
        /****** Superstep 0 ******/
        /* Update of A */
        for(i=kr1; i<nlr; i++){
            for(j=kc1; j<nlc; j++)
                a[i][j] -= lk[i-kr1]*uk[j-kc1];
        }
    }
    vecfreed(lk);
    vecfreed(uk);

} /* end mpilu */
