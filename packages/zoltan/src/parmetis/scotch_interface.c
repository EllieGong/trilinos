/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2008 Sandia National Laboratories.                          *
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


#include <ctype.h>
#include "zz_const.h"
#include "zz_util_const.h"
#include "all_allo_const.h"
#include "params_const.h"
#include "order_const.h"
#include "third_library.h"
#include "scotch_interface.h"

  /**********  parameters structure for Scotch methods **********/
static PARAM_VARS Scotch_params[] = {
  { "SCOTCH_METHOD", NULL, "STRING", 0 },
  { "SCOTCH_STRAT", NULL, "STRING", 0 },
  { "SCOTCH_STRAT_FILE", NULL, "STRING", 0 },
  { NULL, NULL, NULL, 0 } };

static int Zoltan_Scotch_Bind_Param(ZZ * zz, char *alg, char **strat);

static int
Zoltan_Scotch_Construct_Offset(ZOS *order,
			       indextype *children,
			       int root,
			       indextype* size,
			       indextype* tree,
			       int offset, int *leafnum);

static int compar_int (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}




/***************************************************************************
 *  The Scotch ordering routine piggy-backs on the Scotch
 *  partitioning routines.
 **************************************************************************/

/*
 * TODO: at this time, only distributed computations are allowed.
 */

int Zoltan_Scotch_Order(
  ZZ *zz,               /* Zoltan structure */
  int num_obj,		/* Number of (local) objects to order. */
  ZOLTAN_ID_PTR gids,   /* List of global ids (local to this proc) */
  /* The application must allocate enough space */
  ZOLTAN_ID_PTR lids,   /* List of local ids (local to this proc) */
/* The application must allocate enough space */
  int *rank,		/* rank[i] is the rank of gids[i] */
  int *iperm,
  ZOOS *order_opt 	/* Ordering options, parsed by Zoltan_Order */
)
{
  static char *yo = "Zoltan_Scotch_Order";
  int n, ierr;
  ZOLTAN_Output_Order ord;
  ZOLTAN_Third_Graph gr;
  SCOTCH_Strat        stradat;
  SCOTCH_Dgraph       grafdat;
  SCOTCH_Graph        cgrafdat;
  SCOTCH_Dordering    ordedat;
  int edgelocnbr = 0;

  /* The following are used to convert elimination tree in Zoltan format */
  indextype          *tree;
  indextype          *size;
  indextype          *children;
  indextype           leafnum;
  int numbloc;
  indextype           start;
  int root = -1;

  MPI_Comm comm = zz->Communicator;/* want to risk letting external packages */
  int timer_p = 0;
  int get_times = 0;
  int use_timers = 0;
  double times[5];

  char alg[MAX_PARAM_STRING_LEN+1];
  char *strat = NULL;

  ZOLTAN_TRACE_ENTER(zz, yo);

  memset(&gr, 0, sizeof(ZOLTAN_Third_Graph));
  memset(&ord, 0, sizeof(ZOLTAN_Output_Order));

  strcpy (alg, "NODEND");
  ierr = Zoltan_Scotch_Bind_Param(zz, alg, &strat);
  if ((ierr != ZOLTAN_OK) && (ierr != ZOLTAN_WARN)) {
    ZOLTAN_TRACE_EXIT(zz, yo);
    return(ierr);
  }

  ord.order_info = &(zz->Order);
  ord.order_opt = order_opt;

  if (!order_opt){
    /* If for some reason order_opt is NULL, allocate a new ZOOS here. */
    /* This should really never happen. */
    order_opt = (ZOOS *) ZOLTAN_MALLOC(sizeof(ZOOS));
    strcpy(order_opt->method,"SCOTCH");
  }

  /* Scotch only computes the rank vector */
  order_opt->return_args = RETURN_RANK;

  /* Check that num_obj equals the number of objects on this proc. */
  /* This constraint may be removed in the future. */
  n = zz->Get_Num_Obj(zz->Get_Num_Obj_Data, &ierr);
  if ((ierr!= ZOLTAN_OK) && (ierr!= ZOLTAN_WARN)){
    /* Return error code */
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Get_Num_Obj returned error.");
    return(ZOLTAN_FATAL);
  }
  if (n != num_obj){
    /* Currently this is a fatal error. */
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Input num_obj does not equal the number of objects.");
    return(ZOLTAN_FATAL);
  }

  /* Do not use weights for ordering */
  gr.obj_wgt_dim = -1;
  gr.edge_wgt_dim = -1;
  gr.num_obj = num_obj;

  /* Check what ordering type is requested */
  if (order_opt){
    if (strcmp(order_opt->order_type, "LOCAL") == 0)
      gr.graph_type = - LOCAL_GRAPH;
    else if (strcmp(order_opt->order_type, "GLOBAL") == 0)
      gr.graph_type =  - GLOBAL_GRAPH;
    else
      gr.graph_type = - NO_GRAPH;
  }
  gr.get_data = 1;

  /* If reorder is true, we already have the id lists. Ignore weights. */
  if ((order_opt && order_opt->reorder))
    gr.id_known = 1;                        /* We already have global_ids and local_ids */

  timer_p = Zoltan_Preprocess_Timer(zz, &use_timers);

    /* Start timer */
  get_times = (zz->Debug_Level >= ZOLTAN_DEBUG_ATIME);
  if (get_times){
    MPI_Barrier(zz->Communicator);
    times[0] = Zoltan_Time(zz->Timer);
  }

  ierr = Zoltan_Preprocess_Graph(zz, &gids, &lids,  &gr, NULL, NULL, NULL);

  edgelocnbr =  gr.xadj[gr.num_obj];
  if (gr.graph_type==GLOBAL_GRAPH){
    if (SCOTCH_dgraphInit (&grafdat, comm) != 0) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot initialize Scotch graph.");
    }

    if (SCOTCH_dgraphBuild (&grafdat, 0, gr.num_obj, gr.num_obj, gr.xadj, gr.xadj + 1,
			    NULL, NULL,edgelocnbr, edgelocnbr, gr.adjncy, NULL, NULL) != 0) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot construct Scotch graph.");
    }
  }
  else {/* gr.graph_type==GLOBAL_GRAPH */
    if (SCOTCH_graphInit (&cgrafdat) != 0) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot initialize Scotch graph.");
    }

    if (SCOTCH_graphBuild (&cgrafdat, 0, gr.num_obj, gr.xadj, gr.xadj + 1,
			   NULL, NULL,edgelocnbr, gr.adjncy, NULL) != 0) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot construct Scotch graph.");
    }
  }

  /* Allocate space for rank array */
  ord.rank = (indextype *) ZOLTAN_MALLOC(gr.num_obj*sizeof(indextype));
  if (!ord.rank){
    /* Not enough memory */
    Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
    ZOLTAN_THIRD_ERROR(ZOLTAN_MEMERR, "Out of memory.");
  }
  if (gr.graph_type!=GLOBAL_GRAPH){
  /* Allocate space for inverse perm */
    ord.iperm = (indextype *) ZOLTAN_MALLOC(gr.num_obj*sizeof(indextype));
    if (!ord.iperm){
      /* Not enough memory */
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_MEMERR, "Out of memory.");
    }
  }
  else
    ord.iperm = NULL;

  SCOTCH_stratInit (&stradat);
  if (strat != NULL) {
    if (((gr.graph_type==GLOBAL_GRAPH) && (SCOTCH_stratDgraphOrder (&stradat, strat)) != 0) ||
	(SCOTCH_stratGraphOrder (&stradat, strat) != 0)) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Invalid Scotch strat.");
    }
  }

  if (gr.graph_type != GLOBAL_GRAPH) { /* Allocate separators tree */
    if (Zoltan_Order_Init_Tree (&zz->Order, gr.num_obj + 1, gr.num_obj) != ZOLTAN_OK) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_MEMERR, "Out of memory.");
    }
  }

  if ((gr.graph_type==GLOBAL_GRAPH) && (SCOTCH_dgraphOrderInit (&grafdat, &ordedat) != 0)) {
    Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
    ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot construct Scotch graph.");
  }

  /* Get a time here */
  if (get_times) times[1] = Zoltan_Time(zz->Timer);

  if (gr.graph_type==GLOBAL_GRAPH){
    ZOLTAN_TRACE_DETAIL(zz, yo, "Calling the PT-Scotch library");
    ierr = SCOTCH_dgraphOrderCompute (&grafdat, &ordedat, &stradat);
    ZOLTAN_TRACE_DETAIL(zz, yo, "Returned from the PT-Scotch library");
  }
  else {
    ZOLTAN_TRACE_DETAIL(zz, yo, "Calling the Scotch library");
    ierr = SCOTCH_graphOrder (&cgrafdat,  &stradat, ord.rank, NULL,
				     &zz->Order.nbr_blocks, zz->Order.start, zz->Order.ancestor);
    ZOLTAN_TRACE_DETAIL(zz, yo, "Returned from the Scotch library");
  }

  if (ierr != 0) {
    Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
    ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot compute Scotch ordering.");
  }

  /* Get a time here */
  if (get_times) times[2] = Zoltan_Time(zz->Timer);

  if (gr.graph_type != GLOBAL_GRAPH) { /* We already have separator tree, just have to compute the leaves */
    for (numbloc = 0 ; numbloc < zz->Order.nbr_blocks ; ++numbloc) {
      zz->Order.leaves[numbloc] = numbloc;
    }
    for (numbloc = 0 ; numbloc < zz->Order.nbr_blocks ; ++numbloc) {
      if (zz->Order.ancestor[numbloc] < 0)
	continue;
      zz->Order.leaves[zz->Order.ancestor[numbloc]] = zz->Order.nbr_blocks + 1;
    }
    /* TODO : check if there is a normalized sort in Zoltan */
    qsort(zz->Order.leaves, zz->Order.nbr_blocks, sizeof(int), compar_int);
    zz->Order.nbr_leaves = 0;
    for (numbloc = 0 ; numbloc < zz->Order.nbr_blocks ; ++numbloc) {
      if (zz->Order.leaves[numbloc] > zz->Order.nbr_blocks) {
	zz->Order.leaves[numbloc] = -1;
	zz->Order.nbr_leaves = numbloc;
	break;
      }
    }
    if (zz->Order.nbr_leaves == 0) {
      zz->Order.leaves[zz->Order.nbr_blocks] = -1;
      zz->Order.nbr_leaves = zz->Order.nbr_blocks;
    }

  }
  else{
    /* Compute permutation */
    if (SCOTCH_dgraphOrderPerm (&grafdat, &ordedat, ord.rank) != 0) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot compute Scotch rank array");
    }

    /* Construct elimination tree */
    zz->Order.nbr_blocks = SCOTCH_dgraphOrderCblkDist (&grafdat, &ordedat);
    if (zz->Order.nbr_blocks <= 0) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot compute Scotch block");
    }

    if (Zoltan_Order_Init_Tree (&zz->Order, 2*zz->Num_Proc, zz->Num_Proc) != ZOLTAN_OK) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_MEMERR, "Out of memory.");
    }

    tree = (indextype *) ZOLTAN_MALLOC((zz->Order.nbr_blocks+1)*sizeof(indextype));
    size = (indextype *) ZOLTAN_MALLOC((zz->Order.nbr_blocks+1)*sizeof(indextype));

    if ((tree == NULL) || (size == NULL)){
      /* Not enough memory */
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_MEMERR, "Out of memory.");
    }

    if (SCOTCH_dgraphOrderTreeDist (&grafdat, &ordedat, tree, size) != 0) {
      Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot compute Scotch rank array");
    }

    children = (indextype *) ZOLTAN_MALLOC(3*zz->Order.nbr_blocks*sizeof(indextype));
    for (numbloc = 0 ; numbloc < 3*zz->Order.nbr_blocks ; ++numbloc) {
      children[numbloc] = -2;
    }

    /* Now convert scotch separator tree in Zoltan elimination tree */
    root = -1;
    for (numbloc = 0 ; numbloc < zz->Order.nbr_blocks ; ++numbloc) { /* construct a top-bottom tree */
      indextype tmp;
      int index=0;

      tmp = tree[numbloc];
      if (tmp == -1) {
	root = numbloc;
	continue;
    }
      while ((index<3) && (children[3*tmp+index] > 0))
	index ++;

      if (index >= 3) {
	Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, &ord);
	ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot compute Scotch tree array");
      }

      children[3*tmp+index] = numbloc;
    }

    leafnum = 0;
    zz->Order.nbr_blocks = Zoltan_Scotch_Construct_Offset(&zz->Order, children, root, size, tree, 0, &leafnum);
    zz->Order.leaves[leafnum] =-1;
    zz->Order.nbr_leaves = leafnum;

    for (numbloc = 0, start=0 ; numbloc < zz->Order.nbr_blocks ; ++numbloc) {
      int tmp;
      tmp = zz->Order.start[numbloc];
      zz->Order.start[numbloc]  = start;
      start += tmp;
      if (zz->Order.ancestor[numbloc] >= 0)
	zz->Order.ancestor[numbloc] = size[zz->Order.ancestor[numbloc]];
    }
    zz->Order.start[zz->Order.nbr_blocks]  = start;

    /* Free temporary tables */
    ZOLTAN_FREE(&tree);
    ZOLTAN_FREE(&size);
    ZOLTAN_FREE(&children);
  }

  ierr = Zoltan_Postprocess_Graph (zz, gids, lids, &gr, NULL, NULL, NULL, &ord, NULL);

  /* Get a time here */
  if (get_times) times[3] = Zoltan_Time(zz->Timer);

  if (gr.graph_type==GLOBAL_GRAPH) {
    SCOTCH_dgraphOrderExit (&grafdat, &ordedat);
    SCOTCH_dgraphExit (&grafdat);
  }
  else {
    SCOTCH_graphExit (&cgrafdat);
  }
  SCOTCH_stratExit (&stradat);


  if (get_times) Zoltan_Third_DisplayTime(zz, times);

  if (use_timers)
    ZOLTAN_TIMER_STOP(zz->ZTime, timer_p, zz->Communicator);

  if ((ord.iperm != NULL) && (iperm != NULL))
    memcpy(iperm, ord.iperm, gr.num_obj*sizeof(indextype));
  if (ord.iperm != NULL)  ZOLTAN_FREE(&ord.iperm);
  if (order_opt->return_args&RETURN_RANK)
    memcpy(rank, ord.rank, gr.num_obj*sizeof(indextype));
  ZOLTAN_FREE(&ord.rank);
  ZOLTAN_FREE(&strat);

  /* Free all other "graph" stuff */
  Zoltan_Third_Exit(&gr, NULL, NULL, NULL, NULL, NULL);

  ZOLTAN_TRACE_EXIT(zz, yo);

  return (ZOLTAN_OK);
}


static int
Zoltan_Scotch_Construct_Offset(ZOS *order, indextype *children, int root,
			       indextype* size, indextype* tree, int offset, int *leafnum)
{
  int i = 0;
  int childrensize = 0;


  for (i=0 ; i < 2 ; i++) {
    if (children[3*root+i] < 0)
      break;

    childrensize += size[children[3*root+i]];
    offset = Zoltan_Scotch_Construct_Offset(order, children, children[3*root+i], size, tree, offset, leafnum);
  }


  order->start[offset] = size[root] - childrensize;
  order->ancestor[offset] = tree[root];
  size[root] = offset; /* size[root] not used now, can be use to convert indices */
  if (childrensize == 0)  { /* Leaf */
    order->leaves[*leafnum] = offset;
    (*leafnum)++;
  }
  ++offset;
  return (offset);
}



  /**********************************************************/
  /* Interface routine for PT-Scotch: Partitioning          */
  /**********************************************************/

int Zoltan_Scotch(
		    ZZ *zz,               /* Zoltan structure */
		    float *part_sizes,    /* Input:  Array of size zz->Num_Global_Parts
					     containing the percentage of work to be
					     assigned to each partition.               */
		    int *num_imp,         /* number of objects to be imported */
		    ZOLTAN_ID_PTR *imp_gids,  /* global ids of objects to be imported */
		    ZOLTAN_ID_PTR *imp_lids,  /* local  ids of objects to be imported */
		    int **imp_procs,      /* list of processors to import from */
		    int **imp_to_part,    /* list of partitions to which imported objects are
					     assigned.  */
		    int *num_exp,         /* number of objects to be exported */
		    ZOLTAN_ID_PTR *exp_gids,  /* global ids of objects to be exported */
		    ZOLTAN_ID_PTR *exp_lids,  /* local  ids of objects to be exported */
		    int **exp_procs,      /* list of processors to export to */
		    int **exp_to_part     /* list of partitions to which exported objects are
					     assigned. */
		    )
{
  char *yo = "Zoltan_Scotch";
  int ierr;
  ZOLTAN_Third_Graph gr;
  ZOLTAN_Third_Geom  *geo = NULL;
  ZOLTAN_Third_Vsize vsp;
  ZOLTAN_Third_Part  prt;
  ZOLTAN_Output_Part part;

  ZOLTAN_ID_PTR global_ids = NULL;
  ZOLTAN_ID_PTR local_ids = NULL;

  SCOTCH_Strat        stradat;
  SCOTCH_Dgraph       grafdat;

  int use_timers = 0;
  int timer_p = -1;
  int get_times = 0;
  double times[5];

  char alg[MAX_PARAM_STRING_LEN+1];
  char *strat = NULL;
  int edgelocnbr = 0;

  int i;
  float *imb_tols;
  int  ncon;
  int edgecut;
  int wgtflag;
  int   numflag = 0;
  int num_part = zz->LB.Num_Global_Parts;    /* passed to PT-Scotch. Don't             */
  MPI_Comm comm = zz->Communicator;          /* want to risk letting external packages */
                                             /* change our zz struct.                  */


#ifndef ZOLTAN_SCOTCH
  ZOLTAN_PRINT_ERROR(zz->Proc, __func__,
		     "Scotch requested but not compiled into library.");
  return ZOLTAN_FATAL;

#endif /* ZOLTAN_SCOTCH */

  ZOLTAN_TRACE_ENTER(zz, yo);

  Zoltan_Third_Init(&gr, &prt, &vsp, &part,
		    imp_gids, imp_lids, imp_procs, imp_to_part,
		    exp_gids, exp_lids, exp_procs, exp_to_part);

  prt.input_part_sizes = prt.part_sizes = part_sizes;

  strcpy (alg, "RBISECT");
  ierr = Zoltan_Scotch_Bind_Param(zz, alg, &strat);

  timer_p = Zoltan_Preprocess_Timer(zz, &use_timers);

    /* Start timer */
  get_times = (zz->Debug_Level >= ZOLTAN_DEBUG_ATIME);
  if (get_times){
    MPI_Barrier(zz->Communicator);
    times[0] = Zoltan_Time(zz->Timer);
  }

  /* TODO : take care about multidimensional weights */

  ierr = Zoltan_Preprocess_Graph(zz, &global_ids, &local_ids,  &gr, geo, &prt, &vsp);

  /* Get a time here */
  if (get_times) times[1] = Zoltan_Time(zz->Timer);

  wgtflag = 2*(gr.obj_wgt_dim>0) + (gr.edge_wgt_dim>0);
  ncon = (gr.obj_wgt_dim > 0 ? gr.obj_wgt_dim : 1);


  if (SCOTCH_dgraphInit (&grafdat, comm) != 0) {
    Zoltan_Third_Exit(&gr, NULL, &prt, &vsp, NULL, NULL);
    ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot initialize Scotch graph.");
  }

  edgelocnbr =  gr.xadj[gr.num_obj];
  if (SCOTCH_dgraphBuild (&grafdat, 0, gr.num_obj, gr.num_obj, gr.xadj, gr.xadj + 1,
			  gr.vwgt, NULL,edgelocnbr, edgelocnbr, gr.adjncy, NULL, gr.ewgts) != 0) {
    Zoltan_Third_Exit(&gr, NULL, &prt, &vsp, NULL, NULL);
    ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Cannot construct Scotch graph.");
  }

  SCOTCH_stratInit (&stradat);
  if (strat != NULL) {
    if (SCOTCH_stratDgraphOrder (&stradat, strat) != 0) {
      Zoltan_Third_Exit(&gr, NULL, &prt, &vsp, NULL, NULL);
      ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL, "Invalid Scotch strat.");
    }
  }


  if (!prt.part_sizes){
    Zoltan_Third_Exit(&gr, NULL, &prt, &vsp, NULL, NULL);
    ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL,"Input parameter part_sizes is NULL.");
  }

  ZOLTAN_TRACE_DETAIL(zz, yo, "Calling the PT-Scotch library");
  if (SCOTCH_dgraphPart (&grafdat, num_part, &stradat, prt.part) != 0) {
    Zoltan_Third_Exit(&gr, NULL, &prt, &vsp, NULL, NULL);
    ZOLTAN_THIRD_ERROR(ZOLTAN_FATAL,"PT-Scotch partitioning internal error.");
  }
  ZOLTAN_TRACE_DETAIL(zz, yo, "Returned from the PT-Scotch library");

  /* Get a time here */
  if (get_times) times[2] = Zoltan_Time(zz->Timer);

  ierr = Zoltan_Postprocess_Graph(zz, global_ids, local_ids, &gr, geo, &prt, &vsp, NULL, &part);

  Zoltan_Third_Export_User(&part, num_imp, imp_gids, imp_lids, imp_procs, imp_to_part,
			   num_exp, exp_gids, exp_lids, exp_procs, exp_to_part);

  /* Get a time here */
  if (get_times) times[3] = Zoltan_Time(zz->Timer);

  if (get_times) Zoltan_Third_DisplayTime(zz, times);

  if (use_timers && timer_p >= 0)
    ZOLTAN_TIMER_STOP(zz->ZTime, timer_p, zz->Communicator);

  if (gr.final_output) {
    ierr = Zoltan_Postprocess_FinalOutput (zz, &gr, &prt, &vsp,
					   use_timers, 0);
  }

  Zoltan_Third_Exit(&gr, NULL, &prt, &vsp, NULL, NULL);
  ZOLTAN_FREE(&global_ids);
  ZOLTAN_FREE(&local_ids);

  ZOLTAN_TRACE_EXIT(zz, yo);

  return (ierr);
}


/************************
 * Auxiliary function used to parse scotch specific parameters
 ***************************************/

static int Zoltan_Scotch_Bind_Param(ZZ* zz, char *alg, char **strat)
{
  char stratsmall[MAX_PARAM_STRING_LEN+1];
  char stratfilename[MAX_PARAM_STRING_LEN+1];

  *strat = NULL;
  stratsmall[0] = stratfilename[0] = '\0';
  Zoltan_Bind_Param(Scotch_params, "SCOTCH_METHOD",
		    (void *) alg);
  Zoltan_Bind_Param(Scotch_params, "SCOTCH_STRAT",
		    (void *) stratsmall);
  Zoltan_Bind_Param(Scotch_params, "SCOTCH_STRAT_FILE",
		    (void *) stratfilename);
  Zoltan_Assign_Param_Vals(zz->Params, Scotch_params, zz->Debug_Level,
			   zz->Proc, zz->Debug_Proc);

  if ((strlen(stratsmall) > 0) && (strlen(stratfilename) > 0)) {
    ZOLTAN_THIRD_ERROR(ZOLTAN_WARN,
		       "SCOTCH_STRAT and SCOTCH_STRAT_FILE both defined: ignoring\n");
  }

  if (strlen(stratsmall) > 0) {
    *strat = (char *) ZOLTAN_MALLOC((strlen(stratsmall)+1)*sizeof(char));
    strcpy (*strat, stratsmall);
    return (ZOLTAN_OK);
  }

  if (strlen(stratfilename) > 0) {
    long size;
    FILE *stratfile;

    stratfile = fopen(stratfilename, "r");
    if (stratfile == NULL) {
      ZOLTAN_THIRD_ERROR(ZOLTAN_WARN,
			 "Cannot open Scotch strategy file\n");
    }

    fseek(stratfile, (long)0, SEEK_END);
    size = ftell(stratfile);
    *strat = (char *) ZOLTAN_MALLOC((size+2)*sizeof(char));
    if (*strat == NULL) {
      ZOLTAN_THIRD_ERROR(ZOLTAN_MEMERR, "Out of memory.");
    }
    fseek(stratfile, (long)0, SEEK_SET);
    fgets (*strat, size+1, stratfile);
    fclose(stratfile);


    return (ZOLTAN_OK);
  }

  return (ZOLTAN_OK);
}


/*********************************************************************/
/* Scotch parameter routine                                          */
/*********************************************************************/

int Zoltan_Scotch_Set_Param(
char *name,                     /* name of variable */
char *val)                      /* value of variable */
{
  int status, i;
  PARAM_UTYPE result;         /* value returned from Check_Param */
  int index;                  /* index returned from Check_Param */
  char *valid_methods[] = {
    "NODEND", /* for nested dissection ordering */
    NULL };

  status = Zoltan_Check_Param(name, val, Scotch_params, &result, &index);
  if (status == 0){
    /* OK so far, do sanity check of parameter values */

    if (strcmp(name, "SCOTCH_METHOD") == 0){
      status = 2;
      for (i=0; valid_methods[i] != NULL; i++){
	if (strcmp(val, valid_methods[i]) == 0){
	  status = 0;
	  break;
	}
      }
    }
  }
  return(status);
}

#ifdef __cplusplus
}
#endif

