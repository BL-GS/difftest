/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef RUNAHEAD_H
#define RUNAHEAD_H

#include <queue>
#include "common.h"
#include "difftest.h"
#include "memdep.h"
#include "ram.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>

// Runahead framework

typedef struct RunaheadCheckpoint {
  pid_t pid; // slave pid after this checkpoint
  uint64_t checkpoint_id; // checkpoint_id generated by hardware
  uint64_t pc; // branch / load inst's pc
  // bool branch;
  bool may_replay;
} RunaheadCheckpoint;

typedef struct RunaheadRequest {
  long int message_type;
  union {
    uint64_t target_pc;
    uint64_t query_type;
  };
} RunaheadRequest;

typedef struct RunaheadResponsePid {
  long int message_type;
  pid_t pid;
} RunaheadResponsePid;

typedef struct MemdepQueryResponse {
  uint64_t pc;
  bool mem_access;
  bool mem_access_is_load;
  // uint64_t mem_access_paddr;
  uint64_t mem_access_vaddr;
  // uint64_t mem_access_result;
  bool ref_need_wait;
} MemdepQueryResponse;

typedef struct RunaheadResponseQuery {
  long int message_type;
  union {
    MemdepQueryResponse mem_access_info;
  } result;
} RunaheadResponseQuery;

class Runahead: public Difftest {
public:
  // Runahead framework
  Runahead(int coreid);
  ~Runahead();
  void register_checkpoint(pid_t pid, uint64_t branch_checkpoint_id, uint64_t branch_pc, bool may_replay);
  pid_t free_checkpoint();
  void recover_checkpoint(uint64_t checkpoint_id);
  void restart();
  void update_debug_info(void* dest_buffer);
  int step();
  bool checkpoint_num_exceed_limit();
  int do_instr_runahead();
  pid_t do_instr_runahead_pc_guided(uint64_t jump_target_pc);
  pid_t init_runahead_slave();
  pid_t fork_runahead_slave();
  void runahead_slave();
  pid_t request_slave_runahead();
  pid_t request_slave_runahead_pc_guided(uint64_t target_pc);
  void debug_print_checkpoint_list();
  void remove_all_checkpoints();
  void remove_msg_queues();
  void do_first_instr_runahead();
  void request_slave_refquery(void* target, int type);

  DiffTestState *dut_ptr;
  DiffTestState *ref_ptr;
  // Note: dut & ref does not contain valid value in runahead
  // use dut_ptr & ref_ptr instead.
  // To be refactored later.

#ifdef QUERY_MEM_ACCESS
  void do_query_mem_access(RunaheadResponseQuery* result_buffer);
  int memdep_check(int i, RunaheadResponseQuery* ref_mem_query_result);
#endif

#ifdef TRACE_INFLIGHT_MEM_INST
  MemdepWatchWindow* memdep_watcher = new MemdepWatchWindow;
#endif

private:
  std::deque<RunaheadCheckpoint> checkpoints;
  bool branch_reported;
  bool may_replay;
  uint64_t branch_checkpoint_id;
  uint64_t branch_pc;
};

extern Runahead** runahead;
int init_runahead_slave();
int runahead_init();
int runahead_step();
int runahead_cleanup();

enum {
  RUNAHEAD_MSG_REQ_ALL,
  RUNAHEAD_MSG_REQ_EXEC,
  RUNAHEAD_MSG_REQ_GUIDED_EXEC,
  RUNAHEAD_MSG_REQ_QUERY
};

enum {
  RUNAHEAD_MSG_RESP_ALL,
  RUNAHEAD_MSG_RESP_EXEC,
  RUNAHEAD_MSG_RESP_FORK,
  RUNAHEAD_MSG_RESP_QUERY,
};

#ifdef RUNAHEAD_DEBUG
#define runahead_debug(...) \
  do { \
    fprintf_with_pid(stdout, __VA_ARGS__); \
  }while(0)
#else
#define runahead_debug(...)
#endif

#define assert_no_error(func) if((func) == -1) { \
  fprintf(stderr, "%s\n", std::strerror(errno)); \
  assert(0); \
}

// Memory dependency query

typedef enum RefQueryType {
  REF_QUERY_MEM_EVENT
} RefQueryType;

#define loop_if_not(cond) \
  do { \
    if(!(cond)) { \
      printf("Sth went wrong, run while(1); for debugging\n"); \
      while(1); \
    } \
  }while(0)

#endif