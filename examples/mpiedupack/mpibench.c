#include "mpiedupack.h"
 
/*  This program measures p, r, g, and l of a BSP computer
    using MPI_Alltoallv for communication.
*/

#define NITERS 100     /* number of iterations */
#define MAXN 1024      /* maximum length of DAXPY computation */
#define MAXH 256       /* maximum h in h-relation */
#define MEGA 1000000.0

void leastsquares(int h0, int h1, double *t, double *g, double *l){
    /* This function computes the parameters g and l of the 
       linear function T(h)= g*h+l that best fits
       the data points (h,t[h]) with h0 <= h <= h1. */

    double nh, sumt, sumth, sumh, sumhh, a;
    int h;

    nh= h1-h0+1;
    /* Compute sums:
        sumt  =  sum of t[h] over h0 <= h <= h1
        sumth =         t[h]*h
        sumh  =         h
        sumhh =         h*h     */
    sumt= sumth= 0.0;
    for (h=h0; h<=h1; h++){
        sumt  += t[h];
        sumth += t[h]*h;
    }
    sumh= (h1*h1-h0*h0+h1+h0)/2;  
    sumhh= ( h1*(h1+1)*(2*h1+1) - (h0-1)*h0*(2*h0-1))/6;

    /* Solve      nh*l +  sumh*g =  sumt 
                sumh*l + sumhh*g = sumth */
    if(fabs(nh)>fabs(sumh)){
        a= sumh/nh;
        /* subtract a times first eqn from second eqn */
        *g= (sumth-a*sumt)/(sumhh-a*sumh);
        *l= (sumt-sumh* *g)/nh;
    } else {
        a= nh/sumh;
        /* subtract a times second eqn from first eqn */
        *g= (sumt-a*sumth)/(sumh-a*sumhh);
        *l= (sumth-sumhh* *g)/sumh;
    }

} /* end leastsquares */

int main(int argc, char **argv){
    void leastsquares(int h0, int h1, double *t, double *g, double *l);
    int p, s, s1, iter, i, n, h,
        *Nsend, *Nrecv, *Offset_send, *Offset_recv;
    double alpha, beta, x[MAXN], y[MAXN], z[MAXN], src[MAXH], dest[MAXH],
           time0, time1, time, *Time, mintime, maxtime,
           nflops, r, g0, l0, g, l, t[MAXH+1]; 
  
    /**** Determine p ****/
    MPI_Init(&argc,&argv);

    MPI_Comm_size(MPI_COMM_WORLD,&p); /* p = number of processors */
    MPI_Comm_rank(MPI_COMM_WORLD,&s); /* s = processor number */

    Time= vecallocd(p);

    /**** Determine r ****/
    for (n=1; n <= MAXN; n *= 2){
        /* Initialize scalars and vectors */
        alpha= 1.0/3.0;
        beta= 4.0/9.0;
        for (i=0; i<n; i++)
            z[i]= y[i]= x[i]= (double)i;
        /* Measure time of 2*NITERS DAXPY operations of length n */
        time0= MPI_Wtime();
 
        for (iter=0; iter<NITERS; iter++){
            for (i=0; i<n; i++)
                y[i] += alpha*x[i];
            for (i=0; i<n; i++)
                z[i] -= beta*x[i];
        }
        time1= MPI_Wtime();
        time= time1-time0; 
        MPI_Gather(&time,1,MPI_DOUBLE,Time,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
    
        /* Processor 0 determines minimum, maximum, average computing rate */
        if (s==0){
            mintime= maxtime= Time[0];
            for(s1=1; s1<p; s1++){
                mintime= MIN(mintime,Time[s1]);
                maxtime= MAX(maxtime,Time[s1]);
            }
            if (mintime>0.0){
                /* Compute r = average computing rate in flop/s */
                nflops= 4*NITERS*n;
                r= 0.0;
                for(s1=0; s1<p; s1++)
                    r += nflops/Time[s1];
                r /= p; 
                printf("n= %5d min= %7.3lf max= %7.3lf av= %7.3lf Mflop/s ",
                       n, nflops/(maxtime*MEGA),nflops/(mintime*MEGA), r/MEGA);
                fflush(stdout);
                /* Output for fooling benchmark-detecting compilers */
                printf(" fool=%7.1lf\n",y[n-1]+z[n-1]);
            } else
                printf("minimum time is 0\n"); fflush(stdout);
        }
    }

    /**** Determine g and l ****/
    Nsend= vecalloci(p);
    Nrecv= vecalloci(p);
    Offset_send= vecalloci(p);
    Offset_recv= vecalloci(p);


    for (h=0; h<=MAXH; h++){
        /* Initialize communication pattern */

        for (i=0; i<h; i++)
            src[i]= (double)i;

        if (p==1){
            Nsend[0]= Nrecv[0]= h;
        } else {
            for (s1=0; s1<p; s1++)
                Nsend[s1]= h/(p-1);
            for (i=0; i < h%(p-1); i++)
                Nsend[(s+1+i)%p]++;
            Nsend[s]= 0; /* no communication with yourself */
            for (s1=0; s1<p; s1++)
                Nrecv[s1]= h/(p-1);
            for (i=0; i < h%(p-1); i++)
                Nrecv[(s-1-i+p)%p]++;
            Nrecv[s]= 0;
        }
     
        Offset_send[0]= Offset_recv[0]= 0;
        for(s1=1; s1<p; s1++){
            Offset_send[s1]= Offset_send[s1-1] + Nsend[s1-1];
            Offset_recv[s1]= Offset_recv[s1-1] + Nrecv[s1-1];
        }

        /* Measure time of NITERS h-relations */
        MPI_Barrier(MPI_COMM_WORLD); 
        time0= MPI_Wtime(); 
        for (iter=0; iter<NITERS; iter++){
            MPI_Alltoallv(src, Nsend,Offset_send,MPI_DOUBLE,
                          dest,Nrecv,Offset_recv,MPI_DOUBLE,MPI_COMM_WORLD);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        time1= MPI_Wtime();
        time= time1-time0;
 
        /* Compute time of one h-relation */
        if (s==0){
            t[h]= (time*r)/NITERS;
            printf("Time of %5d-relation= %lf sec= %8.0lf flops\n",
                   h, time/NITERS, t[h]); fflush(stdout);
        }
    }

    if (s==0){
        printf("size of double = %d bytes\n",(int)SZDBL);
        leastsquares(0,p,t,&g0,&l0); 
        printf("Range h=0 to p   : g= %.1lf, l= %.1lf\n",g0,l0);
        leastsquares(p,MAXH,t,&g,&l);
        printf("Range h=p to HMAX: g= %.1lf, l= %.1lf\n",g,l);

        printf("The bottom line for this BSP computer is:\n");
        printf("p= %d, r= %.3lf Mflop/s, g= %.1lf, l= %.1lf\n",
               p,r/MEGA,g,l);
        fflush(stdout);
    }
    vecfreei(Offset_recv);
    vecfreei(Offset_send);
    vecfreei(Nrecv);
    vecfreei(Nsend);
    vecfreed(Time);
 
    MPI_Finalize();

    exit(0);

} /* end main */
