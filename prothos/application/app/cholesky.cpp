/*
    Copyright (c) 2005-2018 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

#include <string>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <vector>
#include <map>

#include <mkl_lapack.h>
#include <mkl.h>

#include <thread>

#include "runtime/Task.hh"
#include "runtime/FlowGraph.hh"

using namespace Prothos;

// Application command line arguments parsing
//#include "../../common/utility/utility.h"

/************************************************************
 FORWARD DECLARATIONS
************************************************************/

/**********************************************
 Read or generate a positive-definite matrix
 -- reads from file if fname != NULL
     -- sets n to matrix size
     -- allocates and reads values in to A
 -- otherwise generates a matrix
     -- uses n to determine size
     -- allocates and generates values in to A
**********************************************/
void matrix_init( double * &A, int &n, const char *fname );

/**********************************************
 Writes a lower triangular matrix to a file
 -- first line of file is n 
 -- subsequently 1 row per line
**********************************************/
void matrix_write ( double *A, int n, const char *fname, bool is_triangular = false );

/************************************************************
 GLOBAL VARIABLES
************************************************************/
bool g_benchmark_run = false;
int g_num_tbb_threads = std::thread::hardware_concurrency();
int g_n = -1, g_b = -1, g_num_trials = 1;
char *g_input_file_name = NULL;
char *g_output_prefix = NULL;
std::string g_alg_name;

// Creates tiled array
static double ***create_tile_array( double *A, int n, int b ) {
    const int p = n/b;
    double ***tile = (double ***)calloc( sizeof( double ** ), p );

    for ( int j = 0; j < p; ++j ) {
        tile[j] = (double **)calloc( sizeof( double * ), p );
    }

    for ( int j = 0; j < p; ++j ) {
        for ( int i = 0; i < p; ++i ) {
            double *temp_block = (double *)calloc( sizeof( double ), b*b );

            for ( int A_j = j*b, T_j = 0; T_j < b; ++A_j, ++T_j ) {
                for ( int A_i = i*b, T_i = 0; T_i < b; ++A_i, ++T_i ) {
                    temp_block[T_j*b+T_i] = A[A_j*n+A_i];
                }
            }

            tile[j][i] = temp_block;
        }
    }
    return tile;
}

static void collapse_tile_array( double ***tile, double *A, int n, int b ) {
    const int p = n/b;

    for ( int j = 0; j < p; ++j ) {
        for ( int i = 0; i < p; ++i ) {
            double *temp_block = tile[j][i];

            for ( int A_j = j*b, T_j = 0; T_j < b; ++A_j, ++T_j ) {
                for ( int A_i = i*b, T_i = 0; T_i < b; ++A_i, ++T_i ) {
                    A[A_j*n+A_i] = temp_block[T_j*b+T_i];
                }
            }

            free( temp_block );
            tile[j][i] = NULL;
        }

        free( tile[j] );
    }

    free( tile );
}

/************************************************************
 Helper base class: algorithm
************************************************************/
class algorithm {

    std::string name;
    bool is_tiled;

    bool check_if_valid( double *A0, double *C, double *A, int n ) {
        char transa = 'n', transb = 't';
        double alpha = 1;
        double beta = 0;

        for ( int i = 0; i < n; ++i ) {
            for ( int j = i+1; j < n; ++j ) {
                A0[j*n+i] = 0.;
            }
        }

        dgemm ( &transa, &transb, &n, &n, &n, &alpha, A0, &n, A0, &n, &beta, C, &n );

        for ( int j = 0; j < n; ++j ) {
            for ( int i = 0; i < n; ++i ) {
                const double epsilon = std::abs( A[j*n+i]*0.1 );

                if ( std::abs( C[j*n+i] - A[j*n+i] ) > epsilon ) {
                    printf( "ERROR: %s did not validate at C(%d,%d) = %lf != A(%d,%d) = %lf\n",
                        name.c_str(), i, j, C[j*n+i], i, j, A[j*n+i] );
                    printf( "ERROR: %g; %g < %g < %g\n", epsilon, A[j*n+i] - epsilon, C[j*n+i], A[j*n+i] + epsilon );
                    return false;
                }
            }
        }
        return true;
    }

public:
    algorithm( const std::string& alg_name, bool t ) : name(alg_name), is_tiled(t) {}

    double operator() ( double *A, int n, int b, int trials ) {
        //tbb::tick_count t0, t1;
        double elapsed_time = 0.0;
        double *A0 = (double *)calloc( sizeof( double ), n*n );
        double *C = (double *)calloc( sizeof( double ), n*n );

        for ( int t = 0; t < trials+1; ++t ) {
            if ( is_tiled ) {
                double ***tile = create_tile_array( A, n, b );
                //t0 = tbb::tick_count::now();
                func( tile, n, b );
                //t1 = tbb::tick_count::now();

                collapse_tile_array( tile, A0, n, b );
            }
            else {
                memcpy( A0, A, sizeof( double )*n*n );
                //t0 = tbb::tick_count::now();
                func( A0, n, b );
                //t1 = tbb::tick_count::now();
            }

            //if ( t ) elapsed_time += (t1-t0).seconds();

            if( !g_benchmark_run && !check_if_valid( A0, C, A, n ) ) {
                if ( g_output_prefix ) {
                    std::string s( g_output_prefix );
                    s += "_" + name + ".txt";
                    matrix_write( A0, g_n, s.c_str(), true );
                    free( A0 );
                    free( C );
                    return 0.;
                }
            }
        }

        if ( g_output_prefix ) {
            std::string s( g_output_prefix );
            s += "_" + name + ".txt";
            matrix_write( A0, g_n, s.c_str(), true );
        }

        //printf( "%s %d %d %d %d %lf %lf\n", name.c_str(), g_num_tbb_threads, trials, n, b, elapsed_time, elapsed_time/trials );
        free( A0 );
        free( C );
        return 0; //elapsed_time;
    }

protected:
    // Main algorithm body function must be defined in any direved class
    virtual void func( void * ptr, int n, int b ) = 0;
};

/***********************************************************/

static void call_dpotf2( double ***tile, int b, int k ) {
    double *A_block = tile[k][k];
    char uplo = 'l';
    int info = 0;
    dpotf2( &uplo, &b, A_block, &b, &info ); 
    return;
}

static void call_dtrsm( double ***tile, int b, int k, int j ) {
    double *A_block = tile[k][j];
    double *L_block = tile[k][k];
    char uplo = 'l', side = 'r', transa = 't', diag = 'n';
    double alpha = 1;
    dtrsm( &side, &uplo, &transa, &diag, &b, &b, &alpha, L_block, &b, A_block, &b );
    return;
}

static void call_dsyr2k( double ***tile, int b, int k, int j, int i ) {
    double *A_block = tile[i][j];
    char transa = 'n', transb = 't';
    char uplo = 'l';
    double alpha = -1;
    double beta = 1;

    if ( i == j ) {   // Diagonal block
        double *L_block = tile[k][i];
        dsyrk( &uplo, &transa, &b, &b, &alpha, L_block, &b, &beta, A_block, &b );
    } else {   // Non-diagonal block
        double *L2_block = tile[k][i];
        double *L1_block = tile[k][j];
        dgemm( &transa, &transb, &b, &b, &b, &alpha, L1_block, &b, L2_block, &b, &beta, A_block, &b );
    }
    return;
}

class algorithm_crout : public algorithm
{
public:
    algorithm_crout() : algorithm("crout_cholesky", true) {}

protected:
    virtual void func( void * ptr, int n, int b ) {
        double ***tile = (double ***)ptr;
        const int p = n/b;

        for ( int k = 0; k < p; ++k ) {
            call_dpotf2( tile, b, k );

            for ( int j = k+1; j < p; ++j ) {
                call_dtrsm( tile, b, k, j );

                for ( int i = k+1; i <= j; ++i ) {
                    call_dsyr2k( tile, b, k, j, i );
                }
            }
        }
    }
};

class algorithm_dpotrf : public algorithm
{
public:
    algorithm_dpotrf() : algorithm("dpotrf_cholesky", false) {}

protected:
    virtual void func( void * ptr, int n, int /* b */ ) {
        double *A = (double *)ptr;
        int lda = n;
        int info = 0;
        char uplo = 'l';
        dpotrf( &uplo, &n, A, &lda, &info );
    }
};

/************************************************************
 Begin dependence graph based version of cholesky
************************************************************/

typedef FlowGraph::FunctionNode<int, int> continue_type;
typedef continue_type * continue_ptr_type;

class algorithm_depend : public algorithm
{
public:
    algorithm_depend() : algorithm("depend_cholesky", true) {}

protected:
    virtual void func( void * ptr, int n, int b ) {
        double ***tile = (double ***)ptr;
        int counter = 0;
        int counter2 = 0;
        const int p = n/b;
        continue_ptr_type *c = new continue_ptr_type[p];
        continue_ptr_type **t = new continue_ptr_type *[p];
        continue_ptr_type ***u = new continue_ptr_type **[p];

        FlowGraph::Graph g;
        for ( int k = p-1; k >= 0; --k ) {
            counter++;
            c[k] = new continue_type( g,
                [=]( const int & ) { call_dpotf2( tile, b, k ); return 0;} );
            t[k] = new continue_ptr_type[p];
            u[k] = new continue_ptr_type *[p];

            for ( int j = k+1; j < p; ++j ) {
                counter++;
                t[k][j] = new continue_type( g,
                    [=]( const int & ) { call_dtrsm( tile, b, k, j ); return 0; } );
                FlowGraph::makeEdge( *c[k], *t[k][j] );
                u[k][j] = new continue_ptr_type[p];

                for ( int i = k+1; i <= j; ++i ) {
                    counter++;
                    u[k][j][i] = new continue_type( g,
                        [=]( const int & ) { call_dsyr2k( tile, b, k, j, i ); return 0; } );

                    if ( k < p-2 && k+1 != j && k+1 != i ) {
                        counter2++;
                        FlowGraph::makeEdge( *u[k][j][i], *u[k+1][j][i] );
                    }
                    counter2++;
                    FlowGraph::makeEdge( *t[k][j], *u[k][j][i] );

                    if ( i != j ) {
                        counter2++;
                        FlowGraph::makeEdge( *t[k][i], *u[k][j][i] );
                    }

                    if ( k < p-2 && j > i && i == k+1 ) {
                        counter2++;
                        FlowGraph::makeEdge( *u[k][j][i], *t[i][j] );
                    }
                }
            }

            if ( k != p-1 ) {
                counter2++;
                FlowGraph::makeEdge( *u[k][k+1][k+1], *c[k+1] );
            }
        }

        struct Trigger{
			Trigger() {}

			bool operator()(int &i){
				return false;
			}
		};

        auto source = new FlowGraph::SourceNode<int>(g, Trigger());
        FlowGraph::makeEdge(*source, *c[0]);
        source->activate();
        printf( "Nodes: %d, Edges: %d\n", counter, counter2 );
    }
}; // class algorithm_depend

/************************************************************
 End dependence graph based version of cholesky
************************************************************/

bool process_args( int argc, char *argv[] ) {
    // utility::parse_cli_arguments( argc, argv,
    //     utility::cli_argument_pack()
    //     //"-h" option for displaying help is present implicitly
    //     .positional_arg( g_n, "size", "the row/column size of NxN matrix (size <= 46000)" )
    //     .positional_arg( g_b, "blocksize", "the block size; size must be a multiple of the blocksize" )
    //     .positional_arg( g_num_trials, "num_trials", "the number of times to run each algorithm" )
    //     .positional_arg( g_output_prefix, "output_prefix",
    //         "if provided the prefix will be preappended to output files:\n"
    //         "                     output_prefix_posdef.txt\n"
    //         "                     output_prefix_X.txt; where X is the algorithm used\n"
    //         "                 if output_prefix is not provided, no output will be written" )
    //     .positional_arg( g_alg_name, "algorithm", "name of the used algorithm - can be dpotrf, crout, depend or join" )
    //     .positional_arg( g_num_tbb_threads, "num_tbb_threads", "number of started TBB threads" )

    //     .arg( g_input_file_name, "input_file", "if provided it will be read to get the input matrix" )
    //     .arg( g_benchmark_run, "-x", "skips all validation" )
    // );

    // if ( g_n > 46000 ) {
    //     printf( "ERROR: invalid 'size' value (must be less or equal 46000): %d\n", g_n );
    //     return false;
    // }

    // if ( g_n%g_b != 0 ) {
    //     printf( "ERROR: size %d must be a multiple of the blocksize %d\n", g_n, g_b );
    //     return false;
    // }

    // if ( g_n/g_b > 256 ) {
    //     // Because tile index size is 1 byte only in tag_t type
    //     printf( "ERROR: size / blocksize must be less or equal 256, but %d / %d = %d\n", g_n, g_b, g_n/g_b );
    //     return false;
    // }

    // if ( g_b == -1 || (g_n == -1 && g_input_file_name == NULL) ) {
    //     return false;
    // }

    //Static initialization of arguments
    g_n = 2000;
    g_b = 10;
    g_num_trials = 1;
    g_alg_name = "depend";
    g_benchmark_run = true;

    return true;
}

int userMain(int argc, char *argv[]) {
    typedef std::map< std::string, algorithm * > algmap_t;
    algmap_t algmap;

    // Init algorithms
    //algmap.insert(std::pair<std::string, algorithm *>("dpotrf", new algorithm_dpotrf));
    //algmap.insert(std::pair<std::string, algorithm *>("crout", new algorithm_crout));
    //algmap.insert(std::pair<std::string, algorithm *>("depend", new algorithm_depend));
    //algmap.insert(std::pair<std::string, algorithm *>("join", new algorithm_join));

    if ( !process_args( argc, argv ) ) {
        printf( "ERROR: Invalid arguments. Run: %s -h\n", argv[0] );
        exit( 1 );
    }

    //tbb::task_scheduler_init init( g_num_tbb_threads );
    double *A = NULL;

    // Read input matrix
    matrix_init( A, g_n, g_input_file_name );

    // Write input matrix if output_prefix is set and we didn't read from a file
    if ( !g_input_file_name && g_output_prefix ) {
        std::string s( g_output_prefix );
        s += "_posdef.txt";
        matrix_write( A, g_n, s.c_str() );
    }
    auto alg = new algorithm_depend;
    Task* fgt = new UserTask( [&](){(*alg)( A, g_n, g_b, g_num_trials );});

    WorkerGroup* wg = new FixedWorkerGroup<1>;
	wg->start();
	wg->pushTask(fgt);
	wg->pushTask(new TerminationMarkerTask());
	wg->finalize();
	MLOG_INFO(mlog::app, "Good Bye!");

    free( A );
    return 0;
}
