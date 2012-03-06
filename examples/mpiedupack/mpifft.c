#include "mpiedupack.h"

/****************** Sequential functions ********************************/
void ufft(double *x, int n, int sign, double *w){

    /* This sequential function computes the unordered discrete Fourier
       transform of a complex vector x of length n, stored in a real array
       of length 2n as pairs (Re x[j], Im x[j]), 0 <= j < n.
       n=2^m, m >= 0.
       If sign = 1, then the forward unordered dft FRx is computed;
       if sign =-1, the backward unordered dft conjg(F)Rx is computed, 
       where F is the n by n Fourier matrix and R the n by n bit-reversal
       matrix. The output overwrites x.
       w is a table of n/2 complex weights, stored as pairs of reals,
          exp(-2*pi*i*j/n), 0 <= j < n/2,
       which must have been initialized before calling this function.
    */
    
    int k, nk, r, rk, j, j0, j1, j2, j3;
    double wr, wi, taur, taui;

    for(k=2; k<=n; k *=2){
        nk= n/k;
        for(r=0; r<nk; r++){
            rk= 2*r*k;
            for(j=0; j<k; j +=2){
                wr= w[j*nk];
                if (sign==1) {
                    wi= w[j*nk+1];
                } else {
                    wi= -w[j*nk+1];
                }
                j0= rk+j;
                j1= j0+1;
                j2= j0+k;
                j3= j2+1;
                taur= wr*x[j2] - wi*x[j3];
                taui= wi*x[j2] + wr*x[j3];
                x[j2]= x[j0]-taur;   
                x[j3]= x[j1]-taui;   
                x[j0] += taur;   
                x[j1] += taui;   
            }
        }
    }

} /* end ufft */

void ufft_init(int n, double *w){

    /* This function initializes the n/2 weights to be used
       in a sequential radix-2 FFT of length n.
       n=2^m, m >= 0.
       w is a table of n/2 complex weights, stored as pairs of reals,
          exp(-2*pi*i*j/n), 0 <= j < n/2.
    */

    int j, n4j, n2j;
    double theta;

    if (n==1)
        return;
    theta= -2.0 * M_PI / (double)n;
    w[0]= 1.0;
    w[1]= 0.0;
    if (n==4){ 
        w[2]=  0.0;
        w[3]= -1.0;
    } else if (n>=8) {
        /* weights 1 .. n/8 */
        for(j=1; j<=n/8; j++){
            w[2*j]=   cos(j*theta);
            w[2*j+1]= sin(j*theta);
        }
        /* weights n/8+1 .. n/4 */
        for(j=0; j<n/8; j++){
            n4j= n/4-j;
            w[2*n4j]=   -w[2*j+1];
            w[2*n4j+1]= -w[2*j];
        }
        /* weights n/4+1 .. n/2-1 */
        for(j=1; j<n/4; j++){
            n2j= n/2-j;
            w[2*n2j]=   -w[2*j];
            w[2*n2j+1]=  w[2*j+1];
        }
    }

} /* end ufft_init */

void twiddle(double *x, int n, int sign, double *w){

    /* This sequential function multiplies a complex vector x
       of length n, stored as pairs of reals, componentwise
       by a complex vector w of length n, if sign=1, and
       by conjg(w), if sign=-1. The result overwrites x.
    */

    int j, j1;
    double wr, wi, xr, xi;

    for(j=0; j<2*n; j +=2){
        j1= j+1;
        wr= w[j];
        if (sign==1) {
            wi= w[j1];
        } else {
            wi= -w[j1];
        }
        xr= x[j];
        xi= x[j1];
        x[j]=  wr*xr - wi*xi;
        x[j1]= wi*xr + wr*xi;
    }

} /* end twiddle */

void twiddle_init(int n, double alpha, int *rho, double  *w){

    /* This sequential function initializes the weight table w
       to be used in twiddling with a complex vector of length n, 
       stored as pairs of reals.
       n=2^m, m >= 0.
       alpha is a real shift parameter.
       rho is the bit-reversal permutation of length n,
       which must have been initialized before calling this function.
       The output w is a table of n complex values, stored as pairs of reals,
          exp(-2*pi*i*rho(j)*alpha/n), 0 <= j < n.
    */

    int j;
    double theta;

    theta= -2.0 * M_PI * alpha / (double)n;
    for(j=0; j<n; j++){
        w[2*j]=   cos(rho[j]*theta);
        w[2*j+1]= sin(rho[j]*theta);
    }
    
} /* end twiddle_init */

void permute(double *x, int n, int *sigma){

    /* This in-place sequential function permutes a complex vector x
       of length n >= 1, stored as pairs of reals, by the permutation sigma,
           y[j] = x[sigma[j]], 0 <= j < n.
       The output overwrites the vector x.
       sigma is a permutation of length n that must be decomposable
       into disjoint swaps.
    */

    int j, j0, j1, j2, j3;
    double tmpr, tmpi;

    for(j=0; j<n; j++){
        if (j<sigma[j]){
            /* swap components j and sigma[j] */
            j0= 2*j;
            j1= j0+1;
            j2= 2*sigma[j];
            j3= j2+1;
            tmpr= x[j0];
            tmpi= x[j1];
            x[j0]= x[j2];
            x[j1]= x[j3];
            x[j2]= tmpr;
            x[j3]= tmpi;
        }
    }
           
} /* end permute */

void bitrev_init(int n, int *rho){

    /* This function initializes the bit-reversal permutation rho 
       of length n, with n=2^m, m >= 0.
    */

    int j;
    unsigned int n1, rem, val, k, lastbit, one=1;

    if (n==1){
        rho[0]= 0;
        return;
    }
    n1= n;
    for(j=0; j<n; j++){
        rem= j; /* j= (b(m-1), ... ,b1,b0) in binary */
        val= 0;
        for (k=1; k<n1; k <<= 1){
            lastbit= rem & one; /* lastbit = b(i) with i= log2(k) */
            rem >>= 1;          /* rem = (b(m-1), ... , b(i+1)) */
            val <<= 1;
            val |= lastbit;     /* val = (b0, ... , b(i)) */
        }
        rho[j]= (int)val;
   }

} /* end bitrev_init */

/****************** Parallel functions ********************************/
int k1_init(int n, int p){
    
    /* This function computes the largest butterfly size k1 of the first
       superstep in a parallel FFT of length n on p processors with p < n.
    */
 
    int np, c, k1;

    np= n/p;
    for(c=1; c<p; c *=np)
        ;
    k1= n/c;

    return k1;

} /* end k1_init */

void mpiredistr(double *x, int n, int p, int s, int c0, int c1,
                char rev, int *rho_p){

    /* This function redistributes the complex vector x of length n,
       stored as pairs of reals, from group-cyclic distribution
       over p processors with cycle c0 to cycle c1, where
       c0, c1, p, n are powers of two with 1 <= c0 <= c1 <= p <= n.
       s is the processor number, 0 <= s < p.
       If rev=true, the function assumes the processor numbering
       is bit reversed on input.
       rho_p is the bit-reversal permutation of length p.
    */

    double *tmp;
    int np, j0, j2, j, jglob, ratio, size, npackets, t, offset, r,
        destproc, srcproc,
        *Nsend, *Nrecv, *Offset_send, *Offset_recv;
        
    np= n/p;
    ratio= c1/c0;
    size= MAX(np/ratio,1);
    npackets= np/size;
    tmp= vecallocd(2*np);
    Nsend= vecalloci(p);
    Nrecv= vecalloci(p);
    Offset_send= vecalloci(p);
    Offset_recv= vecalloci(p);
    
    for(t=0; t<p; t++){
        Nsend[t]= Nrecv[t]= 0;
        Offset_send[t]= Offset_recv[t]= 0;
    }
    
    /* Initialize sender info and copy data */
    offset= 0;
    if (rev) {
        j0= rho_p[s]%c0;
        j2= rho_p[s]/c0;
    } else {
        j0= s%c0;
        j2= s/c0;
    }
    for(j=0; j<npackets; j++){
        jglob= j2*c0*np + j*c0 + j0;
        destproc=  (jglob/(c1*np))*c1 + jglob%c1;
        Nsend[destproc]= 2*size;
        Offset_send[destproc]= offset;
        for(r=0; r<size; r++){
            tmp[offset + 2*r]=   x[2*(j+r*ratio)]; 
            tmp[offset + 2*r+1]= x[2*(j+r*ratio)+1]; 
        }
        offset += 2*size;
    }
    
    /* Initialize receiver info */
 
    offset= 0;
    j0= s%c1; /* indices for after the redistribution */
    j2= s/c1;
    for(r=0; r<npackets; r++){
        j= r*size;
        jglob= j2*c1*np + j*c1 + j0;
        srcproc=  (jglob/(c0*np))*c0 + jglob%c0; 
        if (rev) 
            srcproc= rho_p[srcproc];
        Nrecv[srcproc]= 2*size;
        Offset_recv[srcproc]= offset;
        offset += 2*size;
    }
    
    /* Necessary for safety */
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Alltoallv(tmp,Nsend,Offset_send,MPI_DOUBLE,
                  x,  Nrecv,Offset_recv,MPI_DOUBLE,MPI_COMM_WORLD);
    
    vecfreei(Offset_recv);
    vecfreei(Offset_send);
    vecfreei(Nrecv);
    vecfreei(Nsend);
    vecfreed(tmp);

} /* end mpiredistr */

void mpifft(double *x, int n, int p, int s, int sign, double *w0, double *w,
            double *tw, int *rho_np, int *rho_p){

    /* This parallel function computes the discrete Fourier transform
       of a complex array x of length n=2^m, m >= 1, stored in a real array
       of length 2n as pairs (Re x[j], Im x[j]), 0 <= j < n.
       x must have been registered before calling this function.
       p is the number of processors, p=2^q, 0 <= q < m.
       s is the processor number, 0 <= s < p.
       The function uses three weight tables:
           w0 for the unordered fft of length k1,
           w  for the unordered fft of length n/p,
           tw for a number of twiddles, each of length n/p.
       The function uses two bit-reversal permutations:
           rho_np of length n/p,
           rho_p of length p. 
       The weight tables and bit-reversal permutations must have been
       initialized before calling this function.
       If sign = 1, then the dft is computed,
           y[k] = sum j=0 to n-1 exp(-2*pi*i*k*j/n)*x[j], for 0 <= k < n.
       If sign =-1, then the inverse dft is computed,
           y[k] = (1/n) sum j=0 to n-1 exp(+2*pi*i*k*j/n)*x[j], for 0 <= k < n.
       Here, i=sqrt(-1). The output vector y overwrites x.
    */

    char rev;
    int np, k1, r, c0, c, ntw, j; 
    double ninv;

    np= n/p;
    k1= k1_init(n,p);
    permute(x,np,rho_np);
    rev= TRUE;
    for(r=0; r<np/k1; r++)
        ufft(&x[2*r*k1],k1,sign,w0);

    c0= 1;
    ntw= 0;
    for (c=k1; c<=p; c *=np){   
        mpiredistr(x,n,p,s,c0,c,rev,rho_p);
        rev= FALSE;
        twiddle(x,np,sign,&tw[2*ntw*np]);
        ufft(x,np,sign,w);
        c0= c;
        ntw++;
    }

    if (sign==-1){
        ninv= 1 / (double)n;
        for(j=0; j<2*np; j++)
            x[j] *= ninv;
    }

} /* end mpifft */

void mpifft_init(int n, int p, int s, double *w0, double *w, double *tw,
                 int *rho_np, int *rho_p){

    /* This parallel function initializes all the tables used in the FFT. */

    int np, k1, ntw, c;
    double alpha;

    np= n/p;
    bitrev_init(np,rho_np);
    bitrev_init(p,rho_p);

    k1= k1_init(n,p);
    ufft_init(k1,w0);
    ufft_init(np,w);

    ntw= 0;
    for (c=k1; c<=p; c *=np){   
        alpha= (s%c) / (double)(c);
        twiddle_init(np,alpha,rho_np,&tw[2*ntw*np]);
        ntw++;
    }

} /* end mpifft_init */
