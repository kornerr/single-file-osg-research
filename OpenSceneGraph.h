
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

