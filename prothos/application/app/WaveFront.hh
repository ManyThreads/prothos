#pragma once

#include "runtime/mlog.hh"
#include "runtime/Task.hh"
#include "runtime/FlowGraph.hh"
#include <string>

using namespace Prothos;

const int M=1000;				  // matrix size
const int N=1000;   			  // matrix size
const int B = 100;                // block size calculated by each node
const int MB = (M/B) + (M%B>0);   // number of blocks in dim M
const int NB = (M/B) + (M%B>0);   // number of block sin dim N

// matrix calculation method
inline double calc( double v0, double v1 ) {
    if ( v0 == v1 )
        return 2*v0;
    else
        return std::max(v0,v1);
}

int waveMain() {

    Task* fgt = new UserTask( []() {
        FlowGraph::Graph g;
		
		// contrinue node matrix
		FlowGraph::ContinueNode<FlowGraph::ContinueMsg> * nodes[MB][NB];

		double **value = new double *[M]; // value matrix pointer
		for ( int i = 0; i < M; ++i ) value[i] = new double [N];

        // setup node graph
        value[M-1][N-1] = 0;
        for( int i=MB; --i>=0; )
            for( int j=NB; --j>=0; ) {
                nodes[i][j] =
                    new FlowGraph::ContinueNode<FlowGraph::ContinueMsg>( g,
                [=]( FlowGraph::ContinueMsg ) mutable {
				    MLOG_INFO(mlog::app, "starting block", i);
                    int start_i = i*B;
                    int end_i = (i*B+B > M) ? M : i*B+B;
                    int start_j = j*B;
                    int end_j = (j*B+B > N) ? N : j*B+B;

                    for ( int ii = start_i; ii < end_i; ++ii ) {
                        for ( int jj = start_j; jj < end_j; ++jj ) {
                            double v0 = ii == 0 ? 0 : value[ii-1][jj];
                            double v1 = jj == 0 ? 0 : value[ii][jj-1];
							if (ii==0 && jj==0) {
								value[ii][jj] = 1;
							} else {
								value[ii][jj] = calc(v0, v1);
							}
						}
				    MLOG_INFO(mlog::app, "block done", i);
				    return FlowGraph::ContinueMsg();
				    }
				});
				
                if ( i + 1 < MB ) {
				   MLOG_INFO(mlog::app, "making edge");
				   makeEdge( *nodes[i][j], *nodes[i+1][j] );
                }

				if ( j + 1 < NB ) {
					MLOG_INFO(mlog::app, "making edge");
					makeEdge( *nodes[i][j], *nodes[i][j+1] );
				}
            }

        auto trigger = new FlowGraph::SourceNode<FlowGraph::ContinueMsg>(g,
        []( FlowGraph::ContinueMsg& ) {
				static bool triggered = false;
				MLOG_INFO(mlog::app, "triggered");
				if (!triggered) {
						triggered = true;
						return true;
				} 
				return false;
        });

        makeEdge( *trigger, *nodes[0][0] );

        trigger->activate();
		// TODO how to concatenate strings correctly here
        //MLOG_INFO(mlog::app, "calculation result is: " + std::to_string(value[M-1][N-1]));

    });

    UserTask* t0 = new UserTask([]() {
        MLOG_INFO(mlog::app, "UserTask t0 start");
        for(int i = 0; i < 5; i++) {
            (new MsgDagTask(0,"Hello"))->addSucc(new MsgDagTask(1," World"));
        }
    });
    
    WorkerGroup* wg = new FixedWorkerGroup<3>;
	wg->start();
	wg->pushTask(t0);
	wg->pushTask(fgt);
	wg->pushTask(new TerminationMarkerTask());
	wg->finalize();
	MLOG_INFO(mlog::app, "Good Bye!");
    return 0;

} // waveMain
