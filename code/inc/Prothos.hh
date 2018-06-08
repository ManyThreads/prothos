#pragma once

#include "Task.hh"

namespace Prothos{

void prothos_init();
void prothos_schedule_task(Task *t);
void prothos_finalize();

} //Prothos
