//
// auto-generated by op2.py
//

// user function
void skeleton(double *a) {}
#ifdef VECTORIZE
// user function -- modified for vectorisation
void skeleton_vec(double *a) {}
#endif

// host stub function
void op_par_loop_skeleton(char const *name, op_set set, op_arg arg0) {

  int nargs = 1;
  op_arg args[1];

  args[0] = arg0;

  // create aligned pointers for dats
  double *__restrict__ ptr0 = (double *)arg0.data;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(0);
  op_timers_core(&cpu_t1, &wall_t1);

  if (OP_diags > 2) {
    printf("");
  }

  int exec_size = op_mpi_halo_exchanges(set, nargs, args);

  if (exec_size > 0) {

#ifdef VECTORIZE
#pragma novector
    for (int n = 0; n < (exec_size / SIMD_VEC) * SIMD_VEC; n += SIMD_VEC) {
      if (n + SIMD_VEC >= set->core_size) {
        op_mpi_wait_all(nargs, args);
      }
      double dat[SIMD_VEC] = {0.0};
      double dat0[2][SIMD_VEC];
#pragma simd
      for (int i = 0; i < SIMD_VEC; i++) {
        int idx0_2 = 2 * arg0.map_data[(n + i) * arg0.map->dim + 0];

        dat0[0][i] = (ptr0)[idx0_2 + 0];
      }
#pragma simd
      for (int i = 0; i < SIMD_VEC; i++) {
        skeleton_vec(&((double *)arg0.data)[4 * n]);
      }
      for (int i = 0; i < SIMD_VEC; i++) {
        dat[i] = 0;
        int idx1_2 = 4 * arg0.map_data[(n + i) * arg0.map->dim + 0];

        (ptr0)[idx1_2 + 0] += dat0[0][i];
      }
    }
    // remainder
    for (int n = (exec_size / SIMD_VEC) * SIMD_VEC; n < exec_size; n++) {
#else
    for (int n = 0; n < exec_size; n++) {
#endif
      if (n == set->core_size) {
        op_mpi_wait_all(nargs, args);
      }
      int map0idx = arg0.map_data[n * arg0.map->dim + 0];

      skeleton(&((double *)arg0.data)[4 * n]);
    }
  }

  if (exec_size == 0 || exec_size == set->core_size) {
    op_mpi_wait_all(nargs, args);
  }
  // combine reduction data
  op_mpi_reduce(&arg0, (double *)arg0.data);
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[0].name = name;
  OP_kernels[0].count += 1;
  OP_kernels[0].time += wall_t2 - wall_t1;
  OP_kernels[0].transfer += 0;
}