#define NTHREAD 50 
#define MAX_MUTEXES 64

enum threadstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE};

// Per-thread state
struct thread {
  char *kstack;                // Bottom of kernel stack for this process  
  enum threadstate state;        // Process state
  int tid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
};

// Mutual exclusion lock.
struct kthread_mutex_t {
  uint id;       	 
  uint locked;       // Is the lock held?
  struct thread *thread;   // The cpu holding the lock.
  struct proc *proc;   // The cpu holding the lock.
};

