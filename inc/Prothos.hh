#pragma once

#include "Task.hh"
#include "DAG.hh"
#include "Worker.hh"
#include "Singleton.hh"
#include "FlowGraph.hh"

namespace Prothos{

void prothos_init();
//void prothos_push_task(Task *t);
void prothos_finalize();

} //Prothos
