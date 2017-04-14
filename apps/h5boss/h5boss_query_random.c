#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>

#define ENABLE_MPI 1

#ifdef ENABLE_MPI
#include "mpi.h"
#endif

#include "pdc.h"
#include "pdc_client_server_common.h"
#include "pdc_client_connect.h"


void print_usage() {
    printf("Usage: srun -n ./h5boss_query_random /path/to/pm_list.txt n_query\n");
}

int main(int argc, char **argv)
{
    int rank = 0, size = 1;

#ifdef ENABLE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

    const char *pm_filename = argv[1];

    if (argc < 3 || pm_filename == NULL) {
        print_usage();
        exit(-1);
    }

    int n_query, my_query;
    n_query = atoi(argv[2]);

    int i, j, count, my_count;
    int plate[3000], mjd[3000];
    int my_plate[3000], my_mjd[3000];
    int *plate_ptr, *mjd_ptr;

    // Rank 0 read from pm file
    if (rank == 0) {
        printf("Reading from %s\n", pm_filename);
        FILE *pm_file = fopen(pm_filename, "r");

        if (NULL == pm_file) {
            fprintf(stderr, "Error opening file: %s\n", pm_filename);
            return (-1);
        }

        i = 0;
        while (EOF != fscanf(pm_file, "%d %d", &plate[i], &mjd[i])) {
            /* printf("%d, %d\n", plate[i], mjd[i]); */
            i++;
        }

        fclose(pm_file);
        count = i;
    }

    

    my_query  = n_query;
    my_count  = count;
    plate_ptr = plate;
    mjd_ptr   = mjd;

#ifdef ENABLE_MPI
    MPI_Bcast( &count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (size > count) {
        if (rank == 0) {
            printf("Number of process is more than plate-mjd combination, exiting...\n");
        }
        MPI_Finalize();
        exit (-1);
    }

    // Distribute work evenly
    my_count = count / size;
    my_query = n_query/ size;

    // Last rank may have extra work
    if (rank == size - 1) {
        my_count += count % size;
        my_query += n_query % size;
    }

    /* int *sendcount = (int*)malloc(sizeof(int)* size); */
    /* int *displs = (int*)malloc(sizeof(int)* size); */
    /* for (i = 0; i < size; i++) { */
    /*     sendcount[i] = count / size; */
    /*     if (i == size - 1) */ 
    /*         sendcount[i] += count % size; */
    /*     displs[i] = i * (count / size); */
    /* } */
    
    MPI_Bcast( &plate, count, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast( &mjd  , count, MPI_INT, 0, MPI_COMM_WORLD);
    /* MPI_Scatterv(plate, sendcount, displs, MPI_INT, my_plate, my_count, MPI_INT, 0, MPI_COMM_WORLD); */
    /* MPI_Scatterv(mjd,   sendcount, displs, MPI_INT, my_mjd,   my_count, MPI_INT, 0, MPI_COMM_WORLD); */
    plate_ptr = plate;
    mjd_ptr   = mjd;
    /* MPI_Barrier(MPI_COMM_WORLD); */
    /* free(displs); */
    /* free(sendcount); */
#endif 

    /* printf("%d: myquery = %d\n", rank, my_query); */


    PDC_prop_t p;
    pdcid_t pdc = PDC_init(p);

    pdcid_t cont_prop = PDCprop_create(PDC_CONT_CREATE, pdc);
    if(cont_prop <= 0)
        printf("Fail to create container property @ line  %d!\n", __LINE__);

    pdcid_t cont = PDCcont_create(pdc, "c1", cont_prop);
    if(cont <= 0)
        printf("Fail to create container @ line  %d!\n", __LINE__);

    pdcid_t obj_prop = PDCprop_create(PDC_OBJ_CREATE, pdc);
    if(obj_prop <= 0)
        printf("Fail to create object property @ line  %d!\n", __LINE__);

    char data_loc[128];
    char obj_name[128];
    uint64_t dims[2] = {1000, 2};
    PDCprop_set_obj_dims(obj_prop, 2, dims, pdc);

    pdcid_t test_obj = -1;

    struct timeval  ht_total_start;
    struct timeval  ht_total_end;
    struct timeval  ht_query_start;
    struct timeval  ht_query_end;
    struct timeval  ht_update_start;
    struct timeval  ht_update_end;
    struct timeval  ht_query_tag_start;
    struct timeval  ht_query_tag_end;
    long long ht_total_elapsed;
    double ht_total_sec = 0.0;
    double ht_query_sec = 0.0;
    double ht_query_tag_sec = 0.0;
    double ht_update_sec = 0.0;


#ifdef ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    if (rank == 0) {
        printf("Starting to query...\n");
    }

    pdc_metadata_t *res = NULL;
    pdc_metadata_t a;
    a.user_id              = 0;
    a.time_step            = 0;
    a.obj_id               = 0;
    a.ndim                 = 0;
    a.create_time          = 0;
    a.last_modified_time   = 0;
    a.app_name[0]          = 0;
    a.obj_name[0]          = 0;
    a.data_location[0]     = 0;

    /* sprintf(&a.app_name[0], "%s", "New App Name"); */
    sprintf(&a.tags[0], "select%d", n_query);

    srand(rank); 

    gettimeofday(&ht_total_start, 0);

    int n_fiber = 1000;

    for (i = 0; i < my_query; i++) {
        int r = rand() % count;
        int f = rand() % n_fiber + 1;

            sprintf(obj_name, "%d-%d-%d", plate_ptr[r], mjd_ptr[r], f);
            /* printf("%d: querying %s\n", rank, obj_name); */

            gettimeofday(&ht_query_start, 0);

            // Retrieve metdata object with name
            PDC_Client_query_metadata_name_timestep(obj_name, 0, &res);

            gettimeofday(&ht_query_end, 0);
            ht_query_sec += ( (ht_query_end.tv_sec-ht_query_start.tv_sec)*1000000LL + 
                              ht_query_end.tv_usec-ht_query_start.tv_usec ) / 1000000.0;

            if (res == NULL) {
                printf("%d: No result found for current query with name [%s]\n", rank, obj_name);
                exit(-1);
            }
            /* else { */
            /*     PDC_print_metadata(res); */
            /* } */

            gettimeofday(&ht_update_start, 0);

            // Update retrieved metadata
            /* printf("Adding the tag\n"); */
            PDC_Client_update_metadata(res, &a);

            gettimeofday(&ht_update_end, 0);
            ht_update_sec += ( (ht_update_end.tv_sec-ht_update_start.tv_sec)*1000000LL + 
                              ht_update_end.tv_usec-ht_update_start.tv_usec ) / 1000000.0;

            // Query update metadata and print
            /* printf("Getting updated metadata\n"); */
            /* PDC_Client_query_metadata_name_timestep(obj_name, 0, &res); */
            /* if (res != NULL) { */
            /*     PDC_print_metadata(res); */
            /* } */

        // Print progress
        /* int progress_factor = my_count < 10 ? 1 : 10; */
        /* if (i > 0 && i % (my_count/progress_factor) == 0) { */
        /*     gettimeofday(&ht_total_end, 0); */
        /*     ht_total_elapsed    = (ht_total_end.tv_sec-ht_total_start.tv_sec)*1000000LL + ht_total_end.tv_usec-ht_total_start.tv_usec; */
        /*     ht_total_sec        = ht_total_elapsed / 1000000.0; */
        /*     if (rank == 0) { */
        /*         printf("%10d created ... %.4f s\n", i * size * j, ht_total_sec); */
        /*         fflush(stdout); */
        /*     } */
        /* } */

    }
    fflush(stdout);
#ifdef ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    gettimeofday(&ht_total_end, 0);
    ht_total_elapsed    = (ht_total_end.tv_sec-ht_total_start.tv_sec)*1000000LL + ht_total_end.tv_usec-ht_total_start.tv_usec;
    ht_total_sec        = ht_total_elapsed / 1000000.0;
    if (rank == 0) {
        printf("Time to retrieve %d metadata objects with %d ranks: %.6f\n", n_query, size, ht_query_sec);
        printf("Time to update   %d metadata objects with %d ranks: %.6f\n", n_query, size, ht_update_sec);
        printf("Total time: %.6f\n", ht_total_sec);
        fflush(stdout);
    }

    int n_res;
    pdc_metadata_t **res_arr;
    if (rank == 0) {
        gettimeofday(&ht_query_tag_start, 0);

        PDC_partial_query(0 , -1, NULL, NULL, -1, -1, -1, a.tags, &n_res, &res_arr);

        gettimeofday(&ht_query_tag_end, 0);
        ht_query_tag_sec += ( (ht_query_tag_end.tv_sec-ht_query_tag_start.tv_sec)*1000000LL + 
                          ht_query_tag_end.tv_usec-ht_query_tag_start.tv_usec ) / 1000000.0;
        printf("Received %d metadata objects with query tag: %s\n, time: %.6f", n_res, a.tags, ht_query_tag_sec);
    }

done:
    if(PDCcont_close(cont, pdc) < 0)
        printf("fail to close container %lld\n", cont);

    if(PDCprop_close(cont_prop, pdc) < 0)
        printf("Fail to close property @ line %d\n", __LINE__);

    if(PDC_close(pdc) < 0)
        printf("fail to close PDC\n");

#ifdef ENABLE_MPI
    MPI_Finalize();
#endif

    return 0;
}