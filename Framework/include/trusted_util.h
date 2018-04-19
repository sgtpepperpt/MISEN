#ifndef __TRUSTED_UTIL_H
#define __TRUSTED_UTIL_H

unsigned tutil_thread_get_count();
int tutil_thread_add_work(void* args);
void tutil_thread_do_work();

#endif
