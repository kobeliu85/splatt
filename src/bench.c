

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "base.h"
#include "bench.h"
#include "mttkrp.h"
#include "timer.h"
#include "thd_info.h"

#include <omp.h>
#include "io.h"



/******************************************************************************
 * PUBLIC FUNCTIONS
 *****************************************************************************/
void bench_splatt(
  sptensor_t * const tt,
  matrix_t ** mats,
  idx_t const niters,
  idx_t const * const threads,
  idx_t const nruns)
{
  sp_timer_t itertime;
  sp_timer_t modetime;

  /* add 64 bytes to avoid false sharing */
  thd_info * thds = thd_init(threads[nruns-1],
    mats[0]->J * sizeof(val_t) + 64);

  ftensor_t * ft = ften_alloc(tt);
  timer_start(&timers[TIMER_SPLATT]);
  printf("SPLATT ---------------------------------------------------------\n");

  /* for each # threads */
  for(idx_t t=0; t < nruns; ++t) {
    idx_t const nthreads = threads[t];
    omp_set_num_threads(nthreads);
    if(nruns > 1) {
      printf("## THREADS %u\n", nthreads);
    }

    for(idx_t i=0; i < niters; ++i) {
      timer_fstart(&itertime);
      /* time each mode */
      for(idx_t m=0; m < tt->nmodes; ++m) {
        timer_fstart(&modetime);
        mttkrp_splatt(ft, mats, m, thds, nthreads);
        timer_stop(&modetime);
        printf("  mode %u %0.3fs\n", m+1, modetime.seconds);
      }
      timer_stop(&itertime);
      printf("    its = %3u (%0.3fs)\n", i+1, itertime.seconds);
    }

    /* output load balance info */
    if(nruns > 1 || nthreads > 1) {
      thd_times(thds, nthreads);
      thd_reset(thds, nthreads);
      printf("\n");
    }
  }
  timer_stop(&timers[TIMER_SPLATT]);

  //mat_write(mats[0], NULL);

  thd_free(thds, threads[nruns-1]);
  ften_free(ft);
}

void bench_giga(
  sptensor_t * const tt,
  matrix_t ** mats,
  idx_t const niters,
  idx_t const * const threads,
  idx_t const nruns)
{
  printf("bench giga!\n");
  val_t * scratch = (val_t *) malloc(tt->nnz * sizeof(val_t));
  spmatrix_t * unfolds[MAX_NMODES];
  for(idx_t m=0; m < tt->nmodes; ++m) {
    unfolds[m] = tt_unfold(tt, m);
  }

  timer_start(&timers[TIMER_GIGA]);
  for(idx_t i=0; i < niters; ++i) {
    for(idx_t m=0; m < tt->nmodes; ++m) {
      mttkrp_giga(unfolds[m], mats, m, scratch);
    }
  }
  timer_stop(&timers[TIMER_GIGA]);

  for(idx_t m=0; m < tt->nmodes; ++m) {
    spmat_free(unfolds[m]);
  }
  free(scratch);
}

void bench_ttbox(
  sptensor_t * const tt,
  matrix_t ** mats,
  idx_t const niters,
  idx_t const * const threads,
  idx_t const nruns)
{
  printf("bench ttbox!\n");
  val_t * scratch = (val_t *) malloc(tt->nnz * sizeof(val_t));

  timer_start(&timers[TIMER_TTBOX]);
  for(idx_t i=0; i < niters; ++i) {
    for(idx_t m=0; m < tt->nmodes; ++m) {
      mttkrp_ttbox(tt, mats, m, scratch);
    }
  }
  timer_stop(&timers[TIMER_TTBOX]);

  free(scratch);
}

