#ifndef __METRICS_H__
#define __METRICS_H__


typedef int metric_key_t;


void metrics_init ();
metric_key_t metric_register (const char* name, int id);
int metric_update (metric_key_t key, zbx_uint64_t val);


#endif
