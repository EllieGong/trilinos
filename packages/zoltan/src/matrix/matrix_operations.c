/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2009 Sandia National Laboratories.                          *
 * For more info, see the README file in the top-level Zoltan directory.     *
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/

#ifdef __cplusplus
/* if C++, define the rest of this header file as extern C */
extern "C" {
#endif

#include <math.h>
#include "zz_const.h"
#include "zz_util_const.h"
#include "zoltan_dd.h"
#include "phg.h"
#include "matrix.h"

void intSort2asc3(int *, int);

/************************************/
/* Auxiliary functions declarations */
/************************************/

/* Functions used when we merge duplicate arcs */

/* Definition of the merge wgt operator : arguments are accumulation wgt, partial wgt, wgt dim */
/* Return !0 when an error is found */
typedef int(*WgtFctPtr)(float*, float*, int);

/* This function compare if the wgt are the same for both arcs*/
static int
wgtFctCmp(float* current, float* new, int dim);

/* This function adds the weights */
static int
wgtFctAdd(float* current, float* new, int dim);

/* This function chooses the maximum weight */
static int
wgtFctMax(float* current, float* new, int dim);

/****************************************/
/* Function definitions are here        */
/****************************************/

/* This function compare if the wgt are the same for both arcs*/
static int
wgtFctCmp(float* current, float* new, int dim)
{
  int i; int diff;

  for (i = 0, diff=0 ; (i <dim)&&(!diff) ; ++i) {
    diff |= (new[i] != current[i]);
  }
  return (diff);
}

/* This function adds the weights */
static int
wgtFctAdd(float* current, float* new, int dim)
{
  int i;
  for (i = 0 ; i <dim ; ++i) {
    current[i] += new[i];
  }
  return (0);
}

/* This function chooses the maximum weight */
static int
wgtFctMax(float* current, float* new, int dim)
{
  int i;
  for (i = 0 ; i <dim ; ++i) {
    current[i] = MAX(current[i],new[i]);
  }
  return (0);
}


/* Function that removes locale duplicated nnz */
/* TODO: Add an option to deal with disconnected vertices */
int
Zoltan_Matrix_Remove_DupArcs(ZZ *zz, int size, Zoltan_Arc *arcs, float* pinwgt,
			     Zoltan_matrix *outmat)
{
  static char *yo = "Zoltan_Matrix_Remove_DupArcs";
  int ierr = ZOLTAN_OK;
  WgtFctPtr wgtfct;
  int i;
  ZOLTAN_MAP *nnz_map=NULL, *y_map = NULL;
  int *ysize = NULL;
  int *ptrGNO;
  int *tabGNO;
  int *perm;
  int *iperm;
  void *indexptr ;        /* for typecasting integers, not a "true" pointer */
  void *prev_indexptr ;   /* for typecasting integers, not a "true" pointer */
#ifdef CC_TIMERS
  double time;
#endif
  ZOLTAN_TRACE_ENTER(zz, yo);

#ifdef CC_TIMERS
  time = MPI_Wtime();
#endif

  switch (outmat->opts.pinwgtop) {
  case MAX_WEIGHT:
    wgtfct = &wgtFctMax;
    break;
  case CMP_WEIGHT:
    wgtfct = &wgtFctCmp;
    break;
  case ADD_WEIGHT:
  default:
    wgtfct = &wgtFctAdd;
  }

  ZOLTAN_FREE(&outmat->yGNO);
  ZOLTAN_FREE(&outmat->pinwgt);
  ZOLTAN_FREE(&outmat->pinGNO);
  if (outmat->yend != NULL && (outmat->yend != outmat->ystart +1))
    ZOLTAN_FREE(&outmat->yend);
  ZOLTAN_FREE(&outmat->ystart);

  nnz_map = Zoltan_Map_Create(zz, 0, 2, 0, size);
  if (nnz_map == NULL) MEMORY_ERROR;
  y_map = Zoltan_Map_Create(zz, 0, 1, 0, size);
  if (y_map == NULL) MEMORY_ERROR;
  ysize = (int*) ZOLTAN_CALLOC(size, sizeof(int));
  if (size > 0 && ysize == NULL) MEMORY_ERROR;

  /* First remove duplicated nnz and extract local yGNO*/
  for(i = 0; i < size ; ++i) {
    int position;
    if (arcs[i].GNO[1] >= 0) {/* real arc */
      int prev = -1;
      /* Store the ints as void pointers in Map */
      indexptr = (void *) i ;
      Zoltan_Map_Find_Add(zz, nnz_map, arcs[i].GNO, indexptr, &prev_indexptr);
      prev = (int) prev_indexptr ;
      if (prev != i) {/* Duplicate arcs */
	wgtfct(pinwgt+i*outmat->pinwgtdim, pinwgt+prev*outmat->pinwgtdim,
	       outmat->pinwgtdim);
	continue;
      }
    }
    position = Zoltan_Map_Size(zz, y_map);
    indexptr = (void *) position ;
    Zoltan_Map_Find_Add(zz, y_map, &arcs[i].GNO[0], indexptr, &prev_indexptr);
    position = (int) prev_indexptr ;
    if (arcs[i].GNO[1] >= 0)
      ysize[position] += 1; /* One arc for yGNO */
  }

  /* Now order yGNO */
  outmat->nY = Zoltan_Map_Size(zz, y_map);
  outmat->yGNO = (int*) ZOLTAN_MALLOC(outmat->nY*sizeof(int));
  if (outmat->nY > 0 && outmat->yGNO == NULL) MEMORY_ERROR;
  iperm = (int*) ZOLTAN_MALLOC(outmat->nY*sizeof(int));
  if (outmat->nY > 0 && iperm == NULL) MEMORY_ERROR;
  for (i = 0 ; i < outmat->nY ; ++i)
    iperm[i] = i;

  Zoltan_Map_First(zz, y_map, &ptrGNO, &indexptr);
  i = (int) indexptr ;
  while (ptrGNO != NULL) {
    outmat->yGNO[i] = ptrGNO[0];
    Zoltan_Map_Next(zz, y_map, &ptrGNO, &indexptr);
    i = (int) indexptr ;
  }
  Zoltan_Comm_Sort_Ints(outmat->yGNO, iperm, outmat->nY);

  perm = (int*) ZOLTAN_MALLOC(outmat->nY*sizeof(int));
  if (outmat->nY > 0 && perm == NULL) MEMORY_ERROR;
  for (i = 0 ; i < outmat->nY ; ++i)
    perm[iperm[i]]= i;
  ZOLTAN_FREE(&iperm);

  /* Build indirection table */
  outmat->ystart = (int*) ZOLTAN_MALLOC((outmat->nY+1)*sizeof(int));
  if (outmat->ystart == NULL) MEMORY_ERROR;
  outmat->yend = outmat->ystart+1;

  Zoltan_Map_First(zz, y_map, &ptrGNO, &indexptr);
  i = (int) indexptr ;
  while (ptrGNO != NULL) {
    outmat->ystart[perm[i]+1] = ysize[i]; /* Trick +1 to allow easy build of indirection */
    Zoltan_Map_Next(zz, y_map, &ptrGNO, &indexptr);
    i = (int) indexptr ;
  }
  outmat->ystart[0] = 0;
  for (i = 1 ; i < outmat->nY + 1 ; ++i)
    outmat->ystart[i] += outmat->ystart[i-1];


  outmat->nPins = Zoltan_Map_Size (zz, nnz_map);
  outmat->pinGNO = (int*) ZOLTAN_MALLOC(outmat->nPins*sizeof(int));
  if (outmat->nPins > 0 && outmat->pinGNO == NULL) MEMORY_ERROR;
  outmat->pinwgt = (float*) ZOLTAN_MALLOC(outmat->nPins*outmat->pinwgtdim*sizeof(float));
  if (outmat->nPins > 0 && outmat->pinwgtdim >0 && outmat->pinwgt == NULL) MEMORY_ERROR;

  /* Now put the nnz at the correct place */
  memset(ysize, 0, outmat->nY*sizeof(int));
  Zoltan_Map_First(zz, nnz_map, &tabGNO, &indexptr);
  i = (int) indexptr ;
  while (tabGNO != NULL) {
    int nnz_index;
    int y_index;

    Zoltan_Map_Find(zz, y_map, &tabGNO[0], &indexptr);
    y_index = (int) indexptr ;
    y_index = perm[y_index];

    nnz_index = outmat->ystart[y_index] + ysize[y_index];
    ysize[y_index] ++;
    outmat->pinGNO[nnz_index] = tabGNO[1];
    if (outmat->pinwgtdim > 0 )
      memcpy(outmat->pinwgt+nnz_index*outmat->pinwgtdim,
	     pinwgt+i*outmat->pinwgtdim, /* No *sizeof(float) as it is float* pointer */
	     outmat->pinwgtdim*sizeof(float));
    Zoltan_Map_Next(zz, nnz_map, &tabGNO, &indexptr);
    i = (int) indexptr ;
  }

#ifdef CC_TIMERS
  fprintf(stderr, "(%d) remove arcs: %g\n", zz->Proc, MPI_Wtime()-time);
#endif

#undef MATRIX_DEBUG

#ifdef MATRIX_DEBUG
 {
  /* Sort edges for each vertex to have the same answers than previously */
   float *tmpwgt = NULL;
   ZOLTAN_FREE(&perm);
   perm = (int*)ZOLTAN_MALLOC(outmat->nPins*sizeof(int));
   tmpwgt = (float*) ZOLTAN_MALLOC(outmat->nPins*outmat->pinwgtdim*sizeof(float));

  for (i = 0 ; i < outmat->nY ; ++i) {
    int j;
    int degree = outmat->yend[i]-outmat->ystart[i];
    for (j=0;j < degree; ++j) perm[j] = j;
    Zoltan_Comm_Sort_Ints(outmat->pinGNO + outmat->ystart[i],
			  perm, degree);
    memcpy(tmpwgt, outmat->pinwgt+outmat->ystart[i], degree*sizeof(float)*outmat->pinwgtdim);
    for (j = 0 ; j < degree ; ++j) {
      memcpy(outmat->pinwgt+(j+outmat->ystart[i])*outmat->pinwgtdim,
	     tmpwgt+perm[j]*outmat->pinwgtdim,
	     outmat->pinwgtdim*sizeof(float));
    }
  }
  ZOLTAN_FREE(&tmpwgt);
  ZOLTAN_FREE(&perm);
 }
#endif /* MATRIX_DEBUG */

 End:
  Zoltan_Map_Destroy(zz, &nnz_map);
  Zoltan_Map_Destroy(zz, &y_map);

  ZOLTAN_FREE(&ysize);
  ZOLTAN_FREE(&perm);

  ZOLTAN_TRACE_EXIT(zz, yo);
  return (ierr);
}

/* Function that removes locale duplicated nnz */
int
Zoltan_Matrix_Remove_Duplicates(ZZ *zz, Zoltan_matrix inmat, Zoltan_matrix *outmat)
{
  static char *yo = "Zoltan_Matrix_Remove_Duplicates";
  int ierr = ZOLTAN_OK;
  Zoltan_Arc *arcs = NULL;
  float *pinwgt = NULL;
  int freeflag = 0;
  int size;
  int i, j, cnt;

  ZOLTAN_TRACE_ENTER(zz, yo);
  if (inmat.opts.symmetrize == 0)  /* No symmetrization, we hope no duplicates ...*/
    goto End;

  size = inmat.nPins + inmat.nY; /* We add fake arcs for non connected vertices */
  arcs = (Zoltan_Arc*) ZOLTAN_MALLOC(size*sizeof(Zoltan_Arc));
  if (inmat.nPins && arcs == NULL) MEMORY_ERROR;

  for (i = 0, cnt=0 ; i < inmat.nY; i++) {
    /* Fake arc in order to be sure to keep this vertex */
    arcs[cnt].GNO[0] = inmat.yGNO[i];
    arcs[cnt].GNO[1] = -1;
    cnt++;
    for (j = inmat.ystart[i]; j < inmat.yend[i]; j++) {
      arcs[cnt].GNO[0] = inmat.yGNO[i];
      arcs[cnt].GNO[1] = inmat.pinGNO[j];
      cnt ++;
    }
  }

  pinwgt = inmat.pinwgt;
  if (pinwgt == outmat->pinwgt) {
    freeflag = 1;
    outmat->pinwgt = (float*) ZOLTAN_MALLOC(inmat.pinwgtdim*inmat.nPins*sizeof(float));
    if (inmat.pinwgtdim && inmat.nPins && outmat->pinwgt == NULL) MEMORY_ERROR;
  }

  ierr = Zoltan_Matrix_Remove_DupArcs(zz, size, arcs, pinwgt,outmat);

  if (freeflag)
    ZOLTAN_FREE(&pinwgt);

 End:
  ZOLTAN_FREE(&arcs);

  ZOLTAN_TRACE_EXIT(zz, yo);
  return (ierr);
}


int
Zoltan_Matrix_Construct_CSR(ZZ *zz, int size, Zoltan_Arc *arcs, float* pinwgt,
			     Zoltan_matrix *outmat, int offset)
{
  static char *yo = "Zoltan_Matrix_Remove_DupArcs";
  int *tmparray=NULL;
  WgtFctPtr wgtfct;
  int ierr = ZOLTAN_OK;
  int i;

  ZOLTAN_TRACE_ENTER(zz, yo);

  switch (outmat->opts.pinwgtop) {
  case MAX_WEIGHT:
    wgtfct = &wgtFctMax;
    break;
  case CMP_WEIGHT:
    wgtfct = &wgtFctCmp;
    break;
  case ADD_WEIGHT:
  default:
    wgtfct = &wgtFctAdd;
  }


  tmparray = (int*)ZOLTAN_CALLOC(outmat->nY, sizeof(int));

  /* Count degree for each vertex */
  for (i = 0 ; i < size ; i++) {
    int lno = arcs[i].GNO[0] - offset;
    if (arcs[i].GNO[1] != -1)
      tmparray[lno] ++;
  }

  outmat->ystart[0] = 0;
  outmat->yend = outmat->ystart + 1;
  for (i = 0 ; i < outmat->nY ; i++) { /* Assume compact mode */
    outmat->yend[i] = outmat->ystart[i] + tmparray[i] ;
  }

  memset(tmparray, 0, sizeof(int)*outmat->nY);
  outmat->nPins = 0;
  for(i = 0 ; i <size; i++) {
    int lno = arcs[i].GNO[0] - offset;
    int index;
    if (arcs[i].GNO[1] == -1)
      continue;
    index = outmat->ystart[lno] + tmparray[lno];
    outmat->pinGNO[index] = arcs[i].GNO[1];
    memcpy(outmat->pinwgt + index*outmat->pinwgtdim, pinwgt + i*outmat->pinwgtdim,
	   outmat->pinwgtdim*sizeof(float));
    tmparray[lno]++;
    outmat->nPins ++;
  }


  outmat->pinGNO = (int *) ZOLTAN_REALLOC(outmat->pinGNO, outmat->nPins * sizeof(int));

  outmat->pinwgt = (float *) ZOLTAN_REALLOC(outmat->pinwgt,
			       outmat->nPins*outmat->pinwgtdim*sizeof(float));

  ZOLTAN_FREE(&tmparray);

  ZOLTAN_TRACE_EXIT(zz, yo);
  return (ierr);

}

/* This function compute the indices of the diagonal terms.
   This function needs that diagonal terms are declared at most
   1 time locally.
 */
int
Zoltan_Matrix_Mark_Diag(ZZ* zz, const Zoltan_matrix* const m,
			int *n_nnz, int **nnz)
{
  static char *yo = "Zoltan_Matrix_Mark_Diag";
  int ierr = ZOLTAN_OK;
  int y;

  ZOLTAN_TRACE_ENTER(zz, yo);

  (*nnz) = (int*)ZOLTAN_MALLOC(m->nY*sizeof(int));
  if (m->nY && (*nnz) == NULL)
    MEMORY_ERROR;

  (*n_nnz) = 0;
  for (y = 0 ; y < m->nY ; ++y) {
    int pin;
    for (pin = m->ystart[y] ; pin < m->yend[y] ; ++pin) {
      if (m->pinGNO[pin] == m->yGNO[y]) {
	(*nnz)[(*n_nnz)] = pin;
	(*n_nnz)++;
      }
    }
  }

  if (*n_nnz == 0) ZOLTAN_FREE(nnz); /* Avoid memory leaks */

 End:
  ZOLTAN_TRACE_EXIT(zz, yo);

  return (ierr);
}


  /* This function removes nnz which are listed as arguments (list of indexes in
   pin* arrays.
   nnz array have to be sorted.
  */
int
Zoltan_Matrix_Delete_nnz(ZZ* zz, Zoltan_matrix* m,
			 const int n_nnz, const int* const nnz)
{
  static char *yo = "Zoltan_Matrix_Delete_nnz";
  int ierr = ZOLTAN_OK;
  int i;
  int y;

  if (n_nnz == 0)
    return (ZOLTAN_OK);

  ZOLTAN_TRACE_ENTER(zz, yo);

  if (m->yend == m->ystart + 1) { /* Cannot do this "efficiently" in compact mode */
    m->yend = (int*)ZOLTAN_MALLOC(m->nY*sizeof(int));
    if (m->nY && m->yend == NULL)
      MEMORY_ERROR;
    memcpy(m->yend, m->ystart+1, m->nY*sizeof(int));
  }

  /* Loop over elements we have to remove */
  for (i = 0, y=0; i < n_nnz ; ) {
    int lenght=0;
    int n_removed = 0;

    while (y < m->nY && !(m->ystart[y] <= nnz[i] && m->yend[y] > nnz[i] )) {
      y++;
    }
    if (y >= m->nY){
      ierr = ZOLTAN_WARN;
      break;
    }

    while (i<n_nnz && nnz[i] < m->yend[y]) {
      if (i+1 < n_nnz) lenght = MIN(nnz[i+1], m->yend[y]);
      else lenght = m->yend[y];

      lenght -= nnz[i]+1; /* We remove at least nnz[i] */
      memmove(m->pinGNO+nnz[i], m->pinGNO+nnz[i]+1, lenght*sizeof(int));
      memmove(m->pinwgt+nnz[i]*m->pinwgtdim, m->pinwgt+(nnz[i]+1)*m->pinwgtdim,
	     lenght*sizeof(float)*m->pinwgtdim);
      n_removed ++;
      i++;
    }
    m->yend[y] -= n_removed;
  }
  m->nPins -= n_nnz;

 End:
  ZOLTAN_TRACE_EXIT(zz, yo);

  return (ierr);
}




/* Performs a permutation of the matrix, perm_y A perm_y^t.
 * TODO: at this time we only do symmetric permutations (don't know xGNO !).
 */
int
Zoltan_Matrix_Permute(ZZ* zz, Zoltan_matrix *m, int* perm_y)
{
  static char *yo = "Zoltan_Matrix_Permute";
  int ierr = ZOLTAN_OK;
  int *pinGNO = NULL;
  ZOLTAN_ID_PTR yGID=NULL;
  struct Zoltan_DD_Struct *dd;
  int *ypid;
  int *ybipart;

  ZOLTAN_TRACE_ENTER(zz, yo);

  /* First apply y permutation */
  if (m->completed) { /* We directly know the good arrays */
    yGID = m->yGID;
    ypid = m->ypid;
    ybipart = m->ybipart;

    if (m->ddY == NULL || m->ddY != m->ddX) { /* We have to create again the DD */
      /* We have to define ddY : yGNO, yGID, ywgt */
      ierr = Zoltan_DD_Create (&m->ddY, zz->Communicator, 1, zz->Num_GID,
			       1, m->globalY/zz->Num_Proc, 0);
      /* Hope a linear assignment will help a little */
      Zoltan_DD_Set_Neighbor_Hash_Fn1(m->ddY, m->globalY/zz->Num_Proc);
    }
    m->yGID = NULL;
    m->ypid = NULL;
    m->ybipart = NULL;
  }
  else { /* We have to get these fields */
    /* Update data directories */
    yGID = ZOLTAN_MALLOC_GID_ARRAY(zz, m->nY);
    ypid = (int*) ZOLTAN_MALLOC(m->nY*sizeof(int));
    if (m->bipartite)
      ybipart = (int*) ZOLTAN_MALLOC(m->nY*sizeof(int));
    else
      ybipart = NULL;
    /* Get Informations about Y */
    Zoltan_DD_Find (m->ddY, (ZOLTAN_ID_PTR)m->yGNO, yGID, (ZOLTAN_ID_PTR)ypid, ybipart,
		    m->nY, NULL);
  }

  /* Get Informations about Y */
  Zoltan_DD_Update (m->ddY, (ZOLTAN_ID_PTR)perm_y, yGID, (ZOLTAN_ID_PTR)ybipart, ybipart,
		    m->nY);
  ZOLTAN_FREE (&yGID);
  ZOLTAN_FREE (&ypid);
  ZOLTAN_FREE (&ybipart);

  /* We have to define dd : old_yGNO, new_yGNO */
  ierr = Zoltan_DD_Create (&dd, zz->Communicator, 1, 1, 0, m->globalY/zz->Num_Proc, 0);
  /* Hope a linear assignment will help a little */
  Zoltan_DD_Set_Neighbor_Hash_Fn1(dd, m->globalY/zz->Num_Proc);

  Zoltan_DD_Update (dd, (ZOLTAN_ID_PTR)m->yGNO, (ZOLTAN_ID_PTR)perm_y, NULL, NULL, m->nY);
  memcpy (m->yGNO, perm_y, m->nY*sizeof(int));

  pinGNO = (int*)ZOLTAN_MALLOC(m->nPins*sizeof(int));
  if (m->nPins && pinGNO == NULL)
    MEMORY_ERROR;

  Zoltan_DD_Find (dd, (ZOLTAN_ID_PTR)m->pinGNO, (ZOLTAN_ID_PTR)pinGNO, NULL, NULL,
		  m->nPins, NULL);

  Zoltan_DD_Destroy(&dd);

  ZOLTAN_FREE(&m->pinGNO);
  m->pinGNO = pinGNO;
  pinGNO = NULL;

 End:
  ZOLTAN_FREE (&pinGNO);
  ZOLTAN_FREE (&yGID);
  ZOLTAN_FREE (&ypid);
  ZOLTAN_FREE (&ybipart);

  ZOLTAN_TRACE_EXIT(zz, yo);
  return (ierr);
}


#ifdef __cplusplus
}
#endif
