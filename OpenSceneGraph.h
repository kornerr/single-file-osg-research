
// OSGFILE OpenThreads/Config

#define _OPENTHREADS_ATOMIC_USE_GCC_BUILTINS
/* #undef _OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS */
/* #undef _OPENTHREADS_ATOMIC_USE_SUN */
/* #undef _OPENTHREADS_ATOMIC_USE_WIN32_INTERLOCKED */
/* #undef _OPENTHREADS_ATOMIC_USE_BSD_ATOMIC */
/* #undef _OPENTHREADS_ATOMIC_USE_MUTEX */
/* #undef OT_LIBRARY_STATIC */


// OSGFILE include/OpenThreads/Exports

//#include <OpenThreads/Config>

#ifndef WIN32
    #define OPENTHREAD_EXPORT_DIRECTIVE
#else
    #if defined( OT_LIBRARY_STATIC )
        #define OPENTHREAD_EXPORT_DIRECTIVE
    #elif defined( OPENTHREADS_EXPORTS )
        #define OPENTHREAD_EXPORT_DIRECTIVE __declspec(dllexport)
    #else
        #define OPENTHREAD_EXPORT_DIRECTIVE __declspec(dllimport)
    #endif
#endif


// OSGFILE include/OpenThreads/Atomic

//#include <OpenThreads/Config>
//#include <OpenThreads/Exports>

#if defined(_OPENTHREADS_ATOMIC_USE_BSD_ATOMIC)
# include <libkern/OSAtomic.h>
# define _OPENTHREADS_ATOMIC_USE_LIBRARY_ROUTINES
#elif defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS) && defined(__i386__)
# define _OPENTHREADS_ATOMIC_USE_LIBRARY_ROUTINES
#elif defined(_OPENTHREADS_ATOMIC_USE_WIN32_INTERLOCKED)
# define _OPENTHREADS_ATOMIC_USE_LIBRARY_ROUTINES
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
# include <atomic.h>
# include "Mutex"
# include "ScopedLock"
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
# include "Mutex"
# include "ScopedLock"
#endif

#if defined(_OPENTHREADS_ATOMIC_USE_LIBRARY_ROUTINES)
#define _OPENTHREADS_ATOMIC_INLINE
#else
#define _OPENTHREADS_ATOMIC_INLINE inline
#endif

namespace OpenThreads {

/**
 *  @class Atomic
 *  @brief  This class provides an atomic increment and decrement operation.
 */
class OPENTHREAD_EXPORT_DIRECTIVE Atomic {
 public:
    Atomic(unsigned value = 0) : _value(value)
    { }
    _OPENTHREADS_ATOMIC_INLINE unsigned operator++();
    _OPENTHREADS_ATOMIC_INLINE unsigned operator--();
    _OPENTHREADS_ATOMIC_INLINE unsigned AND(unsigned value);
    _OPENTHREADS_ATOMIC_INLINE unsigned OR(unsigned value);
    _OPENTHREADS_ATOMIC_INLINE unsigned XOR(unsigned value);
    _OPENTHREADS_ATOMIC_INLINE unsigned exchange(unsigned value = 0);
    _OPENTHREADS_ATOMIC_INLINE operator unsigned() const;
 private:

    Atomic(const Atomic&);
    Atomic& operator=(const Atomic&);

#if defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    mutable Mutex _mutex;
#endif
#if defined(_OPENTHREADS_ATOMIC_USE_WIN32_INTERLOCKED)
    volatile long _value;
#elif defined(_OPENTHREADS_ATOMIC_USE_BSD_ATOMIC)
    volatile int32_t _value;
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    volatile uint_t _value;
    mutable Mutex _mutex;  // needed for xor
#else
    volatile unsigned _value;
#endif
};

/**
 *  @class AtomicPtr
 *  @brief  This class provides an atomic pointer assignment using cas operations.
 */
class OPENTHREAD_EXPORT_DIRECTIVE AtomicPtr {
public:
    AtomicPtr(void* ptr = 0) : _ptr(ptr)
    { }
    ~AtomicPtr()
    { _ptr = 0; }

    // assigns a new pointer
    _OPENTHREADS_ATOMIC_INLINE bool assign(void* ptrNew, const void* const ptrOld);
    _OPENTHREADS_ATOMIC_INLINE void* get() const;

private:
    AtomicPtr(const AtomicPtr&);
    AtomicPtr& operator=(const AtomicPtr&);

#if defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    mutable Mutex _mutex;
#endif
    void* volatile _ptr;
};

#if !defined(_OPENTHREADS_ATOMIC_USE_LIBRARY_ROUTINES)

_OPENTHREADS_ATOMIC_INLINE unsigned
Atomic::operator++()
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    return __sync_add_and_fetch(&_value, 1);
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    return __add_and_fetch(&_value, 1);
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    return atomic_inc_uint_nv(&_value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    return ++_value;
#else
    return ++_value;
#endif
}

_OPENTHREADS_ATOMIC_INLINE unsigned
Atomic::operator--()
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    return __sync_sub_and_fetch(&_value, 1);
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    return __sub_and_fetch(&_value, 1);
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    return atomic_dec_uint_nv(&_value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    return --_value;
#else
    return --_value;
#endif
}

_OPENTHREADS_ATOMIC_INLINE unsigned
Atomic::AND(unsigned value)
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    return __sync_fetch_and_and(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    return __and_and_fetch(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    return atomic_and_uint_nv(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    _value &= value;
    return _value;
#else
    _value &= value;
    return _value;
#endif
}

_OPENTHREADS_ATOMIC_INLINE unsigned
Atomic::OR(unsigned value)
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    return __sync_fetch_and_or(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    return __or_and_fetch(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    return atomic_or_uint_nv(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    _value |= value;
    return _value;
#else
    _value |= value;
    return _value;
#endif
}

_OPENTHREADS_ATOMIC_INLINE unsigned
Atomic::XOR(unsigned value)
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    return __sync_fetch_and_xor(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    return __xor_and_fetch(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    ScopedLock<Mutex> lock(_mutex);
    _value ^= value;
    return _value;
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    _value ^= value;
    return _value;
#else
    _value ^= value;
    return _value;
#endif
}

_OPENTHREADS_ATOMIC_INLINE unsigned
Atomic::exchange(unsigned value)
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    return __sync_lock_test_and_set(&_value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    return __compare_and_swap(&_value, _value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    return atomic_cas_uint(&_value, _value, value);
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    unsigned oldval = _value;
    _value = value;
    return oldval;
#else
    unsigned oldval = _value;
    _value = value;
    return oldval;
#endif
}

_OPENTHREADS_ATOMIC_INLINE
Atomic::operator unsigned() const
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    __sync_synchronize();
    return _value;
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    __synchronize();
    return _value;
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    membar_consumer(); // Hmm, do we need???
    return _value;
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    return _value;
#else
    return _value;
#endif
}

_OPENTHREADS_ATOMIC_INLINE bool
AtomicPtr::assign(void* ptrNew, const void* const ptrOld)
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    return __sync_bool_compare_and_swap(&_ptr, (void *)ptrOld, ptrNew);
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    return __compare_and_swap((unsigned long*)&_ptr, (unsigned long)ptrOld, (unsigned long)ptrNew);
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    return ptrOld == atomic_cas_ptr(&_ptr, const_cast<void*>(ptrOld), ptrNew);
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    if (_ptr != ptrOld)
        return false;
    _ptr = ptrNew;
    return true;
#else
    if (_ptr != ptrOld)
        return false;
    _ptr = ptrNew;
    return true;
#endif
}

_OPENTHREADS_ATOMIC_INLINE void*
AtomicPtr::get() const
{
#if defined(_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS)
    __sync_synchronize();
    return _ptr;
#elif defined(_OPENTHREADS_ATOMIC_USE_MIPOSPRO_BUILTINS)
    __synchronize();
    return _ptr;
#elif defined(_OPENTHREADS_ATOMIC_USE_SUN)
    membar_consumer(); // Hmm, do we need???
    return _ptr;
#elif defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
    ScopedLock<Mutex> lock(_mutex);
    return _ptr;
#else
    return _ptr;
#endif
}

#endif // !defined(_OPENTHREADS_ATOMIC_USE_LIBRARY_ROUTINES)

}


// OSGFILE include/OpenThreads/Mutex

//#include <OpenThreads/Exports>

namespace OpenThreads {

/**
 *  @class Mutex
 *  @brief  This class provides an object-oriented thread mutex interface.
 */
class OPENTHREAD_EXPORT_DIRECTIVE Mutex {

    friend class Condition;

public:

    enum MutexType
    {
        MUTEX_NORMAL,
        MUTEX_RECURSIVE
    };

    /**
     *  Constructor
     */
    Mutex(MutexType type=MUTEX_NORMAL);

    /**
     *  Destructor
     */
    virtual ~Mutex();


    MutexType getMutexType() const { return _mutexType; }


    /**
     *  Lock the mutex
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    virtual int lock();

    /**
     *  Unlock the mutex
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    virtual int unlock();

    /**
     *  Test if mutex can be locked.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    virtual int trylock();

private:

    /**
     *  Private copy constructor, to prevent tampering.
     */
    Mutex(const Mutex &/*m*/) {};

    /**
     *  Private copy assignment, to prevent tampering.
     */
    Mutex &operator=(const Mutex &/*m*/) {return *(this);};

    /**
     *  Implementation-specific private data.
     */
    void *_prvData;
    MutexType _mutexType;

};

}


// OSGFILE include/OpenThreads/Affinity

#include <set>
//#include <OpenThreads/Mutex>

namespace OpenThreads {

/**
 *  @class Affinity
 *  @brief  Simple container for specifying which CPU a thread should have affinity with.
 *  An empty Affinity.activeCPUs/default constructed Affinity signifies that a thread should not have any specific affinity and be able to run on all available CPUs.
 */
class Affinity
{
public:

    Affinity() {}

    Affinity(unsigned int cpuNumber) { activeCPUs.insert(cpuNumber); }

    Affinity(unsigned int cpuNumber, unsigned int cpuCount) { while(cpuCount>0) { activeCPUs.insert(cpuNumber++); --cpuCount; } }

    Affinity(const Affinity& rhs) : activeCPUs(rhs.activeCPUs) {}

    Affinity& operator = (const Affinity& rhs) { if (&rhs!=this) { activeCPUs = rhs.activeCPUs; } return *this; }


    /** add a specified cpu core from the list to have affinity to. */
    void add(unsigned int cpuNmber) { activeCPUs.insert(cpuNmber); }

    /** remove a specified cpu core from the list to have affinity to. */
    void remove(unsigned int cpuNmber) { activeCPUs.erase(cpuNmber); }

    /** return true if affinity has been provided for specific CPU cores.*/
    operator bool () const { return !activeCPUs.empty(); }

    typedef std::set<unsigned int> ActiveCPUs;

    /** Set of CPUs that a thread should have affinity to.*/
    ActiveCPUs activeCPUs;
};

}


// OSGFILE include/OpenThreads/Thread

#include <sys/types.h>

//#include <OpenThreads/Mutex>
//#include <OpenThreads/Affinity>

namespace OpenThreads {

/**
 *  Get the number of processors.
 *
 *  Note, systems where no support exists for querying the number of processors, 1 is returned.
 *
 */
extern OPENTHREAD_EXPORT_DIRECTIVE int GetNumberOfProcessors();

/**
 *  Set the processor affinity of current thread.
 */
extern OPENTHREAD_EXPORT_DIRECTIVE int SetProcessorAffinityOfCurrentThread(const Affinity& affinity);


/**
 *  @class Thread
 *  @brief  This class provides an object-oriented thread interface.
 */
class OPENTHREAD_EXPORT_DIRECTIVE Thread {

public:

    /**
     *  Set the concurrency level for a running application.  This method
     *  only has effect if the pthreads thread model is being used, and
     *  then only when that model is many-to-one (eg. irix).
     *  in other cases it is ignored.  The concurrency level is only a
     *  *hint* as to the number of execution vehicles to use, the actual
     *  implementation may do anything it wants.  Setting the value
     *  to 0 returns things to their default state.
     *
     *  @return previous concurrency level, -1 indicates no-op.
     */
    static int SetConcurrency(int concurrencyLevel);

    /**
     *  Get the concurrency level for a running application.  In this
     *  case, a return code of 0 means that the application is in default
     *  mode.  A return code of -1 means that the application is incapable
     *  of setting an arbitrary concurrency, because it is a one-to-one
     *  execution model (sprocs, linuxThreads)
     */
    static int GetConcurrency();

    /**
     *  Enumerated Type for thread priority
     */
    enum ThreadPriority {

        THREAD_PRIORITY_MAX,      /**< The maximum possible priority  */
        THREAD_PRIORITY_HIGH,     /**< A high (but not max) setting   */
        THREAD_PRIORITY_NOMINAL,  /**< An average priority            */
        THREAD_PRIORITY_LOW,      /**< A low (but not min) setting    */
        THREAD_PRIORITY_MIN,      /**< The miniumum possible priority */
        THREAD_PRIORITY_DEFAULT   /**< Priority scheduling default    */

    };

    /**
     *  Enumerated Type for thread scheduling policy
     */
    enum ThreadPolicy {

        THREAD_SCHEDULE_FIFO,        /**< First in, First out scheduling         */
        THREAD_SCHEDULE_ROUND_ROBIN, /**< Round-robin scheduling (LINUX_DEFAULT) */
        THREAD_SCHEDULE_TIME_SHARE,  /**< Time-share scheduling (IRIX DEFAULT)   */
        THREAD_SCHEDULE_DEFAULT      /**< Default scheduling                     */

    };

    /**
     *  Constructor
     */
    Thread();

    /**
     *  Destructor
     */
    virtual ~Thread();


    /**
     *  Return a pointer to the current running thread
     */
    static Thread *CurrentThread();


    /**
     *  Initialize Threading in a program.  This method must be called before
     *  you can do any threading in a program.
     */
    static void Init();

    /**
     *  Yield the processor.
     *
     *  @note This method operates on the calling process.  And is
     *  equivalent to calling sched_yield().
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    static int YieldCurrentThread();

    /**
     *  This method will return the ThreadPriority of the master process.
     *  (ie, the one calling the thread->start() methods for the first time)
     *  The method will almost certainly return
     *  Thread::THREAD_PRIORITY_DEFAULT if
     *  Init() has not been called.
     *
     *  @return the Thread::ThreadPriority of the master thread.
     */
    static ThreadPriority GetMasterPriority() {return s_masterThreadPriority;};


    /**
     *  Get a unique thread id.  This id is monotonically increasing.
     *
     *  @return a unique thread identifier
     */
    int getThreadId();

    /**
     *  Get the thread's process id.  This is the pthread_t or pid_t value
     *  depending on the threading model being used.
     *
     *  @return thread process id.
     */
    size_t getProcessId();

    /**
     *  Start the thread.  This method will configure the thread, set
     *  it's priority, and spawn it.
     *
     *  @note if the stack size specified setStackSize is smaller than the
     *  smallest allowable stack size,  the threads stack size will be set to
     *  the minimum allowed, and may be retrieved via the getStackSize()
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int start();
    int startThread();

    /**
     * Test the cancel state of the thread.  If the thread has been canceled
     * this method will cause the thread to exit now.  This method operates
     * on the calling thread.
     *
     * Returns 0 if normal, -1 if called from a thread other that this.
     */
    int testCancel();


    /**
     *  Cancel the thread.  Equivalent to SIGKILL.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    virtual int cancel();

    /**
     *  Set the thread's schedule priority.  This is a complex method.
     *  Beware of thread priorities when using a many-to-many kernel
     *  entity implementation (such as IRIX pthreads).  If one is not careful
     *  to manage the thread priorities, a priority inversion deadlock can
     *  easily occur (Although the OpenThreads::Mutex & OpenThreads::Barrier
     *  constructs have been designed with this scenario in mind).  Unless
     *  you have explicit need to set the schedule priorities for a given
     *  task, it is best to leave them alone.
     *
     *  @note some implementations (notably LinuxThreads and IRIX Sprocs)
     *  only allow you to decrease thread priorities dynamically.  Thus,
     *  a lower priority thread will not allow it's priority to be raised
     *  on the fly.
     *
     *  @note setting the environment variable OUTPUT_THREADLIB_SCHEDULING_INFO
     *  will output scheduling information for each thread to stdout.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int setSchedulePriority(ThreadPriority priority);

    /**
     *  Get the thread's schedule priority (if able)
     *
     *  @note setting the environment variable OUTPUT_THREADLIB_SCHEDULING_INFO
     *  will output scheduling information for each thread to stdout.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int getSchedulePriority();

    /**
     *  Set the thread's scheduling policy (if able)
     *
     *  @note On some implementations (notably IRIX Sprocs & LinuxThreads)
     *  The policy may prohibit the use of SCHEDULE_ROUND_ROBIN and
     *  SCHEDULE_FIFO policies - due to their real-time nature, and
     *  the danger of deadlocking the machine when used as super-user.
     *  In such cases, the command is a no-op.
     *
     *  @note setting the environment variable OUTPUT_THREADLIB_SCHEDULING_INFO
     *  will output scheduling information for each thread to stdout.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int setSchedulePolicy(ThreadPolicy policy);

    /**
     *  Get the thread's policy (if able)
     *
     *  @note setting the environment variable OUTPUT_THREADLIB_SCHEDULING_INFO
     *  will output scheduling information for each thread to stdout.
     *
     *  @return policy if normal, -1 if errno set, errno code otherwise.
     */
    int getSchedulePolicy();

    /**
     *  Set the thread's desired stack size (in bytes).
     *  This method is an attribute of the thread and must be called
     *  *before* the start() method is invoked.
     *
     *  @note a return code of 13 (EACESS) means that the thread stack
     *  size can no longer be changed.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int setStackSize(size_t size);

    /**
     *  Get the thread's desired stack size.
     *
     *  @return the thread's stack size.  0 indicates that the stack size
     *   has either not yet been initialized, or not yet been specified by
     *   the application.
     */
    size_t getStackSize();

    /**
     *  Print the thread's scheduling information to stdout.
     */
    void printSchedulingInfo();

    /**
     *  Detach the thread from the calling process.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int detach();

    /**
     *  Join the calling process with the thread
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int join();

    /**
     *  Disable thread cancellation altogether. Thread::cancel() has no effect.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int setCancelModeDisable();

    /**
     *  Mark the thread to cancel asynchronously on Thread::cancel().
     *  (May not be available with process-level implementations).
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int setCancelModeAsynchronous();

    /**
     *  Mark the thread to cancel at the earliest convenience on
     *  Thread::cancel() (This is the default)
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int setCancelModeDeferred();

    /**
     *  Query the thread's running status
     *
     *  @return true if running, false if not.
     */
    bool isRunning();

    /**
     *  Thread's run method.  Must be implemented by derived classes.
     *  This is where the action happens.
     */
    virtual void run() = 0;

    /**
     *  Thread's cancel cleanup routine, called upon cancel(), after the
     *  cancellation has taken place, but before the thread exits completely.
     *  This method should be used to repair parts of the thread's data
     *  that may have been damaged by a pre-mature cancel.  No-op by default.
     */
    virtual void cancelCleanup() {};

    void* getImplementation(){ return _prvData;	};

    /** Set the Thread's processor affinity to all, a single CPU or multiple CPU's
      * This call must be made before
      * start() or startThread() and has no effect after the thread
      * has been running. Returns 0 on success, implementation's
      * error on failure, or -1 if ignored.
      */
    int setProcessorAffinity( const Affinity& affinity);


    /** microSleep method, equivalent to the posix usleep(microsec).
      *  This is not strictly thread API but is used
      * so often with threads. It's basically UNIX usleep. Parameter is
      * number of microseconds we current thread to sleep. Returns 0 on
      * success, non-zero on failure (UNIX errno or GetLastError() will give
      * detailed description.
      */
    static int microSleep( unsigned int microsec);

private:

    /**
     *  The Private Actions class is allowed to operate on private data.
     */
    friend class ThreadPrivateActions;

     /**
      *  Private copy constructor, to prevent tampering.
      */
    Thread(const Thread &/*t*/) {};

    /**
      *  Private copy assignment, to prevent tampering.
      */
    Thread &operator=(const Thread &/*t*/) {return *(this);};

    /**
     *  Implementation-specific data
     */
    void * _prvData;

    /**
     *  Master thread's priority, set by Thread::Init.
     */
    static ThreadPriority s_masterThreadPriority;

    /**
     *  Is initialized flag
     */
    static bool s_isInitialized;
};

}


// OSGFILE include/OpenThreads/Barrier

//#include <OpenThreads/Exports>

namespace OpenThreads {


/**
 *  @class Barrier
 *  @brief This class provides an object-oriented thread barrier interface
 *
 *  @warning It is unwise to use the construct "Barrier barrier" in the
 *           global namespace on sgi's.  The object "barrier"
 *           will confilict with the c-library sproc function "barrier" and
 *           unpredictable results may occur. You have been warned.
 */
class OPENTHREAD_EXPORT_DIRECTIVE Barrier {

public:

    /**
     *  Constructor
     */
    Barrier(int numThreads=0);

    /**
     *  Destructor
     */
    virtual ~Barrier();

    /**
     *  Reset the barrier to it's original state.
     */
    virtual void reset();

    /**
     *  Block until numThreads threads have entered the barrier.
     */
    virtual void block(unsigned int numThreads=0);

    /**
     *  Release the barrier, now.
     */
    virtual void release();

    /**
     *  Return the number of threads currently blocked in the barrier,
     *  Return -1 if error.
     */
    virtual int numThreadsCurrentlyBlocked();


    void invalidate();

private:

    /**
     *  Private copy constructor, to prevent tampering.
     */
    Barrier(const Barrier &/*b*/) {};

    /**
     *  Private copy assignment, to prevent tampering.
     */
    Barrier &operator=(const Barrier &/*b*/) {return *(this);};

    /**
     *  Implementation-specific private data.
     */
    void *_prvData;


    bool _valid;

};

}


// OSGFILE include/osg/Export

//#include<osg/Config>

// disable VisualStudio warnings
#if defined(_MSC_VER) && defined(OSG_DISABLE_MSVC_WARNINGS)
    #pragma warning( disable : 4244 )
    #pragma warning( disable : 4251 )
    #pragma warning( disable : 4275 )
    #pragma warning( disable : 4512 )
    #pragma warning( disable : 4267 )
    #pragma warning( disable : 4702 )
    #pragma warning( disable : 4511 )
#endif

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
    #  if defined( OSG_LIBRARY_STATIC )
    #    define OSG_EXPORT
    #  elif defined( OSG_LIBRARY )
    #    define OSG_EXPORT   __declspec(dllexport)
    #  else
    #    define OSG_EXPORT   __declspec(dllimport)
    #  endif
#else
    #  define OSG_EXPORT
#endif

// set up define for whether member templates are supported by VisualStudio compilers.
#ifdef _MSC_VER
# if (_MSC_VER >= 1300)
#  define __STL_MEMBER_TEMPLATES
# endif
#endif

/* Define NULL pointer value */

#ifndef NULL
    #ifdef  __cplusplus
        #define NULL    0
    #else
        #define NULL    ((void *)0)
    #endif
#endif

// helper macro's for quieten unused variable warnings
#define OSG_UNUSED(VAR) (void)(VAR)
#define OSG_UNUSED2(VAR1, VAR2) (void)(VAR1); (void)(VAR2);
#define OSG_UNUSED3(VAR1, VAR2, VAR3) (void)(VAR1); (void)(VAR2); (void)(VAR2);
#define OSG_UNUSED4(VAR1, VAR2, VAR3, VAR4) (void)(VAR1); (void)(VAR2); (void)(VAR3); (void)(VAR4);
#define OSG_UNUSED5(VAR1, VAR2, VAR3, VAR4, VAR5) (void)(VAR1); (void)(VAR2); (void)(VAR3); (void)(VAR4); (void)(VAR5);

/**

\namespace osg

The core osg library provides the basic scene graph classes such as Nodes,
State and Drawables, and maths and general helper classes.
*/


// OSGFILE include/OpenThreads/Condition

//#include <OpenThreads/Exports>
//#include <OpenThreads/Mutex>

namespace OpenThreads {

/**
 *  @class Condition
 *  @brief  This class provides an object-oriented thread condition interface.
 */
class OPENTHREAD_EXPORT_DIRECTIVE Condition {

public:

    /**
     *  Constructor
     */
    Condition();

    /**
     *  Destructor
     */
    virtual ~Condition();

    /**
     *  Wait on a mutex.
     */
    virtual int wait(Mutex *mutex);

    /**
     *  Wait on a mutex for a given amount of time (ms)
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    virtual int wait(Mutex *mutex, unsigned long int ms);

    /**
     *  Signal a SINGLE thread to wake if it's waiting.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    virtual int signal();

    /**
     *  Wake all threads waiting on this condition.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    virtual int broadcast();

private:

    /**
     *  Private copy constructor, to prevent tampering.
     */
    Condition(const Condition &/*c*/) {};

    /**
     *  Private copy assignment, to prevent tampering.
     */
    Condition &operator=(const Condition &/*c*/) {return *(this);};

    /**
     *  Implementation-specific data
     */
    void *_prvData;

};

}


// OSGFILE include/OpenThreads/ScopedLock

namespace OpenThreads{

template <class M> class ScopedLock
{
    private:
        M& m_lock;
        ScopedLock(const ScopedLock&); // prevent copy
        ScopedLock& operator=(const ScopedLock&); // prevent assign
    public:
        explicit ScopedLock(M& m):m_lock(m) {m_lock.lock();}
        ~ScopedLock(){m_lock.unlock();}
};

template <class M> class ReverseScopedLock
{
    private:
        M& m_lock;
        ReverseScopedLock(const ReverseScopedLock&); // prevent copy
        ReverseScopedLock& operator=(const ReverseScopedLock&); // prevent assign
    public:
        explicit ReverseScopedLock(M& m):m_lock(m) {m_lock.unlock();}
        ~ReverseScopedLock(){m_lock.lock();}
};


template <class M> class ScopedPointerLock
{
    private:
        M* m_lock;
        ScopedPointerLock(const ScopedPointerLock&); // prevent copy
        ScopedPointerLock& operator=(const ScopedPointerLock&); // prevent assign
    public:
        explicit ScopedPointerLock(M* m):m_lock(m) { if (m_lock) m_lock->lock();}
        ~ScopedPointerLock(){ if (m_lock) m_lock->unlock();}
};

template <class M> class ReverseScopedPointerLock
{
    private:
        M* m_lock;
        ReverseScopedPointerLock(const ReverseScopedPointerLock&); // prevent copy
        ReverseScopedPointerLock& operator=(const ReverseScopedPointerLock&); // prevent assign
    public:
        explicit ReverseScopedPointerLock(M* m):m_lock(m) { if (m_lock) m_lock->unlock();}
        ~ReverseScopedPointerLock(){ if (m_lock) m_lock->lock();}
};

}


// OSGFILE include/OpenThreads/Block

/*
#include <OpenThreads/Thread>
#include <OpenThreads/Barrier>
#include <OpenThreads/Condition>
#include <OpenThreads/ScopedLock>
*/

namespace OpenThreads {

/** Block is a block that can be used to halt a thread that is waiting another thread to release it.*/
class Block
{
    public:

        Block():
            _released(false) {}

        ~Block()
        {
            release();
        }

        inline bool block()
        {
            ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            if( !_released )
            {
                return _cond.wait(&_mut)==0;
            }
            else
            {
                return true;
            }
        }

        inline bool block(unsigned long timeout)
        {
            ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            if( !_released )
            {
                return _cond.wait(&_mut, timeout)==0;
            }
            else
            {
                return true;
            }
        }

        inline void release()
        {
            ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            if (!_released)
            {
                _released = true;
                _cond.broadcast();
            }
        }

        inline void reset()
        {
            ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            _released = false;
        }

        inline void set(bool doRelease)
        {
            if (doRelease!=_released)
            {
                if (doRelease) release();
                else reset();
            }
        }

    protected:

        Mutex _mut;
        Condition _cond;
        bool _released;

    private:

        Block(const Block&) {}
};

/** BlockCount is a block that can be used to halt a thread that is waiting for a specified number of operations to be completed.*/
class BlockCount
{
    public:

        BlockCount(unsigned int blockCount):
            _blockCount(blockCount),
            _currentCount(0) {}

        ~BlockCount()
        {
            _blockCount = 0;
            release();
        }

        inline void completed()
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            if (_currentCount>0)
            {
                --_currentCount;

                if (_currentCount==0)
                {
                    // osg::notify(osg::NOTICE)<<"Released"<<std::endl;
                    _cond.broadcast();
                }
            }
        }

        inline void block()
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            if (_currentCount)
                _cond.wait(&_mut);
        }

        inline void reset()
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            if (_currentCount!=_blockCount)
            {
                if (_blockCount==0) _cond.broadcast();
                _currentCount = _blockCount;
            }
        }

        inline void release()
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> mutlock(_mut);
            if (_currentCount)
            {
                _currentCount = 0;
                _cond.broadcast();
            }
        }

        inline void setBlockCount(unsigned int blockCount) { _blockCount = blockCount; }

        inline unsigned int getBlockCount() const { return _blockCount; }

        inline unsigned int getCurrentCount() const { return _currentCount; }

    protected:

        OpenThreads::Mutex _mut;
        OpenThreads::Condition _cond;
        unsigned int _blockCount;
        unsigned int _currentCount;

    private:

        BlockCount(const BlockCount&) {}

};

}


// OSGFILE include/osg/Types

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef signed __int8    int8_t;
typedef unsigned __int8  uint8_t;
typedef signed __int16   int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32   int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif


// OSGFILE osg/GL

/*
#include <osg/Config>
#include <osg/Export>
#include <osg/Types>
*/
 
#define OSG_GL1_AVAILABLE
#define OSG_GL2_AVAILABLE
/* #undef OSG_GL3_AVAILABLE */
/* #undef OSG_GLES1_AVAILABLE */
/* #undef OSG_GLES2_AVAILABLE */
/* #undef OSG_GLES3_AVAILABLE */
/* #undef OSG_GL_LIBRARY_STATIC */
#define OSG_GL_DISPLAYLISTS_AVAILABLE
#define OSG_GL_MATRICES_AVAILABLE
#define OSG_GL_VERTEX_FUNCS_AVAILABLE
#define OSG_GL_VERTEX_ARRAY_FUNCS_AVAILABLE
#define OSG_GL_FIXED_FUNCTION_AVAILABLE
#define GL_HEADER_HAS_GLINT64
#define GL_HEADER_HAS_GLUINT64

#define OSG_GL1_FEATURES 1
#define OSG_GL2_FEATURES 1
#define OSG_GL3_FEATURES 0
#define OSG_GLES1_FEATURES 0
#define OSG_GLES2_FEATURES 0
#define OSG_GLES3_FEATURES 0
#define OSG_GL_CONTEXT_VERSION "1.0"


#ifndef WIN32

    // Required for compatibility with glext.h style function definitions of
    // OpenGL extensions, such as in src/osg/Point.cpp.
    #ifndef APIENTRY
        #define APIENTRY
    #endif

#else // WIN32

    #if defined(__CYGWIN__) || defined(__MINGW32__)

        #ifndef APIENTRY
                #define GLUT_APIENTRY_DEFINED
                #define APIENTRY __stdcall
        #endif
            // XXX This is from Win32's <windef.h>
        #ifndef CALLBACK
            #define CALLBACK __stdcall
        #endif

    #else // ! __CYGWIN__

        // Under Windows avoid including <windows.h>
        // to avoid name space pollution, but Win32's <GL/gl.h>
        // needs APIENTRY and WINGDIAPI defined properly.
        // XXX This is from Win32's <windef.h>
        #ifndef APIENTRY
            #define GLUT_APIENTRY_DEFINED
            #if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
                #define WINAPI __stdcall
                #define APIENTRY WINAPI
            #else
                #define APIENTRY
            #endif
        #endif

            // XXX This is from Win32's <windef.h>
        #ifndef CALLBACK
            #if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
                #define CALLBACK __stdcall
            #else
                #define CALLBACK
            #endif
        #endif

    #endif // __CYGWIN__

    // XXX This is from Win32's <wingdi.h> and <winnt.h>
    #ifndef WINGDIAPI
        #define GLUT_WINGDIAPI_DEFINED
        #define DECLSPEC_IMPORT __declspec(dllimport)
        #define WINGDIAPI DECLSPEC_IMPORT
    #endif

    // XXX This is from Win32's <ctype.h>
    #if !defined(_WCHAR_T_DEFINED) && !(defined(__GNUC__)&&(__GNUC__ > 2))
        typedef unsigned short wchar_t;
        #define _WCHAR_T_DEFINED
    #endif

#endif // WIN32

#if defined(OSG_GL3_AVAILABLE)
    #define GL3_PROTOTYPES 1
    #define GL_GLEXT_PROTOTYPES 1
#endif


#include <GL/gl.h>



#ifndef GL_APIENTRY
    #define GL_APIENTRY APIENTRY
#endif // GL_APIENTRY


#ifndef GL_HEADER_HAS_GLINT64
    typedef int64_t GLint64;
#endif

#ifndef GL_HEADER_HAS_GLUINT64
    typedef uint64_t GLuint64;
#endif

#ifdef OSG_GL_MATRICES_AVAILABLE

    inline void glLoadMatrix(const float* mat) { glLoadMatrixf(static_cast<const GLfloat*>(mat)); }
    inline void glMultMatrix(const float* mat) { glMultMatrixf(static_cast<const GLfloat*>(mat)); }

    #ifdef OSG_GLES1_AVAILABLE
        inline void glLoadMatrix(const double* mat)
        {
            GLfloat flt_mat[16];
            for(unsigned int i=0;i<16;++i) flt_mat[i] = mat[i];
            glLoadMatrixf(flt_mat);
        }

        inline void glMultMatrix(const double* mat)
        {
            GLfloat flt_mat[16];
            for(unsigned int i=0;i<16;++i) flt_mat[i] = mat[i];
            glMultMatrixf(flt_mat);
        }

    #else
        inline void glLoadMatrix(const double* mat) { glLoadMatrixd(static_cast<const GLdouble*>(mat)); }
        inline void glMultMatrix(const double* mat) { glMultMatrixd(static_cast<const GLdouble*>(mat)); }
    #endif
#endif

// add defines for OpenGL targets that don't define them, just to ease compatibility across targets
#ifndef GL_DOUBLE
    #define GL_DOUBLE 0x140A
    typedef double GLdouble;
#endif

#ifndef GL_INT
    #define GL_INT 0x1404
#endif

#ifndef GL_UNSIGNED_INT
    #define GL_UNSIGNED_INT 0x1405
#endif

#ifndef GL_NONE
    // OpenGL ES1 doesn't provide GL_NONE
    #define GL_NONE 0x0
#endif

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    //GLES defines (OES)
    #define GL_RGB8_OES                                             0x8051
    #define GL_RGBA8_OES                                            0x8058
#endif

#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    #define GL_POLYGON                         0x0009
    #define GL_QUADS                           0x0007
    #define GL_QUAD_STRIP                      0x0008
#endif

#if defined(OSG_GL3_AVAILABLE)
    #define GL_LUMINANCE                      0x1909
    #define GL_LUMINANCE_ALPHA                0x190A
#endif



// OSGFILE include/osg/Referenced

//#include <osg/Export>

/*
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Mutex>
#include <OpenThreads/Atomic>
*/

#if !defined(_OPENTHREADS_ATOMIC_USE_MUTEX)
# define _OSG_REFERENCED_USE_ATOMIC_OPERATIONS
#endif

namespace osg {

// forward declare, declared after Referenced below.
class DeleteHandler;
class Observer;
class ObserverSet;

/** template class to help enforce static initialization order. */
template <typename T, T M()>
struct depends_on
{
    depends_on() { M(); }
};

/** Base class for providing reference counted objects.*/
class OSG_EXPORT Referenced
{

    public:


        Referenced();

        /** Deprecated, Referenced is now always uses thread safe ref/unref, use default Referenced() constructor instead */
        explicit Referenced(bool threadSafeRefUnref);

        Referenced(const Referenced&);

        inline Referenced& operator = (const Referenced&) { return *this; }

        /** Deprecated, Referenced is always theadsafe so there method now has no effect and does not need to be called.*/
        virtual void setThreadSafeRefUnref(bool /*threadSafe*/) {}

        /** Get whether a mutex is used to ensure ref() and unref() are thread safe.*/
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
        bool getThreadSafeRefUnref() const { return true; }
#else
        bool getThreadSafeRefUnref() const { return _refMutex!=0; }
#endif

        /** Get the mutex used to ensure thread safety of ref()/unref(). */
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
        OpenThreads::Mutex* getRefMutex() const { return getGlobalReferencedMutex(); }
#else
        OpenThreads::Mutex* getRefMutex() const { return _refMutex; }
#endif

        /** Get the optional global Referenced mutex, this can be shared between all osg::Referenced.*/
        static OpenThreads::Mutex* getGlobalReferencedMutex();

        /** Increment the reference count by one, indicating that
            this object has another pointer which is referencing it.*/
        inline int ref() const;

        /** Decrement the reference count by one, indicating that
            a pointer to this object is no longer referencing it.  If the
            reference count goes to zero, it is assumed that this object
            is no longer referenced and is automatically deleted.*/
        inline int unref() const;

        /** Decrement the reference count by one, indicating that
            a pointer to this object is no longer referencing it.  However, do
            not delete it, even if ref count goes to 0.  Warning, unref_nodelete()
            should only be called if the user knows exactly who will
            be responsible for, one should prefer unref() over unref_nodelete()
            as the latter can lead to memory leaks.*/
        int unref_nodelete() const;

        /** Return the number of pointers currently referencing this object. */
        inline int referenceCount() const { return _refCount; }


        /** Get the ObserverSet if one is attached, otherwise return NULL.*/
        ObserverSet* getObserverSet() const
        {
            #if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
                return static_cast<ObserverSet*>(_observerSet.get());
            #else
                return static_cast<ObserverSet*>(_observerSet);
            #endif
        }

        /** Get the ObserverSet if one is attached, otherwise create an ObserverSet, attach it, then return this newly created ObserverSet.*/
        ObserverSet* getOrCreateObserverSet() const;

        /** Add a Observer that is observing this object, notify the Observer when this object gets deleted.*/
        void addObserver(Observer* observer) const;

        /** Remove Observer that is observing this object.*/
        void removeObserver(Observer* observer) const;

    public:

        friend class DeleteHandler;

        /** Set a DeleteHandler to which deletion of all referenced counted objects
          * will be delegated.*/
        static void setDeleteHandler(DeleteHandler* handler);

        /** Get a DeleteHandler.*/
        static DeleteHandler* getDeleteHandler();


    protected:

        virtual ~Referenced();

        void signalObserversAndDelete(bool signalDelete, bool doDelete) const;

        void deleteUsingDeleteHandler() const;

#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
        mutable OpenThreads::AtomicPtr  _observerSet;

        mutable OpenThreads::Atomic     _refCount;
#else

        mutable OpenThreads::Mutex*     _refMutex;

        mutable int                     _refCount;

        mutable void*                   _observerSet;
#endif
};

inline int Referenced::ref() const
{
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    return ++_refCount;
#else
    if (_refMutex)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(*_refMutex);
        return ++_refCount;
    }
    else
    {
        return ++_refCount;
    }
#endif
}

inline int Referenced::unref() const
{
    int newRef;
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    newRef = --_refCount;
    bool needDelete = (newRef == 0);
#else
    bool needDelete = false;
    if (_refMutex)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(*_refMutex);
        newRef = --_refCount;
        needDelete = newRef==0;
    }
    else
    {
        newRef = --_refCount;
        needDelete = newRef==0;
    }
#endif

    if (needDelete)
    {
        signalObserversAndDelete(true,true);
    }
    return newRef;
}

// intrusive_ptr_add_ref and intrusive_ptr_release allow
// use of osg Referenced classes with boost::intrusive_ptr
inline void intrusive_ptr_add_ref(Referenced* p) { p->ref(); }
inline void intrusive_ptr_release(Referenced* p) { p->unref(); }

}


// OSGFILE include/osg/ref_ptr

//#include <osg/Config>

#ifdef OSG_USE_REF_PTR_SAFE_DEREFERENCE
#include <typeinfo>
#include <stdexcept>
#include <string>
#endif

namespace osg {

template<typename T> class observer_ptr;

/** Smart pointer for handling referenced counted objects.*/
template<class T>
class ref_ptr
{
    public:
        typedef T element_type;

        ref_ptr() : _ptr(0) {}
        ref_ptr(T* ptr) : _ptr(ptr) { if (_ptr) _ptr->ref(); }
        ref_ptr(const ref_ptr& rp) : _ptr(rp._ptr) { if (_ptr) _ptr->ref(); }
        template<class Other> ref_ptr(const ref_ptr<Other>& rp) : _ptr(rp._ptr) { if (_ptr) _ptr->ref(); }
        ref_ptr(observer_ptr<T>& optr) : _ptr(0) { optr.lock(*this); }
        ~ref_ptr() { if (_ptr) _ptr->unref();  _ptr = 0; }

        ref_ptr& operator = (const ref_ptr& rp)
        {
            assign(rp);
            return *this;
        }

        template<class Other> ref_ptr& operator = (const ref_ptr<Other>& rp)
        {
            assign(rp);
            return *this;
        }

        inline ref_ptr& operator = (T* ptr)
        {
            if (_ptr==ptr) return *this;
            T* tmp_ptr = _ptr;
            _ptr = ptr;
            if (_ptr) _ptr->ref();
            // unref second to prevent any deletion of any object which might
            // be referenced by the other object. i.e rp is child of the
            // original _ptr.
            if (tmp_ptr) tmp_ptr->unref();
            return *this;
        }

#ifdef OSG_USE_REF_PTR_IMPLICIT_OUTPUT_CONVERSION
        // implicit output conversion
        operator T*() const { return _ptr; }
#else
        // comparison operators for ref_ptr.
        bool operator == (const ref_ptr& rp) const { return (_ptr==rp._ptr); }
        bool operator == (const T* ptr) const { return (_ptr==ptr); }
        friend bool operator == (const T* ptr, const ref_ptr& rp) { return (ptr==rp._ptr); }

        bool operator != (const ref_ptr& rp) const { return (_ptr!=rp._ptr); }
        bool operator != (const T* ptr) const { return (_ptr!=ptr); }
        friend bool operator != (const T* ptr, const ref_ptr& rp) { return (ptr!=rp._ptr); }

        bool operator < (const ref_ptr& rp) const { return (_ptr<rp._ptr); }


    // follows is an implementation of the "safe bool idiom", details can be found at:
    //   http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Safe_bool
    //   http://lists.boost.org/Archives/boost/2003/09/52856.php

    private:
        typedef T* ref_ptr::*unspecified_bool_type;

    public:
        // safe bool conversion
        operator unspecified_bool_type() const { return valid()? &ref_ptr::_ptr : 0; }
#endif

        T& operator*() const
        {
#ifdef OSG_USE_REF_PTR_SAFE_DEREFERENCE
            if( !_ptr ) {
                // pointer is invalid, so throw an exception
                throw std::runtime_error(std::string("could not dereference invalid osg pointer ") + std::string(typeid(T).name()));
            }
#endif
            return *_ptr;
        }
        T* operator->() const
        {
#ifdef OSG_USE_REF_PTR_SAFE_DEREFERENCE
            if( !_ptr ) {
                // pointer is invalid, so throw an exception.
                throw std::runtime_error(std::string("could not call invalid osg pointer ") + std::string(typeid(T).name()));
            }
#endif
            return _ptr;
        }

        T* get() const { return _ptr; }

        bool operator!() const   { return _ptr==0; } // not required
        bool valid() const       { return _ptr!=0; }

        /** release the pointer from ownership by this ref_ptr<>, decrementing the objects refencedCount() via unref_nodelete() to prevent the Object
          * object from being deleted even if the reference count goes to zero.  Use when using a local ref_ptr<> to an Object that you want to return
          * from a function/method via a C pointer, whilst preventing the normal ref_ptr<> destructor from cleaning up the object. When using release()
          * you are implicitly expecting other code to take over management of the object, otherwise a memory leak will result. */
        T* release() { T* tmp=_ptr; if (_ptr) _ptr->unref_nodelete(); _ptr=0; return tmp; }

        void swap(ref_ptr& rp) { T* tmp=_ptr; _ptr=rp._ptr; rp._ptr=tmp; }

    private:

        template<class Other> void assign(const ref_ptr<Other>& rp)
        {
            if (_ptr==rp._ptr) return;
            T* tmp_ptr = _ptr;
            _ptr = rp._ptr;
            if (_ptr) _ptr->ref();
            // unref second to prevent any deletion of any object which might
            // be referenced by the other object. i.e rp is child of the
            // original _ptr.
            if (tmp_ptr) tmp_ptr->unref();
        }

        template<class Other> friend class ref_ptr;

        T* _ptr;
};


template<class T> inline
void swap(ref_ptr<T>& rp1, ref_ptr<T>& rp2) { rp1.swap(rp2); }

template<class T> inline
T* get_pointer(const ref_ptr<T>& rp) { return rp.get(); }

template<class T, class Y> inline
ref_ptr<T> static_pointer_cast(const ref_ptr<Y>& rp) { return static_cast<T*>(rp.get()); }

template<class T, class Y> inline
ref_ptr<T> dynamic_pointer_cast(const ref_ptr<Y>& rp) { return dynamic_cast<T*>(rp.get()); }

template<class T, class Y> inline
ref_ptr<T> const_pointer_cast(const ref_ptr<Y>& rp) { return const_cast<T*>(rp.get()); }

}


// OSGFILE include/osg/Observer

//#include <OpenThreads/Mutex>
//#include <osg/Referenced>
#include <set>

namespace osg {

/** Observer base class for tracking when objects are unreferenced (their reference count goes to 0) and are being deleted.*/
class OSG_EXPORT Observer
{
    public:
        Observer();
        virtual ~Observer();

        /** objectDeleted is called when the observed object is about to be deleted.  The observer will be automatically
        * removed from the observed object's observer set so there is no need for the objectDeleted implementation
        * to call removeObserver() on the observed object. */
        virtual void objectDeleted(void*) {}

};

/** Class used by osg::Referenced to track the observers associated with it.*/
class OSG_EXPORT ObserverSet : public osg::Referenced
{
    public:

        ObserverSet(const Referenced* observedObject);

        Referenced* getObserverdObject() { return _observedObject; }
        const Referenced* getObserverdObject() const { return _observedObject; }

        /** "Lock" a Referenced object i.e., protect it from being deleted
          *  by incrementing its reference count.
          *
          * returns null if object doesn't exist anymore. */
        Referenced* addRefLock();

        inline OpenThreads::Mutex* getObserverSetMutex() const { return &_mutex; }

        void addObserver(Observer* observer);
        void removeObserver(Observer* observer);

        void signalObjectDeleted(void* ptr);

        typedef std::set<Observer*> Observers;
        Observers& getObservers() { return _observers; }
        const Observers& getObservers() const { return _observers; }

    protected:

        ObserverSet(const ObserverSet& rhs): osg::Referenced(rhs) {}
        ObserverSet& operator = (const ObserverSet& /*rhs*/) { return *this; }
        virtual ~ObserverSet();

        mutable OpenThreads::Mutex      _mutex;
        Referenced*                     _observedObject;
        Observers                       _observers;
};

}


// OSGFILE include/osg/observer_ptr

/*
#include <osg/Notify>
#include <osg/ref_ptr>
#include <osg/Observer>

#include <OpenThreads/ScopedLock>
#include <OpenThreads/Mutex>
*/

namespace osg {

/** Smart pointer for observed objects, that automatically set pointers to them to null when they are deleted.
  * To use the observer_ptr<> robustly in multi-threaded applications it is recommend to access the pointer via
  * the lock() method that passes back a ref_ptr<> that safely takes a reference to the object to prevent deletion
  * during usage of the object.  In certain conditions it may be safe to use the pointer directly without using lock(),
  * which will confer a performance advantage, the conditions are:
  *   1) The data structure is only accessed/deleted in single threaded/serial way.
  *   2) The data strucutre is guaranteed by high level management of data strucutures and threads which avoid
  *      possible situations where the observer_ptr<>'s object may be deleted by one thread whilst being accessed
  *      by another.
  * If you are in any doubt about whether it is safe to access the object safe then use the
  * ref_ptr<> observer_ptr<>.lock() combination. */
template<class T>
class observer_ptr
{
public:
    typedef T element_type;
    observer_ptr() : _reference(0), _ptr(0) {}

    /**
     * Create a observer_ptr from a ref_ptr.
     */
    observer_ptr(const ref_ptr<T>& rp)
    {
        _reference = rp.valid() ? rp->getOrCreateObserverSet() : 0;
        _ptr = (_reference.valid() && _reference->getObserverdObject()!=0) ? rp.get() : 0;
    }

    /**
     * Create a observer_ptr from a raw pointer. For compatibility;
     * the result might not be lockable.
     */
    observer_ptr(T* rp)
    {
        _reference = rp ? rp->getOrCreateObserverSet() : 0;
        _ptr = (_reference.valid() && _reference->getObserverdObject()!=0) ? rp : 0;
    }

    observer_ptr(const observer_ptr& wp) :
        _reference(wp._reference),
        _ptr(wp._ptr)
    {
    }

    ~observer_ptr()
    {
    }

    observer_ptr& operator = (const observer_ptr& wp)
    {
        if (&wp==this) return *this;

        _reference = wp._reference;
        _ptr = wp._ptr;
        return *this;
    }

    observer_ptr& operator = (const ref_ptr<T>& rp)
    {
        _reference = rp.valid() ? rp->getOrCreateObserverSet() : 0;
        _ptr = (_reference.valid() && _reference->getObserverdObject()!=0) ? rp.get() : 0;
        return *this;
    }

    observer_ptr& operator = (T* rp)
    {
        _reference = rp ? rp->getOrCreateObserverSet() : 0;
        _ptr = (_reference.valid() && _reference->getObserverdObject()!=0) ? rp : 0;
        return *this;
    }

    /**
     * Assign the observer_ptr to a ref_ptr. The ref_ptr will be valid if the
     * referenced object hasn't been deleted and has a ref count > 0.
     */
    bool lock(ref_ptr<T>& rptr) const
    {
        if (!_reference)
        {
            rptr = 0;
            return false;
        }

        Referenced* obj = _reference->addRefLock();
        if (!obj)
        {
            rptr = 0;
            return false;
        }

        rptr = _ptr;
        obj->unref_nodelete();
        return rptr.valid();
    }

    /** Comparison operators. These continue to work even after the
     * observed object has been deleted.
     */
    bool operator == (const observer_ptr& wp) const { return _reference == wp._reference; }
    bool operator != (const observer_ptr& wp) const { return _reference != wp._reference; }
    bool operator < (const observer_ptr& wp) const { return _reference < wp._reference; }
    /*
     * OPERATOR>
     * NOTE Somehow `reference`s do not have operator> for the below to work
     * NOTE Fix later if necessary
    bool operator > (const observer_ptr& wp) const { return _reference > wp._reference; }
    */

    // Non-strict interface, for compatibility
    // comparison operator for const T*.
    inline bool operator == (const T* ptr) const { return _ptr == ptr; }
    inline bool operator != (const T* ptr) const { return _ptr != ptr; }
    inline bool operator < (const T* ptr) const { return _ptr < ptr; }
    inline bool operator > (const T* ptr) const { return _ptr > ptr; }

    // Convenience methods for operating on object, however, access is not automatically threadsafe.
    // To make thread safe, one should either ensure at a high level
    // that the object will not be deleted while operating on it, or
    // by using the observer_ptr<>::lock() to get a ref_ptr<> that
    // ensures the objects stay alive throughout all access to it.

    // Throw an error if _reference is null?
    inline T& operator*() const { return *_ptr; }
    inline T* operator->() const { return _ptr; }

    // get the raw C pointer
    inline T* get() const { return (_reference.valid() && _reference->getObserverdObject()!=0) ? _ptr : 0; }

    inline bool operator!() const   { return get() == 0; }
    inline bool valid() const       { return get() != 0; }

protected:

    osg::ref_ptr<ObserverSet>   _reference;
    T*                          _ptr;
};

}


// OSGFILE include/osg/CopyOp

//#include <osg/Export>

namespace osg {

class Referenced;
class Object;
class Image;
class Texture;
class StateSet;
class StateAttribute;
class StateAttributeCallback;
class Uniform;
class UniformBase;
class UniformCallback;
class Node;
class Drawable;
class Array;
class PrimitiveSet;
class Shape;
class Callback;


/** Copy Op(erator) used to control whether shallow or deep copy is used
  * during copy construction and clone operation.*/
class OSG_EXPORT CopyOp
{

    public:

        enum Options
        {
            SHALLOW_COPY                = 0,
            DEEP_COPY_OBJECTS           = 1<<0,
            DEEP_COPY_NODES             = 1<<1,
            DEEP_COPY_DRAWABLES         = 1<<2,
            DEEP_COPY_STATESETS         = 1<<3,
            DEEP_COPY_STATEATTRIBUTES   = 1<<4,
            DEEP_COPY_TEXTURES          = 1<<5,
            DEEP_COPY_IMAGES            = 1<<6,
            DEEP_COPY_ARRAYS            = 1<<7,
            DEEP_COPY_PRIMITIVES        = 1<<8,
            DEEP_COPY_SHAPES            = 1<<9,
            DEEP_COPY_UNIFORMS          = 1<<10,
            DEEP_COPY_CALLBACKS         = 1<<11,
            DEEP_COPY_USERDATA          = 1<<12,
            DEEP_COPY_ALL               = 0x7FFFFFFF
        };

        typedef unsigned int CopyFlags;

        inline CopyOp(CopyFlags flags=SHALLOW_COPY):_flags(flags) {}
        virtual ~CopyOp() {}

        void setCopyFlags(CopyFlags flags) { _flags = flags; }
        CopyFlags getCopyFlags() const { return _flags; }

        virtual Referenced*     operator() (const Referenced* ref) const;
        virtual Object*         operator() (const Object* obj) const;
        virtual Node*           operator() (const Node* node) const;
        virtual Drawable*       operator() (const Drawable* drawable) const;
        virtual StateSet*       operator() (const StateSet* stateset) const;
        virtual StateAttribute* operator() (const StateAttribute* attr) const;
        virtual Texture*        operator() (const Texture* text) const;
        virtual Image*          operator() (const Image* image) const;
        virtual Array*          operator() (const Array* array) const;
        virtual PrimitiveSet*   operator() (const PrimitiveSet* primitives) const;
        virtual Shape*          operator() (const Shape* shape) const;
        virtual UniformBase*    operator() (const UniformBase* uniform) const;
        virtual Uniform*        operator() (const Uniform* uniform) const;
        virtual Callback*       operator() (const Callback* nodecallback) const;
        virtual StateAttributeCallback* operator() (const StateAttributeCallback* stateattributecallback) const;
        virtual UniformCallback*        operator() (const UniformCallback* uniformcallback) const;

    protected:

        CopyFlags _flags;
};

}


// OSGFILE include/osg/Notify

//#include <osg/Export>
//#include <osg/Referenced> // for NotifyHandler

#include <ostream>

namespace osg {

/** Range of notify levels from DEBUG_FP through to FATAL, ALWAYS
  * is reserved for forcing the absorption of all messages.  The
  * keywords are also used verbatim when specified by the environmental
  * variable OSGNOTIFYLEVEL or OSG_NOTIFY_LEVEL.
  * See documentation on osg::notify() for further details.
  */
enum NotifySeverity {
    ALWAYS=0,
    FATAL=1,
    WARN=2,
    NOTICE=3,
    INFO=4,
    DEBUG_INFO=5,
    DEBUG_FP=6
};

/** set the notify level, overriding the default or the value set by
  * the environmental variable OSGNOTIFYLEVEL or OSG_NOTIFY_LEVEL.
  */
extern OSG_EXPORT void setNotifyLevel(NotifySeverity severity);

/** get the notify level. */
extern OSG_EXPORT NotifySeverity getNotifyLevel();

/** initialize notify level. */
extern OSG_EXPORT bool initNotifyLevel();

#ifdef OSG_NOTIFY_DISABLED
    inline bool isNotifyEnabled(NotifySeverity) { return false; }
#else
    /** is notification enabled, given the current setNotifyLevel() setting? */
    extern OSG_EXPORT bool isNotifyEnabled(NotifySeverity severity);
#endif

/** notify messaging function for providing fatal through to verbose
  * debugging messages.  Level of messages sent to the console can
  * be controlled by setting the NotifyLevel either within your
  * application or via the an environmental variable i.e.
  * - setenv OSGNOTIFYLEVEL DEBUG (for tsh)
  * - export OSGNOTIFYLEVEL=DEBUG (for bourne shell)
  * - set OSGNOTIFYLEVEL=DEBUG (for Windows)
  *
  * All tell the osg to redirect all debugging and more important messages
  * to the notification stream (useful for debugging) setting ALWAYS will force
  * all messages to be absorbed, which might be appropriate for final
  * applications.  Default NotifyLevel is NOTICE.  Check the enum
  * #NotifySeverity for full range of possibilities.  To use the notify
  * with your code simply use the notify function as a normal file
  * stream (like std::cout) i.e
  * @code
  * osg::notify(osg::DEBUG) << "Hello Bugs!" << std::endl;
  * @endcode
  * @see setNotifyLevel, setNotifyHandler
  */
extern OSG_EXPORT std::ostream& notify(const NotifySeverity severity);

inline std::ostream& notify(void) { return notify(osg::INFO); }

#define OSG_NOTIFY(level) if (osg::isNotifyEnabled(level)) osg::notify(level)
#define OSG_ALWAYS OSG_NOTIFY(osg::ALWAYS)
#define OSG_FATAL OSG_NOTIFY(osg::FATAL)
#define OSG_WARN OSG_NOTIFY(osg::WARN)
#define OSG_NOTICE OSG_NOTIFY(osg::NOTICE)
#define OSG_INFO OSG_NOTIFY(osg::INFO)
#define OSG_DEBUG OSG_NOTIFY(osg::DEBUG_INFO)
#define OSG_DEBUG_FP OSG_NOTIFY(osg::DEBUG_FP)

/** Handler processing output of notification stream. It acts as a sink to
  * notification messages. It is called when notification stream needs to be
  * synchronized (i.e. after osg::notify() << std::endl).
  * StandardNotifyHandler is used by default, it writes notifications to stderr
  * (severity <= WARN) or stdout (severity > WARN).
  * Notifications can be redirected to other sinks such as GUI widgets or
  * windows debugger (WinDebugNotifyHandler) with custom handlers.
  * Use setNotifyHandler to set custom handler.
  * Note that osg notification API is not thread safe although notification
  * handler is called from many threads. When incorporating handlers into GUI
  * widgets you must take care of thread safety on your own.
  * @see setNotifyHandler
  */
class OSG_EXPORT NotifyHandler : public osg::Referenced
{
public:
    virtual void notify(osg::NotifySeverity severity, const char *message) = 0;
};

/** Set notification handler, by default StandardNotifyHandler is used.
  * @see NotifyHandler
  */
extern OSG_EXPORT void setNotifyHandler(NotifyHandler *handler);

/** Get currrent notification handler. */
extern OSG_EXPORT NotifyHandler *getNotifyHandler();

/** Redirects notification stream to stderr (severity <= WARN) or stdout (severity > WARN).
  * The fputs() function is used to write messages to standard files. Note that
  * std::out and std::cerr streams are not used.
  * @see setNotifyHandler
  */
class OSG_EXPORT StandardNotifyHandler : public NotifyHandler
{
public:
    void notify(osg::NotifySeverity severity, const char *message);
};

#if defined(WIN32) && !defined(__CYGWIN__)

/** Redirects notification stream to windows debugger with use of
  * OuputDebugString functions.
  * @see setNotifyHandler
  */
class OSG_EXPORT WinDebugNotifyHandler : public NotifyHandler
{
public:
    void notify(osg::NotifySeverity severity, const char *message);
};

#endif

}


// OSGFILE include/osg/Object

/*
#include <osg/Referenced>
#include <osg/CopyOp>
#include <osg/ref_ptr>
#include <osg/Notify>
*/

#include <string>
#include <vector>

namespace osg {

// forward declare
class State;
class UserDataContainer;
class Node;
class NodeVisitor;
class StateAttribute;
class Uniform;
class Drawable;
class Camera;
class Callback;
class CallbackObject;
class ValueObject;

#define _ADDQUOTES(def) #def
#define ADDQUOTES(def) _ADDQUOTES(def)

/** META_Object macro define the standard clone, isSameKindAs and className methods.
  * Use when subclassing from Object to make it more convenient to define
  * the standard pure virtual clone, isSameKindAs and className methods
  * which are required for all Object subclasses.*/
#define META_Object(library,name) \
        virtual osg::Object* cloneType() const { return new name (); } \
        virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new name (*this,copyop); } \
        virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const name *>(obj)!=NULL; } \
        virtual const char* libraryName() const { return #library; }\
        virtual const char* className() const { return #name; }

/** Helper macro that creates a static proxy object to call singleton function on it's construction, ensuring that the singleton gets initialized at startup.*/
#define OSG_INIT_SINGLETON_PROXY(ProxyName, Func) static struct ProxyName{ ProxyName() { Func; } } s_##ProxyName;

/** Base class/standard interface for objects which require IO support,
    cloning and reference counting.
    Based on GOF Composite, Prototype and Template Method patterns.
*/
class OSG_EXPORT Object : public Referenced
{
    public:


        /** Construct an object. Note Object is a pure virtual base class
            and therefore cannot be constructed on its own, only derived
            classes which override the clone and className methods are
            concrete classes and can be constructed.*/
        inline Object():Referenced(),_dataVariance(UNSPECIFIED), _userDataContainer(0) {}

        inline explicit Object(bool threadSafeRefUnref):Referenced(threadSafeRefUnref),_dataVariance(UNSPECIFIED),_userDataContainer(0) {}

        /** Copy constructor, optional CopyOp object can be used to control
          * shallow vs deep copying of dynamic data.*/
        Object(const Object&,const CopyOp& copyop=CopyOp::SHALLOW_COPY);

        /** Clone the type of an object, with Object* return type.
            Must be defined by derived classes.*/
        virtual Object* cloneType() const = 0;

        /** Clone an object, with Object* return type.
            Must be defined by derived classes.*/
        virtual Object* clone(const CopyOp&) const = 0;

        virtual bool isSameKindAs(const Object*) const { return true; }


        /** return the name of the object's library. Must be defined
            by derived classes. The OpenSceneGraph convention is that the
            namespace of a library is the same as the library name.*/
        virtual const char* libraryName() const = 0;

        /** return the name of the object's class type. Must be defined
            by derived classes.*/
        virtual const char* className() const = 0;

        /** return the compound class name that combines the library name and class name.*/
        std::string getCompoundClassName() const { return std::string(libraryName()) + std::string("::") + std::string(className()); }


        /** Convert 'this' into a Node pointer if Object is a Node, otherwise return 0.
          * Equivalent to dynamic_cast<Node*>(this).*/
        virtual Node* asNode() { return 0; }

        /** convert 'const this' into a const Node pointer if Object is a Node, otherwise return 0.
          * Equivalent to dynamic_cast<const Node*>(this).*/
        virtual const Node* asNode() const { return 0; }

        /** Convert 'this' into a NodeVisitor pointer if Object is a NodeVisitor, otherwise return 0.
          * Equivalent to dynamic_cast<NodeVisitor*>(this).*/
        virtual NodeVisitor* asNodeVisitor() { return 0; }

        /** convert 'const this' into a const NodeVisitor pointer if Object is a NodeVisitor, otherwise return 0.
          * Equivalent to dynamic_cast<const NodeVisitor*>(this).*/
        virtual const NodeVisitor* asNodeVisitor() const { return 0; }

        /** Convert 'this' into a StateSet pointer if Object is a StateSet, otherwise return 0.
          * Equivalent to dynamic_cast<StateSet*>(this).*/
        virtual StateSet* asStateSet() { return 0; }

        /** convert 'const this' into a const StateSet pointer if Object is a StateSet, otherwise return 0.
          * Equivalent to dynamic_cast<const StateSet*>(this).*/
        virtual const StateSet* asStateSet() const { return 0; }

        /** Convert 'this' into a StateAttribute pointer if Object is a StateAttribute, otherwise return 0.
          * Equivalent to dynamic_cast<StateAttribute*>(this).*/
        virtual StateAttribute* asStateAttribute() { return 0; }

        /** convert 'const this' into a const StateAttribute pointer if Object is a StateAttribute, otherwise return 0.
          * Equivalent to dynamic_cast<const StateAttribute*>(this).*/
        virtual const StateAttribute* asStateAttribute() const { return 0; }

        /** Convert 'this' into a Uniform pointer if Object is a Uniform, otherwise return 0.
          * Equivalent to dynamic_cast<Uniform*>(this).*/
        virtual Uniform* asUniform() { return 0; }

        /** convert 'const this' into a const Uniform pointer if Object is a Uniform, otherwise return 0.
          * Equivalent to dynamic_cast<const UniformBase*>(this).*/
        virtual const UniformBase* asUniformBase() const { return 0; }

        /** Convert 'this' into a Uniform pointer if Object is a Uniform, otherwise return 0.
          * Equivalent to dynamic_cast<UniformBase*>(this).*/
        virtual UniformBase* asUniformBase() { return 0; }

        /** convert 'const this' into a const Uniform pointer if Object is a Uniform, otherwise return 0.
          * Equivalent to dynamic_cast<const Uniform*>(this).*/
        virtual const Uniform* asUniform() const { return 0; }

        /** Convert 'this' into a Camera pointer if Node is a Camera, otherwise return 0.
          * Equivalent to dynamic_cast<Camera*>(this).*/
        virtual Camera* asCamera() { return 0; }

        /** convert 'const this' into a const Camera pointer if Node is a Camera, otherwise return 0.
          * Equivalent to dynamic_cast<const Camera*>(this).*/
        virtual const Camera* asCamera() const { return 0; }

        /** Convert 'this' into a Drawable pointer if Object is a Drawable, otherwise return 0.
          * Equivalent to dynamic_cast<Drawable*>(this).*/
        virtual Drawable* asDrawable() { return 0; }

        /** convert 'const this' into a const Drawable pointer if Object is a Drawable, otherwise return 0.
          * Equivalent to dynamic_cast<const Drawable*>(this).*/
        virtual const Drawable* asDrawable() const { return 0; }

        /** Convert 'this' into a Callback pointer if Object is a Callback, otherwise return 0.
          * Equivalent to dynamic_cast<Callback*>(this).*/
        virtual Callback* asCallback() { return 0; }

        /** convert 'const this' into a const Callback pointer if Object is a Callback, otherwise return 0.
          * Equivalent to dynamic_cast<const Callback*>(this).*/
        virtual const Callback* asCallback() const { return 0; }

        /** Convert 'this' into a CallbackObject pointer if Object is a CallbackObject, otherwise return 0.
          * Equivalent to dynamic_cast<CallbackObject*>(this).*/
        virtual CallbackObject* asCallbackObject() { return 0; }

        /** convert 'const this' into a const CallbackObject pointer if Object is a CallbackObject, otherwise return 0.
          * Equivalent to dynamic_cast<const CallbackObject*>(this).*/
        virtual const CallbackObject* asCallbackObject() const { return 0; }

        /** Convert 'this' into a UserDataContainer pointer if Object is a UserDataContainer, otherwise return 0.
          * Equivalent to dynamic_cast<UserDataContainer*>(this).*/
        virtual UserDataContainer* asUserDataContainer() { return 0; }

        /** convert 'const this' into a const UserDataContainer pointer if Object is a UserDataContainer, otherwise return 0.
          * Equivalent to dynamic_cast<const UserDataContainer*>(this).*/
        virtual const UserDataContainer* asUserDataContainer() const { return 0; }

        /** Convert 'this' into a ValueObject pointer if Object is a ValueObject, otherwise return 0.
          * Equivalent to dynamic_cast<ValueObject*>(this).*/
        virtual ValueObject* asValueObject() { return 0; }

        /** Convert 'this' into a ValueObject pointer if Object is a ValueObject, otherwise return 0.
          * Equivalent to dynamic_cast<ValueObject*>(this).*/
        virtual const ValueObject* asValueObject() const { return 0; }

        /** Convert 'this' into a Image pointer if Object is a Image, otherwise return 0.
          * Equivalent to dynamic_cast<Image*>(this).*/
        virtual Image* asImage() { return 0; }

        /** Convert 'this' into a Image pointer if Object is a Image, otherwise return 0.
          * Equivalent to dynamic_cast<Image*>(this).*/
        virtual const Image* asImage() const { return 0; }



        /** Set whether to use a mutex to ensure ref() and unref() are thread safe.*/
        virtual void setThreadSafeRefUnref(bool threadSafe);

        /** Set the name of object using C++ style string.*/
        virtual void setName( const std::string& name ) { _name = name; }

        /** Set the name of object using a C style string.*/
        inline void setName( const char* name )
        {
            if (name) setName(std::string(name));
            else setName(std::string());
        }

        /** Get the name of object.*/
        inline const std::string& getName() const { return _name; }


        enum DataVariance
        {
            DYNAMIC,
            STATIC,
            UNSPECIFIED
        };

        /** Set the data variance of this object.
           * Can be set to either STATIC for values that do not change over the lifetime of the object,
           * or DYNAMIC for values that vary over the lifetime of the object. The DataVariance value
           * can be used by routines such as optimization codes that wish to share static data.
           * UNSPECIFIED is used to specify that the DataVariance hasn't been set yet. */
        inline void setDataVariance(DataVariance dv) { _dataVariance = dv; }

        /** Get the data variance of this object.*/
        inline DataVariance getDataVariance() const { return _dataVariance; }

        /** Compute the DataVariance based on an assessment of callback etc.*/
        virtual void computeDataVariance() {}


        /** set the UserDataContainer object.*/
        void setUserDataContainer(osg::UserDataContainer* udc);

        template<class T> void setUserDataContainer(const ref_ptr<T>& udc) { setUserDataContainer(udc.get()); }

        /** get the UserDataContainer attached to this object.*/
        osg::UserDataContainer* getUserDataContainer() { return _userDataContainer; }

        /** get the const UserDataContainer attached to this object.*/
        const osg::UserDataContainer* getUserDataContainer() const { return _userDataContainer; }

        /** Convenience method that returns the UserDataContainer, and if one doesn't already exist creates and assigns
         * a DefaultUserDataContainer to the Object and then return this new UserDataContainer.*/
        osg::UserDataContainer* getOrCreateUserDataContainer();


        /**
         * Set user data, data must be subclassed from Referenced to allow
         * automatic memory handling.  If your own data isn't directly
         * subclassed from Referenced then create an adapter object
         * which points to your own object and handles the memory addressing.
         */
        virtual void setUserData(Referenced* obj);

        template<class T> void setUserData(const ref_ptr<T>& ud) { setUserData(ud.get()); }

        /** Get user data.*/
        virtual Referenced* getUserData();

        /** Get const user data.*/
        virtual const Referenced* getUserData() const;



        /** Convenience method that casts the named UserObject to osg::TemplateValueObject<T> and gets the value.
          * To use this template method you need to include the osg/ValueObject header.*/
        template<typename T>
        bool getUserValue(const std::string& name, T& value) const;

        /** Convenience method that creates the osg::TemplateValueObject<T> to store the
          * specified value and adds it as a named UserObject.
          * To use this template method you need to include the osg/ValueObject header. */
        template<typename T>
        void setUserValue(const std::string& name, const T& value);


        /** Resize any per context GLObject buffers to specified size. */
        virtual void resizeGLObjectBuffers(unsigned int /*maxSize*/) {}

        /** If State is non-zero, this function releases any associated OpenGL objects for
           * the specified graphics context. Otherwise, releases OpenGL objects
           * for all graphics contexts. */
        virtual void releaseGLObjects(osg::State* = 0) const {}


    protected:

        /** Object destructor. Note, is protected so that Objects cannot
            be deleted other than by being dereferenced and the reference
            count being zero (see osg::Referenced), preventing the deletion
            of nodes which are still in use. This also means that
            Nodes cannot be created on stack i.e Node node will not compile,
            forcing all nodes to be created on the heap i.e Node* node
            = new Node().*/
        virtual ~Object();

        std::string _name;
        DataVariance _dataVariance;

        osg::UserDataContainer* _userDataContainer;

    private:

        /** disallow any copy operator.*/
        Object& operator = (const Object&) { return *this; }
};

template<typename T>
T* clone(const T* t, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
{
    if (t)
    {
        osg::ref_ptr<osg::Object> obj = t->clone(copyop);

        T* ptr = dynamic_cast<T*>(obj.get());
        if (ptr)
        {
            obj.release();
            return ptr;
        }
        else
        {
            OSG_WARN<<"Warning: osg::clone(const T*, osg::CopyOp&) cloned object not of type T, returning NULL."<<std::endl;
            return 0;
        }
    }
    else
    {
        OSG_WARN<<"Warning: osg::clone(const T*, osg::CopyOp&) passed null object to clone, returning NULL."<<std::endl;
        return 0;
    }
}

template<typename T>
T* clone(const T* t, const std::string& name, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
{
    T* newObject = osg::clone(t, copyop);
    if (newObject)
    {
        newObject->setName(name);
        return newObject;
    }
    else
    {
        OSG_WARN<<"Warning: osg::clone(const T*, const std::string&, const osg::CopyOp) passed null object to clone, returning NULL."<<std::endl;
        return 0;
    }
}

template<typename T>
T* cloneType(const T* t)
{
    if (t)
    {
        osg::ref_ptr<osg::Object> obj = t->cloneType();

        T* ptr = dynamic_cast<T*>(obj.get());
        if (ptr)
        {
            obj.release();
            return ptr;
        }
        else
        {
            OSG_WARN<<"Warning: osg::cloneType(const T*) cloned object not of type T, returning NULL."<<std::endl;
            return 0;
        }
    }
    else
    {
        OSG_WARN<<"Warning: osg::cloneType(const T*) passed null object to clone, returning NULL."<<std::endl;
        return 0;
    }
}

/** DummyObject that can be used as placeholder but otherwise has no other functionality.*/
class DummyObject : public osg::Object
{
public:
    DummyObject() {}
    DummyObject(const DummyObject& org, const CopyOp& copyop) :
        Object(org, copyop) {}

    META_Object(osg, DummyObject)
protected:
    virtual ~DummyObject() {}
};


inline void resizeGLObjectBuffers(osg::Object* object, unsigned int maxSize) { if (object) object->resizeGLObjectBuffers(maxSize); }

template<class T> void resizeGLObjectBuffers(const osg::ref_ptr<T>& object, unsigned int maxSize) { if (object.valid()) object->resizeGLObjectBuffers(maxSize); }

inline void releaseGLObjects(osg::Object* object, osg::State* state = 0) { if (object) object->releaseGLObjects(state); }

template<class T> void releaseGLObjects(const osg::ref_ptr<T>& object, osg::State* state = 0) { if (object.valid()) object->releaseGLObjects(state); }

}


// OSGFILE include/osg/Math

#include <cmath>
#include <cfloat>

//#include <osg/Export>

namespace osg {

// define the standard trig values
#ifdef PI
#undef PI
#undef PI_2
#undef PI_4
#endif
const double PI   = 3.14159265358979323846;
const double PI_2 = 1.57079632679489661923;
const double PI_4 = 0.78539816339744830962;
const double LN_2 = 0.69314718055994530942;
const double INVLN_2 = 1.0 / LN_2;

const float PIf   = 3.14159265358979323846f;
const float PI_2f = 1.57079632679489661923f;
const float PI_4f = 0.78539816339744830962f;
const float LN_2f = 0.69314718055994530942f;
const float INVLN_2f = 1.0f / LN_2f;


template<typename T>
inline T default_value() { return T(); }

template<> inline float default_value<float>() { return 0.0f; }
template<> inline double default_value<double>() { return 0.0; }
template<> inline char default_value<char>() { return 0; }
template<> inline unsigned char default_value<unsigned char>() { return 0; }
template<> inline short default_value<short>() { return 0; }
template<> inline unsigned short default_value<unsigned short>() { return 0; }
template<> inline int default_value<int>() { return 0; }
template<> inline unsigned int default_value<unsigned int>() { return 0; }


template<typename T>
inline T absolute(T v) { return v<(T)0?-v:v; }

/** return true if float lhs and rhs are equivalent,
  * meaning that the difference between them is less than an epsilon value
  * which defaults to 1e-6.
*/
inline bool equivalent(float lhs,float rhs,float epsilon=1e-6)
  { float delta = rhs-lhs; return delta<0.0f?delta>=-epsilon:delta<=epsilon; }

/** return true if double lhs and rhs are equivalent,
  * meaning that the difference between them is less than an epsilon value
  * which defaults to 1e-6.
*/
inline bool equivalent(double lhs,double rhs,double epsilon=1e-6)
  { double delta = rhs-lhs; return delta<0.0?delta>=-epsilon:delta<=epsilon; }

/** return the minimum of two values, equivalent to std::min.
  * std::min not used because of STL implementation under IRIX not containing
  * std::min.
*/
template<typename T>
inline T minimum(T lhs,T rhs) { return lhs<rhs?lhs:rhs; }

/** return the maximum of two values, equivalent to std::max.
  * std::max not used because of STL implementation under IRIX not containing
  * std::max.
*/
template<typename T>
inline T maximum(T lhs,T rhs) { return lhs>rhs?lhs:rhs; }

template<typename T>
inline T clampTo(T v,T minimum,T maximum) { return v<minimum?minimum:v>maximum?maximum:v; }

template<typename T>
inline T clampAbove(T v,T minimum) { return v<minimum?minimum:v; }

template<typename T>
inline T clampBelow(T v,T maximum) { return v>maximum?maximum:v; }

template<typename T>
inline T clampBetween(T v,T minimum, T maximum)
  { return clampBelow(clampAbove(v,minimum),maximum); }

template<typename T>
inline T sign(T v) { return v<(T)0?(T)-1:(T)1; }

template<typename T>
inline T signOrZero(T v) { return v<(T)0 ? (T)-1 : ( v>(T)0 ? (T)1 : 0 ); }

template<typename T>
inline T square(T v) { return v*v; }

template<typename T>
inline T signedSquare(T v) { return v<(T)0?-v*v:v*v;; }

inline float inDegrees(float angle) { return angle*(float)PI/180.0f; }
inline double inDegrees(double angle) { return angle*PI/180.0; }

template<typename T>
inline T inRadians(T angle) { return angle; }

inline float DegreesToRadians(float angle) { return angle*(float)PI/180.0f; }
inline double DegreesToRadians(double angle) { return angle*PI/180.0; }

inline float RadiansToDegrees(float angle) { return angle*180.0f/(float)PI; }
inline double RadiansToDegrees(double angle) { return angle*180.0/PI; }

inline float round(float v) { return v>=0.0f?floorf(v+0.5f):ceilf(v-0.5f); }
inline double round(double v) { return v>=0.0?floor(v+0.5):ceil(v-0.5); }

#if defined(_MSC_VER)
    inline bool isNaN(double v) { return _isnan(v)!=0; }
#elif defined(__ANDROID__)
    inline bool isNaN(float v) { return isnan(v); }
    inline bool isNaN(double v) { return isnan(v); }
#else
    inline bool isNaN(float v) { return std::isnan(v); }
    inline bool isNaN(double v) { return std::isnan(v); }
#endif


/** compute the volume of a tetrahedron. */
template<typename T>
inline float computeVolume(const T& a,const T& b,const T& c,const T& d)
{
    return fabsf(((b-c)^(a-b))*(d-b));
}

/** compute the volume of a prism. */
template<typename T>
inline float computeVolume(const T& f1,const T& f2,const T& f3,
                           const T& b1,const T& b2,const T& b3)
{
    return computeVolume(f1,f2,f3,b1)+
           computeVolume(b1,b2,b3,f2)+
           computeVolume(b1,b3,f2,f3);
}

/** Convert a ascii number to a double, ignoring locale settings.*/
extern OSG_EXPORT double asciiToDouble(const char* str);

/** Convert a ascii number to a float, ignoring locale settings.*/
inline float asciiToFloat(const char* str) { return static_cast<float>(asciiToDouble(str)); }

/** Detect first ascii POSITIVE number in string and convert to double.*/
extern OSG_EXPORT double findAsciiToDouble(const char* str);

/** Detect first ascii POSITIVE number in string and convert to double.*/
inline float findAsciiToFloat(const char* str) { return static_cast<float>(findAsciiToDouble(str)); }

}


// OSGFILE include/osg/Vec2f

//#include <osg/Math>

namespace osg {

/** General purpose float pair. Uses include representation of
  * texture coordinates.
  * No support yet added for float * Vec2f - is it necessary?
  * Need to define a non-member non-friend operator* etc.
  * BTW: Vec2f * float is okay
*/

class Vec2f
{
    public:

        /** Data type of vector components.*/
        typedef float value_type;

        /** Number of vector components. */
        enum { num_components = 2 };

        /** Vec member variable. */
        value_type _v[2];


        /** Constructor that sets all components of the vector to zero */
        Vec2f() {_v[0]=0.0; _v[1]=0.0;}
        Vec2f(value_type x,value_type y) { _v[0]=x; _v[1]=y; }


        inline bool operator == (const Vec2f& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1]; }

        inline bool operator != (const Vec2f& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1]; }

        inline bool operator <  (const Vec2f& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else return (_v[1]<v._v[1]);
        }

        inline value_type * ptr() { return _v; }
        inline const value_type * ptr() const { return _v; }

        inline void set( value_type x, value_type y ) { _v[0]=x; _v[1]=y; }
        inline void set( const Vec2f& rhs) { _v[0]=rhs._v[0]; _v[1]=rhs._v[1]; }

        inline value_type & operator [] (int i) { return _v[i]; }
        inline value_type operator [] (int i) const { return _v[i]; }

        inline value_type & x() { return _v[0]; }
        inline value_type & y() { return _v[1]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }

        /** Returns true if all components have values that are not NaN. */
        inline bool valid() const { return !isNaN(); }
        /** Returns true if at least one component has value NaN. */
        inline bool isNaN() const { return osg::isNaN(_v[0]) || osg::isNaN(_v[1]); }

        /** Dot product. */
        inline value_type operator * (const Vec2f& rhs) const
        {
            return _v[0]*rhs._v[0]+_v[1]*rhs._v[1];
        }

        /** Multiply by scalar. */
        inline const Vec2f operator * (value_type rhs) const
        {
            return Vec2f(_v[0]*rhs, _v[1]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec2f& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline const Vec2f operator / (value_type rhs) const
        {
            return Vec2f(_v[0]/rhs, _v[1]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec2f& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            return *this;
        }

        /** Binary vector add. */
        inline const Vec2f operator + (const Vec2f& rhs) const
        {
            return Vec2f(_v[0]+rhs._v[0], _v[1]+rhs._v[1]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec2f& operator += (const Vec2f& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            return *this;
        }

        /** Binary vector subtract. */
        inline const Vec2f operator - (const Vec2f& rhs) const
        {
            return Vec2f(_v[0]-rhs._v[0], _v[1]-rhs._v[1]);
        }

        /** Unary vector subtract. */
        inline Vec2f& operator -= (const Vec2f& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec2f. */
        inline const Vec2f operator - () const
        {
            return Vec2f (-_v[0], -_v[1]);
        }

        /** Length of the vector = sqrt( vec . vec ) */
        inline value_type length() const
        {
            return sqrtf( _v[0]*_v[0] + _v[1]*_v[1] );
        }

        /** Length squared of the vector = vec . vec */
        inline value_type length2( void ) const
        {
            return _v[0]*_v[0] + _v[1]*_v[1];
        }

        /** Normalize the vector so that it has length unity.
          * Returns the previous length of the vector.
        */
        inline value_type normalize()
        {
            value_type norm = Vec2f::length();
            if (norm>0.0)
            {
                value_type inv = 1.0f/norm;
                _v[0] *= inv;
                _v[1] *= inv;
            }
            return( norm );
        }

};    // end of class Vec2f

/** multiply by vector components. */
inline Vec2f componentMultiply(const Vec2f& lhs, const Vec2f& rhs)
{
    return Vec2f(lhs[0]*rhs[0], lhs[1]*rhs[1]);
}

/** divide rhs components by rhs vector components. */
inline Vec2f componentDivide(const Vec2f& lhs, const Vec2f& rhs)
{
    return Vec2f(lhs[0]/rhs[0], lhs[1]/rhs[1]);
}

}    // end of namespace osg


// OSGFILE include/osg/Vec2d

//#include <osg/Vec2f>

namespace osg {

/** General purpose double pair, uses include representation of
  * texture coordinates.
  * No support yet added for double * Vec2d - is it necessary?
  * Need to define a non-member non-friend operator* etc.
  * BTW: Vec2d * double is okay
*/

class Vec2d
{
    public:

        /** Data type of vector components.*/
        typedef double value_type;

        /** Number of vector components. */
        enum { num_components = 2 };

        value_type _v[2];

        /** Constructor that sets all components of the vector to zero */
        Vec2d() {_v[0]=0.0; _v[1]=0.0;}

        Vec2d(value_type x,value_type y) { _v[0]=x; _v[1]=y; }

        inline Vec2d(const Vec2f& vec) { _v[0]=vec._v[0]; _v[1]=vec._v[1]; }

        inline operator Vec2f() const { return Vec2f(static_cast<float>(_v[0]),static_cast<float>(_v[1]));}


        inline bool operator == (const Vec2d& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1]; }

        inline bool operator != (const Vec2d& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1]; }

        inline bool operator <  (const Vec2d& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else return (_v[1]<v._v[1]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set( value_type x, value_type y ) { _v[0]=x; _v[1]=y; }

        inline value_type& operator [] (int i) { return _v[i]; }
        inline value_type operator [] (int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }

        /** Returns true if all components have values that are not NaN. */
        inline bool valid() const { return !isNaN(); }
        /** Returns true if at least one component has value NaN. */
        inline bool isNaN() const { return osg::isNaN(_v[0]) || osg::isNaN(_v[1]); }

        /** Dot product. */
        inline value_type operator * (const Vec2d& rhs) const
        {
            return _v[0]*rhs._v[0]+_v[1]*rhs._v[1];
        }

        /** Multiply by scalar. */
        inline const Vec2d operator * (value_type rhs) const
        {
            return Vec2d(_v[0]*rhs, _v[1]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec2d& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline const Vec2d operator / (value_type rhs) const
        {
            return Vec2d(_v[0]/rhs, _v[1]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec2d& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            return *this;
        }

        /** Binary vector add. */
        inline const Vec2d operator + (const Vec2d& rhs) const
        {
            return Vec2d(_v[0]+rhs._v[0], _v[1]+rhs._v[1]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec2d& operator += (const Vec2d& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            return *this;
        }

        /** Binary vector subtract. */
        inline const Vec2d operator - (const Vec2d& rhs) const
        {
            return Vec2d(_v[0]-rhs._v[0], _v[1]-rhs._v[1]);
        }

        /** Unary vector subtract. */
        inline Vec2d& operator -= (const Vec2d& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec2d. */
        inline const Vec2d operator - () const
        {
            return Vec2d (-_v[0], -_v[1]);
        }

        /** Length of the vector = sqrt( vec . vec ) */
        inline value_type length() const
        {
            return sqrt( _v[0]*_v[0] + _v[1]*_v[1] );
        }

        /** Length squared of the vector = vec . vec */
        inline value_type length2( void ) const
        {
            return _v[0]*_v[0] + _v[1]*_v[1];
        }

        /** Normalize the vector so that it has length unity.
          * Returns the previous length of the vector.
        */
        inline value_type normalize()
        {
            value_type norm = Vec2d::length();
            if (norm>0.0)
            {
                value_type inv = 1.0/norm;
                _v[0] *= inv;
                _v[1] *= inv;
            }
            return( norm );
        }

};    // end of class Vec2d


/** multiply by vector components. */
inline Vec2d componentMultiply(const Vec2d& lhs, const Vec2d& rhs)
{
    return Vec2d(lhs[0]*rhs[0], lhs[1]*rhs[1]);
}

/** divide rhs components by rhs vector components. */
inline Vec2d componentDivide(const Vec2d& lhs, const Vec2d& rhs)
{
    return Vec2d(lhs[0]/rhs[0], lhs[1]/rhs[1]);
}

}    // end of namespace osg


// OSGFILE include/osg/Vec3f

//#include <osg/Vec2f>
//#include <osg/Math>

namespace osg {

/** General purpose float triple for use as vertices, vectors and normals.
  * Provides general math operations from addition through to cross products.
  * No support yet added for float * Vec3f - is it necessary?
  * Need to define a non-member non-friend operator*  etc.
  * Vec3f * float is okay
*/
class Vec3f
{
    public:

        /** Data type of vector components.*/
        typedef float value_type;

        /** Number of vector components. */
        enum { num_components = 3 };

        value_type _v[3];

        /** Constructor that sets all components of the vector to zero */
        Vec3f() { _v[0]=0.0f; _v[1]=0.0f; _v[2]=0.0f;}
        Vec3f(value_type x,value_type y,value_type z) { _v[0]=x; _v[1]=y; _v[2]=z; }
        Vec3f(const Vec2f& v2,value_type zz)
        {
            _v[0] = v2[0];
            _v[1] = v2[1];
            _v[2] = zz;
        }


        inline bool operator == (const Vec3f& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1] && _v[2]==v._v[2]; }

        inline bool operator != (const Vec3f& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1] || _v[2]!=v._v[2]; }

        inline bool operator <  (const Vec3f& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else if (_v[1]<v._v[1]) return true;
            else if (_v[1]>v._v[1]) return false;
            else return (_v[2]<v._v[2]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set( value_type x, value_type y, value_type z)
        {
            _v[0]=x; _v[1]=y; _v[2]=z;
        }

        inline void set( const Vec3f& rhs)
        {
            _v[0]=rhs._v[0]; _v[1]=rhs._v[1]; _v[2]=rhs._v[2];
        }

        inline value_type& operator [] (int i) { return _v[i]; }
        inline value_type operator [] (int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }
        inline value_type& z() { return _v[2]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }
        inline value_type z() const { return _v[2]; }

        /** Returns true if all components have values that are not NaN. */
        inline bool valid() const { return !isNaN(); }
        /** Returns true if at least one component has value NaN. */
        inline bool isNaN() const { return osg::isNaN(_v[0]) || osg::isNaN(_v[1]) || osg::isNaN(_v[2]); }

        /** Dot product. */
        inline value_type operator * (const Vec3f& rhs) const
        {
            return _v[0]*rhs._v[0]+_v[1]*rhs._v[1]+_v[2]*rhs._v[2];
        }

        /** Cross product. */
        inline const Vec3f operator ^ (const Vec3f& rhs) const
        {
            return Vec3f(_v[1]*rhs._v[2]-_v[2]*rhs._v[1],
                         _v[2]*rhs._v[0]-_v[0]*rhs._v[2] ,
                         _v[0]*rhs._v[1]-_v[1]*rhs._v[0]);
        }

        /** Multiply by scalar. */
        inline const Vec3f operator * (value_type rhs) const
        {
            return Vec3f(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec3f& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            _v[2]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline const Vec3f operator / (value_type rhs) const
        {
            return Vec3f(_v[0]/rhs, _v[1]/rhs, _v[2]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec3f& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            _v[2]/=rhs;
            return *this;
        }

        /** Binary vector add. */
        inline const Vec3f operator + (const Vec3f& rhs) const
        {
            return Vec3f(_v[0]+rhs._v[0], _v[1]+rhs._v[1], _v[2]+rhs._v[2]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec3f& operator += (const Vec3f& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            _v[2] += rhs._v[2];
            return *this;
        }

        /** Binary vector subtract. */
        inline const Vec3f operator - (const Vec3f& rhs) const
        {
            return Vec3f(_v[0]-rhs._v[0], _v[1]-rhs._v[1], _v[2]-rhs._v[2]);
        }

        /** Unary vector subtract. */
        inline Vec3f& operator -= (const Vec3f& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            _v[2]-=rhs._v[2];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec3f. */
        inline const Vec3f operator - () const
        {
            return Vec3f (-_v[0], -_v[1], -_v[2]);
        }

        /** Length of the vector = sqrt( vec . vec ) */
        inline value_type length() const
        {
            return sqrtf( _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] );
        }

        /** Length squared of the vector = vec . vec */
        inline value_type length2() const
        {
            return _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2];
        }

        /** Normalize the vector so that it has length unity.
          * Returns the previous length of the vector.
        */
        inline value_type normalize()
        {
            value_type norm = Vec3f::length();
            if (norm>0.0)
            {
                value_type inv = 1.0f/norm;
                _v[0] *= inv;
                _v[1] *= inv;
                _v[2] *= inv;
            }
            return( norm );
        }

};    // end of class Vec3f

/** multiply by vector components. */
inline Vec3f componentMultiply(const Vec3f& lhs, const Vec3f& rhs)
{
    return Vec3f(lhs[0]*rhs[0], lhs[1]*rhs[1], lhs[2]*rhs[2]);
}

/** divide rhs components by rhs vector components. */
inline Vec3f componentDivide(const Vec3f& lhs, const Vec3f& rhs)
{
    return Vec3f(lhs[0]/rhs[0], lhs[1]/rhs[1], lhs[2]/rhs[2]);
}

const Vec3f X_AXIS(1.0,0.0,0.0);
const Vec3f Y_AXIS(0.0,1.0,0.0);
const Vec3f Z_AXIS(0.0,0.0,1.0);

}    // end of namespace osg


// OSGFILE include/osg/Vec3d

//#include <osg/Vec2d>
//#include <osg/Vec3f>

namespace osg {

/** General purpose double triple for use as vertices, vectors and normals.
  * Provides general math operations from addition through to cross products.
  * No support yet added for double * Vec3d - is it necessary?
  * Need to define a non-member non-friend operator*  etc.
  *    Vec3d * double is okay
*/

class Vec3d
{
    public:

        /** Data type of vector components.*/
        typedef double value_type;

        /** Number of vector components. */
        enum { num_components = 3 };

        value_type _v[3];

        /** Constructor that sets all components of the vector to zero */
        Vec3d() { _v[0]=0.0; _v[1]=0.0; _v[2]=0.0;}

        inline Vec3d(const Vec3f& vec) { _v[0]=vec._v[0]; _v[1]=vec._v[1]; _v[2]=vec._v[2];}

        inline operator Vec3f() const { return Vec3f(static_cast<float>(_v[0]),static_cast<float>(_v[1]),static_cast<float>(_v[2]));}

        Vec3d(value_type x,value_type y,value_type z) { _v[0]=x; _v[1]=y; _v[2]=z; }
        Vec3d(const Vec2d& v2,value_type zz)
        {
            _v[0] = v2[0];
            _v[1] = v2[1];
            _v[2] = zz;
        }

        inline bool operator == (const Vec3d& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1] && _v[2]==v._v[2]; }

        inline bool operator != (const Vec3d& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1] || _v[2]!=v._v[2]; }

        inline bool operator <  (const Vec3d& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else if (_v[1]<v._v[1]) return true;
            else if (_v[1]>v._v[1]) return false;
            else return (_v[2]<v._v[2]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set( value_type x, value_type y, value_type z)
        {
            _v[0]=x; _v[1]=y; _v[2]=z;
        }

        inline void set( const Vec3d& rhs)
        {
            _v[0]=rhs._v[0]; _v[1]=rhs._v[1]; _v[2]=rhs._v[2];
        }

        inline value_type& operator [] (int i) { return _v[i]; }
        inline value_type operator [] (int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }
        inline value_type& z() { return _v[2]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }
        inline value_type z() const { return _v[2]; }

        /** Returns true if all components have values that are not NaN. */
        inline bool valid() const { return !isNaN(); }
        /** Returns true if at least one component has value NaN. */
        inline bool isNaN() const { return osg::isNaN(_v[0]) || osg::isNaN(_v[1]) || osg::isNaN(_v[2]); }

        /** Dot product. */
        inline value_type operator * (const Vec3d& rhs) const
        {
            return _v[0]*rhs._v[0]+_v[1]*rhs._v[1]+_v[2]*rhs._v[2];
        }

        /** Cross product. */
        inline const Vec3d operator ^ (const Vec3d& rhs) const
        {
            return Vec3d(_v[1]*rhs._v[2]-_v[2]*rhs._v[1],
                         _v[2]*rhs._v[0]-_v[0]*rhs._v[2] ,
                         _v[0]*rhs._v[1]-_v[1]*rhs._v[0]);
        }

        /** Multiply by scalar. */
        inline const Vec3d operator * (value_type rhs) const
        {
            return Vec3d(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec3d& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            _v[2]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline const Vec3d operator / (value_type rhs) const
        {
            return Vec3d(_v[0]/rhs, _v[1]/rhs, _v[2]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec3d& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            _v[2]/=rhs;
            return *this;
        }

        /** Binary vector add. */
        inline const Vec3d operator + (const Vec3d& rhs) const
        {
            return Vec3d(_v[0]+rhs._v[0], _v[1]+rhs._v[1], _v[2]+rhs._v[2]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec3d& operator += (const Vec3d& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            _v[2] += rhs._v[2];
            return *this;
        }

        /** Binary vector subtract. */
        inline const Vec3d operator - (const Vec3d& rhs) const
        {
            return Vec3d(_v[0]-rhs._v[0], _v[1]-rhs._v[1], _v[2]-rhs._v[2]);
        }

        /** Unary vector subtract. */
        inline Vec3d& operator -= (const Vec3d& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            _v[2]-=rhs._v[2];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec3d. */
        inline const Vec3d operator - () const
        {
            return Vec3d (-_v[0], -_v[1], -_v[2]);
        }

        /** Length of the vector = sqrt( vec . vec ) */
        inline value_type length() const
        {
            return sqrt( _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] );
        }

        /** Length squared of the vector = vec . vec */
        inline value_type length2() const
        {
            return _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2];
        }

        /** Normalize the vector so that it has length unity.
          * Returns the previous length of the vector.
          * If the vector is zero length, it is left unchanged and zero is returned.
        */
        inline value_type normalize()
        {
            value_type norm = Vec3d::length();
            if (norm>0.0)
            {
                value_type inv = 1.0/norm;
                _v[0] *= inv;
                _v[1] *= inv;
                _v[2] *= inv;
            }
            return( norm );
        }

};    // end of class Vec3d

/** multiply by vector components. */
inline Vec3d componentMultiply(const Vec3d& lhs, const Vec3d& rhs)
{
    return Vec3d(lhs[0]*rhs[0], lhs[1]*rhs[1], lhs[2]*rhs[2]);
}

/** divide rhs components by rhs vector components. */
inline Vec3d componentDivide(const Vec3d& lhs, const Vec3d& rhs)
{
    return Vec3d(lhs[0]/rhs[0], lhs[1]/rhs[1], lhs[2]/rhs[2]);
}

}    // end of namespace osg


// OSGFILE include/osg/Vec4f

//#include <osg/Vec3f>

namespace osg {

/** General purpose float quad. Uses include representation
  * of color coordinates.
  * No support yet added for float * Vec4f - is it necessary?
  * Need to define a non-member non-friend operator*  etc.
  *    Vec4f * float is okay
*/
class Vec4f
{
    public:

        /** Data type of vector components.*/
        typedef float value_type;

        /** Number of vector components. */
        enum { num_components = 4 };

        /** Vec member variable. */
        value_type _v[4];

        // Methods are defined here so that they are implicitly inlined

        /** Constructor that sets all components of the vector to zero */
        Vec4f() { _v[0]=0.0f; _v[1]=0.0f; _v[2]=0.0f; _v[3]=0.0f;}

        Vec4f(value_type x, value_type y, value_type z, value_type w)
        {
            _v[0]=x;
            _v[1]=y;
            _v[2]=z;
            _v[3]=w;
        }

        Vec4f(const Vec3f& v3,value_type w)
        {
            _v[0]=v3[0];
            _v[1]=v3[1];
            _v[2]=v3[2];
            _v[3]=w;
        }

        inline bool operator == (const Vec4f& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1] && _v[2]==v._v[2] && _v[3]==v._v[3]; }

        inline bool operator != (const Vec4f& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1] || _v[2]!=v._v[2] || _v[3]!=v._v[3]; }

        inline bool operator <  (const Vec4f& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else if (_v[1]<v._v[1]) return true;
            else if (_v[1]>v._v[1]) return false;
            else if (_v[2]<v._v[2]) return true;
            else if (_v[2]>v._v[2]) return false;
            else return (_v[3]<v._v[3]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set( value_type x, value_type y, value_type z, value_type w)
        {
            _v[0]=x; _v[1]=y; _v[2]=z; _v[3]=w;
        }

        inline value_type& operator [] (unsigned int i) { return _v[i]; }
        inline value_type  operator [] (unsigned int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }
        inline value_type& z() { return _v[2]; }
        inline value_type& w() { return _v[3]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }
        inline value_type z() const { return _v[2]; }
        inline value_type w() const { return _v[3]; }

        inline value_type& r() { return _v[0]; }
        inline value_type& g() { return _v[1]; }
        inline value_type& b() { return _v[2]; }
        inline value_type& a() { return _v[3]; }

        inline value_type r() const { return _v[0]; }
        inline value_type g() const { return _v[1]; }
        inline value_type b() const { return _v[2]; }
        inline value_type a() const { return _v[3]; }

        inline unsigned int asABGR() const
        {
            return (unsigned int)clampTo((_v[0]*255.0f),0.0f,255.0f)<<24 |
                   (unsigned int)clampTo((_v[1]*255.0f),0.0f,255.0f)<<16 |
                   (unsigned int)clampTo((_v[2]*255.0f),0.0f,255.0f)<<8  |
                   (unsigned int)clampTo((_v[3]*255.0f),0.0f,255.0f);
        }

        inline unsigned int asRGBA() const
        {
            return (unsigned int)clampTo((_v[3]*255.0f),0.0f,255.0f)<<24 |
                   (unsigned int)clampTo((_v[2]*255.0f),0.0f,255.0f)<<16 |
                   (unsigned int)clampTo((_v[1]*255.0f),0.0f,255.0f)<<8  |
                   (unsigned int)clampTo((_v[0]*255.0f),0.0f,255.0f);
        }

        /** Returns true if all components have values that are not NaN. */
        inline bool valid() const { return !isNaN(); }
        /** Returns true if at least one component has value NaN. */
        inline bool isNaN() const { return osg::isNaN(_v[0]) || osg::isNaN(_v[1]) || osg::isNaN(_v[2]) || osg::isNaN(_v[3]); }

        /** Dot product. */
        inline value_type operator * (const Vec4f& rhs) const
        {
            return _v[0]*rhs._v[0]+
                   _v[1]*rhs._v[1]+
                   _v[2]*rhs._v[2]+
                   _v[3]*rhs._v[3] ;
        }

        /** Multiply by scalar. */
        inline Vec4f operator * (value_type rhs) const
        {
            return Vec4f(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs, _v[3]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec4f& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            _v[2]*=rhs;
            _v[3]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline Vec4f operator / (value_type rhs) const
        {
            return Vec4f(_v[0]/rhs, _v[1]/rhs, _v[2]/rhs, _v[3]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec4f& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            _v[2]/=rhs;
            _v[3]/=rhs;
            return *this;
        }

        /** Binary vector add. */
        inline Vec4f operator + (const Vec4f& rhs) const
        {
            return Vec4f(_v[0]+rhs._v[0], _v[1]+rhs._v[1],
                         _v[2]+rhs._v[2], _v[3]+rhs._v[3]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec4f& operator += (const Vec4f& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            _v[2] += rhs._v[2];
            _v[3] += rhs._v[3];
            return *this;
        }

        /** Binary vector subtract. */
        inline Vec4f operator - (const Vec4f& rhs) const
        {
            return Vec4f(_v[0]-rhs._v[0], _v[1]-rhs._v[1],
                         _v[2]-rhs._v[2], _v[3]-rhs._v[3] );
        }

        /** Unary vector subtract. */
        inline Vec4f& operator -= (const Vec4f& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            _v[2]-=rhs._v[2];
            _v[3]-=rhs._v[3];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec4f. */
        inline const Vec4f operator - () const
        {
            return Vec4f (-_v[0], -_v[1], -_v[2], -_v[3]);
        }

        /** Length of the vector = sqrt( vec . vec ) */
        inline value_type length() const
        {
            return sqrtf( _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3]);
        }

        /** Length squared of the vector = vec . vec */
        inline value_type length2() const
        {
            return _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3];
        }

        /** Normalize the vector so that it has length unity.
          * Returns the previous length of the vector.
        */
        inline value_type normalize()
        {
            value_type norm = Vec4f::length();
            if (norm>0.0f)
            {
                value_type inv = 1.0f/norm;
                _v[0] *= inv;
                _v[1] *= inv;
                _v[2] *= inv;
                _v[3] *= inv;
            }
            return( norm );
        }

};    // end of class Vec4f

/** Compute the dot product of a (Vec3,1.0) and a Vec4f. */
inline Vec4f::value_type operator * (const Vec3f& lhs,const Vec4f& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+rhs[3];
}

/** Compute the dot product of a Vec4f and a (Vec3,1.0). */
inline Vec4f::value_type operator * (const Vec4f& lhs,const Vec3f& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+lhs[3];
}

/** multiply by vector components. */
inline Vec4f componentMultiply(const Vec4f& lhs, const Vec4f& rhs)
{
    return Vec4f(lhs[0]*rhs[0], lhs[1]*rhs[1], lhs[2]*rhs[2], lhs[3]*rhs[3]);
}

/** divide rhs components by rhs vector components. */
inline Vec4f componentDivide(const Vec4f& lhs, const Vec4f& rhs)
{
    return Vec4f(lhs[0]/rhs[0], lhs[1]/rhs[1], lhs[2]/rhs[2], lhs[3]/rhs[3]);
}

}    // end of namespace osg


// OSGFILE include/osg/Vec4d

//#include <osg/Vec3d>
//#include <osg/Vec4f>

namespace osg {

/** General purpose double quad. Uses include representation
  * of color coordinates.
  * No support yet added for double * Vec4d - is it necessary?
  * Need to define a non-member non-friend operator*  etc.
  * Vec4d * double is okay
*/
class Vec4d
{
    public:

        /** Data type of vector components.*/
        typedef double value_type;

        /** Number of vector components. */
        enum { num_components = 4 };

        value_type _v[4];

        /** Constructor that sets all components of the vector to zero */
        Vec4d() { _v[0]=0.0; _v[1]=0.0; _v[2]=0.0; _v[3]=0.0; }

        Vec4d(value_type x, value_type y, value_type z, value_type w)
        {
            _v[0]=x;
            _v[1]=y;
            _v[2]=z;
            _v[3]=w;
        }

        Vec4d(const Vec3d& v3,value_type w)
        {
            _v[0]=v3[0];
            _v[1]=v3[1];
            _v[2]=v3[2];
            _v[3]=w;
        }

        inline Vec4d(const Vec4f& vec) { _v[0]=vec._v[0]; _v[1]=vec._v[1]; _v[2]=vec._v[2]; _v[3]=vec._v[3];}

        inline operator Vec4f() const { return Vec4f(static_cast<float>(_v[0]),static_cast<float>(_v[1]),static_cast<float>(_v[2]),static_cast<float>(_v[3]));}


        inline bool operator == (const Vec4d& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1] && _v[2]==v._v[2] && _v[3]==v._v[3]; }

        inline bool operator != (const Vec4d& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1] || _v[2]!=v._v[2] || _v[3]!=v._v[3]; }

        inline bool operator <  (const Vec4d& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else if (_v[1]<v._v[1]) return true;
            else if (_v[1]>v._v[1]) return false;
            else if (_v[2]<v._v[2]) return true;
            else if (_v[2]>v._v[2]) return false;
            else return (_v[3]<v._v[3]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set( value_type x, value_type y, value_type z, value_type w)
        {
            _v[0]=x; _v[1]=y; _v[2]=z; _v[3]=w;
        }

        inline value_type& operator [] (unsigned int i) { return _v[i]; }
        inline value_type  operator [] (unsigned int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }
        inline value_type& z() { return _v[2]; }
        inline value_type& w() { return _v[3]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }
        inline value_type z() const { return _v[2]; }
        inline value_type w() const { return _v[3]; }

        inline value_type& r() { return _v[0]; }
        inline value_type& g() { return _v[1]; }
        inline value_type& b() { return _v[2]; }
        inline value_type& a() { return _v[3]; }

        inline value_type r() const { return _v[0]; }
        inline value_type g() const { return _v[1]; }
        inline value_type b() const { return _v[2]; }
        inline value_type a() const { return _v[3]; }


        inline unsigned int asABGR() const
        {
            return (unsigned int)clampTo((_v[0]*255.0),0.0,255.0)<<24 |
                   (unsigned int)clampTo((_v[1]*255.0),0.0,255.0)<<16 |
                   (unsigned int)clampTo((_v[2]*255.0),0.0,255.0)<<8  |
                   (unsigned int)clampTo((_v[3]*255.0),0.0,255.0);
        }

        inline unsigned int asRGBA() const
        {
            return (unsigned int)clampTo((_v[3]*255.0),0.0,255.0)<<24 |
                   (unsigned int)clampTo((_v[2]*255.0),0.0,255.0)<<16 |
                   (unsigned int)clampTo((_v[1]*255.0),0.0,255.0)<<8  |
                   (unsigned int)clampTo((_v[0]*255.0),0.0,255.0);
        }

        /** Returns true if all components have values that are not NaN. */
        inline bool valid() const { return !isNaN(); }
        /** Returns true if at least one component has value NaN. */
        inline bool isNaN() const { return osg::isNaN(_v[0]) || osg::isNaN(_v[1]) || osg::isNaN(_v[2]) || osg::isNaN(_v[3]); }

        /** Dot product. */
        inline value_type operator * (const Vec4d& rhs) const
        {
            return _v[0]*rhs._v[0]+
                   _v[1]*rhs._v[1]+
                   _v[2]*rhs._v[2]+
                   _v[3]*rhs._v[3] ;
        }

        /** Multiply by scalar. */
        inline Vec4d operator * (value_type rhs) const
        {
            return Vec4d(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs, _v[3]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec4d& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            _v[2]*=rhs;
            _v[3]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline Vec4d operator / (value_type rhs) const
        {
            return Vec4d(_v[0]/rhs, _v[1]/rhs, _v[2]/rhs, _v[3]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec4d& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            _v[2]/=rhs;
            _v[3]/=rhs;
            return *this;
        }

        /** Binary vector add. */
        inline Vec4d operator + (const Vec4d& rhs) const
        {
            return Vec4d(_v[0]+rhs._v[0], _v[1]+rhs._v[1],
                         _v[2]+rhs._v[2], _v[3]+rhs._v[3]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec4d& operator += (const Vec4d& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            _v[2] += rhs._v[2];
            _v[3] += rhs._v[3];
            return *this;
        }

        /** Binary vector subtract. */
        inline Vec4d operator - (const Vec4d& rhs) const
        {
            return Vec4d(_v[0]-rhs._v[0], _v[1]-rhs._v[1],
                         _v[2]-rhs._v[2], _v[3]-rhs._v[3] );
        }

        /** Unary vector subtract. */
        inline Vec4d& operator -= (const Vec4d& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            _v[2]-=rhs._v[2];
            _v[3]-=rhs._v[3];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec4d. */
        inline const Vec4d operator - () const
        {
            return Vec4d (-_v[0], -_v[1], -_v[2], -_v[3]);
        }

        /** Length of the vector = sqrt( vec . vec ) */
        inline value_type length() const
        {
            return sqrt( _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3]);
        }

        /** Length squared of the vector = vec . vec */
        inline value_type length2() const
        {
            return _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3];
        }

        /** Normalize the vector so that it has length unity.
          * Returns the previous length of the vector.
        */
        inline value_type normalize()
        {
            value_type norm = Vec4d::length();
            if (norm>0.0f)
            {
                value_type inv = 1.0/norm;
                _v[0] *= inv;
                _v[1] *= inv;
                _v[2] *= inv;
                _v[3] *= inv;
            }
            return( norm );
        }

};    // end of class Vec4d



/** Compute the dot product of a (Vec3,1.0) and a Vec4d. */
inline Vec4d::value_type operator * (const Vec3d& lhs,const Vec4d& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+rhs[3];
}

/** Compute the dot product of a (Vec3,1.0) and a Vec4d. */
inline Vec4d::value_type operator * (const Vec3f& lhs,const Vec4d& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+rhs[3];
}

/** Compute the dot product of a (Vec3,1.0) and a Vec4d. */
inline Vec4d::value_type operator * (const Vec3d& lhs,const Vec4f& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+rhs[3];
}


/** Compute the dot product of a Vec4d and a (Vec3,1.0). */
inline Vec4d::value_type operator * (const Vec4d& lhs,const Vec3d& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+lhs[3];
}

/** Compute the dot product of a Vec4d and a (Vec3,1.0). */
inline Vec4d::value_type operator * (const Vec4d& lhs,const Vec3f& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+lhs[3];
}

/** Compute the dot product of a Vec4d and a (Vec3,1.0). */
inline Vec4d::value_type operator * (const Vec4f& lhs,const Vec3d& rhs)
{
    return lhs[0]*rhs[0]+lhs[1]*rhs[1]+lhs[2]*rhs[2]+lhs[3];
}

/** multiply by vector components. */
inline Vec4d componentMultiply(const Vec4d& lhs, const Vec4d& rhs)
{
    return Vec4d(lhs[0]*rhs[0], lhs[1]*rhs[1], lhs[2]*rhs[2], lhs[3]*rhs[3]);
}

/** divide rhs components by rhs vector components. */
inline Vec4d componentDivide(const Vec4d& lhs, const Vec4d& rhs)
{
    return Vec4d(lhs[0]/rhs[0], lhs[1]/rhs[1], lhs[2]/rhs[2], lhs[3]/rhs[3]);
}

}    // end of namespace osg


// OSGFILE include/osg/Vec4

//#include <osg/Vec4f>

namespace osg {

    typedef Vec4f Vec4;

}


// OSGFILE include/osg/Quat

/*
#include <osg/Export>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Vec3d>
#include <osg/Vec4d>
*/

namespace osg {

class Matrixf;
class Matrixd;

/** A quaternion class. It can be used to represent an orientation in 3D space.*/
class OSG_EXPORT Quat
{

    public:

        /** Data type of vector components.*/
        typedef double value_type;

        /** Number of vector components. */
        enum { num_components = 4 };

        value_type  _v[4];    // a four-vector

        inline Quat() { _v[0]=0.0; _v[1]=0.0; _v[2]=0.0; _v[3]=1.0; }

        inline Quat( value_type x, value_type y, value_type z, value_type w )
        {
            _v[0]=x;
            _v[1]=y;
            _v[2]=z;
            _v[3]=w;
        }

        inline Quat( const Vec4f& v )
        {
            _v[0]=v.x();
            _v[1]=v.y();
            _v[2]=v.z();
            _v[3]=v.w();
        }

        inline Quat( const Vec4d& v )
        {
            _v[0]=v.x();
            _v[1]=v.y();
            _v[2]=v.z();
            _v[3]=v.w();
        }

        inline Quat( value_type angle, const Vec3f& axis)
        {
            makeRotate(angle,axis);
        }
        inline Quat( value_type angle, const Vec3d& axis)
        {
            makeRotate(angle,axis);
        }

        inline Quat( value_type angle1, const Vec3f& axis1,
                     value_type angle2, const Vec3f& axis2,
                     value_type angle3, const Vec3f& axis3)
        {
            makeRotate(angle1,axis1,angle2,axis2,angle3,axis3);
        }

        inline Quat( value_type angle1, const Vec3d& axis1,
                     value_type angle2, const Vec3d& axis2,
                     value_type angle3, const Vec3d& axis3)
        {
            makeRotate(angle1,axis1,angle2,axis2,angle3,axis3);
        }

        inline Quat& operator = (const Quat& v) { _v[0]=v._v[0];  _v[1]=v._v[1]; _v[2]=v._v[2]; _v[3]=v._v[3]; return *this; }

        inline bool operator == (const Quat& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1] && _v[2]==v._v[2] && _v[3]==v._v[3]; }

        inline bool operator != (const Quat& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1] || _v[2]!=v._v[2] || _v[3]!=v._v[3]; }

        inline bool operator <  (const Quat& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else if (_v[1]<v._v[1]) return true;
            else if (_v[1]>v._v[1]) return false;
            else if (_v[2]<v._v[2]) return true;
            else if (_v[2]>v._v[2]) return false;
            else return (_v[3]<v._v[3]);
        }

        /* ----------------------------------
           Methods to access data members
        ---------------------------------- */

        inline Vec4d asVec4() const
        {
            return Vec4d(_v[0], _v[1], _v[2], _v[3]);
        }

        inline Vec3d asVec3() const
        {
            return Vec3d(_v[0], _v[1], _v[2]);
        }

        inline void set(value_type x, value_type y, value_type z, value_type w)
        {
            _v[0]=x;
            _v[1]=y;
            _v[2]=z;
            _v[3]=w;
        }

        inline void set(const osg::Vec4f& v)
        {
            _v[0]=v.x();
            _v[1]=v.y();
            _v[2]=v.z();
            _v[3]=v.w();
        }

        inline void set(const osg::Vec4d& v)
        {
            _v[0]=v.x();
            _v[1]=v.y();
            _v[2]=v.z();
            _v[3]=v.w();
        }

        void set(const Matrixf& matrix);

        void set(const Matrixd& matrix);

        void get(Matrixf& matrix) const;

        void get(Matrixd& matrix) const;


        inline value_type & operator [] (int i) { return _v[i]; }
        inline value_type   operator [] (int i) const { return _v[i]; }

        inline value_type & x() { return _v[0]; }
        inline value_type & y() { return _v[1]; }
        inline value_type & z() { return _v[2]; }
        inline value_type & w() { return _v[3]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }
        inline value_type z() const { return _v[2]; }
        inline value_type w() const { return _v[3]; }

        /** return true if the Quat represents a zero rotation, and therefore can be ignored in computations.*/
        bool zeroRotation() const { return _v[0]==0.0 && _v[1]==0.0 && _v[2]==0.0 && _v[3]==1.0; }


         /* -------------------------------------------------------------
                   BASIC ARITHMETIC METHODS
        Implemented in terms of Vec4s.  Some Vec4 operators, e.g.
        operator* are not appropriate for quaternions (as
        mathematical objects) so they are implemented differently.
        Also define methods for conjugate and the multiplicative inverse.
        ------------------------------------------------------------- */
        /// Multiply by scalar
        inline const Quat operator * (value_type rhs) const
        {
            return Quat(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs, _v[3]*rhs);
        }

        /// Unary multiply by scalar
        inline Quat& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            _v[2]*=rhs;
            _v[3]*=rhs;
            return *this;        // enable nesting
        }

        /// Binary multiply
        inline const Quat operator*(const Quat& rhs) const
        {
            return Quat( rhs._v[3]*_v[0] + rhs._v[0]*_v[3] + rhs._v[1]*_v[2] - rhs._v[2]*_v[1],
                 rhs._v[3]*_v[1] - rhs._v[0]*_v[2] + rhs._v[1]*_v[3] + rhs._v[2]*_v[0],
                 rhs._v[3]*_v[2] + rhs._v[0]*_v[1] - rhs._v[1]*_v[0] + rhs._v[2]*_v[3],
                 rhs._v[3]*_v[3] - rhs._v[0]*_v[0] - rhs._v[1]*_v[1] - rhs._v[2]*_v[2] );
        }

        /// Unary multiply
        inline Quat& operator*=(const Quat& rhs)
        {
            value_type x = rhs._v[3]*_v[0] + rhs._v[0]*_v[3] + rhs._v[1]*_v[2] - rhs._v[2]*_v[1];
            value_type y = rhs._v[3]*_v[1] - rhs._v[0]*_v[2] + rhs._v[1]*_v[3] + rhs._v[2]*_v[0];
            value_type z = rhs._v[3]*_v[2] + rhs._v[0]*_v[1] - rhs._v[1]*_v[0] + rhs._v[2]*_v[3];
            _v[3]   = rhs._v[3]*_v[3] - rhs._v[0]*_v[0] - rhs._v[1]*_v[1] - rhs._v[2]*_v[2];

            _v[2] = z;
            _v[1] = y;
            _v[0] = x;

            return (*this);            // enable nesting
        }

        /// Divide by scalar
        inline Quat operator / (value_type rhs) const
        {
            value_type div = 1.0/rhs;
            return Quat(_v[0]*div, _v[1]*div, _v[2]*div, _v[3]*div);
        }

        /// Unary divide by scalar
        inline Quat& operator /= (value_type rhs)
        {
            value_type div = 1.0/rhs;
            _v[0]*=div;
            _v[1]*=div;
            _v[2]*=div;
            _v[3]*=div;
            return *this;
        }

        /// Binary divide
        inline const Quat operator/(const Quat& denom) const
        {
            return ( (*this) * denom.inverse() );
        }

        /// Unary divide
        inline Quat& operator/=(const Quat& denom)
        {
            (*this) = (*this) * denom.inverse();
            return (*this);            // enable nesting
        }

        /// Binary addition
        inline const Quat operator + (const Quat& rhs) const
        {
            return Quat(_v[0]+rhs._v[0], _v[1]+rhs._v[1],
                _v[2]+rhs._v[2], _v[3]+rhs._v[3]);
        }

        /// Unary addition
        inline Quat& operator += (const Quat& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            _v[2] += rhs._v[2];
            _v[3] += rhs._v[3];
            return *this;            // enable nesting
        }

        /// Binary subtraction
        inline const Quat operator - (const Quat& rhs) const
        {
            return Quat(_v[0]-rhs._v[0], _v[1]-rhs._v[1],
                _v[2]-rhs._v[2], _v[3]-rhs._v[3] );
        }

        /// Unary subtraction
        inline Quat& operator -= (const Quat& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            _v[2]-=rhs._v[2];
            _v[3]-=rhs._v[3];
            return *this;            // enable nesting
        }

        /** Negation operator - returns the negative of the quaternion.
        Basically just calls operator - () on the Vec4 */
        inline const Quat operator - () const
        {
            return Quat (-_v[0], -_v[1], -_v[2], -_v[3]);
        }

        /// Length of the quaternion = sqrt( vec . vec )
        value_type length() const
        {
            return sqrt( _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3]);
        }

        /// Length of the quaternion = vec . vec
        value_type length2() const
        {
            return _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3];
        }

        /// Conjugate
        inline Quat conj () const
        {
             return Quat( -_v[0], -_v[1], -_v[2], _v[3] );
        }

        /// Multiplicative inverse method: q^(-1) = q^*/(q.q^*)
        inline const Quat inverse () const
        {
             return conj() / length2();
         }

      /* --------------------------------------------------------
               METHODS RELATED TO ROTATIONS
        Set a quaternion which will perform a rotation of an
        angle around the axis given by the vector (x,y,z).
        Should be written to also accept an angle and a Vec3?

        Define Spherical Linear interpolation method also

        Not inlined - see the Quat.cpp file for implementation
        -------------------------------------------------------- */
        void makeRotate( value_type  angle,
                          value_type  x, value_type  y, value_type  z );
        void makeRotate ( value_type  angle, const Vec3f& vec );
        void makeRotate ( value_type  angle, const Vec3d& vec );

        void makeRotate ( value_type  angle1, const Vec3f& axis1,
                          value_type  angle2, const Vec3f& axis2,
                          value_type  angle3, const Vec3f& axis3);
        void makeRotate ( value_type  angle1, const Vec3d& axis1,
                          value_type  angle2, const Vec3d& axis2,
                          value_type  angle3, const Vec3d& axis3);

        /** Make a rotation Quat which will rotate vec1 to vec2.
            Generally take a dot product to get the angle between these
            and then use a cross product to get the rotation axis
            Watch out for the two special cases when the vectors
            are co-incident or opposite in direction.*/
        void makeRotate( const Vec3f& vec1, const Vec3f& vec2 );
        /** Make a rotation Quat which will rotate vec1 to vec2.
            Generally take a dot product to get the angle between these
            and then use a cross product to get the rotation axis
            Watch out for the two special cases of when the vectors
            are co-incident or opposite in direction.*/
        void makeRotate( const Vec3d& vec1, const Vec3d& vec2 );

        void makeRotate_original( const Vec3d& vec1, const Vec3d& vec2 );

        /** Return the angle and vector components represented by the quaternion.*/
        void getRotate ( value_type & angle, value_type & x, value_type & y, value_type & z ) const;

        /** Return the angle and vector represented by the quaternion.*/
        void getRotate ( value_type & angle, Vec3f& vec ) const;

        /** Return the angle and vector represented by the quaternion.*/
        void getRotate ( value_type & angle, Vec3d& vec ) const;

        /** Spherical Linear Interpolation.
        As t goes from 0 to 1, the Quat object goes from "from" to "to". */
        void slerp   ( value_type  t, const Quat& from, const Quat& to);

        /** Rotate a vector by this quaternion.*/
        Vec3f operator* (const Vec3f& v) const
        {
            // nVidia SDK implementation
            Vec3f uv, uuv;
            Vec3f qvec(_v[0], _v[1], _v[2]);
            uv = qvec ^ v;
            uuv = qvec ^ uv;
            uv *= ( 2.0f * _v[3] );
            uuv *= 2.0f;
            return v + uv + uuv;
        }

        /** Rotate a vector by this quaternion.*/
        Vec3d operator* (const Vec3d& v) const
        {
            // nVidia SDK implementation
            Vec3d uv, uuv;
            Vec3d qvec(_v[0], _v[1], _v[2]);
            uv = qvec ^ v;
            uuv = qvec ^ uv;
            uv *= ( 2.0f * _v[3] );
            uuv *= 2.0f;
            return v + uv + uuv;
        }

    protected:

};    // end of class prototype

}    // end of namespace


// OSGFILE include/osg/Matrixd

/*
#include <osg/Object>
#include <osg/Vec3d>
#include <osg/Vec4d>
#include <osg/Quat>
*/

namespace osg {

class Matrixf;

class OSG_EXPORT Matrixd
{
    public:

        typedef double value_type;
        typedef float other_value_type;

        inline Matrixd() { makeIdentity(); }
        inline Matrixd( const Matrixd& mat) { set(mat.ptr()); }
        Matrixd( const Matrixf& mat );
        inline explicit Matrixd( float const * const ptr ) { set(ptr); }
        inline explicit Matrixd( double const * const ptr ) { set(ptr); }
        inline explicit Matrixd( const Quat& quat ) { makeRotate(quat); }

        Matrixd(value_type a00, value_type a01, value_type a02, value_type a03,
                value_type a10, value_type a11, value_type a12, value_type a13,
                value_type a20, value_type a21, value_type a22, value_type a23,
                value_type a30, value_type a31, value_type a32, value_type a33);

        ~Matrixd() {}

        int compare(const Matrixd& m) const;

        bool operator < (const Matrixd& m) const { return compare(m)<0; }
        bool operator == (const Matrixd& m) const { return compare(m)==0; }
        bool operator != (const Matrixd& m) const { return compare(m)!=0; }

        inline value_type& operator()(int row, int col) { return _mat[row][col]; }
        inline value_type operator()(int row, int col) const { return _mat[row][col]; }

        inline bool valid() const { return !isNaN(); }
        inline bool isNaN() const { return osg::isNaN(_mat[0][0]) || osg::isNaN(_mat[0][1]) || osg::isNaN(_mat[0][2]) || osg::isNaN(_mat[0][3]) ||
                                                 osg::isNaN(_mat[1][0]) || osg::isNaN(_mat[1][1]) || osg::isNaN(_mat[1][2]) || osg::isNaN(_mat[1][3]) ||
                                                 osg::isNaN(_mat[2][0]) || osg::isNaN(_mat[2][1]) || osg::isNaN(_mat[2][2]) || osg::isNaN(_mat[2][3]) ||
                                                 osg::isNaN(_mat[3][0]) || osg::isNaN(_mat[3][1]) || osg::isNaN(_mat[3][2]) || osg::isNaN(_mat[3][3]); }

        inline Matrixd& operator = (const Matrixd& rhs)
        {
            if( &rhs == this ) return *this;
            set(rhs.ptr());
            return *this;
        }

        Matrixd& operator = (const Matrixf& other);

        inline void set(const Matrixd& rhs) { set(rhs.ptr()); }

        void set(const Matrixf& rhs);

        inline void set(float const * const ptr)
        {
            value_type* local_ptr = (value_type*)_mat;
            for(int i=0;i<16;++i) local_ptr[i]=(value_type)ptr[i];
        }

        inline void set(double const * const ptr)
        {
            value_type* local_ptr = (value_type*)_mat;
            for(int i=0;i<16;++i) local_ptr[i]=(value_type)ptr[i];
        }

        void set(value_type a00, value_type a01, value_type a02,value_type a03,
                 value_type a10, value_type a11, value_type a12,value_type a13,
                 value_type a20, value_type a21, value_type a22,value_type a23,
                 value_type a30, value_type a31, value_type a32,value_type a33);

        value_type * ptr() { return (value_type*)_mat; }
        const value_type * ptr() const { return (const value_type *)_mat; }

        bool isIdentity() const
        {
            return _mat[0][0]==1.0 && _mat[0][1]==0.0 && _mat[0][2]==0.0 &&  _mat[0][3]==0.0 &&
                   _mat[1][0]==0.0 && _mat[1][1]==1.0 && _mat[1][2]==0.0 &&  _mat[1][3]==0.0 &&
                   _mat[2][0]==0.0 && _mat[2][1]==0.0 && _mat[2][2]==1.0 &&  _mat[2][3]==0.0 &&
                   _mat[3][0]==0.0 && _mat[3][1]==0.0 && _mat[3][2]==0.0 &&  _mat[3][3]==1.0;
        }

        void makeIdentity();

        void makeScale( const Vec3f& );
        void makeScale( const Vec3d& );
        void makeScale( value_type, value_type, value_type );

        void makeTranslate( const Vec3f& );
        void makeTranslate( const Vec3d& );
        void makeTranslate( value_type, value_type, value_type );

        void makeRotate( const Vec3f& from, const Vec3f& to );
        void makeRotate( const Vec3d& from, const Vec3d& to );
        void makeRotate( value_type angle, const Vec3f& axis );
        void makeRotate( value_type angle, const Vec3d& axis );
        void makeRotate( value_type angle, value_type x, value_type y, value_type z );
        void makeRotate( const Quat& );
        void makeRotate( value_type angle1, const Vec3f& axis1,
                         value_type angle2, const Vec3f& axis2,
                         value_type angle3, const Vec3f& axis3);
        void makeRotate( value_type angle1, const Vec3d& axis1,
                         value_type angle2, const Vec3d& axis2,
                         value_type angle3, const Vec3d& axis3);


        /** decompose the matrix into translation, rotation, scale and scale orientation.*/
        void decompose( osg::Vec3f& translation,
                        osg::Quat& rotation,
                        osg::Vec3f& scale,
                        osg::Quat& so ) const;

        /** decompose the matrix into translation, rotation, scale and scale orientation.*/
        void decompose( osg::Vec3d& translation,
                        osg::Quat& rotation,
                        osg::Vec3d& scale,
                        osg::Quat& so ) const;


        /** Set to an orthographic projection.
         * See glOrtho for further details.
        */
        void makeOrtho(double left,   double right,
                       double bottom, double top,
                       double zNear,  double zFar);

        /** Get the orthographic settings of the orthographic projection matrix.
          * Note, if matrix is not an orthographic matrix then invalid values
          * will be returned.
        */
        bool getOrtho(double& left,   double& right,
                      double& bottom, double& top,
                      double& zNear,  double& zFar) const;

        /** float version of getOrtho(..) */
        bool getOrtho(float& left,   float& right,
                      float& bottom, float& top,
                      float& zNear,  float& zFar) const;


        /** Set to a 2D orthographic projection.
          * See glOrtho2D for further details.
        */
        inline void makeOrtho2D(double left,   double right,
                                double bottom, double top)
        {
            makeOrtho(left,right,bottom,top,-1.0,1.0);
        }


        /** Set to a perspective projection.
          * See glFrustum for further details.
        */
        void makeFrustum(double left,   double right,
                         double bottom, double top,
                         double zNear,  double zFar);

        /** Get the frustum settings of a perspective projection matrix.
          * Note, if matrix is not a perspective matrix then invalid values
          * will be returned.
        */
        bool getFrustum(double& left,   double& right,
                        double& bottom, double& top,
                        double& zNear,  double& zFar) const;

        /** float version of getFrustum(..) */
        bool getFrustum(float& left,   float& right,
                        float& bottom, float& top,
                        float& zNear,  float& zFar) const;

        /** Set to a symmetrical perspective projection.
          * See gluPerspective for further details.
          * Aspect ratio is defined as width/height.
        */
        void makePerspective(double fovy,  double aspectRatio,
                             double zNear, double zFar);

        /** Get the frustum settings of a symmetric perspective projection
          * matrix.
          * Return false if matrix is not a perspective matrix,
          * where parameter values are undefined.
          * Note, if matrix is not a symmetric perspective matrix then the
          * shear will be lost.
          * Asymmetric matrices occur when stereo, power walls, caves and
          * reality center display are used.
          * In these configuration one should use the AsFrustum method instead.
        */
        bool getPerspective(double& fovy,  double& aspectRatio,
                            double& zNear, double& zFar) const;

        /** float version of getPerspective(..) */
        bool getPerspective(float& fovy,  float& aspectRatio,
                            float& zNear, float& zFar) const;

        /** Set the position and orientation to be a view matrix,
          * using the same convention as gluLookAt.
        */
        void makeLookAt(const Vec3d& eye,const Vec3d& center,const Vec3d& up);

        /** Get to the position and orientation of a modelview matrix,
          * using the same convention as gluLookAt.
        */
        void getLookAt(Vec3f& eye,Vec3f& center,Vec3f& up,
                       value_type lookDistance=1.0f) const;

        /** Get to the position and orientation of a modelview matrix,
          * using the same convention as gluLookAt.
        */
        void getLookAt(Vec3d& eye,Vec3d& center,Vec3d& up,
                       value_type lookDistance=1.0f) const;

        /** invert the matrix rhs, automatically select invert_4x3 or invert_4x4. */
        inline bool invert( const Matrixd& rhs)
        {
            bool is_4x3 = (rhs._mat[0][3]==0.0 && rhs._mat[1][3]==0.0 &&  rhs._mat[2][3]==0.0 && rhs._mat[3][3]==1.0);
            return is_4x3 ? invert_4x3(rhs) :  invert_4x4(rhs);
        }

        /** 4x3 matrix invert, not right hand column is assumed to be 0,0,0,1. */
        bool invert_4x3( const Matrixd& rhs);

        /** full 4x4 matrix invert. */
        bool invert_4x4( const Matrixd& rhs);

        /** transpose a matrix */
        bool transpose(const Matrixd&rhs);

        /** transpose orthogonal part of the matrix **/
        bool transpose3x3(const Matrixd&rhs);

        /** ortho-normalize the 3x3 rotation & scale matrix */
        void orthoNormalize(const Matrixd& rhs);

        // basic utility functions to create new matrices
        inline static Matrixd identity( void );
        inline static Matrixd scale( const Vec3f& sv);
        inline static Matrixd scale( const Vec3d& sv);
        inline static Matrixd scale( value_type sx, value_type sy, value_type sz);
        inline static Matrixd translate( const Vec3f& dv);
        inline static Matrixd translate( const Vec3d& dv);
        inline static Matrixd translate( value_type x, value_type y, value_type z);
        inline static Matrixd rotate( const Vec3f& from, const Vec3f& to);
        inline static Matrixd rotate( const Vec3d& from, const Vec3d& to);
        inline static Matrixd rotate( value_type angle, value_type x, value_type y, value_type z);
        inline static Matrixd rotate( value_type angle, const Vec3f& axis);
        inline static Matrixd rotate( value_type angle, const Vec3d& axis);
        inline static Matrixd rotate( value_type angle1, const Vec3f& axis1,
                                      value_type angle2, const Vec3f& axis2,
                                      value_type angle3, const Vec3f& axis3);
        inline static Matrixd rotate( value_type angle1, const Vec3d& axis1,
                                      value_type angle2, const Vec3d& axis2,
                                      value_type angle3, const Vec3d& axis3);
        inline static Matrixd rotate( const Quat& quat);
        inline static Matrixd inverse( const Matrixd& matrix);
        inline static Matrixd orthoNormal(const Matrixd& matrix);
        /** Create an orthographic projection matrix.
          * See glOrtho for further details.
        */
        inline static Matrixd ortho(double left,   double right,
                                    double bottom, double top,
                                    double zNear,  double zFar);

        /** Create a 2D orthographic projection.
          * See glOrtho for further details.
        */
        inline static Matrixd ortho2D(double left,   double right,
                                      double bottom, double top);

        /** Create a perspective projection.
          * See glFrustum for further details.
        */
        inline static Matrixd frustum(double left,   double right,
                                      double bottom, double top,
                                      double zNear,  double zFar);

        /** Create a symmetrical perspective projection.
          * See gluPerspective for further details.
          * Aspect ratio is defined as width/height.
        */
        inline static Matrixd perspective(double fovy,  double aspectRatio,
                                          double zNear, double zFar);

        /** Create the position and orientation as per a camera,
          * using the same convention as gluLookAt.
        */
        inline static Matrixd lookAt(const Vec3f& eye,
                                     const Vec3f& center,
                                     const Vec3f& up);

        /** Create the position and orientation as per a camera,
          * using the same convention as gluLookAt.
        */
        inline static Matrixd lookAt(const Vec3d& eye,
                                     const Vec3d& center,
                                     const Vec3d& up);

        inline Vec3f preMult( const Vec3f& v ) const;
        inline Vec3d preMult( const Vec3d& v ) const;
        inline Vec3f postMult( const Vec3f& v ) const;
        inline Vec3d postMult( const Vec3d& v ) const;
        inline Vec3f operator* ( const Vec3f& v ) const;
        inline Vec3d operator* ( const Vec3d& v ) const;
        inline Vec4f preMult( const Vec4f& v ) const;
        inline Vec4d preMult( const Vec4d& v ) const;
        inline Vec4f postMult( const Vec4f& v ) const;
        inline Vec4d postMult( const Vec4d& v ) const;
        inline Vec4f operator* ( const Vec4f& v ) const;
        inline Vec4d operator* ( const Vec4d& v ) const;

#ifdef USE_DEPRECATED_API
        inline void set(const Quat& q) { makeRotate(q); }   /// deprecated, replace with makeRotate(q)
        inline void get(Quat& q) const { q = getRotate(); } /// deprecated, replace with getRotate()
#endif

        void setRotate(const Quat& q);
        /** Get the matrix rotation as a Quat. Note that this function
          * assumes a non-scaled matrix and will return incorrect results
          * for scaled matrixces. Consider decompose() instead.
          */
        Quat getRotate() const;

        void setTrans( value_type tx, value_type ty, value_type tz );
        void setTrans( const Vec3f& v );
        void setTrans( const Vec3d& v );

        inline Vec3d getTrans() const { return Vec3d(_mat[3][0],_mat[3][1],_mat[3][2]); }

        inline Vec3d getScale() const {
          Vec3d x_vec(_mat[0][0],_mat[1][0],_mat[2][0]);
          Vec3d y_vec(_mat[0][1],_mat[1][1],_mat[2][1]);
          Vec3d z_vec(_mat[0][2],_mat[1][2],_mat[2][2]);
          return Vec3d(x_vec.length(), y_vec.length(), z_vec.length());
        }

        /** apply a 3x3 transform of v*M[0..2,0..2]. */
        inline static Vec3f transform3x3(const Vec3f& v,const Matrixd& m);

        /** apply a 3x3 transform of v*M[0..2,0..2]. */
        inline static Vec3d transform3x3(const Vec3d& v,const Matrixd& m);

        /** apply a 3x3 transform of M[0..2,0..2]*v. */
        inline static Vec3f transform3x3(const Matrixd& m,const Vec3f& v);

        /** apply a 3x3 transform of M[0..2,0..2]*v. */
        inline static Vec3d transform3x3(const Matrixd& m,const Vec3d& v);

        // basic Matrixd multiplication, our workhorse methods.
        void mult( const Matrixd&, const Matrixd& );
        void preMult( const Matrixd& );
        void postMult( const Matrixd& );

        /** Optimized version of preMult(translate(v)); */
        inline void preMultTranslate( const Vec3d& v );
        inline void preMultTranslate( const Vec3f& v );
        /** Optimized version of postMult(translate(v)); */
        inline void postMultTranslate( const Vec3d& v );
        inline void postMultTranslate( const Vec3f& v );

        /** Optimized version of preMult(scale(v)); */
        inline void preMultScale( const Vec3d& v );
        inline void preMultScale( const Vec3f& v );
        /** Optimized version of postMult(scale(v)); */
        inline void postMultScale( const Vec3d& v );
        inline void postMultScale( const Vec3f& v );

        /** Optimized version of preMult(rotate(q)); */
        inline void preMultRotate( const Quat& q );
        /** Optimized version of postMult(rotate(q)); */
        inline void postMultRotate( const Quat& q );

        inline void operator *= ( const Matrixd& other )
        {    if( this == &other ) {
                Matrixd temp(other);
                postMult( temp );
            }
            else postMult( other );
        }

        inline Matrixd operator * ( const Matrixd &m ) const
        {
            osg::Matrixd r;
            r.mult(*this,m);
            return  r;
        }

    protected:
        value_type _mat[4][4];

};

class RefMatrixd : public Object, public Matrixd
{
    public:

        RefMatrixd():Object(false), Matrixd() {}
        RefMatrixd( const Matrixd& other) : Object(false), Matrixd(other) {}
        RefMatrixd( const Matrixf& other) : Object(false), Matrixd(other) {}
        RefMatrixd( const RefMatrixd& other) : Object(other), Matrixd(other) {}
        explicit RefMatrixd( Matrixd::value_type const * const def ):Object(false), Matrixd(def) {}
        RefMatrixd( Matrixd::value_type a00, Matrixd::value_type a01, Matrixd::value_type a02, Matrixd::value_type a03,
            Matrixd::value_type a10, Matrixd::value_type a11, Matrixd::value_type a12, Matrixd::value_type a13,
            Matrixd::value_type a20, Matrixd::value_type a21, Matrixd::value_type a22, Matrixd::value_type a23,
            Matrixd::value_type a30, Matrixd::value_type a31, Matrixd::value_type a32, Matrixd::value_type a33):
            Object(false),
            Matrixd(a00, a01, a02, a03,
                    a10, a11, a12, a13,
                    a20, a21, a22, a23,
                    a30, a31, a32, a33) {}

        virtual Object* cloneType() const { return new RefMatrixd(); }
        virtual Object* clone(const CopyOp&) const { return new RefMatrixd(*this); }
        virtual bool isSameKindAs(const Object* obj) const { return dynamic_cast<const RefMatrixd*>(obj)!=NULL; }
        virtual const char* libraryName() const { return "osg"; }
        virtual const char* className() const { return "Matrix"; }


    protected:

        virtual ~RefMatrixd() {}
};


// static utility methods
inline Matrixd Matrixd::identity(void)
{
    Matrixd m;
    m.makeIdentity();
    return m;
}

inline Matrixd Matrixd::scale(value_type sx, value_type sy, value_type sz)
{
    Matrixd m;
    m.makeScale(sx,sy,sz);
    return m;
}

inline Matrixd Matrixd::scale(const Vec3f& v )
{
    return scale(v.x(), v.y(), v.z() );
}

inline Matrixd Matrixd::scale(const Vec3d& v )
{
    return scale(v.x(), v.y(), v.z() );
}

inline Matrixd Matrixd::translate(value_type tx, value_type ty, value_type tz)
{
    Matrixd m;
    m.makeTranslate(tx,ty,tz);
    return m;
}

inline Matrixd Matrixd::translate(const Vec3f& v )
{
    return translate(v.x(), v.y(), v.z() );
}

inline Matrixd Matrixd::translate(const Vec3d& v )
{
    return translate(v.x(), v.y(), v.z() );
}

inline Matrixd Matrixd::rotate( const Quat& q )
{
    return Matrixd(q);
}
inline Matrixd Matrixd::rotate(value_type angle, value_type x, value_type y, value_type z )
{
    Matrixd m;
    m.makeRotate(angle,x,y,z);
    return m;
}
inline Matrixd Matrixd::rotate(value_type angle, const Vec3f& axis )
{
    Matrixd m;
    m.makeRotate(angle,axis);
    return m;
}
inline Matrixd Matrixd::rotate(value_type angle, const Vec3d& axis )
{
    Matrixd m;
    m.makeRotate(angle,axis);
    return m;
}
inline Matrixd Matrixd::rotate( value_type angle1, const Vec3f& axis1,
                                value_type angle2, const Vec3f& axis2,
                                value_type angle3, const Vec3f& axis3)
{
    Matrixd m;
    m.makeRotate(angle1,axis1,angle2,axis2,angle3,axis3);
    return m;
}
inline Matrixd Matrixd::rotate( value_type angle1, const Vec3d& axis1,
                                value_type angle2, const Vec3d& axis2,
                                value_type angle3, const Vec3d& axis3)
{
    Matrixd m;
    m.makeRotate(angle1,axis1,angle2,axis2,angle3,axis3);
    return m;
}
inline Matrixd Matrixd::rotate(const Vec3f& from, const Vec3f& to )
{
    Matrixd m;
    m.makeRotate(from,to);
    return m;
}
inline Matrixd Matrixd::rotate(const Vec3d& from, const Vec3d& to )
{
    Matrixd m;
    m.makeRotate(from,to);
    return m;
}

inline Matrixd Matrixd::inverse( const Matrixd& matrix)
{
    Matrixd m;
    m.invert(matrix);
    return m;
}

inline Matrixd Matrixd::orthoNormal(const Matrixd& matrix)
{
  Matrixd m;
  m.orthoNormalize(matrix);
  return m;
}

inline Matrixd Matrixd::ortho(double left,   double right,
                              double bottom, double top,
                              double zNear,  double zFar)
{
    Matrixd m;
    m.makeOrtho(left,right,bottom,top,zNear,zFar);
    return m;
}

inline Matrixd Matrixd::ortho2D(double left,   double right,
                                double bottom, double top)
{
    Matrixd m;
    m.makeOrtho2D(left,right,bottom,top);
    return m;
}

inline Matrixd Matrixd::frustum(double left,   double right,
                                double bottom, double top,
                                double zNear,  double zFar)
{
    Matrixd m;
    m.makeFrustum(left,right,bottom,top,zNear,zFar);
    return m;
}

inline Matrixd Matrixd::perspective(double fovy,  double aspectRatio,
                                    double zNear, double zFar)
{
    Matrixd m;
    m.makePerspective(fovy,aspectRatio,zNear,zFar);
    return m;
}

inline Matrixd Matrixd::lookAt(const Vec3f& eye,
                               const Vec3f& center,
                               const Vec3f& up)
{
    Matrixd m;
    m.makeLookAt(eye,center,up);
    return m;
}

inline Matrixd Matrixd::lookAt(const Vec3d& eye,
                               const Vec3d& center,
                               const Vec3d& up)
{
    Matrixd m;
    m.makeLookAt(eye,center,up);
    return m;
}

inline Vec3f Matrixd::postMult( const Vec3f& v ) const
{
    value_type d = 1.0f/(_mat[3][0]*v.x()+_mat[3][1]*v.y()+_mat[3][2]*v.z()+_mat[3][3]) ;
    return Vec3f( (_mat[0][0]*v.x() + _mat[0][1]*v.y() + _mat[0][2]*v.z() + _mat[0][3])*d,
        (_mat[1][0]*v.x() + _mat[1][1]*v.y() + _mat[1][2]*v.z() + _mat[1][3])*d,
        (_mat[2][0]*v.x() + _mat[2][1]*v.y() + _mat[2][2]*v.z() + _mat[2][3])*d) ;
}

inline Vec3d Matrixd::postMult( const Vec3d& v ) const
{
    value_type d = 1.0f/(_mat[3][0]*v.x()+_mat[3][1]*v.y()+_mat[3][2]*v.z()+_mat[3][3]) ;
    return Vec3d( (_mat[0][0]*v.x() + _mat[0][1]*v.y() + _mat[0][2]*v.z() + _mat[0][3])*d,
        (_mat[1][0]*v.x() + _mat[1][1]*v.y() + _mat[1][2]*v.z() + _mat[1][3])*d,
        (_mat[2][0]*v.x() + _mat[2][1]*v.y() + _mat[2][2]*v.z() + _mat[2][3])*d) ;
}

inline Vec3f Matrixd::preMult( const Vec3f& v ) const
{
    value_type d = 1.0f/(_mat[0][3]*v.x()+_mat[1][3]*v.y()+_mat[2][3]*v.z()+_mat[3][3]) ;
    return Vec3f( (_mat[0][0]*v.x() + _mat[1][0]*v.y() + _mat[2][0]*v.z() + _mat[3][0])*d,
        (_mat[0][1]*v.x() + _mat[1][1]*v.y() + _mat[2][1]*v.z() + _mat[3][1])*d,
        (_mat[0][2]*v.x() + _mat[1][2]*v.y() + _mat[2][2]*v.z() + _mat[3][2])*d);
}

inline Vec3d Matrixd::preMult( const Vec3d& v ) const
{
    value_type d = 1.0f/(_mat[0][3]*v.x()+_mat[1][3]*v.y()+_mat[2][3]*v.z()+_mat[3][3]) ;
    return Vec3d( (_mat[0][0]*v.x() + _mat[1][0]*v.y() + _mat[2][0]*v.z() + _mat[3][0])*d,
        (_mat[0][1]*v.x() + _mat[1][1]*v.y() + _mat[2][1]*v.z() + _mat[3][1])*d,
        (_mat[0][2]*v.x() + _mat[1][2]*v.y() + _mat[2][2]*v.z() + _mat[3][2])*d);
}

inline Vec4f Matrixd::postMult( const Vec4f& v ) const
{
    return Vec4f( (_mat[0][0]*v.x() + _mat[0][1]*v.y() + _mat[0][2]*v.z() + _mat[0][3]*v.w()),
        (_mat[1][0]*v.x() + _mat[1][1]*v.y() + _mat[1][2]*v.z() + _mat[1][3]*v.w()),
        (_mat[2][0]*v.x() + _mat[2][1]*v.y() + _mat[2][2]*v.z() + _mat[2][3]*v.w()),
        (_mat[3][0]*v.x() + _mat[3][1]*v.y() + _mat[3][2]*v.z() + _mat[3][3]*v.w())) ;
}
inline Vec4d Matrixd::postMult( const Vec4d& v ) const
{
    return Vec4d( (_mat[0][0]*v.x() + _mat[0][1]*v.y() + _mat[0][2]*v.z() + _mat[0][3]*v.w()),
        (_mat[1][0]*v.x() + _mat[1][1]*v.y() + _mat[1][2]*v.z() + _mat[1][3]*v.w()),
        (_mat[2][0]*v.x() + _mat[2][1]*v.y() + _mat[2][2]*v.z() + _mat[2][3]*v.w()),
        (_mat[3][0]*v.x() + _mat[3][1]*v.y() + _mat[3][2]*v.z() + _mat[3][3]*v.w())) ;
}

inline Vec4f Matrixd::preMult( const Vec4f& v ) const
{
    return Vec4f( (_mat[0][0]*v.x() + _mat[1][0]*v.y() + _mat[2][0]*v.z() + _mat[3][0]*v.w()),
        (_mat[0][1]*v.x() + _mat[1][1]*v.y() + _mat[2][1]*v.z() + _mat[3][1]*v.w()),
        (_mat[0][2]*v.x() + _mat[1][2]*v.y() + _mat[2][2]*v.z() + _mat[3][2]*v.w()),
        (_mat[0][3]*v.x() + _mat[1][3]*v.y() + _mat[2][3]*v.z() + _mat[3][3]*v.w()));
}

inline Vec4d Matrixd::preMult( const Vec4d& v ) const
{
    return Vec4d( (_mat[0][0]*v.x() + _mat[1][0]*v.y() + _mat[2][0]*v.z() + _mat[3][0]*v.w()),
        (_mat[0][1]*v.x() + _mat[1][1]*v.y() + _mat[2][1]*v.z() + _mat[3][1]*v.w()),
        (_mat[0][2]*v.x() + _mat[1][2]*v.y() + _mat[2][2]*v.z() + _mat[3][2]*v.w()),
        (_mat[0][3]*v.x() + _mat[1][3]*v.y() + _mat[2][3]*v.z() + _mat[3][3]*v.w()));
}

inline Vec3f Matrixd::transform3x3(const Vec3f& v,const Matrixd& m)
{
    return Vec3f( (m._mat[0][0]*v.x() + m._mat[1][0]*v.y() + m._mat[2][0]*v.z()),
                 (m._mat[0][1]*v.x() + m._mat[1][1]*v.y() + m._mat[2][1]*v.z()),
                 (m._mat[0][2]*v.x() + m._mat[1][2]*v.y() + m._mat[2][2]*v.z()));
}
inline Vec3d Matrixd::transform3x3(const Vec3d& v,const Matrixd& m)
{
    return Vec3d( (m._mat[0][0]*v.x() + m._mat[1][0]*v.y() + m._mat[2][0]*v.z()),
                 (m._mat[0][1]*v.x() + m._mat[1][1]*v.y() + m._mat[2][1]*v.z()),
                 (m._mat[0][2]*v.x() + m._mat[1][2]*v.y() + m._mat[2][2]*v.z()));
}

inline Vec3f Matrixd::transform3x3(const Matrixd& m,const Vec3f& v)
{
    return Vec3f( (m._mat[0][0]*v.x() + m._mat[0][1]*v.y() + m._mat[0][2]*v.z()),
                 (m._mat[1][0]*v.x() + m._mat[1][1]*v.y() + m._mat[1][2]*v.z()),
                 (m._mat[2][0]*v.x() + m._mat[2][1]*v.y() + m._mat[2][2]*v.z()) ) ;
}
inline Vec3d Matrixd::transform3x3(const Matrixd& m,const Vec3d& v)
{
    return Vec3d( (m._mat[0][0]*v.x() + m._mat[0][1]*v.y() + m._mat[0][2]*v.z()),
                 (m._mat[1][0]*v.x() + m._mat[1][1]*v.y() + m._mat[1][2]*v.z()),
                 (m._mat[2][0]*v.x() + m._mat[2][1]*v.y() + m._mat[2][2]*v.z()) ) ;
}

inline void Matrixd::preMultTranslate( const Vec3d& v )
{
    for (unsigned i = 0; i < 3; ++i)
    {
        double tmp = v[i];
        if (tmp == 0)
            continue;
        _mat[3][0] += tmp*_mat[i][0];
        _mat[3][1] += tmp*_mat[i][1];
        _mat[3][2] += tmp*_mat[i][2];
        _mat[3][3] += tmp*_mat[i][3];
    }
}

inline void Matrixd::preMultTranslate( const Vec3f& v )
{
    for (unsigned i = 0; i < 3; ++i)
    {
        float tmp = v[i];
        if (tmp == 0)
            continue;
        _mat[3][0] += tmp*_mat[i][0];
        _mat[3][1] += tmp*_mat[i][1];
        _mat[3][2] += tmp*_mat[i][2];
        _mat[3][3] += tmp*_mat[i][3];
    }
}

inline void Matrixd::postMultTranslate( const Vec3d& v )
{
    for (unsigned i = 0; i < 3; ++i)
    {
        double tmp = v[i];
        if (tmp == 0)
            continue;
        _mat[0][i] += tmp*_mat[0][3];
        _mat[1][i] += tmp*_mat[1][3];
        _mat[2][i] += tmp*_mat[2][3];
        _mat[3][i] += tmp*_mat[3][3];
    }
}

inline void Matrixd::postMultTranslate( const Vec3f& v )
{
    for (unsigned i = 0; i < 3; ++i)
    {
        float tmp = v[i];
        if (tmp == 0)
            continue;
        _mat[0][i] += tmp*_mat[0][3];
        _mat[1][i] += tmp*_mat[1][3];
        _mat[2][i] += tmp*_mat[2][3];
        _mat[3][i] += tmp*_mat[3][3];
    }
}

inline void Matrixd::preMultScale( const Vec3d& v )
{
    _mat[0][0] *= v[0]; _mat[0][1] *= v[0]; _mat[0][2] *= v[0]; _mat[0][3] *= v[0];
    _mat[1][0] *= v[1]; _mat[1][1] *= v[1]; _mat[1][2] *= v[1]; _mat[1][3] *= v[1];
    _mat[2][0] *= v[2]; _mat[2][1] *= v[2]; _mat[2][2] *= v[2]; _mat[2][3] *= v[2];
}

inline void Matrixd::preMultScale( const Vec3f& v )
{
    _mat[0][0] *= v[0]; _mat[0][1] *= v[0]; _mat[0][2] *= v[0]; _mat[0][3] *= v[0];
    _mat[1][0] *= v[1]; _mat[1][1] *= v[1]; _mat[1][2] *= v[1]; _mat[1][3] *= v[1];
    _mat[2][0] *= v[2]; _mat[2][1] *= v[2]; _mat[2][2] *= v[2]; _mat[2][3] *= v[2];
}

inline void Matrixd::postMultScale( const Vec3d& v )
{
    _mat[0][0] *= v[0]; _mat[1][0] *= v[0]; _mat[2][0] *= v[0]; _mat[3][0] *= v[0];
    _mat[0][1] *= v[1]; _mat[1][1] *= v[1]; _mat[2][1] *= v[1]; _mat[3][1] *= v[1];
    _mat[0][2] *= v[2]; _mat[1][2] *= v[2]; _mat[2][2] *= v[2]; _mat[3][2] *= v[2];
}

inline void Matrixd::postMultScale( const Vec3f& v )
{
    _mat[0][0] *= v[0]; _mat[1][0] *= v[0]; _mat[2][0] *= v[0]; _mat[3][0] *= v[0];
    _mat[0][1] *= v[1]; _mat[1][1] *= v[1]; _mat[2][1] *= v[1]; _mat[3][1] *= v[1];
    _mat[0][2] *= v[2]; _mat[1][2] *= v[2]; _mat[2][2] *= v[2]; _mat[3][2] *= v[2];
}

inline void Matrixd::preMultRotate( const Quat& q )
{
    if (q.zeroRotation())
        return;
    Matrixd r;
    r.setRotate(q);
    preMult(r);
}

inline void Matrixd::postMultRotate( const Quat& q )
{
    if (q.zeroRotation())
        return;
    Matrixd r;
    r.setRotate(q);
    postMult(r);
}

inline Vec3f operator* (const Vec3f& v, const Matrixd& m )
{
    return m.preMult(v);
}

inline Vec3d operator* (const Vec3d& v, const Matrixd& m )
{
    return m.preMult(v);
}

inline Vec4f operator* (const Vec4f& v, const Matrixd& m )
{
    return m.preMult(v);
}

inline Vec4d operator* (const Vec4d& v, const Matrixd& m )
{
    return m.preMult(v);
}

inline Vec3f Matrixd::operator* (const Vec3f& v) const
{
    return postMult(v);
}

inline Vec3d Matrixd::operator* (const Vec3d& v) const
{
    return postMult(v);
}

inline Vec4f Matrixd::operator* (const Vec4f& v) const
{
    return postMult(v);
}

inline Vec4d Matrixd::operator* (const Vec4d& v) const
{
    return postMult(v);
}


} //namespace osg

// OSGFILE include/osg/DisplaySettings

/*
#include <osg/Referenced>
#include <osg/Matrixd>
#include <osg/ref_ptr>
*/

#include <string>
#include <vector>
#include <map>

namespace osg {

// forward declare
class ArgumentParser;
class ApplicationUsage;

/** DisplaySettings class for encapsulating what visuals are required and
  * have been set up, and the status of stereo viewing.*/
class OSG_EXPORT DisplaySettings : public osg::Referenced
{

    public:

        /** Maintain a DisplaySettings singleton for objects to query at runtime.*/
        static ref_ptr<DisplaySettings>& instance();

        DisplaySettings():
            Referenced(true)
        {
            setDefaults();
            readEnvironmentalVariables();
        }

        DisplaySettings(ArgumentParser& arguments):
            Referenced(true)
        {
            setDefaults();
            readEnvironmentalVariables();
            readCommandLine(arguments);
        }

        DisplaySettings(const DisplaySettings& vs);


        DisplaySettings& operator = (const DisplaySettings& vs);

        void setDisplaySettings(const DisplaySettings& vs);

        void merge(const DisplaySettings& vs);

        void setDefaults();

        /** read the environmental variables.*/
        void readEnvironmentalVariables();

        /** read the commandline arguments.*/
        void readCommandLine(ArgumentParser& arguments);


        enum DisplayType
        {
            MONITOR,
            POWERWALL,
            REALITY_CENTER,
            HEAD_MOUNTED_DISPLAY
        };

        void setDisplayType(DisplayType type) { _displayType = type; }

        DisplayType getDisplayType() const { return _displayType; }


        void setStereo(bool on) { _stereo = on; }
        bool getStereo() const { return _stereo; }

        enum StereoMode
        {
            QUAD_BUFFER,
            ANAGLYPHIC,
            HORIZONTAL_SPLIT,
            VERTICAL_SPLIT,
            LEFT_EYE,
            RIGHT_EYE,
            HORIZONTAL_INTERLACE,
            VERTICAL_INTERLACE,
            CHECKERBOARD
        };

        void setStereoMode(StereoMode mode) { _stereoMode = mode; }
        StereoMode getStereoMode() const { return _stereoMode; }

        void setEyeSeparation(float eyeSeparation) { _eyeSeparation = eyeSeparation; }
        float getEyeSeparation() const { return _eyeSeparation; }

        enum SplitStereoHorizontalEyeMapping
        {
            LEFT_EYE_LEFT_VIEWPORT,
            LEFT_EYE_RIGHT_VIEWPORT
        };

        void setSplitStereoHorizontalEyeMapping(SplitStereoHorizontalEyeMapping m) { _splitStereoHorizontalEyeMapping = m; }
        SplitStereoHorizontalEyeMapping getSplitStereoHorizontalEyeMapping() const { return _splitStereoHorizontalEyeMapping; }

        void setSplitStereoHorizontalSeparation(int s) { _splitStereoHorizontalSeparation = s; }
        int getSplitStereoHorizontalSeparation() const { return _splitStereoHorizontalSeparation; }

        enum SplitStereoVerticalEyeMapping
        {
            LEFT_EYE_TOP_VIEWPORT,
            LEFT_EYE_BOTTOM_VIEWPORT
        };

        void setSplitStereoVerticalEyeMapping(SplitStereoVerticalEyeMapping m) { _splitStereoVerticalEyeMapping = m; }
        SplitStereoVerticalEyeMapping getSplitStereoVerticalEyeMapping() const { return _splitStereoVerticalEyeMapping; }

        void setSplitStereoVerticalSeparation(int s) { _splitStereoVerticalSeparation = s; }
        int getSplitStereoVerticalSeparation() const { return _splitStereoVerticalSeparation; }

        void setSplitStereoAutoAdjustAspectRatio(bool flag) { _splitStereoAutoAdjustAspectRatio=flag; }
        bool getSplitStereoAutoAdjustAspectRatio() const { return _splitStereoAutoAdjustAspectRatio; }


        void setScreenWidth(float width) { _screenWidth = width; }
        float getScreenWidth() const { return _screenWidth; }

        void setScreenHeight(float height) { _screenHeight = height; }
        float getScreenHeight() const { return _screenHeight; }

        void setScreenDistance(float distance) { _screenDistance = distance; }
        float getScreenDistance() const { return _screenDistance; }



        void setDoubleBuffer(bool flag) { _doubleBuffer = flag; }
        bool getDoubleBuffer() const { return _doubleBuffer; }


        void setRGB(bool flag) { _RGB = flag; }
        bool getRGB() const { return _RGB; }


        void setDepthBuffer(bool flag) { _depthBuffer = flag; }
        bool getDepthBuffer() const { return _depthBuffer; }


        void setMinimumNumAlphaBits(unsigned int bits) { _minimumNumberAlphaBits = bits; }
        unsigned int getMinimumNumAlphaBits() const { return _minimumNumberAlphaBits; }
        bool getAlphaBuffer() const { return _minimumNumberAlphaBits!=0; }

        void setMinimumNumStencilBits(unsigned int bits) { _minimumNumberStencilBits = bits; }
        unsigned int getMinimumNumStencilBits() const { return _minimumNumberStencilBits; }
        bool getStencilBuffer() const { return _minimumNumberStencilBits!=0; }

        void setMinimumNumAccumBits(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha);
        unsigned int getMinimumNumAccumRedBits() const { return _minimumNumberAccumRedBits; }
        unsigned int getMinimumNumAccumGreenBits() const { return _minimumNumberAccumGreenBits; }
        unsigned int getMinimumNumAccumBlueBits() const { return _minimumNumberAccumBlueBits; }
        unsigned int getMinimumNumAccumAlphaBits() const { return _minimumNumberAccumAlphaBits; }
        bool getAccumBuffer() const { return (_minimumNumberAccumRedBits+_minimumNumberAccumGreenBits+_minimumNumberAccumBlueBits+_minimumNumberAccumAlphaBits)!=0; }


        void setMaxNumberOfGraphicsContexts(unsigned int num);
        unsigned int getMaxNumberOfGraphicsContexts() const;

        void setNumMultiSamples(unsigned int samples) { _numMultiSamples = samples; }
        unsigned int getNumMultiSamples() const { return _numMultiSamples; }
        bool getMultiSamples() const { return _numMultiSamples!=0; }

        void setCompileContextsHint(bool useCompileContexts) { _compileContextsHint = useCompileContexts; }
        bool getCompileContextsHint() const { return _compileContextsHint; }

        void setSerializeDrawDispatch(bool serializeDrawDispatch) { _serializeDrawDispatch = serializeDrawDispatch; }
        bool getSerializeDrawDispatch() const { return _serializeDrawDispatch; }


        void setUseSceneViewForStereoHint(bool hint) { _useSceneViewForStereoHint = hint; }
        bool getUseSceneViewForStereoHint() const { return _useSceneViewForStereoHint; }


        /** Set the hint for the total number of threads in the DatbasePager set up, inclusive of the number of http dedicated threads.*/
        void setNumOfDatabaseThreadsHint(unsigned int numThreads) { _numDatabaseThreadsHint = numThreads; }

        /** Get the hint for total number of threads in the DatbasePager set up, inclusive of the number of http dedicated threads.*/
        unsigned int getNumOfDatabaseThreadsHint() const { return _numDatabaseThreadsHint; }

        /** Set the hint for number of threads in the DatbasePager to dedicate to reading http requests.*/
        void setNumOfHttpDatabaseThreadsHint(unsigned int numThreads) { _numHttpDatabaseThreadsHint = numThreads; }

        /** Get the hint for number of threads in the DatbasePager dedicated to reading http requests.*/
        unsigned int getNumOfHttpDatabaseThreadsHint() const { return _numHttpDatabaseThreadsHint; }

        void setApplication(const std::string& application) { _application = application; }
        const std::string& getApplication() { return _application; }


        void setMaxTexturePoolSize(unsigned int size) { _maxTexturePoolSize = size; }
        unsigned int getMaxTexturePoolSize() const { return _maxTexturePoolSize; }

        void setMaxBufferObjectPoolSize(unsigned int size) { _maxBufferObjectPoolSize = size; }
        unsigned int getMaxBufferObjectPoolSize() const { return _maxBufferObjectPoolSize; }

        /**
         Methods used to set and get defaults for Cameras implicit buffer attachments.
         For more info: See description of Camera::setImplicitBufferAttachment method

         DisplaySettings implicit buffer attachment selection defaults to: DEPTH and COLOR
         for both primary (Render) FBO and secondary Multisample (Resolve) FBO
         ie: IMPLICIT_DEPTH_BUFFER_ATTACHMENT | IMPLICIT_COLOR_BUFFER_ATTACHMENT
        **/
        enum ImplicitBufferAttachment
        {
            IMPLICIT_DEPTH_BUFFER_ATTACHMENT = (1 << 0),
            IMPLICIT_STENCIL_BUFFER_ATTACHMENT = (1 << 1),
            IMPLICIT_COLOR_BUFFER_ATTACHMENT = (1 << 2),
            DEFAULT_IMPLICIT_BUFFER_ATTACHMENT = IMPLICIT_COLOR_BUFFER_ATTACHMENT | IMPLICIT_DEPTH_BUFFER_ATTACHMENT
        };

        typedef int ImplicitBufferAttachmentMask;

        void setImplicitBufferAttachmentMask(ImplicitBufferAttachmentMask renderMask = DisplaySettings::DEFAULT_IMPLICIT_BUFFER_ATTACHMENT, ImplicitBufferAttachmentMask resolveMask = DisplaySettings::DEFAULT_IMPLICIT_BUFFER_ATTACHMENT )
        {
            _implicitBufferAttachmentRenderMask = renderMask;
            _implicitBufferAttachmentResolveMask = resolveMask;
        }

        void setImplicitBufferAttachmentRenderMask(ImplicitBufferAttachmentMask implicitBufferAttachmentRenderMask)
        {
            _implicitBufferAttachmentRenderMask = implicitBufferAttachmentRenderMask;
        }

        void setImplicitBufferAttachmentResolveMask(ImplicitBufferAttachmentMask implicitBufferAttachmentResolveMask)
        {
            _implicitBufferAttachmentResolveMask = implicitBufferAttachmentResolveMask;
        }

        /** Get mask selecting default implicit buffer attachments for Cameras primary FBOs. */
        ImplicitBufferAttachmentMask getImplicitBufferAttachmentRenderMask() const {  return _implicitBufferAttachmentRenderMask; }

        /** Get mask selecting default implicit buffer attachments for Cameras secondary MULTISAMPLE FBOs. */
        ImplicitBufferAttachmentMask getImplicitBufferAttachmentResolveMask() const { return _implicitBufferAttachmentResolveMask;}

        enum SwapMethod
        {
            SWAP_DEFAULT,   // Leave swap method at default returned by choose Pixel Format.
            SWAP_EXCHANGE,  // Flip front / back buffer.
            SWAP_COPY,      // Copy back to front buffer.
            SWAP_UNDEFINED  // Move back to front buffer leaving contents of back buffer undefined.
        };

        /** Select preferred swap method */
        void setSwapMethod( SwapMethod swapMethod ) { _swapMethod = swapMethod; }

        /** Get preferred swap method */
        SwapMethod getSwapMethod( void ) { return _swapMethod; }


        /** Set whether Arb Sync should be used to manage the swaps buffers, 0 disables the use of the sync, greater than zero enables sync based on number of frames specified.*/
        void setSyncSwapBuffers(unsigned int numFrames=0) { _syncSwapBuffers = numFrames; }

        /** Set whether Arb Sync should be used to manage the swaps buffers.*/
        unsigned int getSyncSwapBuffers() const { return _syncSwapBuffers; }



        /** Set the hint of which OpenGL version to attempt to create a graphics context for.*/
        void setGLContextVersion(const std::string& version) { _glContextVersion = version; }

        /** Get the hint of which OpenGL version to attempt to create a graphics context for.*/
        const std::string getGLContextVersion() const { return _glContextVersion; }

        /** Set the hint of the flags to use in when creating graphic contexts.*/
        void setGLContextFlags(unsigned int flags) { _glContextFlags = flags; }

        /** Get the hint of the flags to use in when creating graphic contexts.*/
        unsigned int getGLContextFlags() const { return _glContextFlags; }

        /** Set the hint of the profile mask to use in when creating graphic contexts.*/
        void setGLContextProfileMask(unsigned int mask) { _glContextProfileMask = mask; }

        /** Get the hint of the profile mask to use in when creating graphic contexts.*/
        unsigned int getGLContextProfileMask() const { return _glContextProfileMask; }

        /** Set the NvOptimusEnablement value. Default can be set using OSG_NvOptimusEnablement env var.*/
        void setNvOptimusEnablement(int value);
        /** Get the NvOptimusEnablement value. */
        int getNvOptimusEnablement() const;


        enum VertexBufferHint
        {
            NO_PREFERENCE,
            VERTEX_BUFFER_OBJECT,
            VERTEX_ARRAY_OBJECT
        };

        void setVertexBufferHint(VertexBufferHint gi) { _vertexBufferHint = gi; }
        VertexBufferHint getVertexBufferHint() const { return _vertexBufferHint; }


        enum ShaderHint
        {
            SHADER_NONE,
            SHADER_GL2,
            SHADER_GLES2,
            SHADER_GL3,
            SHADER_GLES3
        };

        /** set the ShaderHint to tells shader generating cdoes  version to create.
         * By default also OSG_GLSL_VERSION and OSG_PRECISION_FLOAT values that can get use directly in shaders using $OSG_GLSL_VERSION and $OSG_PRECISION_FLOAT respectively.*/
        void setShaderHint(ShaderHint hint, bool setShaderValues=true);
        ShaderHint getShaderHint() const { return _shaderHint; }

        /** Set the TextShaderTechnique that is used in the Text default constructor to choose which osgText::ShaderTechnique to use.*/
        void setTextShaderTechnique(const std::string& str) { _textShaderTechnique = str; }
        const std::string& getTextShaderTechnique() const { return _textShaderTechnique; }


        void setKeystoneHint(bool enabled) { _keystoneHint = enabled; }
        bool getKeystoneHint() const { return _keystoneHint; }

        typedef std::vector<std::string> FileNames;
        void setKeystoneFileNames(const FileNames& filenames) { _keystoneFileNames = filenames; }
        FileNames& getKeystoneFileNames() { return _keystoneFileNames; }
        const FileNames& getKeystoneFileNames() const { return _keystoneFileNames; }

        typedef std::vector< osg::ref_ptr<osg::Object> > Objects;
        void setKeystones(const Objects& objects) { _keystones = objects; }
        Objects& getKeystones() { return _keystones; }
        const Objects& getKeystones() const { return _keystones; }

        enum OSXMenubarBehavior
        {
            MENUBAR_AUTO_HIDE,
            MENUBAR_FORCE_HIDE,
            MENUBAR_FORCE_SHOW
        };

        OSXMenubarBehavior getOSXMenubarBehavior() const { return _OSXMenubarBehavior; }
        void setOSXMenubarBehavior(OSXMenubarBehavior hint) { _OSXMenubarBehavior = hint; }

        /** helper function for computing the left eye projection matrix.*/
        virtual osg::Matrixd computeLeftEyeProjectionImplementation(const osg::Matrixd& projection) const;

        /** helper function for computing the left eye view matrix.*/
        virtual osg::Matrixd computeLeftEyeViewImplementation(const osg::Matrixd& view, double eyeSeperationScale=1.0) const;

        /** helper function for computing the right eye view matrix.*/
        virtual osg::Matrixd computeRightEyeProjectionImplementation(const osg::Matrixd& projection) const;

        /** helper function for computing the right eye view matrix.*/
        virtual osg::Matrixd computeRightEyeViewImplementation(const osg::Matrixd& view, double eyeSeperationScale=1.0) const;


        typedef std::vector< std::string > Filenames;

        void setShaderPipeline(bool flag) { _shaderPipeline = flag; }
        bool getShaderPipeline() const { return _shaderPipeline; }

        void setShaderPipelineFiles(const Filenames& filename) { _shaderPipelineFiles = filename; }
        const Filenames& getShaderPipelineFiles() const { return _shaderPipelineFiles; }

        void setShaderPipelineNumTextureUnits(unsigned int units) { _shaderPipelineNumTextureUnits = units; }
        unsigned int getShaderPipelineNumTextureUnits() const { return _shaderPipelineNumTextureUnits; }

        void setValue(const std::string& name, const std::string& value);
        bool getValue(const std::string& name, std::string& value, bool use_getenv_fallback=true) const;

        void setObject(const std::string& name, osg::Object* object) { _objectMap[name] = object; }
        Object* getObject(const std::string& name) { ObjectMap::iterator itr = _objectMap.find(name); return itr!=_objectMap.end() ? itr->second.get() : 0; }
        const Object* getObject(const std::string& name) const { ObjectMap::const_iterator itr = _objectMap.find(name); return itr!=_objectMap.end() ? itr->second.get() : 0; }

protected:

        virtual ~DisplaySettings();


        DisplayType                     _displayType;
        bool                            _stereo;
        StereoMode                      _stereoMode;
        float                           _eyeSeparation;
        float                           _screenWidth;
        float                           _screenHeight;
        float                           _screenDistance;

        SplitStereoHorizontalEyeMapping _splitStereoHorizontalEyeMapping;
        int                             _splitStereoHorizontalSeparation;
        SplitStereoVerticalEyeMapping   _splitStereoVerticalEyeMapping;
        int                             _splitStereoVerticalSeparation;
        bool                            _splitStereoAutoAdjustAspectRatio;

        bool                            _doubleBuffer;
        bool                            _RGB;
        bool                            _depthBuffer;
        unsigned int                    _minimumNumberAlphaBits;
        unsigned int                    _minimumNumberStencilBits;
        unsigned int                    _minimumNumberAccumRedBits;
        unsigned int                    _minimumNumberAccumGreenBits;
        unsigned int                    _minimumNumberAccumBlueBits;
        unsigned int                    _minimumNumberAccumAlphaBits;

        unsigned int                    _maxNumOfGraphicsContexts;

        unsigned int                    _numMultiSamples;

        bool                            _compileContextsHint;
        bool                            _serializeDrawDispatch;
        bool                            _useSceneViewForStereoHint;

        unsigned int                    _numDatabaseThreadsHint;
        unsigned int                    _numHttpDatabaseThreadsHint;

        std::string                     _application;

        unsigned int                    _maxTexturePoolSize;
        unsigned int                    _maxBufferObjectPoolSize;

        ImplicitBufferAttachmentMask    _implicitBufferAttachmentRenderMask;
        ImplicitBufferAttachmentMask    _implicitBufferAttachmentResolveMask;

        std::string                     _glContextVersion;
        unsigned int                    _glContextFlags;
        unsigned int                    _glContextProfileMask;

        SwapMethod                      _swapMethod;
        unsigned int                    _syncSwapBuffers;

        VertexBufferHint                _vertexBufferHint;
        ShaderHint                      _shaderHint;
        std::string                     _textShaderTechnique;

        bool                            _keystoneHint;
        FileNames                       _keystoneFileNames;
        Objects                         _keystones;

        OSXMenubarBehavior              _OSXMenubarBehavior;


        bool                            _shaderPipeline;
        Filenames                       _shaderPipelineFiles;
        unsigned int                    _shaderPipelineNumTextureUnits;

        typedef std::map<std::string, std::string> ValueMap;
        typedef std::map<std::string, ref_ptr<Object> > ObjectMap;

        mutable OpenThreads::Mutex      _valueMapMutex;
        mutable ValueMap                _valueMap;
        mutable ObjectMap               _objectMap;

};

}


// OSGFILE include/osg/OperationThread

/*
#include <osg/observer_ptr>
#include <osg/Object>

#include <OpenThreads/Thread>
#include <OpenThreads/Barrier>
#include <OpenThreads/Condition>
#include <OpenThreads/Block>
*/

#include <list>
#include <set>

namespace osg {

class RefBlock : virtual public osg::Referenced, public OpenThreads::Block
{
    public:

        RefBlock():
            osg::Referenced(true) {}

};

class RefBlockCount : virtual public osg::Referenced, public OpenThreads::BlockCount
{
    public:

        RefBlockCount(unsigned blockCount):
            osg::Referenced(true),
            OpenThreads::BlockCount(blockCount) {}

};

/** Base class for implementing graphics operations.*/
class Operation : virtual public Referenced
{
    public:

        Operation(const std::string& name, bool keep):
            _name(name),
            _keep(keep) {}


        /** Set the human readable name of the operation.*/
        void setName(const std::string& name) { _name = name; }

        /** Get the human readable name of the operation.*/
        const std::string& getName() const { return _name; }

        /** Set whether the operation should be kept once its been applied.*/
        void setKeep(bool keep) { _keep = keep; }

        /** Get whether the operation should be kept once its been applied.*/
        bool getKeep() const { return _keep; }

        /** if this operation is a barrier then release it.*/
        virtual void release() {}

        /** Do the actual task of this operation.*/
        virtual void operator () (Object*) = 0;

protected:

        Operation():
            _keep(false) {}

        virtual ~Operation() {}

        std::string _name;
        bool        _keep;
};

class OperationThread;

class OSG_EXPORT OperationQueue : public Referenced
{
    public:

        OperationQueue();

        /** Get the next operation from the operation queue.
          * Return null ref_ptr<> if no operations are left in queue. */
        osg::ref_ptr<Operation> getNextOperation(bool blockIfEmpty = false);

        /** Return true if the operation queue is empty. */
        bool empty();

        /** Return the num of pending operations that are sitting in the OperationQueue.*/
        unsigned int getNumOperationsInQueue();

        /** Add operation to end of OperationQueue, this will be
          * executed by the operation thread once this operation gets to the head of the queue.*/
        void add(Operation* operation);

        /** Remove operation from OperationQueue.*/
        void remove(Operation* operation);

        /** Remove named operation from OperationQueue.*/
        void remove(const std::string& name);

        /** Remove all operations from OperationQueue.*/
        void removeAllOperations();

        /** Run the operations. */
        void runOperations(Object* callingObject=0);

        /** Call release on all operations. */
        void releaseAllOperations();

        /** Release operations block that is used to block threads that are waiting on an empty operations queue.*/
        void releaseOperationsBlock();

        typedef std::set<OperationThread*> OperationThreads;

        /** Get the set of OperationThreads that are sharing this OperationQueue. */
        const OperationThreads& getOperationThreads() const { return _operationThreads; }

    protected:

        virtual ~OperationQueue();

        friend class OperationThread;

        void addOperationThread(OperationThread* thread);
        void removeOperationThread(OperationThread* thread);

        typedef std::list< osg::ref_ptr<Operation> > Operations;

        OpenThreads::Mutex          _operationsMutex;
        osg::ref_ptr<osg::RefBlock> _operationsBlock;
        Operations                  _operations;
        Operations::iterator        _currentOperationIterator;

        OperationThreads            _operationThreads;
};

/** OperationThread is a helper class for running Operation within a single thread.*/
class OSG_EXPORT OperationThread : public Referenced, public OpenThreads::Thread
{
    public:
        OperationThread();

        void setParent(Object* parent) { _parent = parent; }

        Object* getParent() { return _parent.get(); }

        const Object* getParent() const { return _parent.get(); }


        /** Set the OperationQueue. */
        void setOperationQueue(OperationQueue* opq);

        /** Get the OperationQueue. */
        OperationQueue* getOperationQueue() { return _operationQueue.get(); }

        /** Get the const OperationQueue. */
        const OperationQueue* getOperationQueue() const { return _operationQueue.get(); }


        /** Add operation to end of OperationQueue, this will be
          * executed by the graphics thread once this operation gets to the head of the queue.*/
        void add(Operation* operation);

        /** Remove operation from OperationQueue.*/
        void remove(Operation* operation);

        /** Remove named operation from OperationQueue.*/
        void remove(const std::string& name);

        /** Remove all operations from OperationQueue.*/
        void removeAllOperations();


        /** Get the operation currently being run.*/
        osg::ref_ptr<Operation> getCurrentOperation() { return _currentOperation; }

        /** Run does the opertion thread run loop.*/
        virtual void run();

        void setDone(bool done);

        bool getDone() const { return _done!=0; }

        /** Cancel this graphics thread.*/
        virtual int cancel();

    protected:

        virtual ~OperationThread();

        observer_ptr<Object>            _parent;

        OpenThreads::Atomic             _done;

        OpenThreads::Mutex              _threadMutex;
        osg::ref_ptr<OperationQueue>    _operationQueue;
        osg::ref_ptr<Operation>         _currentOperation;

};

typedef OperationThread OperationsThread;

}


// OSGFILE include/osg/State

/*
#include <osg/Export>
#include <osg/GLExtensions>
#include <osg/StateSet>
#include <osg/Matrix>
#include <osg/Uniform>
#include <osg/BufferObject>
#include <osg/Observer>
#include <osg/Timer>
#include <osg/VertexArrayState>

#include <osg/ShaderComposer>
#include <osg/FrameStamp>
#include <osg/DisplaySettings>
#include <osg/Polytope>
#include <osg/Viewport>
#include <osg/AttributeDispatchers>
#include <osg/GraphicsCostEstimator>
*/

#include <iosfwd>
#include <vector>
#include <map>
#include <set>
#include <string>

#ifndef GL_TEXTURE0
    #define GL_TEXTURE0 0x84C0
    #define GL_TEXTURE1 0x84C1
    #define GL_TEXTURE2 0x84C2
    #define GL_TEXTURE3 0x84C3
    #define GL_TEXTURE4 0x84C4
    #define GL_TEXTURE5 0x84C5
    #define GL_TEXTURE6 0x84C6
    #define GL_TEXTURE7 0x84C7
#endif

namespace osg {

/** macro for use with osg::StateAttribute::apply methods for detecting and
  * reporting OpenGL error messages.*/
#define OSG_GL_DEBUG(message) \
    if (state.getFineGrainedErrorDetection()) \
    { \
        GLenum errorNo = glGetError(); \
        if (errorNo!=GL_NO_ERROR) \
        { \
            osg::notify(WARN)<<"Warning: detected OpenGL error '"<<gluErrorString(errorNo)<<" "<<message<<endl; \
        }\
    }


// forward declare GraphicsContext, View and State
class GraphicsContext;

/** Encapsulates the current applied OpenGL modes, attributes and vertex arrays settings,
  * implements lazy state updating and provides accessors for querying the current state.
  * The venerable Red Book says that "OpenGL is a state machine", and this class
  * represents the OpenGL state in OSG. Furthermore, \c State also has other
  * important features:
  * - It works as a stack of states (see \c pushStateSet() and
  *   \c popStateSet()). Manipulating this stack of OpenGL states manually is
  *   seldom needed, since OSG does this in the most common situations.
  * - It implements lazy state updating. This means that, if one requests a
  *   state change and that particular state is already in the requested state,
  *   no OpenGL call will be made. This ensures that the OpenGL pipeline is not
  *   stalled by unnecessary state changes.
  * - It allows to query the current OpenGL state without calls to \c glGet*(),
  *   which typically stall the graphics pipeline (see, for instance,
  *   \c captureCurrentState() and \c getModelViewMatrix()).
  */
class OSG_EXPORT State : public Referenced
{
    public :

        State();


        /** Set the graphics context associated with that owns this State object.*/
        void setGraphicsContext(GraphicsContext* context) { _graphicsContext = context; }

        /** Get the graphics context associated with that owns this State object.*/
        GraphicsContext* getGraphicsContext() { return _graphicsContext; }

        /** Get the const graphics context associated with that owns this State object.*/
        const GraphicsContext* getGraphicsContext() const { return _graphicsContext; }


        /** Set the current OpenGL context uniqueID.
         *  The ContextID is used by classes like osg::StateAttribute's and osg::Drawable's to
         *  help manage separate OpenGL objects, such as display lists, vertex buffer objects
         *  and texture object for each graphics context. The ContextID simply acts as an index
         *  into arrays that these classes maintain for the purpose of storing GL object handles.
         *
         *  Note, osgViewer::GraphicsWindow will automatically set up the ContextID for you,
         *  so you will rearely need to set this yourself.
         *
         *  The exception is when creating your own graphics context, where you should set
         *  the ContextID uniquely for each graphics context.
         *
         *  Typical settings for ContextID are 0,1,2,3... up to the maximum
         *  number of graphics contexts you have set up. By default contextID is 0.
         */
        inline void setContextID(unsigned int contextID) { _contextID=contextID; }

        /** Get the current OpenGL context unique ID.*/
        inline unsigned int getContextID() const { return _contextID; }


        // ExtensionMap contains GL Extentsions objects used by StateAttribue to call OpenGL extensions/advanced features
        typedef std::map<const std::type_info*, osg::ref_ptr<osg::Referenced> > ExtensionMap;
        ExtensionMap _extensionMap;

        /** Get a specific GL extensions object or GraphicsObjectManager, initialize if not already present.
          * Note, must only be called from a the graphics context thread associated with this osg::State. */
        template<typename T>
        T* get()
        {
            const std::type_info* id(&typeid(T));
            osg::ref_ptr<osg::Referenced>& ptr = _extensionMap[id];
            if (!ptr)
            {
                ptr = new T(_contextID);
            }
            return static_cast<T*>(ptr.get());
        }

        /** Get a specific GL extensions object or GraphicsObjectManager if it already exists in the extension map.
          * Note, safe to call outwith a the graphics context thread associated with this osg::State.
          * Returns NULL if the desired extension object has not been created yet.*/
        template<typename T>
        const T* get() const
        {
            const std::type_info* id(&typeid(T));
            ExtensionMap::const_iterator itr = _extensionMap.find(id);
            if (itr==_extensionMap.end()) return 0;
            else return itr->second.get();
        }

        /** Set a specific GL extensions object pr GraphicsObjectManager. */
        template<typename T>
        void set(T* ptr)
        {
            const std::type_info* id(&typeid(T));
            _extensionMap[id] = ptr;
        }

        /* deprecated.*/
        void setShaderCompositionEnabled(bool flag) { _shaderCompositionEnabled = flag; }

        /* deprecated.*/
        bool getShaderCompositionEnabled() const { return _shaderCompositionEnabled; }

        /** deprecated.*/
        void setShaderComposer(ShaderComposer* sc) { _shaderComposer = sc; }

        /** deprecated.*/
        ShaderComposer* getShaderComposer() { return _shaderComposer.get(); }

        /** deprecated.*/
        const ShaderComposer* getShaderComposer() const { return _shaderComposer.get(); }

        /** Get the unform list in which to inject any uniforms that StateAttribute::apply(State&) methods provide.*/
        StateSet::UniformList& getCurrentShaderCompositionUniformList() { return _currentShaderCompositionUniformList; }

        /** Convenience method for StateAttribute::apply(State&) methods to pass on their uniforms to osg::State so it can apply them at the appropriate point.*/
        void applyShaderCompositionUniform(const osg::UniformBase* uniform, StateAttribute::OverrideValue value=StateAttribute::ON)
        {
            StateSet::RefUniformPair& up = _currentShaderCompositionUniformList[uniform->getName()];
            up.first = const_cast<UniformBase*>(uniform);
            up.second = value;
        }


        /** Convenience method for StateAttribute::apply(State&) methods to pass on their defines to osg::State so it can apply them at the appropriate point.*/
        void applyShaderCompositionDefines(const StateSet::DefineList& defineList)
        {
            _currentShaderCompositionDefines.insert(defineList.begin(), defineList.end());
        }



        /** Push stateset onto state stack.*/
        void pushStateSet(const StateSet* dstate);

        /** Pop stateset off state stack.*/
        void popStateSet();

        /** pop all statesets off state stack, ensuring it is empty ready for the next frame.
          * Note, to return OpenGL to default state, one should do any state.popAllStatSets(); state.apply().*/
        void popAllStateSets();

        /** Insert stateset onto state stack.*/
        void insertStateSet(unsigned int pos,const StateSet* dstate);

        /** Pop stateset off state stack.*/
        void removeStateSet(unsigned int pos);

        /** Get the number of StateSet's on the StateSet stack.*/
        unsigned int getStateSetStackSize() { return static_cast<unsigned int>(_stateStateStack.size()); }

        /** Pop StateSet's for the StateSet stack till its size equals the specified size.*/
        void popStateSetStackToSize(unsigned int size) { while (_stateStateStack.size()>size) popStateSet(); }

        typedef std::vector<const StateSet*> StateSetStack;

        /** Get the StateSet stack.*/
        StateSetStack& getStateSetStack() { return _stateStateStack; }


        /** Copy the modes and attributes which capture the current state.*/
        void captureCurrentState(StateSet& stateset) const;

        /** Release all OpenGL objects associated cached by this osg::State object.*/
        void releaseGLObjects();

        /** reset the state object to an empty stack.*/
        void reset();

        inline const Viewport* getCurrentViewport() const
        {
            return static_cast<const Viewport*>(getLastAppliedAttribute(osg::StateAttribute::VIEWPORT));
        }


        void setUseStateAttributeShaders(bool flag);
        bool getUseStateAttributeShaders() const { return _useStateAttributeShaders; }

        void setUseStateAttributeFixedFunction(bool flag);
        bool getUseStateAttributeFixedFunction() const { return _useStateAttributeFixedFunction; }

        void setUseModelViewAndProjectionUniforms(bool flag);
        bool getUseModelViewAndProjectionUniforms() const { return _useModelViewAndProjectionUniforms; }

        void setUseVertexAttributeAliasing(bool flag);
        bool getUseVertexAttributeAliasing() const { return _useVertexAttributeAliasing ; }



        void setInitialViewMatrix(const osg::RefMatrix* matrix);

        inline const osg::Matrix& getInitialViewMatrix() const { return *_initialViewMatrix; }
        inline const osg::Matrix& getInitialInverseViewMatrix() const { return _initialInverseViewMatrix; }

        void applyProjectionMatrix(const osg::RefMatrix* matrix);

        inline const osg::Matrix& getProjectionMatrix() const { return *_projection; }

        void applyModelViewMatrix(const osg::RefMatrix* matrix);
        void applyModelViewMatrix(const osg::Matrix&);

        const osg::Matrix& getModelViewMatrix() const { return *_modelView; }

        void updateModelViewAndProjectionMatrixUniforms();

        void applyModelViewAndProjectionUniformsIfRequired();

        osg::Uniform* getModelViewMatrixUniform() { return _modelViewMatrixUniform.get(); }
        osg::Uniform* getProjectionMatrixUniform() { return _projectionMatrixUniform.get(); }
        osg::Uniform* getModelViewProjectionMatrixUniform() { return _modelViewProjectionMatrixUniform.get(); }
        osg::Uniform* getNormalMatrixUniform() { return _normalMatrixUniform.get(); }


        Polytope getViewFrustum() const;


        typedef std::vector<VertexAttribAlias> VertexAttribAliasList;

        /** Reset the vertex attribute aliasing to osg's default. This method needs to be called before render anything unless you really know what you're doing !*/
        void resetVertexAttributeAlias(bool compactAliasing=true, unsigned int numTextureUnits=8);

        /** Set the vertex attribute aliasing for "vertex". This method needs to be called before render anything unless you really know what you're doing !*/
        void setVertexAlias(const VertexAttribAlias& alias) { _vertexAlias = alias; }
        const VertexAttribAlias& getVertexAlias() { return _vertexAlias; }

        /** Set the vertex attribute aliasing for "normal". This method needs to be called before render anything unless you really know what you're doing !*/
        void setNormalAlias(const VertexAttribAlias& alias) { _normalAlias = alias; }
        const VertexAttribAlias& getNormalAlias() { return _normalAlias; }

        /** Set the vertex attribute aliasing for "color". This method needs to be called before render anything unless you really know what you're doing !*/
        void setColorAlias(const VertexAttribAlias& alias) { _colorAlias = alias; }
        const VertexAttribAlias& getColorAlias() { return _colorAlias; }

        /** Set the vertex attribute aliasing for "secondary color". This method needs to be called before render anything unless you really know what you're doing !*/
        void setSecondaryColorAlias(const VertexAttribAlias& alias) { _secondaryColorAlias = alias; }
        const VertexAttribAlias& getSecondaryColorAlias() { return _secondaryColorAlias; }

        /** Set the vertex attribute aliasing for "fog coord". This method needs to be called before render anything unless you really know what you're doing !*/
        void setFogCoordAlias(const VertexAttribAlias& alias) { _fogCoordAlias = alias; }
        const VertexAttribAlias& getFogCoordAlias() { return _fogCoordAlias; }

        /** Set the vertex attribute aliasing list for texture coordinates. This method needs to be called before render anything unless you really know what you're doing !*/
        void setTexCoordAliasList(const VertexAttribAliasList& aliasList) { _texCoordAliasList = aliasList; }
        const VertexAttribAliasList& getTexCoordAliasList() { return _texCoordAliasList; }

        /** Set the vertex attribute binding list. This method needs to be called before render anything unless you really know what you're doing !*/
        void  setAttributeBindingList(const Program::AttribBindingList& attribBindingList) { _attributeBindingList = attribBindingList; }
        const Program::AttribBindingList&  getAttributeBindingList() { return _attributeBindingList; }

        bool convertVertexShaderSourceToOsgBuiltIns(std::string& source) const;


        /** Apply stateset.*/
        void apply(const StateSet* dstate);

        /** Updates the OpenGL state so that it matches the \c StateSet at the
          * top of the stack of <tt>StateSet</tt>s maintained internally by a
          * \c State.
         */
        void apply();

        /** Apply any shader composed state.*/
        void applyShaderComposition();


        void glDrawBuffer(GLenum buffer);
        GLenum getDrawBuffer() const { return _drawBuffer; }

        void glReadBuffer(GLenum buffer);
        GLenum getReadBuffer() const { return _readBuffer; }


        /** Set whether a particular OpenGL mode is valid in the current graphics context.
          * Use to disable OpenGL modes that are not supported by current graphics drivers/context.*/
        inline void setModeValidity(StateAttribute::GLMode mode,bool valid)
        {
            ModeStack& ms = _modeMap[mode];
            ms.valid = valid;
        }

        /** Get whether a particular OpenGL mode is valid in the current graphics context.
          * Use to disable OpenGL modes that are not supported by current graphics drivers/context.*/
        inline bool getModeValidity(StateAttribute::GLMode mode)
        {
            ModeStack& ms = _modeMap[mode];
            return ms.valid;
        }

        inline void setGlobalDefaultModeValue(StateAttribute::GLMode mode,bool enabled)
        {
            ModeStack& ms = _modeMap[mode];
            ms.global_default_value = enabled;
        }

        inline bool getGlobalDefaultModeValue(StateAttribute::GLMode mode)
        {
            return _modeMap[mode].global_default_value;
        }

        inline bool getLastAppliedModeValue(StateAttribute::GLMode mode)
        {
            return _modeMap[mode].last_applied_value;
        }

        /** Proxy helper class for applyig a model in a local scope, with the preivous value being resotred automatically on leaving the scope that proxy was created.*/
        struct ApplyModeProxy
        {
            inline ApplyModeProxy(osg::State& state, GLenum mode, bool value):_state(state), _mode(mode)
            {
                _previous_value = _state.getLastAppliedModeValue(mode);
                _need_to_apply_value = (_previous_value!=value);
                if (_need_to_apply_value) _state.applyMode(_mode, value);
            }
            inline ~ApplyModeProxy()
            {
                if (_need_to_apply_value) _state.applyMode(_mode, _previous_value);
            }

            osg::State& _state;
            GLenum      _mode;
            bool        _previous_value;
            bool        _need_to_apply_value;
        };

        struct ApplyTextureModeProxy
        {
            inline ApplyTextureModeProxy(osg::State& state, unsigned int unit, GLenum mode, bool value):_state(state), _unit(unit), _mode(mode)
            {
                _previous_value = _state.getLastAppliedTextureModeValue(_unit, _mode);
                _need_to_apply_value = (_previous_value!=value);
                if (_need_to_apply_value) _state.applyTextureMode(_unit, _mode, value);
            }
            inline ~ApplyTextureModeProxy()
            {
                if (_need_to_apply_value) _state.applyTextureMode(_unit, _mode, _previous_value);
            }

            osg::State&     _state;
            unsigned int    _unit;
            GLenum          _mode;
            bool            _previous_value;
            bool            _need_to_apply_value;
        };

        /** Apply an OpenGL mode if required. This is a wrapper around
          * \c glEnable() and \c glDisable(), that just actually calls these
          * functions if the \c enabled flag is different than the current
          * state.
          * @return \c true if the state was actually changed. \c false
          *         otherwise. Notice that a \c false return does not indicate
          *         an error, it just means that the mode was already set to the
          *         same value as the \c enabled parameter.
        */
        inline bool applyMode(StateAttribute::GLMode mode,bool enabled)
        {
            ModeStack& ms = _modeMap[mode];
            ms.changed = true;
            return applyMode(mode,enabled,ms);
        }

        inline void setGlobalDefaultTextureModeValue(unsigned int unit, StateAttribute::GLMode mode,bool enabled)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            ms.global_default_value = enabled;
        }

        inline bool getGlobalDefaultTextureModeValue(unsigned int unit, StateAttribute::GLMode mode)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            return ms.global_default_value;
        }

        inline bool applyTextureMode(unsigned int unit, StateAttribute::GLMode mode,bool enabled)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            ms.changed = true;
            return applyModeOnTexUnit(unit,mode,enabled,ms);
        }

        inline bool getLastAppliedTextureModeValue(unsigned int unit, StateAttribute::GLMode mode)
        {
            ModeMap& modeMap = getOrCreateTextureModeMap(unit);
            ModeStack& ms = modeMap[mode];
            return ms.last_applied_value;
        }

        inline void setGlobalDefaultAttribute(const StateAttribute* attribute)
        {
            AttributeStack& as = _attributeMap[attribute->getTypeMemberPair()];
            as.global_default_attribute = attribute;
        }

        inline const StateAttribute* getGlobalDefaultAttribute(StateAttribute::Type type, unsigned int member=0)
        {
            AttributeStack& as = _attributeMap[StateAttribute::TypeMemberPair(type,member)];
            return as.global_default_attribute.get();
        }

        /** Apply an attribute if required. */
        inline bool applyAttribute(const StateAttribute* attribute)
        {
            AttributeStack& as = _attributeMap[attribute->getTypeMemberPair()];
            as.changed = true;
            return applyAttribute(attribute,as);
        }

        inline void setGlobalDefaultTextureAttribute(unsigned int unit, const StateAttribute* attribute)
        {
            AttributeMap& attributeMap = getOrCreateTextureAttributeMap(unit);
            AttributeStack& as = attributeMap[attribute->getTypeMemberPair()];
            as.global_default_attribute = attribute;
        }

        inline const StateAttribute* getGlobalDefaultTextureAttribute(unsigned int unit, StateAttribute::Type type, unsigned int member = 0)
        {
            AttributeMap& attributeMap = getOrCreateTextureAttributeMap(unit);
            AttributeStack& as = attributeMap[StateAttribute::TypeMemberPair(type,member)];
            return as.global_default_attribute.get();
        }


        inline bool applyTextureAttribute(unsigned int unit, const StateAttribute* attribute)
        {
            AttributeMap& attributeMap = getOrCreateTextureAttributeMap(unit);
            AttributeStack& as = attributeMap[attribute->getTypeMemberPair()];
            as.changed = true;
            return applyAttributeOnTexUnit(unit,attribute,as);
        }

        /** Mode has been set externally, update state to reflect this setting.*/
        void haveAppliedMode(StateAttribute::GLMode mode,StateAttribute::GLModeValue value);

        /** Mode has been set externally, therefore dirty the associated mode in osg::State
          * so it is applied on next call to osg::State::apply(..)*/
        void haveAppliedMode(StateAttribute::GLMode mode);

        /** Attribute has been applied externally, update state to reflect this setting.*/
        void haveAppliedAttribute(const StateAttribute* attribute);

        /** Attribute has been applied externally,
          * and therefore this attribute type has been dirtied
          * and will need to be re-applied on next osg::State.apply(..).
          * note, if you have an osg::StateAttribute which you have applied externally
          * then use the have_applied(attribute) method as this will cause the osg::State to
          * track the current state more accurately and enable lazy state updating such
          * that only changed state will be applied.*/
        void haveAppliedAttribute(StateAttribute::Type type, unsigned int member=0);

        /** Get whether the current specified mode is enabled (true) or disabled (false).*/
        bool getLastAppliedMode(StateAttribute::GLMode mode) const;

        /** Get the current specified attribute, return NULL if one has not yet been applied.*/
        const StateAttribute* getLastAppliedAttribute(StateAttribute::Type type, unsigned int member=0) const;

        /** texture Mode has been set externally, update state to reflect this setting.*/
        void haveAppliedTextureMode(unsigned int unit, StateAttribute::GLMode mode,StateAttribute::GLModeValue value);

        /** texture Mode has been set externally, therefore dirty the associated mode in osg::State
          * so it is applied on next call to osg::State::apply(..)*/
        void haveAppliedTextureMode(unsigned int unit, StateAttribute::GLMode mode);

        /** texture Attribute has been applied externally, update state to reflect this setting.*/
        void haveAppliedTextureAttribute(unsigned int unit, const StateAttribute* attribute);

        /** texture Attribute has been applied externally,
          * and therefore this attribute type has been dirtied
          * and will need to be re-applied on next osg::State.apply(..).
          * note, if you have an osg::StateAttribute which you have applied externally
          * then use the have_applied(attribute) method as this will the osg::State to
          * track the current state more accurately and enable lazy state updating such
          * that only changed state will be applied.*/
        void haveAppliedTextureAttribute(unsigned int unit, StateAttribute::Type type, unsigned int member=0);

        /** Get whether the current specified texture mode is enabled (true) or disabled (false).*/
        bool getLastAppliedTextureMode(unsigned int unit, StateAttribute::GLMode mode) const;

        /** Get the current specified texture attribute, return NULL if one has not yet been applied.*/
        const StateAttribute* getLastAppliedTextureAttribute(unsigned int unit, StateAttribute::Type type, unsigned int member=0) const;





        /** Dirty the modes previously applied in osg::State.*/
        void dirtyAllModes();

        /** Dirty the modes attributes previously applied in osg::State.*/
        void dirtyAllAttributes();


        /** Proxy helper class for applyig a VertexArrayState in a local scope, with the preivous value being resotred automatically on leaving the scope that proxy was created.*/
        struct SetCurrentVertexArrayStateProxy
        {
            SetCurrentVertexArrayStateProxy(osg::State& state, VertexArrayState* vas):_state(state) { _state.setCurrentVertexArrayState(vas); }
            ~SetCurrentVertexArrayStateProxy() { _state.setCurrentToGlobalVertexArrayState(); }
            osg::State& _state;
        };

        /** Set the CurrentVetexArrayState object that take which vertex arrays are bound.*/
        void setCurrentVertexArrayState(VertexArrayState* vas) { _vas = vas; }

        /** Get the CurrentVetexArrayState object that take which vertex arrays are bound.*/
        VertexArrayState* getCurrentVertexArrayState() const { return _vas; }

        /** Set the getCurrentVertexArrayState to the GlobalVertexArrayState.*/
        void setCurrentToGlobalVertexArrayState() { _vas = _globalVertexArrayState.get(); }


        /** disable the vertex, normal, color, tex coords, secondary color, fog coord and index arrays.*/
        void disableAllVertexArrays();


        void lazyDisablingOfVertexAttributes() { _vas->lazyDisablingOfVertexAttributes(); }
        void applyDisablingOfVertexAttributes() { _vas->applyDisablingOfVertexAttributes(*this); }

        void setCurrentVertexBufferObject(osg::GLBufferObject* vbo) { _vas->setCurrentVertexBufferObject(vbo); }
        const GLBufferObject* getCurrentVertexBufferObject() { return _vas->getCurrentVertexBufferObject(); }

        void bindVertexBufferObject(osg::GLBufferObject* vbo) { _vas->bindVertexBufferObject(vbo); }
        void unbindVertexBufferObject() { _vas->unbindVertexBufferObject(); }

        void setCurrentElementBufferObject(osg::GLBufferObject* ebo) { _vas->setCurrentElementBufferObject(ebo); }
        const GLBufferObject* getCurrentElementBufferObject() { return _vas->getCurrentElementBufferObject(); }

        void bindElementBufferObject(osg::GLBufferObject* ebo) { _vas->bindElementBufferObject(ebo); }
        void unbindElementBufferObject() { _vas->unbindElementBufferObject(); }


        void setCurrentPixelBufferObject(osg::GLBufferObject* pbo) { _currentPBO = pbo; }
        const GLBufferObject* getCurrentPixelBufferObject() const { return _currentPBO; }

        inline void bindPixelBufferObject(osg::GLBufferObject* pbo)
        {
            if (pbo)
            {
                if (pbo == _currentPBO) return;

                if (pbo->isDirty()) pbo->compileBuffer();
                else pbo->bindBuffer();

                _currentPBO = pbo;
            }
            else
            {
                unbindPixelBufferObject();
            }
        }

        inline void unbindPixelBufferObject()
        {
            if (!_currentPBO) return;

            _glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB,0);
            _currentPBO = 0;
        }


        inline void bindDrawIndirectBufferObject(osg::GLBufferObject* ibo)
        {
            if (ibo->isDirty())
            {
                ibo->compileBuffer();
                _currentDIBO = ibo;
            }
            else if (ibo != _currentDIBO)
            {
                ibo->bindBuffer();
                _currentDIBO = ibo;
            }
        }

        inline void unbindDrawIndirectBufferObject()
        {
            if (!_currentDIBO) return;
            _glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);
            _currentDIBO = 0;
        }

        void setCurrentVertexArrayObject(GLuint vao) { _currentVAO = vao; }
        GLuint getCurrentVertexArrayObject() const { return _currentVAO; }

        inline void bindVertexArrayObject(const VertexArrayState* vas) { bindVertexArrayObject(vas->getVertexArrayObject()); }

        inline void bindVertexArrayObject(GLuint vao) { if (_currentVAO!=vao) { _glExtensions->glBindVertexArray(vao); _currentVAO = vao; } }

        inline void unbindVertexArrayObject() { if (_currentVAO!=0) { _glExtensions->glBindVertexArray(0); _currentVAO = 0; } }


        typedef std::vector<GLushort> IndicesGLushort;
        IndicesGLushort _quadIndicesGLushort[4];

        typedef std::vector<GLuint> IndicesGLuint;
        IndicesGLuint _quadIndicesGLuint[4];

        void drawQuads(GLint first, GLsizei count, GLsizei primCount=0);

        inline void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
        {
            if (primcount>=1 && _glDrawArraysInstanced!=0) _glDrawArraysInstanced(mode, first, count, primcount);
            else glDrawArrays(mode, first, count);
        }

        inline void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount )
        {
            if (primcount>=1 && _glDrawElementsInstanced!=0) _glDrawElementsInstanced(mode, count, type, indices, primcount);
            else glDrawElements(mode, count, type, indices);
        }


        inline void Vertex(float x, float y, float z, float w=1.0f)
        {
        #if defined(OSG_GL_VERTEX_FUNCS_AVAILABLE) && !defined(OSG_GLES1_AVAILABLE)
            if (_useVertexAttributeAliasing) _glVertexAttrib4f( _vertexAlias._location, x,y,z,w);
            else glVertex4f(x,y,z,w);
        #else
            _glVertexAttrib4f( _vertexAlias._location, x,y,z,w);
        #endif
        }

        inline void Color(float r, float g, float b, float a=1.0f)
        {
        #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
            if (_useVertexAttributeAliasing) _glVertexAttrib4f( _colorAlias._location, r,g,b,a);
            else glColor4f(r,g,b,a);
        #else
            _glVertexAttrib4f( _colorAlias._location, r,g,b,a);
        #endif
        }

        void Normal(float x, float y, float z)
        {
        #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
            if (_useVertexAttributeAliasing) _glVertexAttrib4f( _normalAlias._location, x,y,z,0.0);
            else glNormal3f(x,y,z);
        #else
            _glVertexAttrib4f( _normalAlias._location, x,y,z,0.0);
        #endif
        }

        void TexCoord(float x, float y=0.0f, float z=0.0f, float w=1.0f)
        {
        #if !defined(OSG_GLES1_AVAILABLE)
            #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
                if (_useVertexAttributeAliasing) _glVertexAttrib4f( _texCoordAliasList[0]._location, x,y,z,w);
                else glTexCoord4f(x,y,z,w);
            #else
                _glVertexAttrib4f( _texCoordAliasList[0]._location, x,y,z,w);
            #endif
        #endif
        }

        void MultiTexCoord(unsigned int unit, float x, float y=0.0f, float z=0.0f, float w=1.0f)
        {
        #if !defined(OSG_GLES1_AVAILABLE)
            #ifdef OSG_GL_VERTEX_FUNCS_AVAILABLE
                if (_useVertexAttributeAliasing) _glVertexAttrib4f( _texCoordAliasList[unit]._location, x,y,z,w);
                else _glMultiTexCoord4f(GL_TEXTURE0+unit,x,y,z,w);
            #else
                _glVertexAttrib4f( _texCoordAliasList[unit]._location, x,y,z,w);
            #endif
        #endif
        }

        void VerteAttrib(unsigned int location, float x, float y=0.0f, float z=0.0f, float w=0.0f)
        {
            _glVertexAttrib4f( location, x,y,z,w);
        }


        /** Wrapper around glInterleavedArrays(..).
          * also resets the internal array points and modes within osg::State to keep the other
          * vertex array operations consistent. */
        void setInterleavedArrays( GLenum format, GLsizei stride, const GLvoid* pointer) { _vas->setInterleavedArrays( *this, format, stride, pointer); }

        /** Set the vertex pointer using an osg::Array, and manage any VBO that are required.*/
        inline void setVertexPointer(const Array* array) { _vas->setVertexArray(*this, array); }
        inline void setVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_FALSE) { _vas->setVertexArray( *this, size, type, stride, ptr, normalized); }
        inline void disableVertexPointer() { _vas->disableVertexArray(*this); }


        inline void setNormalPointer(const Array* array) { _vas->setNormalArray(*this, array); }
        inline void setNormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_FALSE ) { _vas->setNormalArray( *this, type, stride, ptr, normalized); }
        inline void disableNormalPointer() { _vas->disableNormalArray(*this); }

        inline void setColorPointer(const Array* array) { _vas->setColorArray(*this, array); }
        inline void setColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_TRUE ) { _vas->setColorArray(*this, size, type, stride, ptr, normalized); }
        inline void disableColorPointer() { _vas->disableColorArray(*this); }

        inline bool isSecondaryColorSupported() const { return _isSecondaryColorSupported; }
        inline void setSecondaryColorPointer(const Array* array) { _vas->setSecondaryColorArray(*this, array); }
        inline void disableSecondaryColorPointer() { _vas->disableSecondaryColorArray(*this); }

        inline bool isFogCoordSupported() const { return _isFogCoordSupported; }
        inline void setFogCoordPointer(const Array* array) { _vas->setFogCoordArray(*this, array); }
        inline void disableFogCoordPointer() { _vas->disableFogCoordArray(*this); }


        inline void setTexCoordPointer(unsigned int unit, const Array* array) { _vas->setTexCoordArray(*this, unit, array); }
        inline void setTexCoordPointer( unsigned int unit, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized=GL_FALSE ) { _vas->setTexCoordArray(*this, unit, size, type, stride, ptr, normalized); }
        inline void disableTexCoordPointer( unsigned int unit ) { _vas->disableTexCoordArray(*this, unit); }
        inline void disableTexCoordPointersAboveAndIncluding( unsigned int unit ) { _vas->disableTexCoordArrayAboveAndIncluding(*this, unit); }

        /// For GL>=2.0 uses GL_MAX_TEXTURE_COORDS, for GL<2 uses GL_MAX_TEXTURE_UNITS
        inline GLint getMaxTextureCoords() const { return _glMaxTextureCoords; }

        /// For GL>=2.0 uses GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, for GL<2 uses GL_MAX_TEXTURE_UNITS
        inline GLint getMaxTextureUnits() const { return _glMaxTextureUnits; }


        /** Set the current texture unit, return true if selected,
          * false if selection failed such as when multi texturing is not supported.
          * note, only updates values that change.*/
        inline bool setActiveTextureUnit( unsigned int unit );

        /** Get the current texture unit.*/
        unsigned int getActiveTextureUnit() const { return _currentActiveTextureUnit; }


        /** Set the current texture format uniform.*/
        void setCurrentTextureFormat(GLenum format) { _textureFormat->setElement(_currentActiveTextureUnit, format); applyShaderCompositionUniform(_textureFormat.get()); }


        /** Set the current tex coord array texture unit, return true if selected,
          * false if selection failed such as when multi texturing is not supported.
          * note, only updates values that change.*/
        bool setClientActiveTextureUnit( unsigned int unit );

        /** Get the current tex coord array texture unit.*/
        unsigned int getClientActiveTextureUnit() const;

        inline void setVertexAttribPointer(unsigned int unit, const Array* array) { _vas->setVertexAttribArray(*this, unit, array); }
        inline void setVertexAttribLPointer(unsigned int unit, const Array* array) { _vas->setVertexAttribArray(*this, unit, array); }
        inline void setVertexAttribIPointer(unsigned int unit, const Array* array) { _vas->setVertexAttribArray(*this, unit, array); }

        inline void disableVertexAttribPointer( unsigned int index ) { _vas->disableVertexAttribArray(*this, index); }
        inline void disableVertexAttribPointersAboveAndIncluding( unsigned int index ) { _vas->disableVertexAttribArray(*this, index); }


        /** dirty the vertex, normal, color, tex coords, secondary color, fog coord and index arrays.*/
        void dirtyAllVertexArrays();


        inline bool isVertexBufferObjectSupported() const { return _isVertexBufferObjectSupported; }
        inline bool useVertexBufferObject(bool useVBO) const { return _forceVertexBufferObject || (_isVertexBufferObjectSupported && useVBO); }

        inline bool isVertexArrayObjectSupported() const { return _isVertexArrayObjectSupported; }
        inline bool useVertexArrayObject(bool useVAO) const { return _forceVertexArrayObject || (_isVertexArrayObjectSupported && useVAO); }


        inline void setLastAppliedProgramObject(const Program::PerContextProgram* program)
        {
            if (_lastAppliedProgramObject!=program)
            {
                _lastAppliedProgramObject = program;
            }
        }
        inline const Program::PerContextProgram* getLastAppliedProgramObject() const { return _lastAppliedProgramObject; }

        inline GLint getUniformLocation( unsigned int uniformNameID ) const { return _lastAppliedProgramObject ? _lastAppliedProgramObject->getUniformLocation(uniformNameID) : -1; }
        /**
          * Alternative version of getUniformLocation( unsigned int uniformNameID )
          * retrofited into OSG for backward compatibility with osgCal,
          * after uniform ids were refactored from std::strings to GLints in OSG version 2.9.10.
          *
          * Drawbacks: This method is not particularly fast. It has to access mutexed static
          * map of uniform ids. So don't overuse it or your app performance will suffer.
        */
        inline GLint getUniformLocation( const std::string & uniformName ) const { return _lastAppliedProgramObject ? _lastAppliedProgramObject->getUniformLocation(uniformName) : -1; }
        inline GLint getAttribLocation( const std::string& name ) const { return _lastAppliedProgramObject ? _lastAppliedProgramObject->getAttribLocation(name) : -1; }

        typedef std::pair<const StateAttribute*,StateAttribute::OverrideValue>  AttributePair;
        typedef std::vector<AttributePair>                                      AttributeVec;

        AttributeVec& getAttributeVec( const osg::StateAttribute* attribute )
        {
                AttributeStack& as = _attributeMap[ attribute->getTypeMemberPair() ];
                return as.attributeVec;
        }

        /** Set the frame stamp for the current frame.*/
        inline void setFrameStamp(FrameStamp* fs) { _frameStamp = fs; }

        /** Get the frame stamp for the current frame.*/
        inline FrameStamp* getFrameStamp() { return _frameStamp.get(); }

        /** Get the const frame stamp for the current frame.*/
        inline const FrameStamp* getFrameStamp() const { return _frameStamp.get(); }


        /** Set the DisplaySettings. Note, nothing is applied, the visual settings are just
          * used in the State object to pass the current visual settings to Drawables
          * during rendering. */
        inline void setDisplaySettings(DisplaySettings* vs) { _displaySettings = vs; }

        /** Get the const DisplaySettings */
        inline DisplaySettings* getDisplaySettings() { return _displaySettings.get(); }

        /** Get the const DisplaySettings */
        inline const DisplaySettings* getDisplaySettings() const { return _displaySettings.get(); }

        /** Get the const DisplaySettings that is current active DisplaySettings to be used by osg::State, - if DisplaySettings is not directly assigned then fallback to DisplaySettings::instance(). */
        inline DisplaySettings* getActiveDisplaySettings() { return _displaySettings.valid() ? _displaySettings.get() : osg::DisplaySettings::instance().get(); }

        /** Get the const DisplaySettings that is current active DisplaySettings to be used by osg::State, - if DisplaySettings is not directly assigned then fallback to DisplaySettings::instance(). */
        inline const DisplaySettings* getActiveDisplaySettings() const { return _displaySettings.valid() ? _displaySettings.get() : osg::DisplaySettings::instance().get(); }


        /** Set the root StateSet this is applied above all StateSet that are pushed and popped during the draw traversal.*/
        void setRootStateSet(osg::StateSet* stateset);

        /** Get the root StateSet.*/
        osg::StateSet* getRootStateSet() { return _rootStateSet.get(); }


        /** Set flag for early termination of the draw traversal.*/
        void setAbortRenderingPtr(bool* abortPtr) { _abortRenderingPtr = abortPtr; }

        /** Get flag for early termination of the draw traversal,
          * if true steps should be taken to complete rendering early.*/
        bool getAbortRendering() const { return _abortRenderingPtr!=0?(*_abortRenderingPtr):false; }


        struct DynamicObjectRenderingCompletedCallback : public osg::Referenced
        {
            virtual void completed(osg::State*) = 0;
        };

        /** Set the callback to be called when the dynamic object count hits 0.*/
        void setDynamicObjectRenderingCompletedCallback(DynamicObjectRenderingCompletedCallback* cb){ _completeDynamicObjectRenderingCallback = cb; }

        /** Get the callback to be called when the dynamic object count hits 0.*/
        DynamicObjectRenderingCompletedCallback* getDynamicObjectRenderingCompletedCallback() { return _completeDynamicObjectRenderingCallback.get(); }

        /** Set the number of dynamic objects that will be rendered in this graphics context this frame.*/
        void setDynamicObjectCount(unsigned int count, bool callCallbackOnZero = false)
        {
            if (_dynamicObjectCount != count)
            {
                _dynamicObjectCount = count;
                if (_dynamicObjectCount==0 && callCallbackOnZero && _completeDynamicObjectRenderingCallback.valid())
                {
                    _completeDynamicObjectRenderingCallback->completed(this);
                }
            }
        }

        /** Get the number of dynamic objects that will be rendered in this graphics context this frame.*/
        unsigned int getDynamicObjectCount() const { return _dynamicObjectCount; }

        /** Decrement the number of dynamic objects left to render this frame, and once the count goes to zero call the
          * DynamicObjectRenderingCompletedCallback to inform of completion.*/
        inline void decrementDynamicObjectCount()
        {
            --_dynamicObjectCount;
            if (_dynamicObjectCount==0 && _completeDynamicObjectRenderingCallback.valid())
            {
                _completeDynamicObjectRenderingCallback->completed(this);
            }
        }

        void setMaxTexturePoolSize(unsigned int size);
        unsigned int getMaxTexturePoolSize() const { return _maxTexturePoolSize; }

        void setMaxBufferObjectPoolSize(unsigned int size);
        unsigned int getMaxBufferObjectPoolSize() const { return _maxBufferObjectPoolSize; }


        enum CheckForGLErrors
        {
            /** NEVER_CHECK_GL_ERRORS hints that OpenGL need not be checked for, this
                is the fastest option since checking for errors does incur a small overhead.*/
            NEVER_CHECK_GL_ERRORS,
            /** ONCE_PER_FRAME means that OpenGL errors will be checked for once per
                frame, the overhead is still small, but at least OpenGL errors that are occurring
                will be caught, the reporting isn't fine grained enough for debugging purposes.*/
            ONCE_PER_FRAME,
            /** ONCE_PER_ATTRIBUTE means that OpenGL errors will be checked for after
                every attribute is applied, allow errors to be directly associated with
                particular operations which makes debugging much easier.*/
            ONCE_PER_ATTRIBUTE
        };

        /** Set whether and how often OpenGL errors should be checked for.*/
        void setCheckForGLErrors(CheckForGLErrors check) { _checkGLErrors = check; }

        /** Get whether and how often OpenGL errors should be checked for.*/
        CheckForGLErrors getCheckForGLErrors() const { return _checkGLErrors; }

        bool checkGLErrors(const char* str1=0, const char* str2=0) const;
        bool checkGLErrors(const std::string& str) const;
        bool checkGLErrors(StateAttribute::GLMode mode) const;
        bool checkGLErrors(const StateAttribute* attribute) const;

        /** print out the internal details of osg::State - useful for debugging.*/
        void print(std::ostream& fout) const;

        /** Initialize extension used by osg::State.*/
        virtual void initializeExtensionProcs();

        /** Get the helper class for dispatching osg::Arrays as OpenGL attribute data.*/
        inline AttributeDispatchers& getAttributeDispatchers() { return _arrayDispatchers; }


        /** Set the helper class that provides applications with estimate on how much different graphics operations will cost.*/
        inline void setGraphicsCostEstimator(GraphicsCostEstimator* gce) { _graphicsCostEstimator = gce; }

        /** Get the helper class that provides applications with estimate on how much different graphics operations will cost.*/
        inline GraphicsCostEstimator* getGraphicsCostEstimator() { return _graphicsCostEstimator.get(); }

        /** Get the cont helper class that provides applications with estimate on how much different graphics operations will cost.*/
        inline const GraphicsCostEstimator* getGraphicsCostEstimator() const { return _graphicsCostEstimator.get(); }



        /** Support for synchronizing the system time and the timestamp
         * counter available with ARB_timer_query. Note that State
         * doesn't update these values itself.
         */
        Timer_t getStartTick() const { return _startTick; }
        void setStartTick(Timer_t tick) { _startTick = tick; }
        Timer_t getGpuTick() const { return _gpuTick; }

        double getGpuTime() const
        {
            return osg::Timer::instance()->delta_s(_startTick, _gpuTick);
        }
        GLuint64 getGpuTimestamp() const { return _gpuTimestamp; }

        void setGpuTimestamp(Timer_t tick, GLuint64 timestamp)
        {
            _gpuTick = tick;
            _gpuTimestamp = timestamp;
        }
        int getTimestampBits() const { return _timestampBits; }
        void setTimestampBits(int bits) { _timestampBits = bits; }

        /** called by the GraphicsContext just before GraphicsContext::swapBuffersImplementation().*/
        virtual void frameCompleted();


        struct ModeStack
        {
            typedef std::vector<StateAttribute::GLModeValue> ValueVec;

            ModeStack()
            {
                valid = true;
                changed = false;
                last_applied_value = false;
                global_default_value = false;
            }

            void print(std::ostream& fout) const;

            bool        valid;
            bool        changed;
            bool        last_applied_value;
            bool        global_default_value;
            ValueVec    valueVec;
        };

        struct AttributeStack
        {
            AttributeStack()
            {
                changed = false;
                last_applied_attribute = 0L;
                last_applied_shadercomponent = 0L;
                global_default_attribute = 0L;

            }

            void print(std::ostream& fout) const;

            /** apply an attribute if required, passing in attribute and appropriate attribute stack */
            bool                    changed;
            const StateAttribute*   last_applied_attribute;
            const ShaderComponent*  last_applied_shadercomponent;
            ref_ptr<const StateAttribute> global_default_attribute;
            AttributeVec            attributeVec;
        };


        struct UniformStack
        {
            typedef std::pair<const UniformBase*, StateAttribute::OverrideValue>    UniformPair;
            typedef std::vector<UniformPair>                                        UniformVec;

            UniformStack() {}

            void print(std::ostream& fout) const;

            UniformVec              uniformVec;
        };

        struct DefineStack
        {
            typedef std::vector<StateSet::DefinePair> DefineVec;

            DefineStack():
                changed(false) {}

            void print(std::ostream& fout) const;

            bool                   changed;
            DefineVec              defineVec;
        };

        struct DefineMap
        {
            DefineMap():
                changed(false) {}

            typedef std::map<std::string, DefineStack> DefineStackMap;
            DefineStackMap map;
            bool changed;
            StateSet::DefineList currentDefines;

            bool updateCurrentDefines();

        };

        typedef std::map<std::string, GLenum>                           StringModeMap;
        typedef std::map<GLenum, std::string>                           ModeDefineMap;
        typedef std::vector<ModeDefineMap>                              TextureModeDefineMapList;

        inline StringModeMap& getStringModeMap() { return _stringModeMap; }
        inline TextureModeDefineMapList& getTextureModeDefineMapList() { return _textureModeDefineMapList; }
        inline ModeDefineMap& getTextureModeDefineMap(unsigned int i) { return _textureModeDefineMapList[i]; }

        typedef std::map<StateAttribute::GLMode,ModeStack>              ModeMap;
        typedef std::vector<ModeMap>                                    TextureModeMapList;

        typedef std::map<StateAttribute::TypeMemberPair,AttributeStack> AttributeMap;
        typedef std::vector<AttributeMap>                               TextureAttributeMapList;

        typedef std::map<std::string, UniformStack>                     UniformMap;

        typedef std::vector< ref_ptr<const Matrix> >                    MatrixStack;

        inline const ModeMap&                                           getModeMap() const {return _modeMap;}
        inline const AttributeMap&                                      getAttributeMap() const {return _attributeMap;}
        inline const UniformMap&                                        getUniformMap() const {return _uniformMap;}
        inline DefineMap&                                               getDefineMap() {return _defineMap;}
        inline const DefineMap&                                         getDefineMap() const {return _defineMap;}
        inline const TextureModeMapList&                                getTextureModeMapList() const {return _textureModeMapList;}
        inline const TextureAttributeMapList&                           getTextureAttributeMapList() const {return _textureAttributeMapList;}
        inline StateSet::DefineList&                                    getCurrentShaderCompositionDefines() { return _currentShaderCompositionDefines; }
        inline const StateSet::DefineList&                              getCurrentShaderCompositionDefines() const { return _currentShaderCompositionDefines; }


        void getDefineString(std::string& shaderDefineStr, const StateSet::DefineList& currentDefines, const osg::ShaderDefines& shaderDefines);
        void getDefineString(std::string& shaderDefineStr, const osg::ShaderPragmas& shaderPragmas);
        bool supportsShaderRequirements(const osg::ShaderPragmas& shaderPragmas);
        bool supportsShaderRequirement(const std::string& shaderRequirement);

    protected:

        virtual ~State();

        GraphicsContext*            _graphicsContext;
        unsigned int                _contextID;

        bool                        _useStateAttributeShaders;
        bool                        _useStateAttributeFixedFunction;
        bool                        _useModelViewAndProjectionUniforms;
        bool                        _useVertexAttributeAliasing;

        osg::ref_ptr<osg::StateSet>     _rootStateSet;

        osg::ref_ptr<VertexArrayState>  _globalVertexArrayState;
        VertexArrayState*               _vas;

        bool                            _shaderCompositionEnabled;
        bool                            _shaderCompositionDirty;
        osg::ref_ptr<ShaderComposer>    _shaderComposer;
        osg::Program*                   _currentShaderCompositionProgram;
        StateSet::UniformList           _currentShaderCompositionUniformList;
        StateSet::DefineList            _currentShaderCompositionDefines;

        ref_ptr<FrameStamp>         _frameStamp;

        GLenum                      _drawBuffer;
        GLenum                      _readBuffer;

        ref_ptr<const RefMatrix>    _identity;
        ref_ptr<const RefMatrix>    _initialViewMatrix;
        ref_ptr<const RefMatrix>    _projection;
        ref_ptr<const RefMatrix>    _modelView;
        ref_ptr<RefMatrix>          _modelViewCache;


        ref_ptr<Uniform>            _modelViewMatrixUniform;
        ref_ptr<Uniform>            _projectionMatrixUniform;
        ref_ptr<Uniform>            _modelViewProjectionMatrixUniform;
        ref_ptr<Uniform>            _normalMatrixUniform;

        Matrix                      _initialInverseViewMatrix;

        ref_ptr<DisplaySettings>    _displaySettings;

        bool*                       _abortRenderingPtr;
        CheckForGLErrors            _checkGLErrors;


        VertexAttribAlias           _vertexAlias;
        VertexAttribAlias           _normalAlias;
        VertexAttribAlias           _colorAlias;
        VertexAttribAlias           _secondaryColorAlias;
        VertexAttribAlias           _fogCoordAlias;
        VertexAttribAliasList       _texCoordAliasList;

        Program::AttribBindingList  _attributeBindingList;

        void setUpVertexAttribAlias(VertexAttribAlias& alias, GLuint location, const std::string glName, const std::string osgName, const std::string& declaration);

        /** Apply an OpenGL mode if required, passing in mode, enable flag and
          * appropriate mode stack. This is a wrapper around \c glEnable() and
          * \c glDisable(), that just actually calls these functions if the
          * \c enabled flag is different than the current state.
          * @return \c true if the state was actually changed. \c false
          *         otherwise. Notice that a \c false return does not indicate
          *         an error, it just means that the mode was already set to the
          *         same value as the \c enabled parameter.
        */
        inline bool applyMode(StateAttribute::GLMode mode,bool enabled,ModeStack& ms)
        {
            if (ms.valid && ms.last_applied_value != enabled)
            {
                ms.last_applied_value = enabled;

                if (enabled) glEnable(mode);
                else glDisable(mode);

                if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(mode);

                return true;
            }
            else
                return false;
        }

        inline bool applyModeOnTexUnit(unsigned int unit,StateAttribute::GLMode mode,bool enabled,ModeStack& ms)
        {
            if (ms.valid && ms.last_applied_value != enabled)
            {
                if (setActiveTextureUnit(unit))
                {
                    ms.last_applied_value = enabled;

                    if (enabled) glEnable(mode);
                    else glDisable(mode);

                    if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(mode);

                    return true;
                }
                else
                    return false;
            }
            else
                return false;
        }

        /** apply an attribute if required, passing in attribute and appropriate attribute stack */
        inline bool applyAttribute(const StateAttribute* attribute,AttributeStack& as)
        {
            if (as.last_applied_attribute != attribute)
            {
                if (!as.global_default_attribute.valid()) as.global_default_attribute = attribute->cloneType()->asStateAttribute();

                as.last_applied_attribute = attribute;
                attribute->apply(*this);

                const ShaderComponent* sc = attribute->getShaderComponent();
                if (as.last_applied_shadercomponent != sc)
                {
                    as.last_applied_shadercomponent = sc;
                    _shaderCompositionDirty = true;
                }

                if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(attribute);

                return true;
            }
            else
                return false;
        }

        inline bool applyAttributeOnTexUnit(unsigned int unit,const StateAttribute* attribute,AttributeStack& as)
        {
            if (as.last_applied_attribute != attribute)
            {
                if (setActiveTextureUnit(unit))
                {
                    if (!as.global_default_attribute.valid()) as.global_default_attribute = attribute->cloneType()->asStateAttribute();

                    as.last_applied_attribute = attribute;
                    attribute->apply(*this);

                    const ShaderComponent* sc = attribute->getShaderComponent();
                    if (as.last_applied_shadercomponent != sc)
                    {
                        as.last_applied_shadercomponent = sc;
                        _shaderCompositionDirty = true;
                    }

                    if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(attribute);

                    return true;
                }
                else
                    return false;
            }
            else
                return false;
        }


        inline bool applyGlobalDefaultAttribute(AttributeStack& as)
        {
            if (as.last_applied_attribute != as.global_default_attribute.get())
            {
                as.last_applied_attribute = as.global_default_attribute.get();
                if (as.global_default_attribute.valid())
                {
                    as.global_default_attribute->apply(*this);
                    const ShaderComponent* sc = as.global_default_attribute->getShaderComponent();
                    if (as.last_applied_shadercomponent != sc)
                    {
                        as.last_applied_shadercomponent = sc;
                        _shaderCompositionDirty = true;
                    }

                    if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(as.global_default_attribute.get());
                }
                return true;
            }
            else
                return false;
        }

        inline bool applyGlobalDefaultAttributeOnTexUnit(unsigned int unit,AttributeStack& as)
        {
            if (as.last_applied_attribute != as.global_default_attribute.get())
            {
                if (setActiveTextureUnit(unit))
                {
                    as.last_applied_attribute = as.global_default_attribute.get();
                    if (as.global_default_attribute.valid())
                    {
                        as.global_default_attribute->apply(*this);
                        const ShaderComponent* sc = as.global_default_attribute->getShaderComponent();
                        if (as.last_applied_shadercomponent != sc)
                        {
                            as.last_applied_shadercomponent = sc;
                            _shaderCompositionDirty = true;
                        }
                        if (_checkGLErrors==ONCE_PER_ATTRIBUTE) checkGLErrors(as.global_default_attribute.get());
                    }
                    return true;
                }
                else
                    return false;
            }
            else
                return false;
        }

        /** Initialize ModeDefineMaps used in fixed function modes to shader defines.  Called by initializeExtensionProcs().*/
        virtual void initUpModeDefineMaps();

        StringModeMap                                                   _stringModeMap;
        TextureModeDefineMapList                                        _textureModeDefineMapList;

        ModeMap                                                         _modeMap;
        AttributeMap                                                    _attributeMap;
        UniformMap                                                      _uniformMap;
        DefineMap                                                       _defineMap;

        TextureModeMapList                                              _textureModeMapList;
        TextureAttributeMapList                                         _textureAttributeMapList;

        const Program::PerContextProgram*                               _lastAppliedProgramObject;

        StateSetStack                                                   _stateStateStack;

        unsigned int                                                    _maxTexturePoolSize;
        unsigned int                                                    _maxBufferObjectPoolSize;


        unsigned int                                                    _currentActiveTextureUnit;
        unsigned int                                                    _currentClientActiveTextureUnit;
        GLBufferObject*                                                 _currentPBO;
        GLBufferObject*                                                 _currentDIBO;
        GLuint                                                          _currentVAO;
        osg::ref_ptr<osg::IntArrayUniform>                              _textureFormat;


        inline ModeMap& getOrCreateTextureModeMap(unsigned int unit)
        {
            if (unit>=_textureModeMapList.size()) _textureModeMapList.resize(unit+1);
            return _textureModeMapList[unit];
        }


        inline AttributeMap& getOrCreateTextureAttributeMap(unsigned int unit)
        {
            if (unit>=_textureAttributeMapList.size()) _textureAttributeMapList.resize(unit+1);
            return _textureAttributeMapList[unit];
        }

        inline void pushModeList(ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void pushAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);
        inline void pushUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList);
        inline void pushDefineList(DefineMap& defineMap,const StateSet::DefineList& defineList);

        inline void popModeList(ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void popAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);
        inline void popUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList);
        inline void popDefineList(DefineMap& uniformMap,const StateSet::DefineList& defineList);

        inline void applyModeList(ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void applyAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);
        inline void applyUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList);
        inline void applyDefineList(DefineMap& uniformMap,const StateSet::DefineList& defineList);

        inline void applyModeMap(ModeMap& modeMap);
        inline void applyAttributeMap(AttributeMap& attributeMap);
        inline void applyUniformMap(UniformMap& uniformMap);

        inline void applyModeListOnTexUnit(unsigned int unit,ModeMap& modeMap,const StateSet::ModeList& modeList);
        inline void applyAttributeListOnTexUnit(unsigned int unit,AttributeMap& attributeMap,const StateSet::AttributeList& attributeList);

        inline void applyModeMapOnTexUnit(unsigned int unit,ModeMap& modeMap);
        inline void applyAttributeMapOnTexUnit(unsigned int unit,AttributeMap& attributeMap);

        void haveAppliedMode(ModeMap& modeMap,StateAttribute::GLMode mode,StateAttribute::GLModeValue value);
        void haveAppliedMode(ModeMap& modeMap,StateAttribute::GLMode mode);
        void haveAppliedAttribute(AttributeMap& attributeMap,const StateAttribute* attribute);
        void haveAppliedAttribute(AttributeMap& attributeMap,StateAttribute::Type type, unsigned int member);
        bool getLastAppliedMode(const ModeMap& modeMap,StateAttribute::GLMode mode) const;
        const StateAttribute* getLastAppliedAttribute(const AttributeMap& attributeMap,StateAttribute::Type type, unsigned int member) const;

        void loadModelViewMatrix();


        bool _isSecondaryColorSupported;
        bool _isFogCoordSupported;
        bool _isVertexBufferObjectSupported;
        bool _isVertexArrayObjectSupported;
        bool _forceVertexBufferObject;
        bool _forceVertexArrayObject;

        typedef void (GL_APIENTRY * ActiveTextureProc) (GLenum texture);
        typedef void (GL_APIENTRY * FogCoordPointerProc) (GLenum type, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * SecondaryColorPointerProc) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * MultiTexCoord4fProc) (GLenum target, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
        typedef void (GL_APIENTRY * VertexAttrib4fProc)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
        typedef void (GL_APIENTRY * VertexAttrib4fvProc)(GLuint index, const GLfloat *v);
        typedef void (GL_APIENTRY * VertexAttribPointerProc) (unsigned int, GLint, GLenum, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * VertexAttribIPointerProc) (unsigned int, GLint, GLenum, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * VertexAttribLPointerProc) (unsigned int, GLint, GLenum, GLsizei stride, const GLvoid *pointer);
        typedef void (GL_APIENTRY * EnableVertexAttribProc) (unsigned int);
        typedef void (GL_APIENTRY * DisableVertexAttribProc) (unsigned int);
        typedef void (GL_APIENTRY * BindBufferProc) (GLenum target, GLuint buffer);

        typedef void (GL_APIENTRY * DrawArraysInstancedProc)( GLenum mode, GLint first, GLsizei count, GLsizei primcount );
        typedef void (GL_APIENTRY * DrawElementsInstancedProc)( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount );

        bool                        _extensionProcsInitialized;
        GLint                       _glMaxTextureCoords;
        GLint                       _glMaxTextureUnits;
        ActiveTextureProc           _glClientActiveTexture;
        ActiveTextureProc           _glActiveTexture;
        MultiTexCoord4fProc         _glMultiTexCoord4f;
        VertexAttrib4fProc          _glVertexAttrib4f;
        VertexAttrib4fvProc         _glVertexAttrib4fv;
        FogCoordPointerProc         _glFogCoordPointer;
        SecondaryColorPointerProc   _glSecondaryColorPointer;
        VertexAttribPointerProc     _glVertexAttribPointer;
        VertexAttribIPointerProc    _glVertexAttribIPointer;
        VertexAttribLPointerProc    _glVertexAttribLPointer;
        EnableVertexAttribProc      _glEnableVertexAttribArray;
        DisableVertexAttribProc     _glDisableVertexAttribArray;
        BindBufferProc              _glBindBuffer;
        DrawArraysInstancedProc     _glDrawArraysInstanced;
        DrawElementsInstancedProc   _glDrawElementsInstanced;

        osg::ref_ptr<GLExtensions>  _glExtensions;

        unsigned int                                            _dynamicObjectCount;
        osg::ref_ptr<DynamicObjectRenderingCompletedCallback>   _completeDynamicObjectRenderingCallback;

        AttributeDispatchers            _arrayDispatchers;

        osg::ref_ptr<GraphicsCostEstimator> _graphicsCostEstimator;

        Timer_t                      _startTick;
        Timer_t                      _gpuTick;
        GLuint64                     _gpuTimestamp;
        int                          _timestampBits;
};

inline void State::pushModeList(ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    for(StateSet::ModeList::const_iterator mitr=modeList.begin();
        mitr!=modeList.end();
        ++mitr)
    {
        // get the mode stack for incoming GLmode {mitr->first}.
        ModeStack& ms = modeMap[mitr->first];
        if (ms.valueVec.empty())
        {
            // first pair so simply push incoming pair to back.
            ms.valueVec.push_back(mitr->second);
        }
        else if ((ms.valueVec.back() & StateAttribute::OVERRIDE) && !(mitr->second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            ms.valueVec.push_back(ms.valueVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            ms.valueVec.push_back(mitr->second);
        }
        ms.changed = true;
    }
}

inline void State::pushAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    for(StateSet::AttributeList::const_iterator aitr=attributeList.begin();
        aitr!=attributeList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        AttributeStack& as = attributeMap[aitr->first];
        if (as.attributeVec.empty())
        {
            // first pair so simply push incoming pair to back.
            as.attributeVec.push_back(
                AttributePair(aitr->second.first.get(),aitr->second.second));
        }
        else if ((as.attributeVec.back().second & StateAttribute::OVERRIDE) && !(aitr->second.second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            as.attributeVec.push_back(as.attributeVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            as.attributeVec.push_back(
                AttributePair(aitr->second.first.get(),aitr->second.second));
        }
        as.changed = true;
    }
}


inline void State::pushUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList)
{
    for(StateSet::UniformList::const_iterator aitr=uniformList.begin();
        aitr!=uniformList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        UniformStack& us = uniformMap[aitr->first];
        if (us.uniformVec.empty())
        {
            // first pair so simply push incoming pair to back.
            us.uniformVec.push_back(
                UniformStack::UniformPair(aitr->second.first.get(),aitr->second.second));
        }
        else if ((us.uniformVec.back().second & StateAttribute::OVERRIDE) && !(aitr->second.second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            us.uniformVec.push_back(us.uniformVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            us.uniformVec.push_back(
                UniformStack::UniformPair(aitr->second.first.get(),aitr->second.second));
        }
    }
}

inline void State::pushDefineList(DefineMap& defineMap,const StateSet::DefineList& defineList)
{
    for(StateSet::DefineList::const_iterator aitr=defineList.begin();
        aitr!=defineList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        DefineStack& ds = defineMap.map[aitr->first];
        DefineStack::DefineVec& dv = ds.defineVec;
        if (dv.empty())
        {
            // first pair so simply push incoming pair to back.
            dv.push_back(StateSet::DefinePair(aitr->second.first,aitr->second.second));

            ds.changed = true;
            defineMap.changed = true;
        }
        else if ((ds.defineVec.back().second & StateAttribute::OVERRIDE) && !(aitr->second.second & StateAttribute::PROTECTED)) // check the existing override flag
        {
            // push existing back since override keeps the previous value.
            ds.defineVec.push_back(ds.defineVec.back());
        }
        else
        {
            // no override on so simply push incoming pair to back.
            dv.push_back(StateSet::DefinePair(aitr->second.first,aitr->second.second));

            // if the back of the stack has changed since the last then mark it as changed.
            bool changed = (dv[dv.size()-2] != dv.back());
            if (changed)
            {
                ds.changed = true;
                defineMap.changed = true;
            }
        }
    }
}

inline void State::popModeList(ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    for(StateSet::ModeList::const_iterator mitr=modeList.begin();
        mitr!=modeList.end();
        ++mitr)
    {
        // get the mode stack for incoming GLmode {mitr->first}.
        ModeStack& ms = modeMap[mitr->first];
        if (!ms.valueVec.empty())
        {
            ms.valueVec.pop_back();
        }
        ms.changed = true;
    }
}

inline void State::popAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    for(StateSet::AttributeList::const_iterator aitr=attributeList.begin();
        aitr!=attributeList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        AttributeStack& as = attributeMap[aitr->first];
        if (!as.attributeVec.empty())
        {
            as.attributeVec.pop_back();
        }
        as.changed = true;
    }
}

inline void State::popUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList)
{
    for(StateSet::UniformList::const_iterator aitr=uniformList.begin();
        aitr!=uniformList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        UniformStack& us = uniformMap[aitr->first];
        if (!us.uniformVec.empty())
        {
            us.uniformVec.pop_back();
        }
    }
}

inline void State::popDefineList(DefineMap& defineMap,const StateSet::DefineList& defineList)
{
    for(StateSet::DefineList::const_iterator aitr=defineList.begin();
        aitr!=defineList.end();
        ++aitr)
    {
        // get the attribute stack for incoming type {aitr->first}.
        DefineStack& ds = defineMap.map[aitr->first];
        DefineStack::DefineVec& dv = ds.defineVec;
        if (!dv.empty())
        {
            // if the stack has less than 2 entries or new back vs old back are different then mark the DefineStack as changed
            if ((dv.size() < 2) || (dv[dv.size()-2] != dv.back()))
            {
                ds.changed = true;
                defineMap.changed = true;
            }
            dv.pop_back();
        }
    }
}

inline void State::applyModeList(ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    StateSet::ModeList::const_iterator ds_mitr = modeList.begin();
    ModeMap::iterator this_mitr=modeMap.begin();

    while (this_mitr!=modeMap.end() && ds_mitr!=modeList.end())
    {
        if (this_mitr->first<ds_mitr->first)
        {

            // note GLMode = this_mitr->first
            ModeStack& ms = this_mitr->second;
            if (ms.changed)
            {
                ms.changed = false;
                if (!ms.valueVec.empty())
                {
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyMode(this_mitr->first,new_value,ms);
                }
                else
                {
                    // assume default of disabled.
                    applyMode(this_mitr->first,ms.global_default_value,ms);

                }

            }

            ++this_mitr;

        }
        else if (ds_mitr->first<this_mitr->first)
        {

            // ds_mitr->first is a new mode, therefore
            // need to insert a new mode entry for ds_mistr->first.
            ModeStack& ms = modeMap[ds_mitr->first];

            bool new_value = ds_mitr->second & StateAttribute::ON;
            applyMode(ds_mitr->first,new_value,ms);

            // will need to disable this mode on next apply so set it to changed.
            ms.changed = true;

            ++ds_mitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same mode, check the override
            // if any otherwise just apply the incoming mode.

            ModeStack& ms = this_mitr->second;

            if (!ms.valueVec.empty() && (ms.valueVec.back() & StateAttribute::OVERRIDE) && !(ds_mitr->second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on modes.

                if (ms.changed)
                {
                    ms.changed = false;
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyMode(this_mitr->first,new_value,ms);

                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming mode.
                bool new_value = ds_mitr->second & StateAttribute::ON;
                if (applyMode(ds_mitr->first,new_value,ms))
                {
                    ms.changed = true;
                }
            }

            ++this_mitr;
            ++ds_mitr;
        }
    }

    // iterator over the remaining state modes to apply any previous changes.
    for(;
        this_mitr!=modeMap.end();
        ++this_mitr)
    {
        // note GLMode = this_mitr->first
        ModeStack& ms = this_mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyMode(this_mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyMode(this_mitr->first,ms.global_default_value,ms);

            }

        }
    }

    // iterator over the remaining incoming modes to apply any new mode.
    for(;
        ds_mitr!=modeList.end();
        ++ds_mitr)
    {
        ModeStack& ms = modeMap[ds_mitr->first];

        bool new_value = ds_mitr->second & StateAttribute::ON;
        applyMode(ds_mitr->first,new_value,ms);

        // will need to disable this mode on next apply so set it to changed.
        ms.changed = true;
    }
}

inline void State::applyModeListOnTexUnit(unsigned int unit,ModeMap& modeMap,const StateSet::ModeList& modeList)
{
    StateSet::ModeList::const_iterator ds_mitr = modeList.begin();
    ModeMap::iterator this_mitr=modeMap.begin();

    while (this_mitr!=modeMap.end() && ds_mitr!=modeList.end())
    {
        if (this_mitr->first<ds_mitr->first)
        {

            // note GLMode = this_mitr->first
            ModeStack& ms = this_mitr->second;
            if (ms.changed)
            {
                ms.changed = false;
                if (!ms.valueVec.empty())
                {
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyModeOnTexUnit(unit,this_mitr->first,new_value,ms);
                }
                else
                {
                    // assume default of disabled.
                    applyModeOnTexUnit(unit,this_mitr->first,ms.global_default_value,ms);

                }

            }

            ++this_mitr;

        }
        else if (ds_mitr->first<this_mitr->first)
        {

            // ds_mitr->first is a new mode, therefore
            // need to insert a new mode entry for ds_mistr->first.
            ModeStack& ms = modeMap[ds_mitr->first];

            bool new_value = ds_mitr->second & StateAttribute::ON;
            applyModeOnTexUnit(unit,ds_mitr->first,new_value,ms);

            // will need to disable this mode on next apply so set it to changed.
            ms.changed = true;

            ++ds_mitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same mode, check the override
            // if any otherwise just apply the incoming mode.

            ModeStack& ms = this_mitr->second;

            if (!ms.valueVec.empty() && (ms.valueVec.back() & StateAttribute::OVERRIDE) && !(ds_mitr->second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on modes.

                if (ms.changed)
                {
                    ms.changed = false;
                    bool new_value = ms.valueVec.back() & StateAttribute::ON;
                    applyModeOnTexUnit(unit,this_mitr->first,new_value,ms);

                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming mode.
                bool new_value = ds_mitr->second & StateAttribute::ON;
                if (applyModeOnTexUnit(unit,ds_mitr->first,new_value,ms))
                {
                    ms.changed = true;
                }
            }

            ++this_mitr;
            ++ds_mitr;
        }
    }

    // iterator over the remaining state modes to apply any previous changes.
    for(;
        this_mitr!=modeMap.end();
        ++this_mitr)
    {
        // note GLMode = this_mitr->first
        ModeStack& ms = this_mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyModeOnTexUnit(unit,this_mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyModeOnTexUnit(unit,this_mitr->first,ms.global_default_value,ms);

            }

        }
    }

    // iterator over the remaining incoming modes to apply any new mode.
    for(;
        ds_mitr!=modeList.end();
        ++ds_mitr)
    {
        ModeStack& ms = modeMap[ds_mitr->first];

        bool new_value = ds_mitr->second & StateAttribute::ON;
        applyModeOnTexUnit(unit,ds_mitr->first,new_value,ms);

        // will need to disable this mode on next apply so set it to changed.
        ms.changed = true;
    }
}

inline void State::applyAttributeList(AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    StateSet::AttributeList::const_iterator ds_aitr=attributeList.begin();

    AttributeMap::iterator this_aitr=attributeMap.begin();

    while (this_aitr!=attributeMap.end() && ds_aitr!=attributeList.end())
    {
        if (this_aitr->first<ds_aitr->first)
        {

            // note attribute type = this_aitr->first
            AttributeStack& as = this_aitr->second;
            if (as.changed)
            {
                as.changed = false;
                if (!as.attributeVec.empty())
                {
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttribute(new_attr,as);
                }
                else
                {
                    applyGlobalDefaultAttribute(as);
                }
            }

            ++this_aitr;

        }
        else if (ds_aitr->first<this_aitr->first)
        {

            // ds_aitr->first is a new attribute, therefore
            // need to insert a new attribute entry for ds_aitr->first.
            AttributeStack& as = attributeMap[ds_aitr->first];

            const StateAttribute* new_attr = ds_aitr->second.first.get();
            applyAttribute(new_attr,as);

            as.changed = true;

            ++ds_aitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same attribute, check the override
            // if any otherwise just apply the incoming attribute

            AttributeStack& as = this_aitr->second;

            if (!as.attributeVec.empty() && (as.attributeVec.back().second & StateAttribute::OVERRIDE) && !(ds_aitr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on attribute.

                if (as.changed)
                {
                    as.changed = false;
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttribute(new_attr,as);
                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming attribute.
                const StateAttribute* new_attr = ds_aitr->second.first.get();
                if (applyAttribute(new_attr,as))
                {
                    as.changed = true;
                }
            }

            ++this_aitr;
            ++ds_aitr;
        }
    }

    // iterator over the remaining state attributes to apply any previous changes.
    for(;
        this_aitr!=attributeMap.end();
        ++this_aitr)
    {
        // note attribute type = this_aitr->first
        AttributeStack& as = this_aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttribute(new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttribute(as);
            }
        }
    }

    // iterator over the remaining incoming attribute to apply any new attribute.
    for(;
        ds_aitr!=attributeList.end();
        ++ds_aitr)
    {
        // ds_aitr->first is a new attribute, therefore
        // need to insert a new attribute entry for ds_aitr->first.
        AttributeStack& as = attributeMap[ds_aitr->first];

        const StateAttribute* new_attr = ds_aitr->second.first.get();
        applyAttribute(new_attr,as);

        // will need to update this attribute on next apply so set it to changed.
        as.changed = true;
    }

}

inline void State::applyAttributeListOnTexUnit(unsigned int unit,AttributeMap& attributeMap,const StateSet::AttributeList& attributeList)
{
    StateSet::AttributeList::const_iterator ds_aitr=attributeList.begin();

    AttributeMap::iterator this_aitr=attributeMap.begin();

    while (this_aitr!=attributeMap.end() && ds_aitr!=attributeList.end())
    {
        if (this_aitr->first<ds_aitr->first)
        {

            // note attribute type = this_aitr->first
            AttributeStack& as = this_aitr->second;
            if (as.changed)
            {
                as.changed = false;
                if (!as.attributeVec.empty())
                {
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttributeOnTexUnit(unit,new_attr,as);
                }
                else
                {
                    applyGlobalDefaultAttributeOnTexUnit(unit,as);
                }
            }

            ++this_aitr;

        }
        else if (ds_aitr->first<this_aitr->first)
        {

            // ds_aitr->first is a new attribute, therefore
            // need to insert a new attribute entry for ds_aitr->first.
            AttributeStack& as = attributeMap[ds_aitr->first];

            const StateAttribute* new_attr = ds_aitr->second.first.get();
            applyAttributeOnTexUnit(unit,new_attr,as);

            as.changed = true;

            ++ds_aitr;

        }
        else
        {
            // this_mitr & ds_mitr refer to the same attribute, check the override
            // if any otherwise just apply the incoming attribute

            AttributeStack& as = this_aitr->second;

            if (!as.attributeVec.empty() && (as.attributeVec.back().second & StateAttribute::OVERRIDE) && !(ds_aitr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on attribute.

                if (as.changed)
                {
                    as.changed = false;
                    const StateAttribute* new_attr = as.attributeVec.back().first;
                    applyAttributeOnTexUnit(unit,new_attr,as);
                }
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming attribute.
                const StateAttribute* new_attr = ds_aitr->second.first.get();
                if (applyAttributeOnTexUnit(unit,new_attr,as))
                {
                    as.changed = true;
                }
            }

            ++this_aitr;
            ++ds_aitr;
        }
    }

    // iterator over the remaining state attributes to apply any previous changes.
    for(;
        this_aitr!=attributeMap.end();
        ++this_aitr)
    {
        // note attribute type = this_aitr->first
        AttributeStack& as = this_aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttributeOnTexUnit(unit,new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttributeOnTexUnit(unit,as);
            }
        }
    }

    // iterator over the remaining incoming attribute to apply any new attribute.
    for(;
        ds_aitr!=attributeList.end();
        ++ds_aitr)
    {
        // ds_aitr->first is a new attribute, therefore
        // need to insert a new attribute entry for ds_aitr->first.
        AttributeStack& as = attributeMap[ds_aitr->first];

        const StateAttribute* new_attr = ds_aitr->second.first.get();
        applyAttributeOnTexUnit(unit,new_attr,as);

        // will need to update this attribute on next apply so set it to changed.
        as.changed = true;
    }

}

inline void State::applyUniformList(UniformMap& uniformMap,const StateSet::UniformList& uniformList)
{
    if (!_lastAppliedProgramObject) return;

    StateSet::UniformList::const_iterator ds_aitr=uniformList.begin();

    UniformMap::iterator this_aitr=uniformMap.begin();

    while (this_aitr!=uniformMap.end() && ds_aitr!=uniformList.end())
    {
        if (this_aitr->first<ds_aitr->first)
        {
            // note attribute type = this_aitr->first
            UniformStack& as = this_aitr->second;
            if (!as.uniformVec.empty())
            {
                _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
            }

            ++this_aitr;

        }
        else if (ds_aitr->first<this_aitr->first)
        {
            _lastAppliedProgramObject->apply(*(ds_aitr->second.first.get()));

            ++ds_aitr;
        }
        else
        {
            // this_mitr & ds_mitr refer to the same attribute, check the override
            // if any otherwise just apply the incoming attribute

            UniformStack& as = this_aitr->second;

            if (!as.uniformVec.empty() && (as.uniformVec.back().second & StateAttribute::OVERRIDE) && !(ds_aitr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on uniform.
                _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming attribute.
                _lastAppliedProgramObject->apply(*(ds_aitr->second.first.get()));
            }

            ++this_aitr;
            ++ds_aitr;
        }
    }

    // iterator over the remaining state attributes to apply any previous changes.
    for(;
        this_aitr!=uniformMap.end();
        ++this_aitr)
    {
        // note attribute type = this_aitr->first
        UniformStack& as = this_aitr->second;
        if (!as.uniformVec.empty())
        {
            _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
        }
    }

    // iterator over the remaining incoming attribute to apply any new attribute.
    for(;
        ds_aitr!=uniformList.end();
        ++ds_aitr)
    {
        _lastAppliedProgramObject->apply(*(ds_aitr->second.first.get()));
    }

}

inline void State::applyDefineList(DefineMap& defineMap, const StateSet::DefineList& defineList)
{
    StateSet::DefineList::const_iterator dl_itr = defineList.begin();
    DefineMap::DefineStackMap::iterator dm_itr = defineMap.map.begin();

    defineMap.changed = false;
    defineMap.currentDefines.clear();

    while (dm_itr!=defineMap.map.end() && dl_itr!=defineList.end())
    {
        if (dm_itr->first<dl_itr->first)
        {
            DefineStack& ds = dm_itr->second;
            DefineStack::DefineVec& dv = ds.defineVec;
            if (!dv.empty() && (dv.back().second & StateAttribute::ON)!=0) defineMap.currentDefines[dm_itr->first] = dv.back();

            ++dm_itr;
        }
        else if (dl_itr->first<dm_itr->first)
        {
            if ((dl_itr->second.second & StateAttribute::ON)!=0) defineMap.currentDefines[dl_itr->first] = dl_itr->second;

            ++dl_itr;
        }
        else
        {
            // this_mitr & ds_mitr refer to the same mode, check the override
            // if any otherwise just apply the incoming mode.

            DefineStack& ds = dm_itr->second;
            DefineStack::DefineVec& dv = ds.defineVec;

            if (!dv.empty() && (dv.back().second & StateAttribute::OVERRIDE)!=0 && !(dl_itr->second.second & StateAttribute::PROTECTED))
            {
                // override is on, just treat as a normal apply on modes.
                if ((dv.back().second & StateAttribute::ON)!=0) defineMap.currentDefines[dm_itr->first] = dv.back();
            }
            else
            {
                // no override on or no previous entry, therefore consider incoming mode.
                if ((dl_itr->second.second & StateAttribute::ON)!=0) defineMap.currentDefines[dl_itr->first] = dl_itr->second;
            }

            ++dm_itr;
            ++dl_itr;
        }
    }

    // iterator over the remaining state modes to apply any previous changes.
    for(;
        dm_itr!=defineMap.map.end();
        ++dm_itr)
    {
        // note GLMode = this_mitr->first
        DefineStack& ds = dm_itr->second;
        DefineStack::DefineVec& dv = ds.defineVec;
        if (!dv.empty() && (dv.back().second & StateAttribute::ON)!=0) defineMap.currentDefines[dm_itr->first] = dv.back();
    }

    // iterator over the remaining incoming modes to apply any new mode.
    for(;
        dl_itr!=defineList.end();
        ++dl_itr)
    {
        if ((dl_itr->second.second & StateAttribute::ON)!=0) defineMap.currentDefines[dl_itr->first] = dl_itr->second;
    }
}

inline void State::applyModeMap(ModeMap& modeMap)
{
    for(ModeMap::iterator mitr=modeMap.begin();
        mitr!=modeMap.end();
        ++mitr)
    {
        // note GLMode = mitr->first
        ModeStack& ms = mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyMode(mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyMode(mitr->first,ms.global_default_value,ms);
            }

        }
    }
}

inline void State::applyModeMapOnTexUnit(unsigned int unit,ModeMap& modeMap)
{
    for(ModeMap::iterator mitr=modeMap.begin();
        mitr!=modeMap.end();
        ++mitr)
    {
        // note GLMode = mitr->first
        ModeStack& ms = mitr->second;
        if (ms.changed)
        {
            ms.changed = false;
            if (!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back() & StateAttribute::ON;
                applyModeOnTexUnit(unit,mitr->first,new_value,ms);
            }
            else
            {
                // assume default of disabled.
                applyModeOnTexUnit(unit,mitr->first,ms.global_default_value,ms);
            }

        }
    }
}

inline void State::applyAttributeMap(AttributeMap& attributeMap)
{
    for(AttributeMap::iterator aitr=attributeMap.begin();
        aitr!=attributeMap.end();
        ++aitr)
    {
        AttributeStack& as = aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttribute(new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttribute(as);
            }

        }
    }
}

inline void State::applyAttributeMapOnTexUnit(unsigned int unit,AttributeMap& attributeMap)
{
    for(AttributeMap::iterator aitr=attributeMap.begin();
        aitr!=attributeMap.end();
        ++aitr)
    {
        AttributeStack& as = aitr->second;
        if (as.changed)
        {
            as.changed = false;
            if (!as.attributeVec.empty())
            {
                const StateAttribute* new_attr = as.attributeVec.back().first;
                applyAttributeOnTexUnit(unit,new_attr,as);
            }
            else
            {
                applyGlobalDefaultAttributeOnTexUnit(unit,as);
            }

        }
    }
}

inline void State::applyUniformMap(UniformMap& uniformMap)
{
    if (!_lastAppliedProgramObject) return;

    for(UniformMap::iterator aitr=uniformMap.begin();
        aitr!=uniformMap.end();
        ++aitr)
    {
        UniformStack& as = aitr->second;
        if (!as.uniformVec.empty())
        {
            _lastAppliedProgramObject->apply(*as.uniformVec.back().first);
        }
    }
}

inline bool State::setActiveTextureUnit( unsigned int unit )
{
    if (unit!=_currentActiveTextureUnit)
    {
        if (_glActiveTexture && unit < (unsigned int)(maximum(_glMaxTextureCoords,_glMaxTextureUnits)) )
        {
            _glActiveTexture(GL_TEXTURE0+unit);
            _currentActiveTextureUnit = unit;
        }
        else
        {
            return unit==0;
        }
    }
    return true;
}



// forward declare speciailization of State::get() method
template<> inline GLExtensions* State::get<GLExtensions>() { return _glExtensions.get(); }
template<> inline const GLExtensions* State::get<GLExtensions>() const { return _glExtensions.get(); }
template<> inline void State::set<GLExtensions>(GLExtensions* ptr) { _glExtensions = ptr; }

}


// OSGFILE include/osg/GraphicsThread

//#include <osg/OperationThread>
//#include <osg/State>

namespace osg {

class GraphicsContext;

/** GraphicsThread is a helper class for running OpenGL GraphicsOperation within a single thread assigned to a specific GraphicsContext.*/
class OSG_EXPORT GraphicsThread : public osg::OperationThread
{
    public:

        GraphicsThread();

        /** Run does the graphics thread run loop.*/
        virtual void run();
};

struct OSG_EXPORT GraphicsOperation : public Operation
{
    GraphicsOperation(const std::string& name, bool keep):
        Operation(name,keep) {}

    /** Override the standard Operation operator and dynamic cast object to a GraphicsContext,
      * on success call operation()(GraphicsContext*).*/
    virtual void operator () (Object* object);

    virtual void operator () (GraphicsContext* context) = 0;
};


/** SwapBufferOperation calls swap buffers on the GraphicsContext.*/
struct OSG_EXPORT SwapBuffersOperation : public GraphicsOperation
{
    SwapBuffersOperation():
        osg::Referenced(true),
        GraphicsOperation("SwapBuffers",true) {}

    virtual void operator () (GraphicsContext* context);
};

/** BarrierOperation allows one to synchronize multiple GraphicsThreads with each other.*/
struct OSG_EXPORT BarrierOperation : public Operation, public OpenThreads::Barrier
{
    enum PreBlockOp
    {
        NO_OPERATION,
        GL_FLUSH,
        GL_FINISH
    };

    BarrierOperation(int numThreads, PreBlockOp op=NO_OPERATION, bool keep=true):
        osg::Referenced(true),
        Operation("Barrier", keep),
        OpenThreads::Barrier(numThreads),
        _preBlockOp(op) {}

    virtual void release();

    virtual void operator () (Object* object);

    PreBlockOp _preBlockOp;
};

/** ReleaseContext_Block_MakeCurrentOperation releases the context for another thread to acquire,
  * then blocks waiting for context to be released, once the block is release the context is re-acquired.*/
struct OSG_EXPORT ReleaseContext_Block_MakeCurrentOperation : public GraphicsOperation, public RefBlock
{
    ReleaseContext_Block_MakeCurrentOperation():
        osg::Referenced(true),
        GraphicsOperation("ReleaseContext_Block_MakeCurrent", false) {}

    virtual void release();

    virtual void operator () (GraphicsContext* context);
};

struct OSG_EXPORT BlockAndFlushOperation : public GraphicsOperation, public OpenThreads::Block
{
    BlockAndFlushOperation();

    virtual void release();

    virtual void operator () (GraphicsContext*);
};


struct OSG_EXPORT FlushDeletedGLObjectsOperation : public GraphicsOperation
{
    FlushDeletedGLObjectsOperation(double availableTime, bool keep=false);

    virtual void operator () (GraphicsContext*);

    double _availableTime;
};

class OSG_EXPORT RunOperations : public osg::GraphicsOperation
{
public:

    RunOperations():
        osg::GraphicsOperation("RunOperation",true) {}

    virtual void operator () (osg::GraphicsContext* context);

};

class OSG_EXPORT EndOfDynamicDrawBlock : public OpenThreads::BlockCount, public osg::State::DynamicObjectRenderingCompletedCallback
{
    public:

        EndOfDynamicDrawBlock(unsigned int);

        void completed(osg::State* state);

    protected:

        ~EndOfDynamicDrawBlock() {}
};

}


// OSGFILE include/osg/GraphicsContext

/*
#include <osg/State>
#include <osg/GraphicsThread>
#include <osg/Timer>
*/

#include <vector>

namespace osg {

// forward declare osg::Camera
class Camera;

/** Base class for providing Windowing API agnostic access to creating and managing graphics context.*/
class OSG_EXPORT GraphicsContext : public Object
{
    public:

        struct OSG_EXPORT ScreenIdentifier
        {
            ScreenIdentifier();

            ScreenIdentifier(int in_screenNum);

            ScreenIdentifier(const std::string& in_hostName,int in_displayNum, int in_screenNum);

            /** Return the display name in the form hostName::displayNum:screenNum. */
            std::string displayName() const;

            /** Read the DISPLAY environmental variable, and set the ScreenIdentifier accordingly.
              * Note, if either of displayNum or screenNum are not defined then -1 is set respectively to
              * signify that this parameter has not been set. When parameters are undefined one can call
              * call setUndefinedScreenDetailsToDefaultScreen() after readDISPLAY() to ensure valid values. */
            void readDISPLAY();

            /** Set the screenIndentifier from the displayName string.
              * Note, if either of displayNum or screenNum are not defined then -1 is set to
              * signify that this parameter has not been set. When parameters are undefined one can call
              * call setUndefinedScreenDetailsToDefaultScreen() after readDISPLAY() to ensure valid values. */
            void setScreenIdentifier(const std::string& displayName);

            /** Set any undefined displayNum or screenNum values (i.e. -1) to the default display & screen of 0 respectively.*/
            void setUndefinedScreenDetailsToDefaultScreen()
            {
                if (displayNum<0) displayNum = 0;
                if (screenNum<0) screenNum = 0;
            }

            std::string  hostName;
            int displayNum;
            int screenNum;
        };

        /** GraphicsContext Traits object provides the specification of what type of graphics context is required.*/
        struct OSG_EXPORT Traits : public osg::Referenced, public ScreenIdentifier
        {
            Traits(DisplaySettings* ds=0);

            // graphics context original and size
            int x;
            int y;
            int width;
            int height;

            // provide a hint as to which WindowingSystemInterface implementation to use, i.e. "X11", "Win32", "Cocoa", "Carbon" etc.
            // if the windowingSystemPreference string is empty (default) then return the first available WindowingSystemInterface that
            // has been registered with the osg::GraphiccsContext::WindowingSystemInterfaces singleton
            // if the windowingSystemPreference string is not empty then return the first WindowingSystemInterface that matches
            std::string windowingSystemPreference;

            // window decoration and behaviour
            std::string windowName;
            bool        windowDecoration;
            bool        supportsResize;

            // buffer depths, 0 equals off.
            unsigned int red;
            unsigned int blue;
            unsigned int green;
            unsigned int alpha;
            unsigned int depth;
            unsigned int stencil;

            // multi sample parameters
            unsigned int sampleBuffers;
            unsigned int samples;

            // buffer configuration
            bool pbuffer;
            bool quadBufferStereo;
            bool doubleBuffer;

            // render to texture
            GLenum          target;
            GLenum          format;
            unsigned int    level;
            unsigned int    face;
            unsigned int    mipMapGeneration;

            // V-sync
            bool            vsync;

            // Swap Group
            bool            swapGroupEnabled;
            GLuint          swapGroup;
            GLuint          swapBarrier;

            // use multithreaded OpenGL-engine (OS X only)
            bool            useMultiThreadedOpenGLEngine;

            // enable cursor
            bool            useCursor;

            // settings used in set up of graphics context, only presently used by GL3 build of OSG.
            std::string     glContextVersion;
            unsigned int    glContextFlags;
            unsigned int    glContextProfileMask;

            /** return true if glContextVersion is set in the form major.minor, and assign the appropriate major and minor values to the associated parameters.*/
            bool getContextVersion(unsigned int& major, unsigned int& minor) const;

            // shared context
            osg::observer_ptr<GraphicsContext> sharedContext;

            osg::ref_ptr<osg::Referenced> inheritedWindowData;

            // ask the GraphicsWindow implementation to set the pixel format of an inherited window
            bool setInheritedWindowPixelFormat;

            // X11 hint whether to override the window managers window size/position redirection
            bool overrideRedirect;

            DisplaySettings::SwapMethod swapMethod;

            // hint of what affinity to use for any thrads associated with the graphics context created using these Traits
            OpenThreads::Affinity affinity;
        };

        /** Simple resolution structure used by WindowingSystemInterface to get and set screen resolution.
          * Note the '0' value stands for 'unset'. */
        struct ScreenSettings {
            ScreenSettings() :
                width(0),
                height(0),
                refreshRate(0),
                colorDepth(0)
            {}

            ScreenSettings(int in_width, int in_height, double in_refreshRate=0, unsigned int in_colorDepth=0) :
                width(in_width),
                height(in_height),
                refreshRate(in_refreshRate),
                colorDepth(in_colorDepth)
            {}

            int width;
            int height;
            double refreshRate;         ///< Screen refresh rate, in Hz.
            unsigned int colorDepth;    ///< RGB(A) color buffer depth.
        };

        typedef std::vector<ScreenSettings> ScreenSettingsList;

        /** Callback to be implemented to provide access to Windowing API's ability to create Windows/pbuffers.*/
        struct WindowingSystemInterface : public osg::Referenced
        {
            void setName(const std::string& name) { _name = name; }
            const std::string& getName() const { return _name; }

            virtual unsigned int getNumScreens(const ScreenIdentifier& screenIdentifier = ScreenIdentifier()) = 0;

            virtual void getScreenSettings(const ScreenIdentifier& screenIdentifier, ScreenSettings & resolution) = 0;

            virtual bool setScreenSettings(const ScreenIdentifier& /*screenIdentifier*/, const ScreenSettings & /*resolution*/) { return false; }

            virtual void enumerateScreenSettings(const ScreenIdentifier& screenIdentifier, ScreenSettingsList & resolutionList) = 0;

            virtual void setDisplaySettings(DisplaySettings*) {}

            virtual osg::DisplaySettings* getDisplaySettings() const { return 0; }

            virtual GraphicsContext* createGraphicsContext(Traits* traits) = 0;

            /** Gets screen resolution without using the ScreenResolution structure.
              * \deprecated Provided only for backward compatibility. */
            inline void getScreenResolution(const ScreenIdentifier& screenIdentifier, unsigned int& width, unsigned int& height)
            {
                ScreenSettings settings;
                getScreenSettings(screenIdentifier, settings);
                width = settings.width;
                height = settings.height;
            }

            /** Sets screen resolution without using the ScreenSettings structure.
              * \deprecated Provided only for backward compatibility. */
            inline bool setScreenResolution(const ScreenIdentifier& screenIdentifier, unsigned int width, unsigned int height)
            {
                return setScreenSettings(screenIdentifier, ScreenSettings(width, height));
            }

            /** \deprecated Provided only for backward compatibility. */
            inline bool setScreenRefreshRate(const ScreenIdentifier& screenIdentifier, double refreshRate)
            {
                ScreenSettings settings;
                getScreenSettings(screenIdentifier, settings);
                settings.refreshRate = refreshRate;
                return setScreenSettings(screenIdentifier, settings);
            }
        protected:
            WindowingSystemInterface() {}
            virtual ~WindowingSystemInterface() {}

            std::string _name;
        };

        class OSG_EXPORT WindowingSystemInterfaces : public osg::Referenced
        {
        public:
            WindowingSystemInterfaces();

            typedef std::vector< osg::ref_ptr<GraphicsContext::WindowingSystemInterface> > Interfaces;

            Interfaces& getInterfaces() { return _interfaces; }

            void addWindowingSystemInterface(WindowingSystemInterface* wsInterface);

            void removeWindowingSystemInterface(WindowingSystemInterface* wsInterface);

            /** get named WindowingSystemInterface if one is available, otherwise return 0; */
            WindowingSystemInterface* getWindowingSystemInterface(const std::string& name = "");

        private:
            virtual ~WindowingSystemInterfaces();
            Interfaces _interfaces;
        };

        static osg::ref_ptr<WindowingSystemInterfaces>& getWindowingSystemInterfaces();

        /** Get the default WindowingSystemInterface for this OS*/
        static WindowingSystemInterface* getWindowingSystemInterface(const std::string& name = "");

        /** Create a graphics context for a specified set of traits.*/
        static GraphicsContext* createGraphicsContext(Traits* traits);

        /** Create a contextID for a new graphics context, this contextID is used to set up the osg::State associate with context.
          * Automatically increments the usage count of the contextID to 1.*/
        static unsigned int createNewContextID();

        /** Get the current max ContextID.*/
        static unsigned int getMaxContextID();

        /** Increment the usage count associate with a contextID. The usage count specifies how many graphics contexts a specific contextID is shared between.*/
        static void incrementContextIDUsageCount(unsigned int contextID);

        /** Decrement the usage count associate with a contextID. Once the contextID goes to 0 the contextID is then free to be reused.*/
        static void decrementContextIDUsageCount(unsigned int contextID);

        typedef std::vector<GraphicsContext*> GraphicsContexts;

        /** Get all the registered graphics contexts.*/
        static GraphicsContexts getAllRegisteredGraphicsContexts();

        /** Get all the registered graphics contexts associated with a specific contextID.*/
        static GraphicsContexts getRegisteredGraphicsContexts(unsigned int contextID);

        /** Get the GraphicsContext for doing background compilation for GraphicsContexts associated with specified contextID.*/
        static void setCompileContext(unsigned int contextID, GraphicsContext* gc);

        /** Get existing or create a new GraphicsContext to do background compilation for GraphicsContexts associated with specified contextID.*/
        static  GraphicsContext* getOrCreateCompileContext(unsigned int contextID);

        /** Get the GraphicsContext for doing background compilation for GraphicsContexts associated with specified contextID.*/
        static GraphicsContext* getCompileContext(unsigned int contextID);

    public:

        /** Add operation to end of OperationQueue.*/
        void add(Operation* operation);

        /** Remove operation from OperationQueue.*/
        void remove(Operation* operation);

        /** Remove named operation from OperationQueue.*/
        void remove(const std::string& name);

        /** Remove all operations from OperationQueue.*/
        void removeAllOperations();

        /** Run the operations. */
        virtual void runOperations();

        typedef std::list< ref_ptr<Operation> > GraphicsOperationQueue;

        /** Get the operations queue, note you must use the OperationsMutex when accessing the queue.*/
        GraphicsOperationQueue& getOperationsQueue() { return _operations; }

        /** Get the operations queue mutex.*/
        OpenThreads::Mutex* getOperationsMutex() { return &_operationsMutex; }

        /** Get the operations queue block used to mark an empty queue, if you end items into the empty queue you must release this block.*/
        osg::RefBlock* getOperationsBlock() { return _operationsBlock.get(); }

        /** Get the current operations that is being run.*/
        Operation* getCurrentOperation() { return _currentOperation.get(); }


    public:

        /** Get the traits of the GraphicsContext.*/
        inline const Traits* getTraits() const { return _traits.get(); }

        /** Return whether a valid and usable GraphicsContext has been created.*/
        virtual bool valid() const = 0;


        /** Set the State object which tracks the current OpenGL state for this graphics context.*/
        inline void setState(State* state) { _state = state; }

        /** Get the State object which tracks the current OpenGL state for this graphics context.*/
        inline State* getState() { return _state.get(); }

        /** Get the const State object which tracks the current OpenGL state for this graphics context.*/
        inline const State* getState() const { return _state.get(); }


        /** Sets the clear color. */
        inline void setClearColor(const Vec4& color) { _clearColor = color; }

        /** Returns the clear color. */
        inline const Vec4& getClearColor() const { return _clearColor; }

        /** Set the clear mask used in glClear(..).
          * Defaults to 0 - so no clear is done by default by the GraphicsContext, instead the Cameras attached to the GraphicsContext will do the clear.
          * GraphicsContext::setClearMask() is useful for when the Camera Viewports don't cover the whole context, so the context will fill in the gaps. */
        inline void setClearMask(GLbitfield mask) { _clearMask = mask; }

        /** Get the clear mask.*/
        inline GLbitfield getClearMask() const { return _clearMask; }

        /** Do an OpenGL clear of the full graphics context/window.
          * Note, must only be called from a thread with this context current.*/
        virtual void clear();

        double getTimeSinceLastClear() const { return osg::Timer::instance()->delta_s(_lastClearTick, osg::Timer::instance()->tick()); }


        /** Realize the GraphicsContext.*/
        bool realize();

        /** close the graphics context.
          * close(bool) stops any associated graphics threads, releases the contextID for the GraphicsContext then
          * optional calls closeImplementation() to do the actual deletion of the graphics.  This call is made optional
          * as there are times when the graphics context has already been deleted externally and only the OSG side
          * of the its data need to be closed down. */
        void close(bool callCloseImplementation=true);

        /** swap the front and back buffers.*/
        void swapBuffers();

        /** Return true if the graphics context has been realized and is ready to use.*/
        inline bool isRealized() const { return isRealizedImplementation(); }


        /** Make this graphics context current.
          * Implemented by calling makeCurrentImplementation().
          * Returns true on success. */
        bool makeCurrent();

        /** Make this graphics context current with specified read context.
          * Implemented by calling makeContextCurrentImplementation().
          * Returns true on success. */
        bool makeContextCurrent(GraphicsContext* readContext);

        /** Release the graphics context.
          * Returns true on success. */
        bool releaseContext();

        /** Return true if the current thread has this OpenGL graphics context.*/
        inline bool isCurrent() const { return _threadOfLastMakeCurrent == OpenThreads::Thread::CurrentThread(); }

        /** Bind the graphics context to associated texture.*/
        inline void bindPBufferToTexture(GLenum buffer) { bindPBufferToTextureImplementation(buffer); }



        /** Create a graphics thread to the graphics context, so that the thread handles all OpenGL operations.*/
        void createGraphicsThread();

        /** Assign a graphics thread to the graphics context, so that the thread handles all OpenGL operations.*/
        void setGraphicsThread(GraphicsThread* gt);

        /** Get the graphics thread assigned the graphics context.*/
        GraphicsThread* getGraphicsThread() { return _graphicsThread.get(); }

        /** Get the const graphics thread assigned the graphics context.*/
        const GraphicsThread* getGraphicsThread() const { return _graphicsThread.get(); }


        /** Realize the GraphicsContext implementation,
          * Pure virtual - must be implemented by concrete implementations of GraphicsContext. */
        virtual bool realizeImplementation() = 0;

        /** Return true if the graphics context has been realized, and is ready to use, implementation.
          * Pure virtual - must be implemented by concrete implementations of GraphicsContext. */
        virtual bool isRealizedImplementation() const = 0;

        /** Close the graphics context implementation.
          * Pure virtual - must be implemented by concrete implementations of GraphicsContext. */
        virtual void closeImplementation() = 0;

        /** Make this graphics context current implementation.
          * Pure virtual - must be implemented by concrete implementations of GraphicsContext. */
        virtual bool makeCurrentImplementation() = 0;

        /** Make this graphics context current with specified read context implementation.
          * Pure virtual - must be implemented by concrete implementations of GraphicsContext. */
        virtual bool makeContextCurrentImplementation(GraphicsContext* readContext) = 0;

        /** Release the graphics context implementation.*/
        virtual bool releaseContextImplementation() = 0;

        /** Pure virtual, Bind the graphics context to associated texture implementation.
          * Pure virtual - must be implemented by concrete implementations of GraphicsContext. */
        virtual void bindPBufferToTextureImplementation(GLenum buffer) = 0;

        struct SwapCallback : public osg::Referenced
        {
            virtual void swapBuffersImplementation(GraphicsContext* gc) = 0;
        };
        /** Set the swap callback which overrides the
          * GraphicsContext::swapBuffersImplementation(), allowing
          * developers to provide custom behavior for swap.
          * The callback must call
          * GraphicsContext::swapBuffersImplementation() */
        void setSwapCallback(SwapCallback* rc) { _swapCallback = rc; }

        /** Get the swap callback which overrides the GraphicsContext::swapBuffersImplementation().*/
        SwapCallback* getSwapCallback() { return _swapCallback.get(); }

        /** Get the const swap callback which overrides the GraphicsContext::swapBuffersImplementation().*/
        const SwapCallback* getSwapCallback() const { return _swapCallback.get(); }

        /** Convenience method for handling whether to call swapbuffers callback or the standard context swapBuffersImplementation.
          * swapBuffersCallbackOrImplementation() is called by swapBuffers() and osg::SwapBuffersOperation, end users should normally
          * call swapBuffers() rather than swapBuffersCallbackOrImplementation(). */
        void swapBuffersCallbackOrImplementation()
        {
            if (_state.valid()) _state->frameCompleted();

            if (_swapCallback.valid()) _swapCallback->swapBuffersImplementation(this);
            else swapBuffersImplementation();
        }

        /** Swap the front and back buffers implementation.
          * Pure virtual - must be implemented by concrete implementations of GraphicsContext. */
        virtual void swapBuffersImplementation() = 0;



        /** resized method should be called when the underlying window has been resized and the GraphicsWindow and associated Cameras must
            be updated to keep in sync with the new size. */
        void resized(int x, int y, int width, int height)
        {
            if (_resizedCallback.valid()) _resizedCallback->resizedImplementation(this, x, y, width, height);
            else resizedImplementation(x, y, width, height);
        }

        struct ResizedCallback : public osg::Referenced
        {
            virtual void resizedImplementation(GraphicsContext* gc, int x, int y, int width, int height) = 0;
        };

        /** Set the resized callback which overrides the GraphicsConext::realizedImplementation(), allow developers to provide custom behavior
          * in response to a window being resized.*/
        void setResizedCallback(ResizedCallback* rc) { _resizedCallback = rc; }

        /** Get the resized callback which overrides the GraphicsConext::realizedImplementation().*/
        ResizedCallback* getResizedCallback() { return _resizedCallback.get(); }

        /** Get the const resized callback which overrides the GraphicsConext::realizedImplementation().*/
        const ResizedCallback* getResizedCallback() const { return _resizedCallback.get(); }

        /** resized implementation, by default resizes the viewports and aspect ratios the cameras associated with the graphics Window. */
        virtual void resizedImplementation(int x, int y, int width, int height);


        typedef std::list< osg::Camera* > Cameras;

        /** Get the list of cameras associated with this graphics context.*/
        Cameras& getCameras() { return _cameras; }

        /** Get the const list of cameras associated with this graphics context.*/
        const Cameras& getCameras() const { return _cameras; }

        /** set the default FBO-id, this id will be used when the rendering-backend is finished with RTT FBOs */
        void setDefaultFboId(GLuint i) { _defaultFboId = i; }

        GLuint getDefaultFboId() const { return _defaultFboId; }

    public:

        virtual bool isSameKindAs(const Object* object) const { return dynamic_cast<const GraphicsContext*>(object)!=0; }
        virtual const char* libraryName() const { return "osg"; }
        virtual const char* className() const { return "GraphicsContext"; }

    protected:

        GraphicsContext();
        GraphicsContext(const GraphicsContext&, const osg::CopyOp&);

        virtual ~GraphicsContext();

        virtual Object* cloneType() const { return 0; }
        virtual Object* clone(const CopyOp&) const { return 0; }

        /** Register a GraphicsContext.*/
        static void registerGraphicsContext(GraphicsContext* gc);

        /** Unregister a GraphicsContext.*/
        static void unregisterGraphicsContext(GraphicsContext* gc);


        void addCamera(osg::Camera* camera);
        void removeCamera(osg::Camera* camera);

        Cameras _cameras;

        friend class osg::Camera;

        ref_ptr<Traits>         _traits;
        ref_ptr<State>          _state;

        Vec4                    _clearColor;
        GLbitfield              _clearMask;

        OpenThreads::Thread*    _threadOfLastMakeCurrent;

        OpenThreads::Mutex                  _operationsMutex;
        osg::ref_ptr<osg::RefBlock>         _operationsBlock;
        GraphicsOperationQueue              _operations;
        osg::ref_ptr<Operation>             _currentOperation;

        ref_ptr<GraphicsThread>             _graphicsThread;

        ref_ptr<ResizedCallback>            _resizedCallback;
        ref_ptr<SwapCallback>               _swapCallback;

        Timer_t                             _lastClearTick;

        GLuint                              _defaultFboId;
};



class OSG_EXPORT SyncSwapBuffersCallback : public GraphicsContext::SwapCallback
{
public:
    SyncSwapBuffersCallback();

    virtual void swapBuffersImplementation(GraphicsContext* gc);

    GLsync _previousSync;
};


template<class T>
struct WindowingSystemInterfaceProxy
{
    WindowingSystemInterfaceProxy(const std::string& name)
    {
        _wsi = new T;
        _wsi->setName(name);

        osg::GraphicsContext::getWindowingSystemInterfaces()->addWindowingSystemInterface(_wsi.get());
    }

    ~WindowingSystemInterfaceProxy()
    {
        osg::GraphicsContext::getWindowingSystemInterfaces()->removeWindowingSystemInterface(_wsi.get());
    }

    osg::ref_ptr<T> _wsi;
};

#define REGISTER_WINDOWINGSYSTEMINTERFACE(ext, classname) \
    extern "C" void graphicswindow_##ext(void) {} \
    static osg::WindowingSystemInterfaceProxy<classname> s_proxy_##classname(#ext);

}

