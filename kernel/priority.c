#include <priority.h>
#include <module.h>
#include <systime.h>

#ifdef USE_PREEMPTION_PROFILER
void preemption_profile(sos_pid_t id) 
{
  sos_module_t *module = ker_get_module(id);
  uint32_t end = ker_systime32();
  uint32_t total = end - profile_start;

  profile_start = end;
  module->average = ((module->average * module->num_runs) + total) / (module->num_runs + 1);
  module->num_runs++;

  // Record the rate of msging also
}
#endif
