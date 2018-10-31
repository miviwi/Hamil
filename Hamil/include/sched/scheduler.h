#pragma once

#include <common.h>

namespace sched {

void init();
void finalize();

// job_id = scheduleJob(createSomeJob()) -> result = waitJob<ResultType>(job_id)

}