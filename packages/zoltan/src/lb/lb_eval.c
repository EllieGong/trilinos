/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2000,2001,2002, Sandia National Laboratories.               *
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

#include "zz_const.h"
#include "phg.h"
#include "zoltan_eval.h"

#include <search.h>

/************************************************************************/
static void iget_strided_stats(int *v, int stride, int offset, int len,
                             float *min, float *max, float *sum);

static void fget_strided_stats(float *v, int stride, int offset, int len,
                             float *min, float *max, float *sum);

static int get_nbor_parts( ZZ *zz, int nobj, ZOLTAN_ID_PTR global_ids, 
  ZOLTAN_ID_PTR local_ids, int *part, int nnbors, ZOLTAN_ID_PTR nbors_global,
  int *nbors_part);

static int *objects_by_part(ZZ *zz, int num_obj, int *part,
  int *nparts, int *nonempty);

static int
object_metrics(ZZ *zz, int num_obj, int *parts, float *vwgts, int wgt_dim, 
               int *nparts, int *nonempty, float *obj_imbalance, float *imbalance, float *nobj,
               float *obj_wgt, float *xtra_imbalance, float (*xtra_obj_wgt)[EVAL_SIZE]);

static int 
add_graph_extra_weight(ZZ *zz, int num_obj, int *edges_per_obj, int *vwgt_dim, float **vwgts);

extern int zoltan_lb_eval_sort_increasing(const void *a, const void *b);

static int write_unique_ints(int *val, int len);

#define MAX_SIZE_KEY_BUFFER 60

/*****************************************************************************/

int Zoltan_LB_Eval_Balance(ZZ *zz, int print_stats, BALANCE_EVAL *eval)
{
  /*****************************************************************************/
  /* Return performance metrics in BALANCE_EVAL structure.                     */ 
  /* Also print them out if print_stats is true.                               */
  /*****************************************************************************/

  char *yo = "Zoltan_LB_Eval_Balance";
  int vwgt_dim = zz->Obj_Weight_Dim;
  int i, j, ierr;
  int nparts, nonempty_nparts, req_nparts;
  int num_obj = 0;
  BALANCE_EVAL localEval;

  int *parts=NULL;
  float *vwgts=NULL;

  ZOLTAN_ID_PTR global_ids=NULL, local_ids=NULL;

  ZOLTAN_TRACE_ENTER(zz, yo);

  ierr = ZOLTAN_OK;

  if (!eval)
    eval = &localEval;

  memset(eval, 0, sizeof(BALANCE_EVAL));

  /* Get requested number of parts.  Actual number may differ  */

  ierr = Zoltan_LB_Build_PartDist(zz);
  if (ierr != ZOLTAN_OK){
    goto End;
  }

  req_nparts = zz->LB.Num_Global_Parts;

  /* Get object weights and parts */

  ierr = Zoltan_Get_Obj_List(zz, &num_obj, &global_ids, &local_ids, vwgt_dim, &vwgts, &parts);

  if (ierr != ZOLTAN_OK)
    goto End;

  ZOLTAN_FREE(&global_ids);
  ZOLTAN_FREE(&local_ids);

  /* Local stats */

  eval->nobj[EVAL_LOCAL_SUM] = num_obj;

  if (vwgt_dim > 0){
    for (i=0; i < num_obj; i++){
      eval->obj_wgt[EVAL_LOCAL_SUM]  += vwgts[i*vwgt_dim];
      for (j=1; j <= EVAL_MAX_XTRA_VWGTS; j++){
        if (j == vwgt_dim) break;
        eval->xtra_obj_wgt[j-1][EVAL_LOCAL_SUM]  += vwgts[i*vwgt_dim + j];
      }
    }
  }

  /* Get metrics based on number of objects and object weights */

  ierr = object_metrics(zz, num_obj, parts, vwgts, vwgt_dim,
          &nparts,           /* actual number of parts */
          &nonempty_nparts,  /* number of non-empty parts */
          &eval->obj_imbalance,
          &eval->imbalance,
          eval->nobj,
          eval->obj_wgt,
          eval->xtra_imbalance,
          eval->xtra_obj_wgt);

  if (ierr != ZOLTAN_OK)
    goto End;
     
  /************************************************************************
   * Print results
   */

  if (print_stats && (zz->Proc == zz->Debug_Proc)){

    printf("\n%s  Part count: %1d requested, %1d actual , %1d non-empty\n", 
      yo, req_nparts, nparts, nonempty_nparts);

    printf("%s  Statistics with respect to %1d parts: \n", yo, nparts);
    printf("%s                             Min      Max      Sum  Imbalance\n", yo);

    printf("%s  Number of objects  :  %8.3g %8.3g %8.3g     %5.3f\n", yo, 
        eval->nobj[EVAL_GLOBAL_MIN], eval->nobj[EVAL_GLOBAL_MAX], 
        eval->nobj[EVAL_GLOBAL_SUM], eval->obj_imbalance);

    if (vwgt_dim > 0){
      printf("%s  Object weight      :  %8.3g %8.3g %8.3g     %5.3f\n", yo, 
        eval->obj_wgt[EVAL_GLOBAL_MIN], eval->obj_wgt[EVAL_GLOBAL_MAX], 
        eval->obj_wgt[EVAL_GLOBAL_SUM], eval->imbalance);

      for (i=0; i < vwgt_dim-1; i++){
        if (i == EVAL_MAX_XTRA_VWGTS){
          break;
        }
        printf("%s  Object weight %d    :  %8.3g %8.3g %8.3g     %5.3f\n", yo, i+2,
          eval->xtra_obj_wgt[i][EVAL_GLOBAL_MIN], eval->xtra_obj_wgt[i][EVAL_GLOBAL_MAX], 
          eval->xtra_obj_wgt[i][EVAL_GLOBAL_SUM], eval->xtra_imbalance[i] );
      }
      if (vwgt_dim-1 > EVAL_MAX_XTRA_VWGTS){
        printf("(We calculate up to %d extra object weights.  This can be changed.)\n",
              EVAL_MAX_XTRA_VWGTS);
      }
    }

    printf("\n\n");
  }

End:

  /* Free data */

  ZOLTAN_FREE(&vwgts);
  ZOLTAN_FREE(&parts);

  ZOLTAN_TRACE_EXIT(zz, yo);

  return ierr;
}
/*****************************************************************************/

int Zoltan_LB_Eval_Graph(ZZ *zz, int print_stats, GRAPH_EVAL *graph)
{
  /*****************************************************************************/
  /* Return performance metrics in GRAPH_EVAL structure.                       */ 
  /* Also print them out if print_stats is true.                               */
  /*****************************************************************************/

  char *yo = "Zoltan_LB_Eval_Graph";
  MPI_Comm comm = zz->Communicator;
  int vwgt_dim = zz->Obj_Weight_Dim;
  int ewgt_dim = zz->Edge_Weight_Dim;

  ZOLTAN_ID_PTR global_ids=NULL, local_ids=NULL, nbors_global=NULL;

  int i, j, k, e, ierr, count;
  int nparts, nonempty_nparts, req_nparts;
  int num_weights, obj_part, nbor_part, nother_parts;
  int max_pair, num_pairs, num_obj_parts, offset;
  int num_obj = 0;
  int num_edges = 0;

  int *localCount = NULL, *globalCount = NULL;
  int *parts=NULL, *nbors_part=NULL, *part_check=NULL;
  int *edges_per_obj=NULL, *nbors_proc=NULL;
  int *num_boundary=NULL, *cuts=NULL;
  int *partNums = NULL, *partCount=NULL;

  ENTRY *partNumEntries=NULL, *foundEntry=NULL; 
  ENTRY tempEntry;

  float obj_edge_weights;

  float *vwgts=NULL, *ewgts=NULL, *wgt=NULL;
  float *localVals = NULL, *globalVals = NULL;
  float *cutn=NULL, *cutl=NULL, *cut_wgt=NULL;

  char *keys = NULL;
  char tempBuf[MAX_SIZE_KEY_BUFFER];

  GRAPH_EVAL localEval;

  ZOLTAN_TRACE_ENTER(zz, yo);

  ierr = ZOLTAN_OK;

  if (!graph)
    graph = &localEval;

  memset(graph, 0, sizeof(GRAPH_EVAL));

  if ((zz->Get_Num_Edges == NULL && zz->Get_Num_Edges_Multi == NULL) ||
           (zz->Get_Edge_List == NULL && zz->Get_Edge_List_Multi == NULL)) {
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, 
      "This function requires caller-defined graph query functions.\n");
    return ZOLTAN_FATAL;
  }

  /* Get requested number of parts.  Actual number may differ  */

  ierr = Zoltan_LB_Build_PartDist(zz);
  if (ierr != ZOLTAN_OK){
    goto End;
  }

  req_nparts = zz->LB.Num_Global_Parts;

  /* Get object weights and parts */

  ierr = Zoltan_Get_Obj_List(zz, &num_obj, &global_ids, &local_ids, vwgt_dim, &vwgts, &parts);

  if (ierr != ZOLTAN_OK)
    goto End;


  /*****************************************************************
   * Get graph from query functions
   */

  ierr = Zoltan_Graph_Queries(zz, num_obj, global_ids, local_ids,
                              &num_edges, &edges_per_obj, 
                              &nbors_global, &nbors_proc, &ewgts);

  if (ierr != ZOLTAN_OK)
    goto End;

  ZOLTAN_FREE(&nbors_proc);

  /*****************************************************************
   * Add a vertex weight if ADD_OBJ_WEIGHT is set
   */

  ierr = add_graph_extra_weight(zz, num_obj, edges_per_obj, &vwgt_dim, &vwgts);

  if ((ierr != ZOLTAN_OK) && (ierr != ZOLTAN_WARN)){
    goto End;
  }

  /*****************************************************************
   * Local stats 
   */

  graph->nobj[EVAL_LOCAL_SUM] = num_obj;

  if (vwgt_dim > 0){
    for (i=0; i < num_obj; i++){
      graph->obj_wgt[EVAL_LOCAL_SUM]  += vwgts[i*vwgt_dim];
      for (j=1; j <= EVAL_MAX_XTRA_VWGTS; j++){
        if (j == vwgt_dim) break;
        graph->xtra_obj_wgt[j-1][EVAL_LOCAL_SUM]  += vwgts[i*vwgt_dim + j];
      }
    }
  }

  /*****************************************************************
   * Get metrics based on number of objects and object weights 
   */

  ierr = object_metrics(zz, num_obj, parts, vwgts, vwgt_dim,
          &nparts,          /* actual number of parts */
          &nonempty_nparts,  /* number of non-empty parts */
          &graph->obj_imbalance,
          &graph->imbalance,
          graph->nobj,
          graph->obj_wgt,
          graph->xtra_imbalance,
          graph->xtra_obj_wgt);

  if (ierr != ZOLTAN_OK)
    goto End;

  /*****************************************************************
   * Compute the part number of neighboring objects
   */

  nbors_part = (int *)ZOLTAN_MALLOC(num_edges * sizeof(int));
  if (num_edges && !nbors_part){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  ierr = get_nbor_parts(zz, num_obj, global_ids, local_ids, parts, 
                        num_edges, nbors_global, nbors_part);

  if (ierr != ZOLTAN_OK)
    goto End;

  ZOLTAN_FREE(&global_ids);
  ZOLTAN_FREE(&nbors_global);
  ZOLTAN_FREE(&local_ids);

  /*****************************************************************
   * In order to compute the number of neighboring partitions for
   * each partition, get an upper bound on the possible number of 
   * different object-partition/neighbor-partition pairs.
   */


  max_pair = 0;           /* maximum number of obj_part/nbor_part pairs */
  num_obj_parts = 0; /* total number of non-empty partitions on process */

  if (num_obj){

    if (num_edges > num_obj){
      partNums = (int *)ZOLTAN_MALLOC(num_edges * sizeof(int));
    }
    else{
      partNums = (int *)ZOLTAN_MALLOC(num_obj * sizeof(int));
    }

    if (!partNums){
      ierr = ZOLTAN_MEMERR;
      goto End;
    }

    partNums[0] = parts[0];
    for (i=1, k=1; i < num_obj; i++){ 
       if (parts[i] != partNums[k-1]){
         partNums[k++] = parts[i];
       }
    }
    num_obj_parts = write_unique_ints(partNums, k);

    partCount = (int *)ZOLTAN_MALLOC(sizeof(int) * num_obj_parts * 2);
    if (!partCount){
      ierr = ZOLTAN_MEMERR;
      goto End;
    }
   
    for (i=0; i < num_obj_parts; i++){
      /* this list will have number of partition neighbors for each partition */
      partCount[i*2] = partNums[i];
      partCount[i*2 + 1] = 0;
    }

    if (num_edges > 0){
      partNums[0] = nbors_part[0];
      for (i=1,k=1; i < num_edges; i++){
         if (nbors_part[i] != partNums[k-1]){
           partNums[k++] = nbors_part[i];
         }
      }
      max_pair = write_unique_ints(partNums, k) * num_obj_parts;
    }

    ZOLTAN_FREE(&partNums);
  }

  if (max_pair > num_edges) {
    max_pair = num_edges;
  }
  if ((max_pair > 0) && (max_pair < 4)){
    /* because this is the size of a hash table */
    max_pair = 4;
  }

  if (max_pair){
    partNumEntries = (ENTRY *)ZOLTAN_MALLOC(sizeof(ENTRY) * max_pair);
    keys = (char *)ZOLTAN_MALLOC(sizeof(char) * max_pair * MAX_SIZE_KEY_BUFFER);
    if (!partNumEntries || !keys){
      ierr = ZOLTAN_MEMERR;
      goto End;
    }

    if (!hcreate(max_pair)){ /* hash table for partition number pairs */
      ierr = ZOLTAN_MEMERR;
      goto End;
    }

    partNums = (int *)ZOLTAN_MALLOC(max_pair * 2 * sizeof(int));
    if (!partNums){
      ierr = ZOLTAN_MEMERR;
      goto End;
    }
  }

  /*****************************************************************
   * Compute cut statisticics
   */

  cuts = (int *)ZOLTAN_CALLOC(nparts, sizeof(int));
  num_boundary = (int *)ZOLTAN_CALLOC(nparts, sizeof(int));
  cutn = (float *)ZOLTAN_CALLOC(nparts, sizeof(float));
  cutl = (float *)ZOLTAN_CALLOC(nparts, sizeof(float));
  part_check = (int *) ZOLTAN_CALLOC(nparts, sizeof(int));

  if (nparts && (!cuts || !cutn || !cutl || !num_boundary || !part_check)){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  cut_wgt = (float *)ZOLTAN_CALLOC(nparts * ewgt_dim, sizeof(float));

  if (nparts && ewgt_dim && !cut_wgt){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  num_pairs = 0;

  for (i=0,k=0; i < num_obj; i++){   /* object */

    obj_edge_weights = 0;
    obj_part = parts[i];
    nother_parts= 0;

    for (j=0; j < edges_per_obj[i]; j++,k++){    /* neighbor in graph */

      nbor_part = nbors_part[k];

      if (ewgt_dim > 0){
        obj_edge_weights += ewgts[k * ewgt_dim];  /* "hypergraph" weight */
      }
      else{
        obj_edge_weights = 1.0;
      }

      if (nbor_part != obj_part){
        /* 
         * number of edges that have nbor in a different part 
         */

        cuts[obj_part]++; 

        /*
         * save info required to compute, for each part, the number of parts  
         * that it has neighbors in
         */

        sprintf(tempBuf, "%d %d", obj_part, nbor_part);
        tempEntry.key = tempBuf;
        tempEntry.data = NULL;

        foundEntry = hsearch(tempEntry, FIND);

        if (!foundEntry){
          if (num_pairs == max_pair){
            ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Incorrect calculation of max_pair\n");
            ierr = ZOLTAN_FATAL;
            goto End;
          }
          offset = num_pairs * MAX_SIZE_KEY_BUFFER;
          strcpy(keys + offset, tempBuf);
          partNumEntries[num_pairs].key = keys + offset;
          partNumEntries[num_pairs].data = NULL;

          hsearch(partNumEntries[num_pairs], ENTER);  /* add this pair */

          partNums[num_pairs*2] = obj_part;
          partNums[num_pairs*2 + 1] = nbor_part;
          num_pairs++;  /* number of unique obj_part/nbor_part pairs */
        }

        for (e=0; e < ewgt_dim; e++){
          /*
           * For each part, the sum of the weights of the edges
           * whos neighbor is in a different part
           */
          cut_wgt[obj_part * ewgt_dim + e] += ewgts[k * ewgt_dim + e];
        }

        if (part_check[nbor_part] < i+1){
          nother_parts++;
          part_check[nbor_part] = i + 1;
        }
      }
    }

    if (nother_parts){
      /*
       * hypergraph ConCut measure - hyperedge is vertex and all it's neighbors
       */
      cutl[obj_part] += (obj_edge_weights * nother_parts);

      /*
       * hypergraph NetCut measure
       */
      cutn[obj_part] += obj_edge_weights;

      /*
       * for each part, the number of objects with a neighbor outside
       * the part
       */
      num_boundary[obj_part]++;
    }
  }

  ZOLTAN_FREE(&part_check);
  ZOLTAN_FREE(&parts);
  ZOLTAN_FREE(&edges_per_obj);
  ZOLTAN_FREE(&ewgts);
  ZOLTAN_FREE(&nbors_part);

  if (max_pair){
    hdestroy();
    ZOLTAN_FREE(&partNumEntries);
    ZOLTAN_FREE(&keys);

    if (num_pairs){
      qsort(partNums, num_pairs, sizeof(int) * 2, zoltan_lb_eval_sort_increasing);

      for (i=0, k=0; (i < num_obj_parts) && (k < num_pairs); i++){
        count = 0;
        obj_part = partNums[k*2];

        if (obj_part == partCount[2*i]){
          count = 1;
          for (j=k+1; j < num_pairs ; j++){
            if (partNums[j*2] == obj_part){
              count++;
            }
            else{
              break;
            }
          }
        }
        partCount[i*2 + 1] = count;
        k += count; 
      }
    } 
    ZOLTAN_FREE(&partNums);
  }

  /************************************************************************
   * Write cut statistics to the return structure.
   */

  k = ((ewgt_dim > 0) ? ewgt_dim : 1);

  globalVals = (float *)ZOLTAN_MALLOC(nparts * k * sizeof(float));
  if (nparts && !globalVals){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  globalCount = (int *)ZOLTAN_MALLOC(nparts * sizeof(int));
  if (nparts && !globalCount){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  /*
   * CUTN - NetCut
   */

  MPI_Allreduce(cutn, globalVals, nparts, MPI_FLOAT, MPI_SUM, comm);

  ZOLTAN_FREE(&cutn);

  fget_strided_stats(globalVals, 1, 0, nparts,
               graph->cutn + EVAL_GLOBAL_MIN,
               graph->cutn + EVAL_GLOBAL_MAX,
               graph->cutn + EVAL_GLOBAL_SUM);

  graph->cutn[EVAL_GLOBAL_AVG] = graph->cutn[EVAL_GLOBAL_SUM] / nparts;

  /*
   * CUTL - Connectivity Cut
   */

  MPI_Allreduce(cutl, globalVals, nparts, MPI_FLOAT, MPI_SUM, comm);

  ZOLTAN_FREE(&cutl);

  fget_strided_stats(globalVals, 1, 0, nparts,
               graph->cutl + EVAL_GLOBAL_MIN,
               graph->cutl + EVAL_GLOBAL_MAX,
               graph->cutl + EVAL_GLOBAL_SUM);

  graph->cutl[EVAL_GLOBAL_AVG] = graph->cutl[EVAL_GLOBAL_SUM] / nparts;

  /*
   * CUTS - Number of cut edges in each part
   */

  MPI_Allreduce(cuts, globalCount, nparts, MPI_INT, MPI_SUM, comm);

  ZOLTAN_FREE(&cuts);

  iget_strided_stats(globalCount, 1, 0, nparts,
               graph->cuts + EVAL_GLOBAL_MIN,
               graph->cuts + EVAL_GLOBAL_MAX,
               graph->cuts + EVAL_GLOBAL_SUM);

  graph->cuts[EVAL_GLOBAL_AVG] = graph->cuts[EVAL_GLOBAL_SUM] / nparts;

  /*
   * NNBORPARTS - The number of neighboring parts
   */

  localCount = (int *)ZOLTAN_CALLOC(nparts , sizeof(int));

  for (i=0; i < num_obj_parts; i++){
    localCount[partCount[2*i]] = partCount[2*i + 1];
  }

  ZOLTAN_FREE(&partCount);

  /* Note: this is incorrect if partitions are split across processes, as
   * they would be if migration has not yet occured.
   */

  MPI_Allreduce(localCount, globalCount, nparts, MPI_INT, MPI_SUM, comm);

  ZOLTAN_FREE(&localCount);

  iget_strided_stats(globalCount, 1, 0, nparts,
               graph->nnborparts + EVAL_GLOBAL_MIN,
               graph->nnborparts + EVAL_GLOBAL_MAX,
               graph->nnborparts + EVAL_GLOBAL_SUM);

  graph->nnborparts[EVAL_GLOBAL_AVG] = graph->nnborparts[EVAL_GLOBAL_SUM] / nparts;

  /*
   * CUT WEIGHT - The sum of the weights of the cut edges.
   */

  if (ewgt_dim) {
    num_weights = nparts * ewgt_dim;

    MPI_Allreduce(cut_wgt, globalVals, num_weights, MPI_FLOAT, MPI_SUM, comm);

    ZOLTAN_FREE(&cut_wgt);

    fget_strided_stats(globalVals, ewgt_dim, 0, num_weights,
                 graph->cut_wgt + EVAL_GLOBAL_MIN,
                 graph->cut_wgt + EVAL_GLOBAL_MAX,
                 graph->cut_wgt + EVAL_GLOBAL_SUM);

    graph->cut_wgt[EVAL_GLOBAL_AVG] = graph->cut_wgt[EVAL_GLOBAL_SUM] / nparts;
  }

  for (i=0; i < ewgt_dim-1; i++){
    /* end of calculations for multiple edge weights */

    if (i == EVAL_MAX_XTRA_EWGTS){
      break;
    }
   
    wgt = graph->xtra_cut_wgt[i];

    fget_strided_stats(globalVals, ewgt_dim, i+1, num_weights,
                       wgt + EVAL_GLOBAL_MIN,
                       wgt + EVAL_GLOBAL_MAX,
                       wgt + EVAL_GLOBAL_SUM);

    wgt[EVAL_GLOBAL_AVG] = wgt[EVAL_GLOBAL_SUM]/(float)nparts;
  }

  ZOLTAN_FREE(&globalVals);

  /*
   * Number of objects in part that have an off-part neighbor.
   */

  MPI_Allreduce(num_boundary, globalCount, nparts, MPI_INT, MPI_SUM, comm);

  iget_strided_stats(globalCount, 1, 0, nparts,
               graph->num_boundary + EVAL_GLOBAL_MIN,
               graph->num_boundary + EVAL_GLOBAL_MAX,
               graph->num_boundary + EVAL_GLOBAL_SUM);

  graph->num_boundary[EVAL_GLOBAL_AVG] = graph->num_boundary[EVAL_GLOBAL_SUM] / nparts;

  ZOLTAN_FREE(&num_boundary);
  ZOLTAN_FREE(&globalCount);

  /************************************************************************
   * Print results
   */

  if (print_stats && (zz->Proc == zz->Debug_Proc)){

    printf("\n%s  Part count: %1d requested, %1d actual, %1d non-empty\n", 
      yo, req_nparts, nparts, nonempty_nparts);

    printf("%s  Statistics with respect to %1d parts: \n", yo, nparts);
    printf("%s                             Min      Max      Sum  Imbalance\n", yo);

    printf("%s  Number of objects  :  %8.3g %8.3g %8.3g     %5.3g\n", yo, 
      graph->nobj[EVAL_GLOBAL_MIN], graph->nobj[EVAL_GLOBAL_MAX],
      graph->nobj[EVAL_GLOBAL_SUM], graph->obj_imbalance);

    if (vwgt_dim > 0){
      printf("%s  Object weight      :  %8.3g %8.3g %8.3g     %5.3f\n", yo, 
        graph->obj_wgt[EVAL_GLOBAL_MIN], graph->obj_wgt[EVAL_GLOBAL_MAX], 
        graph->obj_wgt[EVAL_GLOBAL_SUM], graph->imbalance);
  
      for (i=0; i < vwgt_dim-1; i++){
        if (i == EVAL_MAX_XTRA_VWGTS){
          break;
        }
        printf("%s  Object weight %d    :  %8.3g %8.3g %8.3g     %5.3f\n", yo, i+2,
          graph->xtra_obj_wgt[i][EVAL_GLOBAL_MIN], graph->xtra_obj_wgt[i][EVAL_GLOBAL_MAX], 
          graph->xtra_obj_wgt[i][EVAL_GLOBAL_SUM], graph->xtra_imbalance[i]);
      }
      if (vwgt_dim-1 > EVAL_MAX_XTRA_VWGTS){
        printf("(We calculate up to %d extra object weights.  This can be changed.)\n",
              EVAL_MAX_XTRA_VWGTS);
      }
    }

    printf("\n\n");

    printf("%s  Statistics with respect to %1d parts: \n", yo, nparts);
    printf("%s                                    Min      Max    Average    Sum\n", yo);

    printf("%s  Num boundary objects      :  %8.3g %8.3g %8.3g %8.3g\n", yo, 
      graph->num_boundary[EVAL_GLOBAL_MIN], graph->num_boundary[EVAL_GLOBAL_MAX], 
      graph->num_boundary[EVAL_GLOBAL_AVG], graph->num_boundary[EVAL_GLOBAL_SUM]);

    printf("%s  Number of cut edges       :  %8.3g %8.3g %8.3g %8.3g\n", yo, 
      graph->cuts[EVAL_GLOBAL_MIN], graph->cuts[EVAL_GLOBAL_MAX], graph->cuts[EVAL_GLOBAL_AVG],
      graph->cuts[EVAL_GLOBAL_SUM]);

    if (ewgt_dim)
      printf("%s  Weight of cut edges (CUTE):  %8.3g %8.3g %8.3g %8.3g\n", yo, 
        graph->cut_wgt[EVAL_GLOBAL_MIN], graph->cut_wgt[EVAL_GLOBAL_MAX], 
        graph->cut_wgt[EVAL_GLOBAL_AVG], graph->cut_wgt[EVAL_GLOBAL_SUM]);

    for (i=0; i < ewgt_dim-1; i++){
      if (i == EVAL_MAX_XTRA_EWGTS){
        break;
      }
      printf("%s  Weight %d                 :  %8.3g %8.3g %8.3g %8.3g\n", yo, i+2,
        graph->xtra_cut_wgt[i][EVAL_GLOBAL_MIN], graph->xtra_cut_wgt[i][EVAL_GLOBAL_MAX], 
        graph->xtra_cut_wgt[i][EVAL_GLOBAL_AVG], graph->xtra_cut_wgt[i][EVAL_GLOBAL_SUM]);
    }
    if (ewgt_dim-1 > EVAL_MAX_XTRA_EWGTS){
      printf("(We calculate up to %d extra edge weights.  This can be changed.)\n",
            EVAL_MAX_XTRA_EWGTS);
    }

    printf("%s  Num Nbor Parts            :  %8.3g %8.3g %8.3g %8.3g\n", yo, 
      graph->nnborparts[EVAL_GLOBAL_MIN], graph->nnborparts[EVAL_GLOBAL_MAX], 
      graph->nnborparts[EVAL_GLOBAL_AVG], graph->nnborparts[EVAL_GLOBAL_SUM]);

    printf("\n");

    printf("%s  CUTN (Sum_over_edges( (nparts>1)*ewgt )): %8.3g\n", yo, 
           graph->cutn[EVAL_GLOBAL_SUM]);
    printf("%s  CUTL (Sum_over_edges( (nparts-1)*ewgt )): %8.3g\n", yo, 
           graph->cutl[EVAL_GLOBAL_SUM]);
    
    printf("\n\n");
  }

End:

  /* Free data */

  ZOLTAN_FREE(&localVals);
  ZOLTAN_FREE(&localCount);
  ZOLTAN_FREE(&globalVals);
  ZOLTAN_FREE(&globalCount);
  ZOLTAN_FREE(&vwgts);
  ZOLTAN_FREE(&parts);
  ZOLTAN_FREE(&nbors_proc);
  ZOLTAN_FREE(&global_ids);
  ZOLTAN_FREE(&local_ids);
  ZOLTAN_FREE(&edges_per_obj);
  ZOLTAN_FREE(&nbors_global);
  ZOLTAN_FREE(&ewgts);
  ZOLTAN_FREE(&nbors_part);
  ZOLTAN_FREE(&num_boundary);
  ZOLTAN_FREE(&cut_wgt);
  ZOLTAN_FREE(&cutn);
  ZOLTAN_FREE(&cutl);
  ZOLTAN_FREE(&cuts);
  ZOLTAN_FREE(&partNums);
  ZOLTAN_FREE(&partCount);
  ZOLTAN_FREE(&part_check);
  ZOLTAN_FREE(&partNumEntries);
  ZOLTAN_FREE(&keys);

  ZOLTAN_TRACE_EXIT(zz, yo);

  return ierr;
}
/*****************************************************************************/

int Zoltan_LB_Eval_HG(ZZ *zz, int print_stats, HG_EVAL *hg)
{
  /*****************************************************************************/
  /* Return performance metrics in HG_EVAL structure.  Also print them out     */
  /* if print_stats is true.  Results are per part, not per process.      */
  /*****************************************************************************/

  char *yo = "Zoltan_LB_Eval_HG";
  MPI_Comm comm = zz->Communicator;

  float *part_sizes=NULL;
  float *localVals=NULL;

  double local[2], global[2];

  int ierr, debug_level, i;
  int nparts, nonempty_nparts, req_nparts;
  int *localCount = NULL;
  PHGPartParams hgp;

  ZHG* zhg=NULL;

  HG_EVAL localEval;
  
  ZOLTAN_TRACE_ENTER(zz, yo);

  /* Set default error code */
  ierr = ZOLTAN_OK;

  if (!hg)
    hg = &localEval;

  memset(hg, 0, sizeof(HG_EVAL));

  if ((zz->Get_HG_Size_CS==NULL) && (zz->Get_HG_CS==NULL) &&
      (zz->Get_Num_Edges==NULL) && (zz->Get_Num_Edges_Multi==NULL) &&
      (zz->Get_Edge_List==NULL) && (zz->Get_Edge_List_Multi==NULL)) {
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, 
      "This function requires caller-defined graph or hypergraph query functions.\n");
    return ZOLTAN_FATAL;
  }

  /* get the requested number of parts */ 

  ierr = Zoltan_LB_Build_PartDist(zz);
  if (ierr != ZOLTAN_OK){
    goto End;
  }

  req_nparts = zz->LB.Num_Global_Parts;

  /*****************************************************************
   * Get the hypergraph, via the hypergraph or graph query functions
   */

  part_sizes = (float*)ZOLTAN_MALLOC(sizeof(float) * req_nparts);
  if (req_nparts && !part_sizes){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  Zoltan_LB_Get_Part_Sizes(zz, zz->LB.Num_Global_Parts, 1, part_sizes);

  debug_level = zz->Debug_Level;
  zz->Debug_Level = 0;

  ierr = Zoltan_PHG_Initialize_Params(zz, part_sizes, &hgp);
  if (ierr != ZOLTAN_OK)
    goto End;

  zz->Debug_Level = debug_level;

  zhg = (ZHG*) ZOLTAN_MALLOC (sizeof(ZHG));
  if (zhg == NULL){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  Zoltan_Input_HG_Init(zhg);

  ierr = Zoltan_Get_Hypergraph_From_Queries(zz, &hgp, zhg);
  if (ierr != ZOLTAN_OK)
    goto End;

  if (zhg->globalObj == 0){
    if (zz->Proc == zz->Debug_Proc){
      printf("%s: No objects to evaluate\n",yo);
    }
    goto End;
  }

  /* Get metrics based on number of objects and object weights */

  ierr = object_metrics(zz, zhg->nObj, zhg->Input_Parts,
          zhg->objWeight, zhg->objWeightDim,
          &nparts,          /* actual number of parts */
          &nonempty_nparts,  /* number of non-empty parts */
          &hg->obj_imbalance,
          &hg->imbalance,
          hg->nobj,
          hg->obj_wgt,
          hg->xtra_imbalance,
          hg->xtra_obj_wgt);

  if (ierr != ZOLTAN_OK)
    goto End;

  /*****************************************************************
   * Local stats 
   */

  hg->nobj[EVAL_LOCAL_SUM] = zhg->nObj;

  if (zhg->objWeightDim > 0){
    for (i=0; i < zhg->nObj; i++){
      hg->obj_wgt[EVAL_LOCAL_SUM]  += zhg->objWeight[i];
    }
  }

  /************************************************************************
   * Compute the cutn and cutl 
   */

  if (!zhg->Output_Parts)
    zhg->Output_Parts = zhg->Input_Parts;

  ierr = Zoltan_PHG_Cuts(zz, zhg, local);

  if (zhg->Output_Parts == zhg->Input_Parts)
    zhg->Output_Parts = NULL;

  if (ierr != ZOLTAN_OK && ierr != ZOLTAN_WARN){
    goto End;
  }

  MPI_Allreduce(local, global, 2, MPI_DOUBLE, MPI_SUM, comm);
  hg->cutl[EVAL_GLOBAL_SUM] = (float)global[0];
  hg->cutn[EVAL_GLOBAL_SUM] = (float)global[1];

  MPI_Allreduce(local, global, 2, MPI_DOUBLE, MPI_MIN, comm);
  hg->cutl[EVAL_GLOBAL_MIN] = (float)global[0];
  hg->cutn[EVAL_GLOBAL_MIN] = (float)global[1];

  MPI_Allreduce(local, global, 2, MPI_DOUBLE, MPI_MAX, comm);
  hg->cutl[EVAL_GLOBAL_MAX] = (float)global[0];
  hg->cutn[EVAL_GLOBAL_MAX] = (float)global[1];

  hg->cutl[EVAL_GLOBAL_AVG] = hg->cutl[EVAL_GLOBAL_SUM] / nparts;
  hg->cutn[EVAL_GLOBAL_AVG] = hg->cutn[EVAL_GLOBAL_SUM] / nparts;
            
  /************************************************************************
   * Print results
   */

  if (print_stats && (zz->Proc == zz->Debug_Proc)){

    printf("\n%s  Part count: %1d requested, %1d actual, %1d non-empty\n", 
      yo, req_nparts, nparts, nonempty_nparts);

    printf("%s  Statistics with respect to %1d parts: \n", yo, nparts);
    printf("%s                            Min      Max     Sum  Imbalance\n", yo);

    printf("%s  Number of objects  :  %8.3g %8.3g %8.3g   %5.3f\n", yo, 
      hg->nobj[EVAL_GLOBAL_MIN], hg->nobj[EVAL_GLOBAL_MAX], 
      hg->nobj[EVAL_GLOBAL_SUM], hg->obj_imbalance);

    if (zhg->objWeightDim > 0){
      printf("%s  Object weight      :  %8.3g %8.3g %8.3g   %5.3f\n", yo, 
        hg->obj_wgt[EVAL_GLOBAL_MIN], hg->obj_wgt[EVAL_GLOBAL_MAX], 
        hg->obj_wgt[EVAL_GLOBAL_SUM], hg->imbalance);

      for (i=0; i < zhg->objWeightDim-1; i++){
        if (i == EVAL_MAX_XTRA_VWGTS){
          break;
        }
        printf("%s  Object weight %d    :  %8.3g %8.3g %8.3g   %5.3f\n", 
               yo, i+2,
               hg->xtra_obj_wgt[i][EVAL_GLOBAL_MIN],
               hg->xtra_obj_wgt[i][EVAL_GLOBAL_MAX],
               hg->xtra_obj_wgt[i][EVAL_GLOBAL_SUM],
               hg->xtra_imbalance[i]);
      }
      if (zhg->objWeightDim-1 > EVAL_MAX_XTRA_VWGTS){
        printf("(We calculate up to %d extra object weights.  "
               "This can be changed.)\n",
               EVAL_MAX_XTRA_VWGTS);
      }
    }
    printf("\n");

    printf("%s  CUTN (Sum_over_edges( (nparts>1)*ewgt )): %8.3g\n", yo, hg->cutn[EVAL_GLOBAL_SUM]);
    printf("%s  CUTL (Sum_over_edges( (nparts-1)*ewgt )): %8.3g\n", yo, hg->cutn[EVAL_GLOBAL_SUM]);


    printf("\n\n");
  }

End:

  /* Free data */

  if (hgp.globalcomm.row_comm != MPI_COMM_NULL)
    MPI_Comm_free(&(hgp.globalcomm.row_comm));
  if (hgp.globalcomm.col_comm != MPI_COMM_NULL)
    MPI_Comm_free(&(hgp.globalcomm.col_comm));
  if (hgp.globalcomm.Communicator != MPI_COMM_NULL)
    MPI_Comm_free(&(hgp.globalcomm.Communicator));

  ZOLTAN_FREE(&part_sizes);
  if (zhg){
    Zoltan_PHG_Free_Hypergraph_Data(zhg);
    ZOLTAN_FREE(&zhg);
  }
  ZOLTAN_FREE(&localVals);
  ZOLTAN_FREE(&localCount);

  ZOLTAN_TRACE_EXIT(zz, yo);

  return ierr;
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/* The old LB_Eval                                                      */
/************************************************************************/
int Zoltan_LB_Eval (ZZ *zzin, int print_stats,
     int *nobj, float *obj_wgt,
     int *ncuts, float *cut_wgt,
     int *nboundary, int *nadj)
/*
 * Input:
 *   zzin        - pointer to Zoltan structure
 *   print_stats - if > 0, compute and print max, min and sum of the metrics
 *                 if == 0, stay silent but compute output arguments
 *
 * Output:
 *   nobj      - number of objects (local for this proc)
 *   obj_wgt   - obj_wgt[0:num_vertex_weights-1] are the object weights
 *               (local for this proc)
 *   ncuts     - number of cuts (average per part)
 *   cut_wgt   - cut_wgt[0:num_vertex_weights-1] are the cut weights
 *               (average per part)
 *   nboundary - number of boundary objects (average per part)
 *   nadj      - the number of adjacent parts (average per part)
 *
 * Output parameters will only be returned if they are
 * not NULL on entry.
 */
{
  char *yo = "Zoltan_LB_Eval";
  int ierr = ZOLTAN_OK, i;
  ZZ *zz = Zoltan_Copy(zzin);
  int nwgt = zz->Obj_Weight_Dim;
  GRAPH_EVAL eval_graph;
  HG_EVAL eval_hg;
  BALANCE_EVAL eval_lb;
  float *object_weight=NULL, *cut_weight=NULL;
  float (*xtra_object_weight)[EVAL_SIZE]=NULL; 
  float (*xtra_cut_weight)[EVAL_SIZE]=NULL; 
  float (*src)[EVAL_SIZE];
  float *dest=NULL;
  float *num_objects=NULL, *num_boundary=NULL, *num_adjacency=NULL, *num_cuts=NULL;
  int graph_callbacks = 0;
  int hypergraph_callbacks = 0;
  int num_extra = 0;

  if (nwgt <= EVAL_MAX_XTRA_VWGTS + 1){
    num_extra = nwgt - 1;
  }
  else{
    num_extra = EVAL_MAX_XTRA_VWGTS;
    ZOLTAN_PRINT_WARN(zz->Proc, yo, 
      "EVAL_MAX_XTRA_VWGTS is not sufficient to report all object weights");
    ierr = ZOLTAN_WARN;
  }

  if (nobj) *nobj = 0;
  if (obj_wgt) *obj_wgt = 0;
  if (ncuts) *ncuts = 0;
  if (cut_wgt) *cut_wgt = 0;
  if (nboundary) *nboundary = 0;
  if (nadj) *nadj = 0;

  if (zz->Get_HG_Size_CS && zz->Get_HG_CS){
    hypergraph_callbacks = 1;
  }
  if ((zz->Get_Num_Edges != NULL || zz->Get_Num_Edges_Multi != NULL) &&
           (zz->Get_Edge_List != NULL || zz->Get_Edge_List_Multi != NULL)) {
    graph_callbacks = 1;
  }

  ierr = Zoltan_LB_Eval_Balance(zz, print_stats, &eval_lb);
  if ((ierr != ZOLTAN_OK) && (ierr != ZOLTAN_WARN)){ 
    ZOLTAN_PRINT_ERROR(zz->Proc, yo,
                       "Error returned from Zoltan_LB_Eval_Balance");
    goto End;
  } 
  num_objects = eval_lb.nobj + EVAL_LOCAL_SUM;
  object_weight = eval_lb.obj_wgt + EVAL_LOCAL_SUM;
  xtra_object_weight = eval_lb.xtra_obj_wgt;

  if (graph_callbacks){
    ierr = Zoltan_LB_Eval_Graph(zz, print_stats, &eval_graph);
    if ((ierr != ZOLTAN_OK) && (ierr != ZOLTAN_WARN)){ 
      ZOLTAN_PRINT_ERROR(zz->Proc, yo,
                         "Error returned from Zoltan_LB_Eval_Graph");
      goto End;
    } 
    num_objects = eval_graph.nobj + EVAL_LOCAL_SUM;
    object_weight = eval_graph.obj_wgt + EVAL_LOCAL_SUM;
    cut_weight = eval_graph.cut_wgt + EVAL_GLOBAL_AVG;
    xtra_object_weight = eval_graph.xtra_obj_wgt;
    xtra_cut_weight = eval_graph.xtra_cut_wgt;
    num_cuts = eval_graph.cuts + EVAL_GLOBAL_AVG;
    num_boundary = eval_graph.num_boundary + EVAL_GLOBAL_AVG;
    num_adjacency = eval_graph.nnborparts + EVAL_GLOBAL_AVG;
  }

  if (graph_callbacks || hypergraph_callbacks){
    ierr = Zoltan_LB_Eval_HG(zz, print_stats, &eval_hg);
    if ((ierr != ZOLTAN_OK) && (ierr != ZOLTAN_WARN)){ 
      ZOLTAN_PRINT_ERROR(zz->Proc, yo,
                         "Error returned from Zoltan_LB_Eval_HG");
      goto End;
    } 
    num_objects = eval_hg.nobj + EVAL_LOCAL_SUM;
    object_weight = eval_hg.obj_wgt + EVAL_LOCAL_SUM;
    cut_weight = eval_hg.cutn + EVAL_GLOBAL_AVG;
  }

  if ((ierr == ZOLTAN_OK) || (ierr == ZOLTAN_WARN)){ 
    if (nobj){
      *nobj = (int)num_objects[0];
    }
    if (nwgt && obj_wgt){
      *obj_wgt = *object_weight;
      dest = obj_wgt + 1;
      src = xtra_object_weight;
      for (i=1; src && (i <= num_extra); i++){
        *dest++ = src[i-1][EVAL_LOCAL_SUM];
      }
    }

    if (ncuts && num_cuts) *ncuts = (int)num_cuts[0];

    if (nwgt && cut_wgt && cut_weight){
     *cut_wgt = *cut_weight;
      dest = cut_wgt + 1;
      src = xtra_cut_weight;
      for (i=1; src && (i <= num_extra); i++){
        *dest++ = src[i-1][EVAL_GLOBAL_AVG];
      }
    }

    if (nboundary && num_boundary) *nboundary = (int)num_boundary[0];
    if (nadj && num_adjacency) *nadj = (int)num_adjacency[0];
  }

End:
  Zoltan_Destroy(&zz);
  return ierr;
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/

static void iget_strided_stats(int *v, int stride, int offset, int len,
                             float *min, float *max, float *sum)
{
  int i;
  int *v0;
  float fmin, fmax, fsum;

  if (len < 1) return;

  v0 = v + offset;

  fmin = fmax = fsum = v0[0];

  for (i=stride; i < len; i += stride){
    if (v0[i] < fmin) fmin = (float)v0[i];
    else if (v0[i] > fmax) fmax = (float)v0[i];
    fsum += (float)v0[i];
  }

  *min = fmin;
  *max = fmax;
  *sum = fsum;
}
/************************************************************************/
static void fget_strided_stats(float *v, int stride, int offset, int len,
                             float *min, float *max, float *sum)
{
  int i;
  float *v0;
  float fmin, fmax, fsum;

  if (len < 1) return;

  v0 = v + offset;

  fmin = fmax = fsum = v0[0];

  for (i=stride; i < len; i += stride){
    if (v0[i] < fmin) fmin = v0[i];
    else if (v0[i] > fmax) fmax = v0[i];
    fsum += v0[i];
  }

  *min = fmin;
  *max = fmax;
  *sum = fsum;
}

/************************************************************************/
#define TEST_DD_ERROR(ierr, yo, proc, fn) \
  if (ierr != ZOLTAN_OK && ierr != ZOLTAN_WARN) { \
    char msg[64];  \
    sprintf(msg, "Error in %s\n", fn); \
    ZOLTAN_PRINT_ERROR(proc, yo, msg); \
    goto End;\
  }


static int get_nbor_parts(
  ZZ *zz,
  int nobj,                     /* Input:  number of local objs */
  ZOLTAN_ID_PTR global_ids,     /* Input:  GIDs of local objs */
  ZOLTAN_ID_PTR local_ids,      /* Input:  LIDs of local objs */
  int *part,                    /* Input:  part assignments of local objs */
  int nnbors,                   /* Input:  number of neighboring objs */
  ZOLTAN_ID_PTR nbors_global,   /* Input:  GIDs of neighboring objs */
  int *nbors_part               /* Output:  part assignments of neighbor objs */
)
{
/* Function to retrieve the part number for neighboring nodes. */
char *yo = "get_nbor_parts";
struct Zoltan_DD_Struct *dd = NULL;
int *owner = NULL;
int maxnobj;
int ierr;
int start, size, i_am_done, alldone;
const int MAXSIZE = 200000;

  ZOLTAN_TRACE_ENTER(zz, yo);

  MPI_Allreduce(&nobj, &maxnobj, 1, MPI_INT, MPI_MAX, zz->Communicator);
  ierr = Zoltan_DD_Create(&dd, zz->Communicator, zz->Num_GID, zz->Num_LID,
                          0, MIN(maxnobj,MAXSIZE), 0);
  TEST_DD_ERROR(ierr, yo, zz->Proc, "Zoltan_DD_Create");

  ierr = Zoltan_DD_Update(dd, global_ids, local_ids, NULL, part, nobj);
  TEST_DD_ERROR(ierr, yo, zz->Proc, "Zoltan_DD_Update");

  /* Do the find in chunks to avoid swamping memory. */
  owner = (int *) ZOLTAN_MALLOC(MIN(MAXSIZE,nnbors) * sizeof(int));
  start = 0;
  alldone = 0;
  while (!alldone) {
    size = MIN(MAXSIZE, (nnbors-start > 0 ? nnbors-start : 0));
    if (start < nnbors)
      ierr = Zoltan_DD_Find(dd, &nbors_global[start*zz->Num_GID],
                            NULL, NULL, &nbors_part[start],
                            size, owner);
    else  /* I'm done, but other processors might not be */
      ierr = Zoltan_DD_Find(dd, NULL, NULL, NULL, NULL, 0, NULL);
    start += size;
    i_am_done = (nnbors - start > 0 ? 0 : 1);
    MPI_Allreduce(&i_am_done, &alldone, 1, MPI_INT, MPI_MIN, zz->Communicator);
  }

  ZOLTAN_FREE(&owner);
  TEST_DD_ERROR(ierr, yo, zz->Proc, "Zoltan_DD_Find");

End:
  Zoltan_DD_Destroy(&dd);
  ZOLTAN_TRACE_EXIT(zz, yo);
  return ierr;
}

/************************************************************************/

static int *
objects_by_part(ZZ *zz, int num_obj, int *part, int *nparts, int *nonempty)
{
  char *yo = "objects_by_part";
  int i, num_parts, num_nonempty, max_part, gmax_part;
  int *partCounts = NULL, *totalCounts;
  int *returnBuf = NULL;

  ZOLTAN_TRACE_ENTER(zz, yo);

  max_part = 0;
  for (i=0; i < num_obj; i++){
    if (part[i] > max_part) max_part = part[i];
  }

  MPI_Allreduce(&max_part, &gmax_part, 1, MPI_INT, MPI_MAX, zz->Communicator);

  max_part = gmax_part + 1;

  /* Allocate and return a buffer containing the local count of objects by part,
     followed by the global count.  Set "nparts" to the number of parts, and
     set "nonempty" to the count of those that have objects in them.
   */

  partCounts = (int *)ZOLTAN_CALLOC(max_part * 2, sizeof(int));
  if (max_part && !partCounts)
    return NULL;

  totalCounts = partCounts + max_part;

  for (i=0; i < num_obj; i++)
    partCounts[part[i]]++;

  MPI_Allreduce(partCounts, totalCounts, max_part, MPI_INT, MPI_SUM, zz->Communicator);

  num_parts = max_part;

  for (i=max_part-1; i > 0; i--){
    if (totalCounts[i] > 0) break;
    num_parts--;
  }

  returnBuf = (int *)ZOLTAN_MALLOC(num_parts * sizeof(int));
  if (num_parts && !returnBuf)
    return NULL;

  memcpy(returnBuf, totalCounts, sizeof(int) * num_parts);

  ZOLTAN_FREE(&partCounts);

  num_nonempty = 0;

  for (i=0; i < num_parts; i++){
    if (returnBuf[i] > 0) num_nonempty++;
  }

  *nparts = num_parts;
  *nonempty = num_nonempty;

  ZOLTAN_TRACE_EXIT(zz, yo);
  return returnBuf;
}

/************************************************************************/

static int
object_metrics(ZZ *zz, int num_obj, int *parts, float *vwgts, int vwgt_dim,
               int *nparts,       /* return actual number of parts */
               int *nonempty,     /* return number of those that are non-empty*/
               float *obj_imbalance,  /* object # imbalance wrt parts */
               float *imbalance,      /* object wgt imbalance wrt parts */
               float *nobj,       /* number of objects */
               float *obj_wgt,    /* object weights */
               float *xtra_imbalance,  /* return if vertex weight dim > 1 */
    float (*xtra_obj_wgt)[EVAL_SIZE])  /* return if vertex weight dim > 1 */
{
  char *yo = "object_metrics";
  MPI_Comm comm = zz->Communicator;

  int i, j, ierr;
  int num_weights, num_parts, num_nonempty_parts;

  int *globalCount = NULL;

  float *wgt=NULL;
  float *localVals = NULL, *globalVals = NULL;

  ierr = ZOLTAN_OK;

  ZOLTAN_TRACE_ENTER(zz, yo);

  /* Get the actual number of parts, and number of objects per part */

  globalCount = objects_by_part(zz, num_obj, parts,
                &num_parts,           /* actual number of parts */
                &num_nonempty_parts); /* number of those which are non-empty */

  if (!globalCount){
    ierr = ZOLTAN_MEMERR;
    goto End;
  }

  *nparts = num_parts;
  *nonempty = num_nonempty_parts;

  iget_strided_stats(globalCount, 1, 0, num_parts,
                     nobj + EVAL_GLOBAL_MIN,
                     nobj + EVAL_GLOBAL_MAX,
                     nobj + EVAL_GLOBAL_SUM);

  nobj[EVAL_GLOBAL_AVG] = nobj[EVAL_GLOBAL_SUM]/(float)num_parts;
 
  *obj_imbalance = (nobj[EVAL_GLOBAL_SUM] > 0 ? 
      nobj[EVAL_GLOBAL_MAX]*num_parts / nobj[EVAL_GLOBAL_SUM]: 1);

  ZOLTAN_FREE(&globalCount);

  if (vwgt_dim > 0){

    num_weights = num_parts * vwgt_dim;
  
    localVals = (float *)ZOLTAN_CALLOC(num_weights*2, sizeof(float));
  
    if (num_weights && !localVals){
      ierr = ZOLTAN_MEMERR;
      goto End;
    }
  
    globalVals = localVals + num_weights;
  
    if (vwgt_dim>0){
      for (i=0; i<num_obj; i++){
        for (j=0; j<vwgt_dim; j++){
          localVals[parts[i]*vwgt_dim+j] += vwgts[i*vwgt_dim+j];
        }
      }
    }
  
    MPI_Allreduce(localVals, globalVals, num_weights, MPI_FLOAT, MPI_SUM, comm);
  
    fget_strided_stats(globalVals, vwgt_dim, 0, num_weights,
                       obj_wgt + EVAL_GLOBAL_MIN,
                       obj_wgt + EVAL_GLOBAL_MAX,
                       obj_wgt + EVAL_GLOBAL_SUM);
  
    obj_wgt[EVAL_GLOBAL_AVG] = obj_wgt[EVAL_GLOBAL_SUM]/(float)num_parts;
  
    *imbalance = (obj_wgt[EVAL_GLOBAL_SUM] > 0 ? 
        obj_wgt[EVAL_GLOBAL_MAX]*num_parts / obj_wgt[EVAL_GLOBAL_SUM]: 1);
  
    for (i=0; i < vwgt_dim-1; i++){
      /* calculations for multiple vertex weights */
  
      if (i == EVAL_MAX_XTRA_VWGTS){
        break;
      }
     
      wgt = xtra_obj_wgt[i];
  
      fget_strided_stats(globalVals, vwgt_dim, i+1, num_weights,
                         wgt + EVAL_GLOBAL_MIN,
                         wgt + EVAL_GLOBAL_MAX,
                         wgt + EVAL_GLOBAL_SUM);
  
      wgt[EVAL_GLOBAL_AVG] = wgt[EVAL_GLOBAL_SUM]/(float)num_parts;
  
      xtra_imbalance[i] = (wgt[EVAL_GLOBAL_SUM] > 0 ? 
            (wgt[EVAL_GLOBAL_MAX]*num_parts / wgt[EVAL_GLOBAL_SUM]) : 1);
    }
  }
  else{
    /* assume an object weight of one per object */
    for (i=0; i < EVAL_SIZE; i++){
      obj_wgt[i] = nobj[i];
    }
    *imbalance = *obj_imbalance;
  }

  globalVals = NULL;

End:
  ZOLTAN_FREE(&localVals);
  ZOLTAN_FREE(&globalCount);

  ZOLTAN_TRACE_EXIT(zz, yo);
  return ierr;
}

/************************************************************************/

static int 
add_graph_extra_weight(ZZ *zz, int num_obj, int *edges_per_obj, int *vwgt_dim, float **vwgts)
{
  char *yo = "add_graph_extra_weight";
  PARAM_VARS params[2] = 
    {{ "ADD_OBJ_WEIGHT",  NULL,  "STRING", 0},
     { NULL, NULL, NULL, 0 } };
  char add_obj_weight[64];
  int ierr, i, j;
  int add_type = 0;
  float *tmpnew, *tmpold;
  float *weights = NULL;
  int weight_dim = 0;

  ierr = ZOLTAN_OK;

  strcpy(add_obj_weight, "NONE");
  Zoltan_Bind_Param(params, "ADD_OBJ_WEIGHT", (void *) add_obj_weight);
  Zoltan_Assign_Param_Vals(zz->Params, params, 0, zz->Proc, zz->Debug_Proc);

  if (!strcasecmp(add_obj_weight, "NONE")){
    return ierr;
  }
  else if ((!strcasecmp(add_obj_weight, "UNIT")) || (!strcasecmp(add_obj_weight, "VERTICES"))){
    add_type = 1;
  }
  else if ((!strcasecmp(add_obj_weight, "VERTEX DEGREE")) ||
           (!strcasecmp(add_obj_weight, "PINS")) ||
           (!strcasecmp(add_obj_weight, "NONZEROS"))){
    add_type = 2;
  }
  else {
    ZOLTAN_PRINT_WARN(zz->Proc, yo, "Invalid parameter value for ADD_OBJ_WEIGHT!\n");
    ierr = ZOLTAN_WARN;
    add_type = 0;
  }

  if (add_type > 0){

    weight_dim = *vwgt_dim + 1;
    weights = (float *)ZOLTAN_CALLOC(num_obj * weight_dim , sizeof(float));
    if (num_obj && weight_dim && !weights){
      ierr = ZOLTAN_MEMERR;
      goto End;
    }
   
    tmpnew = weights;
    tmpold = *vwgts;

    for (i=0; i < num_obj; i++){
      for (j=0; j < *vwgt_dim; j++){
        *tmpnew++ = *tmpold++;
      }
      if (add_type == 1){
        *tmpnew++ = 1.0;
      }
      else{
        *tmpnew++ = edges_per_obj[i];
      }
    }

    tmpold = *vwgts;
    ZOLTAN_FREE(&tmpold);
    *vwgt_dim = weight_dim;
    *vwgts = weights;
  }

End:

  return ierr;
}

/************************************************************************/

int zoltan_lb_eval_sort_increasing(const void *a, const void *b)
{
  const int *val_a = (const int *)a;
  const int *val_b = (const int *)b;

  if (*val_a < *val_b){
    return -1;
  }
  else if (*val_a > *val_b){
    return 1;
  }
  else{
    return 0;
  }
}

/************************************************************************ 
 * rewrite list of ints as increasing list of unique ints, return length
 */

static int 
write_unique_ints(int *val, int len)
{
  int count, i;

  if (len < 2)
    return len;

  qsort(val, len, sizeof(int), zoltan_lb_eval_sort_increasing);

  count = 1;
  for (i=1; i < len; i++){
    if (val[i] != val[count-1]){
      if (i > count){
        val[count] = val[i];
      }
      count++;
    }
  }

  return count;
}

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
