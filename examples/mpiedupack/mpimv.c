#include "mpiedupack.h"

void mpimv(int p, int s, int n, int nz, int nrows, int ncols,
           double *a, int *inc,
           int *srcprocv, int *srcindv, int *destprocu, int *destindu,
           int nv, int nu, double *v, double *u){

    /* This function multiplies a sparse matrix A with a
       dense vector v, giving a dense vector u=Av.
       A is n by n, and u,v are vectors of length n. 
       A, u, and v are distributed arbitrarily on input.
       They are all accessed using local indices, but the local
       matrix indices may differ from the local vector indices.
       The local matrix nonzeros are stored in an incremental
       compressed row storage (ICRS) data structure defined by
       nz, nrows, ncols, a, inc.
       All rows and columns in the local data structure are nonempty.
      
       p is the number of processors.
       s is the processor number, 0 <= s < p.
       n is the global size of the matrix A.
       nz is the number of local nonzeros.
       nrows is the number of local rows.
       ncols is the number of local columns.

       a[k] is the numerical value of the k'th local nonzero of the
            sparse matrix A, 0 <= k < nz.
       inc[k] is the increment in the local column index of the
              k'th local nonzero, compared to the column index of the
              (k-1)th nonzero, if this nonzero is in the same row;
              otherwise, ncols is added to the difference.
              By convention, the column index of the -1'th nonzero is 0.

       srcprocv[j] is the source processor of the component in v
              corresponding to the local column j, 0 <= j < ncols.
       srcindv[j] is the local index on the source processor
              of the component in v corresponding to the local column j.
       destprocu[i] is the destination processor of the partial sum
                  corresponding to the local row i, 0 <= i < nrows.
       destindu[i] is the local index in the vector u on the destination
                   processor corresponding to the local row i.
    
       nv is the number of local components of the input vector v.
       nu is the number of local components of the output vector u.
       v[k] is the k'th local component of v, 0 <= k < nv.
       u[k] is the k'th local component of u, 0 <= k < nu.
    */

    int i, j, *pinc;
    double sum, *psum, *pa, *vloc, *pvloc, *pvloc_end;
    
    MPI_Win v_win, u_win;

    /****** Superstep 0. Initialize and register ******/
    for(i=0; i<nu; i++)
        u[i]= 0.0;
    vloc= vecallocd(ncols);
    MPI_Win_create(v,nv*SZDBL,SZDBL,MPI_INFO_NULL,MPI_COMM_WORLD,&v_win);
    MPI_Win_create(u,nu*SZDBL,SZDBL,MPI_INFO_NULL,MPI_COMM_WORLD,&u_win);

    /****** Superstep 1. Fanout ******/
    MPI_Win_fence(0, v_win);
    for(j=0; j<ncols; j++)
        MPI_Get(&vloc[j],  1,MPI_DOUBLE,srcprocv[j],
                srcindv[j],1,MPI_DOUBLE,v_win);
    MPI_Win_fence(0, v_win);

    /****** Superstep 2. Local matrix-vector multiplication and fanin */
    MPI_Win_fence(0, u_win);
    psum= &sum;
    pa= a;
    pinc= inc;
    pvloc= vloc;
    pvloc_end= pvloc + ncols;

    pvloc += *pinc;
    for(i=0; i<nrows; i++){
        *psum= 0.0;
        while (pvloc<pvloc_end){
            *psum += (*pa) * (*pvloc);
            pa++; 
            pinc++;
            pvloc += *pinc;
        }
        MPI_Accumulate(psum,1,MPI_DOUBLE,destprocu[i],destindu[i],
                            1,MPI_DOUBLE,MPI_SUM,u_win);
        pvloc -= ncols;
    }
    MPI_Win_fence(0, u_win);

    MPI_Win_free(&u_win);
    MPI_Win_free(&v_win);
    vecfreed(vloc);

} /* end mpimv */

int nloc(int p, int s, int n){
    /* Compute number of local components of processor s for vector
       of length n distributed cyclically over p processors. */

    return  (n+p-s-1)/p ;

} /* end nloc */

void mpimv_init(int p, int s, int n, int nrows, int ncols,
                int nv, int nu, int *rowindex, int *colindex,
                int *vindex, int *uindex, int *srcprocv, int *srcindv,
                int *destprocu, int *destindu){

    /* This function initializes the communication data structure
       needed for multiplying a sparse matrix A with a dense vector v,
       giving a dense vector u=Av.

       Input: the arrays rowindex, colindex, vindex, uindex,
       containing the global indices corresponding to the local indices
       of the matrix and the vectors.
       Output: initialized arrays srcprocv, srcindv, destprocu, destindu
       containing the processor number and the local index on the
       remote processor of vector components corresponding to
       local matrix columns and rows. 
      
       p, s, n, nrows, ncols, nv, nu are the same as in mpimv.

       rowindex[i] is the global index of the local row i, 0 <= i < nrows.
       colindex[j] is the global index of the local column j, 0 <= j < ncols.
       vindex[j] is the global index of the local v-component j, 0 <= j < nv.
       uindex[i] is the global index of the local u-component i, 0 <= i < nu.

       srcprocv, srcindv, destprocu, destindu are the same as in mpimv.
    */

    int nloc(int p, int s, int n);
    int np, i, j, iglob, jglob, *tmpprocv, *tmpindv, *tmpprocu, *tmpindu;

    MPI_Win tmpprocv_win, tmpindv_win, tmpprocu_win, tmpindu_win;

    /****** Superstep 0. Allocate and register temporary arrays */
    np= nloc(p,s,n);
    tmpprocv=vecalloci(np);
    tmpindv=vecalloci(np);
    tmpprocu=vecalloci(np);
    tmpindu=vecalloci(np);
    MPI_Win_create(tmpprocv,np*SZINT,SZINT,MPI_INFO_NULL,
                   MPI_COMM_WORLD,&tmpprocv_win);
    MPI_Win_create(tmpindv,np*SZINT,SZINT,MPI_INFO_NULL,
                   MPI_COMM_WORLD,&tmpindv_win);
    MPI_Win_create(tmpprocu,np*SZINT,SZINT,MPI_INFO_NULL,
                   MPI_COMM_WORLD,&tmpprocu_win);
    MPI_Win_create(tmpindu,np*SZINT,SZINT,MPI_INFO_NULL,
                   MPI_COMM_WORLD,&tmpindu_win);

    MPI_Win_fence(0, tmpprocv_win); MPI_Win_fence(0, tmpindv_win);
    MPI_Win_fence(0, tmpprocu_win); MPI_Win_fence(0, tmpindu_win);
 
    /****** Superstep 1. Write into temporary arrays ******/
    for(j=0; j<nv; j++){
        jglob= vindex[j];
        /* Use the cyclic distribution */
        MPI_Put(&s,1,MPI_INT,jglob%p,jglob/p,1,MPI_INT,tmpprocv_win);
        MPI_Put(&j,1,MPI_INT,jglob%p,jglob/p,1,MPI_INT,tmpindv_win);
    }
    for(i=0; i<nu; i++){
        iglob= uindex[i];
        MPI_Put(&s,1,MPI_INT,iglob%p,iglob/p,1,MPI_INT,tmpprocu_win); 
        MPI_Put(&i,1,MPI_INT,iglob%p,iglob/p,1,MPI_INT,tmpindu_win);
    }
    MPI_Win_fence(0, tmpprocv_win); MPI_Win_fence(0, tmpindv_win);
    MPI_Win_fence(0, tmpprocu_win); MPI_Win_fence(0, tmpindu_win);

    /****** Superstep 2. Read from temporary arrays ******/
    for(j=0; j<ncols; j++){
        jglob= colindex[j];
        MPI_Get(&srcprocv[j],1,MPI_INT,jglob%p,jglob/p,1,MPI_INT,tmpprocv_win);
        MPI_Get(&srcindv[j], 1,MPI_INT,jglob%p,jglob/p,1,MPI_INT,tmpindv_win);
    }
    for(i=0; i<nrows; i++){
        iglob= rowindex[i];
        MPI_Get(&destprocu[i],1,MPI_INT,iglob%p,iglob/p,1,MPI_INT,tmpprocu_win); 
        MPI_Get(&destindu[i], 1,MPI_INT,iglob%p,iglob/p,1,MPI_INT,tmpindu_win);
    }
    MPI_Win_fence(0, tmpprocv_win); MPI_Win_fence(0, tmpindv_win);
    MPI_Win_fence(0, tmpprocu_win); MPI_Win_fence(0, tmpindu_win);
 
    /****** Superstep 3. Deregister temporary arrays ******/
    MPI_Win_free(&tmpindu_win); MPI_Win_free(&tmpprocu_win);
    MPI_Win_free(&tmpindv_win); MPI_Win_free(&tmpprocv_win);

    /****** Superstep 4. Free temporary arrays ******/
    vecfreei(tmpindu); vecfreei(tmpprocu);          
    vecfreei(tmpindv); vecfreei(tmpprocv);   

} /* end mpimv_init */
