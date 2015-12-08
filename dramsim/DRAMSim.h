
#ifndef LIB_DRAM_DRAMSIM_H
#define LIB_DRAM_DRAMSIM_H

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../common/intermod-common.h"

#ifdef __cplusplus
extern "C"
#endif
int dramsim_init(const char *deviceIniFilename, unsigned int megsOfMemory, unsigned int num_controllers,
                 int trans_queue_depth, int _llcTagLowBitWidth);

#ifdef __cplusplus
extern "C"
#endif
void dramsim_done();

#ifdef __cplusplus
extern "C"
#endif
void dramsim_update();

#ifdef __cplusplus
extern "C"
#endif
int dramsim_pending();

#ifdef __cplusplus
extern "C"
#endif
void dramsim_stats(char *file_name);

#ifdef __cplusplus
extern "C"
#endif
unsigned int dramsim_controller(uint64_t addr, char stream);

#ifdef __cplusplus
extern "C"
#endif
int dramsim_will_accept_request(uint64_t addr, bool isWrite, char stream);

#ifdef __cplusplus
extern "C"
#endif
void dramsim_request_wait(uint64_t addr, int event, void *stack);

#ifdef __cplusplus
extern "C"
#endif
void dramsim_request(int isWrite, uint64_t addr, char stream, memory_trace *info);

#ifdef __cplusplus
extern "C"
#endif
memory_trace* dram_response();

#ifdef __cplusplus
extern "C"
#endif
void dramsim_set_priority_stream(ub1 stream);

#ifdef __cplusplus
extern "C"
#endif
ub8 dramsim_get_open_row(memory_trace *info);

#endif
