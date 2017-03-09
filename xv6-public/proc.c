#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "spinlock.h"
#include "proc.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

static struct kthread_mutex_t mutexes[MAX_MUTEXES];
struct spinlock mlock;

int nextpid = 1;
int nexttid = 1;
int nextmid = 1;

extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&mlock, "mutex_lock");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
// Must hold ptable.lock.
static struct thread*
allocthread(void)
{
  struct proc *p;
  struct thread *t;

  char *sp;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == PUNUSED)
      goto found;
  return 0;

found:
  p->state = PEMBRYO;
  p->pid = nextpid++;
  initlock(&p->plock, "proc_lock");
  t=&(p->threads[0]);
  t->state = EMBRYO;
  t->tid =  nexttid++;
  t->parent = p; 
  
  // Allocate kernel stack.
  if((t->kstack = kalloc()) == 0){
    p->state = PUNUSED;
    t->state = UNUSED;    
    return 0;
  }

  sp = t->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *t->tf;
  t->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *t->context;
  t->context = (struct context*)sp;
  memset(t->context, 0, sizeof *t->context);
  t->context->eip = (uint)forkret;
  return t;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  struct thread *t;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  acquire(&ptable.lock);

  t = allocthread();
  p = t->parent;

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(t->tf, 0, sizeof(*t->tf));
  t->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  t->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  t->tf->es = t->tf->ds;
  t->tf->ss = t->tf->ds;
  t->tf->eflags = FL_IF;
  t->tf->esp = PGSIZE;
  t->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = PREADY;
  t->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  acquire(&proc->plock);
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  release(&proc->plock);

  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct thread *nt;

  acquire(&ptable.lock);

  // Allocate process.
  if((nt = allocthread()) == 0){
    release(&ptable.lock);
    return -1;
  }
  np = nt->parent;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(nt->kstack);
    nt->kstack = 0;
    nt->state = UNUSED;
    np->state = PUNUSED;
    release(&ptable.lock);
    return -1;
  }
  np->kalloc_num = proc->child_kalloc_num;
  np->sz = proc->sz;
  np->parent = proc;
  *nt->tf = *thread->tf;

  // Clear %eax so that fork returns 0 in the child.
  nt->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;

  np->state = PREADY;
  nt->state  = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  struct thread *t;
  for(t = proc->threads; t < &proc->threads[NTHREAD] ; t++){
    if (t!=thread) t->killed=1;
  }

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == PZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = PZOMBIE;
  thread->state = UNUSED;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == PZOMBIE){
        // Found one.
        struct thread *t;
        for(t = p->threads; t < &p->threads[NTHREAD] ; t++){
          if (t->kstack) kfree(t->kstack);
          t->kstack = 0;
          t->tid = 0;
          t->parent = 0;
          t->killed = 0;
          t->state = UNUSED;
        }
        pid = p->pid;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = PUNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{
  struct proc *p;
  struct thread *t;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != PREADY){
        continue;
      }

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      for(t = p->threads; t < &p->threads[NTHREAD]; t++){
        if(t->state != RUNNABLE){
          continue;
        }
        thread=t;
        t->state=RUNNING;
        switchuvm(p);        
        swtch(&cpu->scheduler, t->context);
        switchkvm();
        thread = 0;
      }

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }

    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(thread->state == RUNNING)
    panic("thread running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&thread->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  thread->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{

  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  thread->chan = chan;
  thread->state = SLEEPING;
  sched();

  // Tidy up.
  thread->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;
  struct thread *t;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    for(t = p->threads; t < &p->threads[NTHREAD] ; t++){
      if(t->state == SLEEPING && t->chan == chan)
        t->state = RUNNABLE;
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

void 
print_proc_memo(struct proc *p){
    int j=0;int k=0;
    pde_t *pde;       // page table directory
    pte_t *pte;       // page table entry
    pte_t *curPagTable;   // current page table entry
    for(k=0; k<NPDENTRIES; k++){ 
    // scanning page directories table

      pde= &p->pgdir[k];
      if (*pde & PTE_P && *pde & PTE_U){
        curPagTable= (pte_t*)P2V(PTE_ADDR(*pde)); // extracting page table pointer
        for(j=0; j<NPTENTRIES;j++){
          // scanning page entries table
          pte= &curPagTable[j];
          if ((*pte & PTE_P) && (*pte & PTE_U)){
            cprintf(" %d -> %d , %s\n",(k* NPTENTRIES + j),*pte >> PGSHIFT,(*pte & PTE_W)!=0 ? "y" : "n" );    
          }
        }
      }
    }
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  struct thread *t;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    for(t = p->threads; t < &p->threads[NTHREAD] ; t++){
      if(t->state == UNUSED)
        continue;
      if(t->state >= 0 && t->state < NELEM(states) && states[t->state])
        state = states[t->state];
      else
        state = "???";
      cprintf("%d %s %s\n", t->tid, state, p->name);

      if(t->state == SLEEPING){
        getcallerpcs((uint*)t->context->ebp+2, pc);
        for(i=0; i<10 && pc[i] != 0; i++)
          cprintf(" %p", pc[i]);    
      }
      cprintf("\n");
      print_proc_memo(p);
      cprintf("\n");
    }
  }
}

int
kthread_create(void*(*start_func)(), void* stack, int stack_size)
{

  struct thread *t;
  char *sp;

  acquire(&ptable.lock);

  for(t = proc->threads; t < &proc->threads[NTHREAD] ; t++)
    if(t->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return -1;

found:
  t->state = EMBRYO;
  t->tid =  nexttid++;
  t->parent = proc; 
  
  // Allocate kernel stack.
  if((t->kstack = kalloc()) == 0){
    t->state = UNUSED;
    release(&ptable.lock);    
    return -1;
  }

  sp = t->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *t->tf;
  t->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *t->context;
  t->context = (struct context*)sp;
  memset(t->context, 0, sizeof *t->context);
  t->context->eip = (uint)forkret;

  *t->tf = *thread->tf;
  t->tf->eip = (uint)start_func;
  t->tf->esp = (uint)(stack + stack_size);

  t->state  = RUNNABLE;

  release(&ptable.lock);

  return t->tid;
}

void
kthread_exit(void)
{

  struct thread *t;

  acquire(&ptable.lock);

  for(t = proc->threads; t < &proc->threads[NTHREAD] ; t++){
    // thread might be sleeping in wait().
    wakeup1(t);
  }

  int bool=1;
  for(t = proc->threads; t < &proc->threads[NTHREAD] ; t++){
    if(t->state != UNUSED && t->state != ZOMBIE && t!=thread)
      bool=0;
  }
  if (bool){
    release(&ptable.lock);
    exit();
  }

  // Jump into the scheduler, never to return.
  thread->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

int
kthread_join(int thread_id)
{
  int found, tid;
  struct thread *t;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    found = 0;
    for(t = proc->threads; t < &proc->threads[NTHREAD] ; t++){
      if(t->tid != thread_id)
        continue;
      found = 1;
      if(t->state == ZOMBIE){
        // Found one.
        if (t->kstack) kfree(t->kstack);
        t->kstack = 0;
        tid = t->tid;
        t->tid = 0;
        t->parent = 0;
        t->killed = 0;
        t->state = UNUSED;
        release(&ptable.lock);
        return tid;
      }
    }

    // No point waiting if we don't have any children.
    if(!found || thread->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(thread, &proc->plock);  //DOC: wait-sleep
  }
}

int kthread_mutex_alloc(void){
  acquire(&mlock);
  struct kthread_mutex_t *m;
  for(m = mutexes; m < &mutexes[MAX_MUTEXES] ; m++){
    if (m->id==0){
      m->id =  nextmid++;
      m->thread = 0;
      m->proc=proc;
      m->locked=0;
      release(&mlock);
      return m->id;
    } 
  }
  release(&mlock);
  return -1;
}

int kthread_mutex_dealloc(int mutex_id){
  acquire(&mlock);
  struct kthread_mutex_t *m;
  for(m = mutexes; m < &mutexes[MAX_MUTEXES] ; m++){
    if (m->id==mutex_id){
      m->id =  0;
      m->thread = 0;
      m->locked= 0;
      release(&mlock);
      return 0;
    } 
  }
  release(&mlock);
  return -1;
}

int kthread_mutex_lock(int mutex_id){
  acquire(&mlock);
  struct kthread_mutex_t *m;
  for(m = mutexes; m < &mutexes[MAX_MUTEXES] ; m++){
    if (m->id!=mutex_id)
      continue;
    if(m->proc!=proc){
      release(&mlock);
      return -1;
    }
    if(m->locked && m->thread==thread){
      release(&mlock);
      return -1;
    }

    // The xchg is atomic.
    while(xchg(&m->locked, 1) != 0){
      release(&mlock);
      yield();
      acquire(&mlock);
    }

    // // Tell the C compiler and the processor to not move loads or stores
    // // past this point, to ensure that the critical section's memory
    // // references happen after the lock is acquired.
    // __sync_synchronize();

    m->thread=thread;
    release(&mlock);
    return 0;
  }
  release(&mlock);
  return -1;

}

int kthread_mutex_unlock(int mutex_id){

  acquire(&mlock);
  struct kthread_mutex_t *m;
  for(m = mutexes; m < &mutexes[MAX_MUTEXES] ; m++){
    if (m->id!=mutex_id)
      continue;
    if(m->proc!=proc){
      release(&mlock);
      return -1;    
    }
    if(!m->locked){
      release(&mlock);
      return -1;
    }

    // // Tell the C compiler and the processor to not move loads or stores
    // // past this point, to ensure that the critical section's memory
    // // references happen after the lock is acquired.
    // __sync_synchronize();

    m->thread=0;
    m->locked=0;
    release(&mlock);
    return 0;
  }
  release(&mlock);
  return -1;
}