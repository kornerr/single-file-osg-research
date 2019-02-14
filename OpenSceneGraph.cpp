
//#include "OpenThreads.h"
#include "OpenSceneGraph.h"

// OSGFILE src/osg/Referenced.cpp

#include <stdlib.h>

/*
#include <osg/Referenced>
#include <osg/Notify>
#include <osg/ApplicationUsage>
#include <osg/Observer>
*/

#include <typeinfo>
#include <memory>
#include <set>

//#include <OpenThreads/ScopedLock>
//#include <OpenThreads/Mutex>

//#include <osg/DeleteHandler>

namespace osg
{

//#define ENFORCE_THREADSAFE
//#define DEBUG_OBJECT_ALLOCATION_DESTRUCTION

// specialized smart pointer, used to get round auto_ptr<>'s lack of the destructor resetting itself to 0.
template<typename T>
struct ResetPointer
{
    ResetPointer():
        _ptr(0) {}

    ResetPointer(T* ptr):
        _ptr(ptr) {}

    ~ResetPointer()
    {
        delete _ptr;
        _ptr = 0;
    }

    inline ResetPointer& operator = (T* ptr)
    {
        if (_ptr==ptr) return *this;
        delete _ptr;
        _ptr = ptr;
        return *this;
    }

    void reset(T* ptr)
    {
        if (_ptr==ptr) return;
        delete _ptr;
        _ptr = ptr;
    }

    inline T& operator*()  { return *_ptr; }

    inline const T& operator*() const { return *_ptr; }

    inline T* operator->() { return _ptr; }

    inline const T* operator->() const   { return _ptr; }

    T* get() { return _ptr; }

    const T* get() const { return _ptr; }

    T* _ptr;
};

typedef ResetPointer<DeleteHandler> DeleteHandlerPointer;
typedef ResetPointer<OpenThreads::Mutex> GlobalMutexPointer;

OpenThreads::Mutex* Referenced::getGlobalReferencedMutex()
{
    static GlobalMutexPointer s_ReferencedGlobalMutext = new OpenThreads::Mutex;
    return s_ReferencedGlobalMutext.get();
}

// helper class for forcing the global mutex to be constructed when the library is loaded.
struct InitGlobalMutexes
{
    InitGlobalMutexes()
    {
        Referenced::getGlobalReferencedMutex();
    }
};
static InitGlobalMutexes s_initGlobalMutexes;

// static std::auto_ptr<DeleteHandler> s_deleteHandler(0);
static DeleteHandlerPointer s_deleteHandler(0);

void Referenced::setDeleteHandler(DeleteHandler* handler)
{
    s_deleteHandler.reset(handler);
}

DeleteHandler* Referenced::getDeleteHandler()
{
    return s_deleteHandler.get();
}

#ifdef DEBUG_OBJECT_ALLOCATION_DESTRUCTION
OpenThreads::Mutex& getNumObjectMutex()
{
    static OpenThreads::Mutex s_numObjectMutex;
    return s_numObjectMutex;
}
static int s_numObjects = 0;
#endif

Referenced::Referenced():
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    _observerSet(0),
    _refCount(0)
#else
    _refMutex(0),
    _refCount(0),
    _observerSet(0)
#endif
{
#if !defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    _refMutex = new OpenThreads::Mutex;
#endif

#ifdef DEBUG_OBJECT_ALLOCATION_DESTRUCTION
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getNumObjectMutex());
        ++s_numObjects;
        printf("Object created, total num=%d\n",s_numObjects);
    }
#endif

}

Referenced::Referenced(bool /*threadSafeRefUnref*/):
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    _observerSet(0),
    _refCount(0)
#else
    _refMutex(0),
    _refCount(0),
    _observerSet(0)
#endif
{
#if !defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    _refMutex = new OpenThreads::Mutex;
#endif

#ifdef DEBUG_OBJECT_ALLOCATION_DESTRUCTION
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getNumObjectMutex());
        ++s_numObjects;
        printf("Object created, total num=%d\n",s_numObjects);
    }
#endif
}

Referenced::Referenced(const Referenced&):
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    _observerSet(0),
    _refCount(0)
#else
    _refMutex(0),
    _refCount(0),
    _observerSet(0)
#endif
{
#if !defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    _refMutex = new OpenThreads::Mutex;
#endif

#ifdef DEBUG_OBJECT_ALLOCATION_DESTRUCTION
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getNumObjectMutex());
        ++s_numObjects;
        printf("Object created, total num=%d\n",s_numObjects);
    }
#endif
}

Referenced::~Referenced()
{
#ifdef DEBUG_OBJECT_ALLOCATION_DESTRUCTION
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(getNumObjectMutex());
        --s_numObjects;
        printf("Object created, total num=%d\n",s_numObjects);
    }
#endif

    if (_refCount>0)
    {
        OSG_WARN<<"Warning: deleting still referenced object "<<this<<" of type '"<<typeid(this).name()<<"'"<<std::endl;
        OSG_WARN<<"         the final reference count was "<<_refCount<<", memory corruption possible."<<std::endl;
    }

    // signal observers that we are being deleted.
    signalObserversAndDelete(true, false);

    // delete the ObserverSet
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    if (_observerSet.get()) static_cast<ObserverSet*>(_observerSet.get())->unref();
#else
    if (_observerSet) static_cast<ObserverSet*>(_observerSet)->unref();
#endif

#if !defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    if (_refMutex) delete _refMutex;
#endif
}

ObserverSet* Referenced::getOrCreateObserverSet() const
{
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    ObserverSet* observerSet = static_cast<ObserverSet*>(_observerSet.get());
    while (0 == observerSet)
    {
        ObserverSet* newObserverSet = new ObserverSet(this);
        newObserverSet->ref();

        if (!_observerSet.assign(newObserverSet, 0))
        {
            newObserverSet->unref();
        }

        observerSet = static_cast<ObserverSet*>(_observerSet.get());
    }
    return observerSet;
#else
    if (_refMutex)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(*_refMutex);
        if (!_observerSet)
        {
            _observerSet = new ObserverSet(this);
            static_cast<ObserverSet*>(_observerSet)->ref();
        }
        return static_cast<ObserverSet*>(_observerSet);
    }
    else
    {
        if (!_observerSet)
        {
            _observerSet = new ObserverSet(this);
            static_cast<ObserverSet*>(_observerSet)->ref();
        }
        return static_cast<ObserverSet*>(_observerSet);
    }
#endif
}

void Referenced::addObserver(Observer* observer) const
{
    getOrCreateObserverSet()->addObserver(observer);
}

void Referenced::removeObserver(Observer* observer) const
{
    getOrCreateObserverSet()->removeObserver(observer);
}

void Referenced::signalObserversAndDelete(bool signalDelete, bool doDelete) const
{
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    ObserverSet* observerSet = static_cast<ObserverSet*>(_observerSet.get());
#else
    ObserverSet* observerSet = static_cast<ObserverSet*>(_observerSet);
#endif

    if (observerSet && signalDelete)
    {
        observerSet->signalObjectDeleted(const_cast<Referenced*>(this));
    }

    if (doDelete)
    {
        if (_refCount!=0)
            OSG_NOTICE<<"Warning Referenced::signalObserversAndDelete(,,) doing delete with _refCount="<<_refCount<<std::endl;

        if (getDeleteHandler()) deleteUsingDeleteHandler();
        else delete this;
    }
}

int Referenced::unref_nodelete() const
{
#if defined(_OSG_REFERENCED_USE_ATOMIC_OPERATIONS)
    return --_refCount;
#else
    if (_refMutex)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(*_refMutex);
        return --_refCount;
    }
    else
    {
        return --_refCount;
    }
#endif
}

void Referenced::deleteUsingDeleteHandler() const
{
    getDeleteHandler()->requestDelete(this);
}

} // end of namespace osg


// OSGFILE src/osg/Observer.cpp

//#include <osg/ObserverNodePath>
//#include <osg/Notify>

using namespace osg;

Observer::Observer()
{
}

Observer::~Observer()
{
}

ObserverSet::ObserverSet(const Referenced* observedObject):
    _observedObject(const_cast<Referenced*>(observedObject))
{
    //OSG_NOTICE<<"ObserverSet::ObserverSet() "<<this<<std::endl;
}

ObserverSet::~ObserverSet()
{
    //OSG_NOTICE<<"ObserverSet::~ObserverSet() "<<this<<", _observers.size()="<<_observers.size()<<std::endl;
}

void ObserverSet::addObserver(Observer* observer)
{
    //OSG_NOTICE<<"ObserverSet::addObserver("<<observer<<") "<<this<<std::endl;
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    _observers.insert(observer);
}

void ObserverSet::removeObserver(Observer* observer)
{
    //OSG_NOTICE<<"ObserverSet::removeObserver("<<observer<<") "<<this<<std::endl;
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    _observers.erase(observer);
}

Referenced* ObserverSet::addRefLock()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

    if (!_observedObject) return 0;

    int refCount = _observedObject->ref();
    if (refCount == 1)
    {
        // The object is in the process of being deleted, but our
        // objectDeleted() method hasn't been run yet (and we're
        // blocking it -- and the final destruction -- with our lock).
        _observedObject->unref_nodelete();
        return 0;
    }

    return _observedObject;
}

void ObserverSet::signalObjectDeleted(void* ptr)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

    for(Observers::iterator itr = _observers.begin();
        itr != _observers.end();
        ++itr)
    {
        (*itr)->objectDeleted(ptr);
    }
    _observers.clear();

    // reset the observed object so that we know that it's now detached.
    _observedObject = 0;
}


// OSGFILE src/osg/Notify.cpp

/*
#include <osg/Notify>
#include <osg/ApplicationUsage>
#include <osg/os_utils>
#include <osg/ref_ptr>
*/
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iostream>

#include <ctype.h>

#define OSG_INIT_SINGLETON_PROXY(ProxyName, Func) static struct ProxyName{ ProxyName() { Func; } } s_##ProxyName;

namespace osg
{

class NullStreamBuffer : public std::streambuf
{
private:
    std::streamsize xsputn(const std::streambuf::char_type * /*str*/, std::streamsize n)
    {
        return n;
    }
};

struct NullStream : public std::ostream
{
public:
    NullStream():
        std::ostream(new NullStreamBuffer)
    {
        _buffer = static_cast<NullStreamBuffer *>(rdbuf());
    }

    ~NullStream()
    {
        rdbuf(0);
        delete _buffer;
    }

protected:
    NullStreamBuffer* _buffer;
};

/** Stream buffer calling notify handler when buffer is synchronized (usually on std::endl).
 * Stream stores last notification severity to pass it to handler call.
 */
struct NotifyStreamBuffer : public std::stringbuf
{
    NotifyStreamBuffer() : _severity(osg::NOTICE)
    {
        /* reduce the need to reallocate the std::ostream buffer behind osg::Notify (causing multitreading issues) by pre-allocating 4095 bytes */
        str(std::string(4095, 0));
        pubseekpos(0, std::ios_base::out);
    }

    void setNotifyHandler(osg::NotifyHandler *handler) { _handler = handler; }
    osg::NotifyHandler *getNotifyHandler() const { return _handler.get(); }

    /** Sets severity for next call of notify handler */
    void setCurrentSeverity(osg::NotifySeverity severity)
    {
        if (_severity != severity)
        {
            sync();
            _severity = severity;
        }
    }

    osg::NotifySeverity getCurrentSeverity() const { return _severity; }

private:

    int sync()
    {
        sputc(0); // string termination
        if (_handler.valid())
            _handler->notify(_severity, pbase());
        pubseekpos(0, std::ios_base::out); // or str(std::string())
        return 0;
    }

    osg::ref_ptr<osg::NotifyHandler> _handler;
    osg::NotifySeverity _severity;
};

struct NotifyStream : public std::ostream
{
public:
    NotifyStream():
        std::ostream(new NotifyStreamBuffer)
    {
        _buffer = static_cast<NotifyStreamBuffer *>(rdbuf());
    }

    void setCurrentSeverity(osg::NotifySeverity severity)
    {
        _buffer->setCurrentSeverity(severity);
    }

    osg::NotifySeverity getCurrentSeverity() const
    {
        return _buffer->getCurrentSeverity();
    }

    ~NotifyStream()
    {
        rdbuf(0);
        delete _buffer;
    }

protected:
    NotifyStreamBuffer* _buffer;
};

}

using namespace osg;

static osg::ApplicationUsageProxy Notify_e0(osg::ApplicationUsage::ENVIRONMENTAL_VARIABLE, "OSG_NOTIFY_LEVEL <mode>", "FATAL | WARN | NOTICE | DEBUG_INFO | DEBUG_FP | DEBUG | INFO | ALWAYS");

struct NotifySingleton
{
    NotifySingleton()
    {
        // _notifyLevel
        // =============

        _notifyLevel = osg::NOTICE; // Default value

        std::string OSGNOTIFYLEVEL;
        if(getEnvVar("OSG_NOTIFY_LEVEL", OSGNOTIFYLEVEL) || getEnvVar("OSGNOTIFYLEVEL", OSGNOTIFYLEVEL))
        {

            std::string stringOSGNOTIFYLEVEL(OSGNOTIFYLEVEL);

            // Convert to upper case
            for(std::string::iterator i=stringOSGNOTIFYLEVEL.begin();
                i!=stringOSGNOTIFYLEVEL.end();
                ++i)
            {
                *i=toupper(*i);
            }

            if(stringOSGNOTIFYLEVEL.find("ALWAYS")!=std::string::npos)          _notifyLevel=osg::ALWAYS;
            else if(stringOSGNOTIFYLEVEL.find("FATAL")!=std::string::npos)      _notifyLevel=osg::FATAL;
            else if(stringOSGNOTIFYLEVEL.find("WARN")!=std::string::npos)       _notifyLevel=osg::WARN;
            else if(stringOSGNOTIFYLEVEL.find("NOTICE")!=std::string::npos)     _notifyLevel=osg::NOTICE;
            else if(stringOSGNOTIFYLEVEL.find("DEBUG_INFO")!=std::string::npos) _notifyLevel=osg::DEBUG_INFO;
            else if(stringOSGNOTIFYLEVEL.find("DEBUG_FP")!=std::string::npos)   _notifyLevel=osg::DEBUG_FP;
            else if(stringOSGNOTIFYLEVEL.find("DEBUG")!=std::string::npos)      _notifyLevel=osg::DEBUG_INFO;
            else if(stringOSGNOTIFYLEVEL.find("INFO")!=std::string::npos)       _notifyLevel=osg::INFO;
            else std::cout << "Warning: invalid OSG_NOTIFY_LEVEL set ("<<stringOSGNOTIFYLEVEL<<")"<<std::endl;

        }

        // Setup standard notify handler
        osg::NotifyStreamBuffer *buffer = dynamic_cast<osg::NotifyStreamBuffer *>(_notifyStream.rdbuf());
        if (buffer && !buffer->getNotifyHandler())
            buffer->setNotifyHandler(new StandardNotifyHandler);
    }

    osg::NotifySeverity _notifyLevel;
    osg::NullStream     _nullStream;
    osg::NotifyStream   _notifyStream;
};

static NotifySingleton& getNotifySingleton()
{
    static NotifySingleton s_NotifySingleton;
    return s_NotifySingleton;
}

bool osg::initNotifyLevel()
{
    getNotifySingleton();
    return true;
}

// Use a proxy to force the initialization of the NotifySingleton during static initialization
OSG_INIT_SINGLETON_PROXY(NotifySingletonProxy, osg::initNotifyLevel())

void osg::setNotifyLevel(osg::NotifySeverity severity)
{
    getNotifySingleton()._notifyLevel = severity;
}

osg::NotifySeverity osg::getNotifyLevel()
{
    return getNotifySingleton()._notifyLevel;
}

void osg::setNotifyHandler(osg::NotifyHandler *handler)
{
    osg::NotifyStreamBuffer *buffer = static_cast<osg::NotifyStreamBuffer*>(getNotifySingleton()._notifyStream.rdbuf());
    if (buffer) buffer->setNotifyHandler(handler);
}

osg::NotifyHandler* osg::getNotifyHandler()
{
    osg::NotifyStreamBuffer *buffer = static_cast<osg::NotifyStreamBuffer *>(getNotifySingleton()._notifyStream.rdbuf());
    return buffer ? buffer->getNotifyHandler() : 0;
}


#ifndef OSG_NOTIFY_DISABLED
bool osg::isNotifyEnabled( osg::NotifySeverity severity )
{
    return severity<=getNotifySingleton()._notifyLevel;
}
#endif

std::ostream& osg::notify(const osg::NotifySeverity severity)
{
    if (osg::isNotifyEnabled(severity))
    {
        getNotifySingleton()._notifyStream.setCurrentSeverity(severity);
        return getNotifySingleton()._notifyStream;
    }
    return getNotifySingleton()._nullStream;
}

void osg::StandardNotifyHandler::notify(osg::NotifySeverity severity, const char *message)
{
    if (severity <= osg::WARN)
        fputs(message, stderr);
    else
        fputs(message, stdout);
}

#if defined(WIN32) && !defined(__CYGWIN__)

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

void osg::WinDebugNotifyHandler::notify(osg::NotifySeverity severity, const char *message)
{
    OSG_UNUSED(severity);

    OutputDebugStringA(message);
}

#endif

// OSGFILE src/osg/Object.cpp

/*
#include <osg/Object>
#include <osg/UserDataContainer>
*/

namespace osg
{

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Object
//
Object::Object(const Object& obj,const CopyOp& copyop):
    Referenced(),
    _name(obj._name),
    _dataVariance(obj._dataVariance),
    _userDataContainer(0)
{
    if (obj._userDataContainer)
    {
        if (copyop.getCopyFlags()&osg::CopyOp::DEEP_COPY_USERDATA)
        {
            setUserDataContainer(osg::clone(obj._userDataContainer,copyop));
        }
        else
        {
            setUserDataContainer(obj._userDataContainer);
        }
    }
}

Object::~Object()
{
    if (_userDataContainer) _userDataContainer->unref();
}


void Object::setThreadSafeRefUnref(bool threadSafe)
{
    Referenced::setThreadSafeRefUnref(threadSafe);
    if (_userDataContainer) _userDataContainer->setThreadSafeRefUnref(threadSafe);
}

void Object::setUserDataContainer(osg::UserDataContainer* udc)
{
    if (_userDataContainer == udc) return;

    if (_userDataContainer) _userDataContainer->unref();

    _userDataContainer = udc;

    if (_userDataContainer) _userDataContainer->ref();
}

osg::UserDataContainer* Object::getOrCreateUserDataContainer()
{
    if (!_userDataContainer) setUserDataContainer(new DefaultUserDataContainer());
    return _userDataContainer;
}

void Object::setUserData(Referenced* obj)
{
    if (getUserData()==obj) return;

    getOrCreateUserDataContainer()->setUserData(obj);
}

Referenced* Object::getUserData()
{
    return _userDataContainer ? _userDataContainer->getUserData() : 0;
}

const Referenced* Object::getUserData() const
{
    return _userDataContainer ? _userDataContainer->getUserData() : 0;
}

} // end of namespace osg


// OSGFILE src/osg/Object.cpp

/*
#include <osg/CopyOp>
#include <osg/Node>
#include <osg/StateSet>
#include <osg/Texture>
#include <osg/Drawable>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Shape>
#include <osg/StateAttribute>
#include <osg/Callback>
*/

using namespace osg;

#define COPY_OP( TYPE, FLAG ) \
TYPE* CopyOp::operator() (const TYPE* obj) const \
{ \
    if (obj && _flags&FLAG) \
        return osg::clone(obj, *this); \
    else \
        return const_cast<TYPE*>(obj); \
}

COPY_OP( Object,                   DEEP_COPY_OBJECTS )
COPY_OP( StateSet,                 DEEP_COPY_STATESETS )
COPY_OP( Image,                    DEEP_COPY_IMAGES )
COPY_OP( UniformBase,              DEEP_COPY_UNIFORMS )
COPY_OP( Uniform,                  DEEP_COPY_UNIFORMS )
COPY_OP( UniformCallback,          DEEP_COPY_CALLBACKS )
COPY_OP( StateAttributeCallback,   DEEP_COPY_CALLBACKS )
COPY_OP( Drawable,                 DEEP_COPY_DRAWABLES )
COPY_OP( Texture,                  DEEP_COPY_TEXTURES )
COPY_OP( Array,                    DEEP_COPY_ARRAYS )
COPY_OP( PrimitiveSet,             DEEP_COPY_PRIMITIVES )
COPY_OP( Shape,                    DEEP_COPY_SHAPES )

Referenced* CopyOp::operator() (const Referenced* ref) const
{
    return const_cast<Referenced*>(ref);
}

Node* CopyOp::operator() (const Node* node) const
{
    if (!node) return 0;

    const Drawable* drawable = node->asDrawable();
    if (drawable) return operator()(drawable);
    else if (_flags&DEEP_COPY_NODES) return osg::clone(node, *this);
    else return const_cast<Node*>(node);
}

StateAttribute* CopyOp::operator() (const StateAttribute* attr) const
{
    if (attr && _flags&DEEP_COPY_STATEATTRIBUTES)
    {
        const Texture* textbase = dynamic_cast<const Texture*>(attr);
        if (textbase)
        {
            return operator()(textbase);
        }
        else
        {
            return osg::clone(attr, *this);
        }
    }
    else
        return const_cast<StateAttribute*>(attr);
}

Callback* CopyOp::operator() (const Callback* nc) const
{
    if (nc && _flags&DEEP_COPY_CALLBACKS)
    {
        // deep copy the full chain of callback
        Callback* first = osg::clone(nc, *this);
        if (!first) return 0;

        first->setNestedCallback(0);
        nc = nc->getNestedCallback();
        while (nc)
        {
            Callback* ucb = osg::clone(nc, *this);
            if (ucb)
            {
                ucb->setNestedCallback(0);
                first->addNestedCallback(ucb);
            }
            nc = nc->getNestedCallback();
        }
        return first;
    }
    else
        return const_cast<Callback*>(nc);
}


// OSGFILE src/osg/Math.cpp

//#include <osg/Math>

#include <string.h>


double osg::asciiToDouble(const char* str)
{
    const char* ptr = str;

    // check if could be a hex number.
    if (strncmp(ptr,"0x",2)==0)
    {

        double value = 0.0;

        // skip over leading 0x, and then go through rest of string
        // checking to make sure all values are 0...9 or a..f.
        ptr+=2;
        while (
               *ptr!=0 &&
               ((*ptr>='0' && *ptr<='9') ||
                (*ptr>='a' && *ptr<='f') ||
                (*ptr>='A' && *ptr<='F'))
              )
        {
            if (*ptr>='0' && *ptr<='9') value = value*16.0 + double(*ptr-'0');
            else if (*ptr>='a' && *ptr<='f') value = value*16.0 + double(*ptr-'a'+10);
            else if (*ptr>='A' && *ptr<='F') value = value*16.0 + double(*ptr-'A'+10);
            ++ptr;
        }

        // OSG_NOTICE<<"Read "<<str<<" result = "<<value<<std::endl;
        return value;
    }

    ptr = str;

    bool    hadDecimal[2];
    double  value[2];
    double  sign[2];
    double  decimalMultiplier[2];

    hadDecimal[0] = hadDecimal[1] = false;
    sign[0] = sign[1] = 1.0;
    value[0] = value[1] = 0.0;
    decimalMultiplier[0] = decimalMultiplier[1] = 0.1;
    int pos = 0;

    // compute mantissa and exponent parts
    while (*ptr!=0 && pos<2)
    {
        if (*ptr=='+')
        {
            sign[pos] = 1.0;
        }
        else if (*ptr=='-')
        {
            sign[pos] = -1.0;
        }
        else if (*ptr>='0' && *ptr<='9')
        {
            if (!hadDecimal[pos])
            {
                value[pos] = value[pos]*10.0 + double(*ptr-'0');
            }
            else
            {
                value[pos] = value[pos] + decimalMultiplier[pos] * double(*ptr-'0');
                decimalMultiplier[pos] *= 0.1;
            }
        }
        else if (*ptr=='.')
        {
            hadDecimal[pos] = true;
        }
        else if (*ptr=='e' || *ptr=='E')
        {
            if (pos==1) break;

            pos = 1;
        }
        else
        {
            break;
        }
        ++ptr;
    }

    if (pos==0)
    {
        // OSG_NOTICE<<"Read "<<str<<" result = "<<value[0]*sign[0]<<std::endl;
        return value[0]*sign[0];
    }
    else
    {
        double mantissa = value[0]*sign[0];
        double exponent = value[1]*sign[1];
        //OSG_NOTICE<<"Read "<<str<<" mantissa = "<<mantissa<<" exponent="<<exponent<<" result = "<<mantissa*pow(10.0,exponent)<<std::endl;
        return mantissa*pow(10.0,exponent);
    }
}

double osg::findAsciiToDouble(const char* str)
{
   const char* ptr = str;
   double value = 0.0;

   while(*ptr != 0) {

       if(*ptr>='0' && *ptr<='9') {
           value = asciiToDouble(ptr);
           return value;
       }

       ++ptr;
   }

   return 0.0;
}


// OSGFILE src/osg/Quat.cpp

#include <stdio.h>
/*
#include <osg/Quat>
#include <osg/Matrixf>
#include <osg/Matrixd>
#include <osg/Notify>
*/

#include <math.h>

/// Good introductions to Quaternions at:
/// http://www.gamasutra.com/features/programming/19980703/quaternions_01.htm
/// http://mathworld.wolfram.com/Quaternion.html

using namespace osg;


void Quat::set(const Matrixf& matrix)
{
    *this = matrix.getRotate();
}

void Quat::set(const Matrixd& matrix)
{
    *this = matrix.getRotate();
}

void Quat::get(Matrixf& matrix) const
{
    matrix.makeRotate(*this);
}

void Quat::get(Matrixd& matrix) const
{
    matrix.makeRotate(*this);
}


/// Set the elements of the Quat to represent a rotation of angle
/// (radians) around the axis (x,y,z)
void Quat::makeRotate( value_type angle, value_type x, value_type y, value_type z )
{
    const value_type epsilon = 0.0000001;

    value_type length = sqrt( x*x + y*y + z*z );
    if (length < epsilon)
    {
        // ~zero length axis, so reset rotation to zero.
        *this = Quat();
        return;
    }

    value_type inversenorm  = 1.0/length;
    value_type coshalfangle = cos( 0.5*angle );
    value_type sinhalfangle = sin( 0.5*angle );

    _v[0] = x * sinhalfangle * inversenorm;
    _v[1] = y * sinhalfangle * inversenorm;
    _v[2] = z * sinhalfangle * inversenorm;
    _v[3] = coshalfangle;
}


void Quat::makeRotate( value_type angle, const Vec3f& vec )
{
    makeRotate( angle, vec[0], vec[1], vec[2] );
}
void Quat::makeRotate( value_type angle, const Vec3d& vec )
{
    makeRotate( angle, vec[0], vec[1], vec[2] );
}


void Quat::makeRotate ( value_type angle1, const Vec3f& axis1,
                        value_type angle2, const Vec3f& axis2,
                        value_type angle3, const Vec3f& axis3)
{
    makeRotate(angle1,Vec3d(axis1),
               angle2,Vec3d(axis2),
               angle3,Vec3d(axis3));
}

void Quat::makeRotate ( value_type angle1, const Vec3d& axis1,
                        value_type angle2, const Vec3d& axis2,
                        value_type angle3, const Vec3d& axis3)
{
    Quat q1; q1.makeRotate(angle1,axis1);
    Quat q2; q2.makeRotate(angle2,axis2);
    Quat q3; q3.makeRotate(angle3,axis3);

    *this = q1*q2*q3;
}


void Quat::makeRotate( const Vec3f& from, const Vec3f& to )
{
    makeRotate( Vec3d(from), Vec3d(to) );
}

/** Make a rotation Quat which will rotate vec1 to vec2

This routine uses only fast geometric transforms, without costly acos/sin computations.
It's exact, fast, and with less degenerate cases than the acos/sin method.

For an explanation of the math used, you may see for example:
http://logiciels.cnes.fr/MARMOTTES/marmottes-mathematique.pdf

@note This is the rotation with shortest angle, which is the one equivalent to the
acos/sin transform method. Other rotations exists, for example to additionally keep
a local horizontal attitude.

@author Nicolas Brodu
*/
void Quat::makeRotate( const Vec3d& from, const Vec3d& to )
{

    // This routine takes any vector as argument but normalized
    // vectors are necessary, if only for computing the dot product.
    // Too bad the API is that generic, it leads to performance loss.
    // Even in the case the 2 vectors are not normalized but same length,
    // the sqrt could be shared, but we have no way to know beforehand
    // at this point, while the caller may know.
    // So, we have to test... in the hope of saving at least a sqrt
    Vec3d sourceVector = from;
    Vec3d targetVector = to;

    value_type fromLen2 = from.length2();
    value_type fromLen;
    // normalize only when necessary, epsilon test
    if ((fromLen2 < 1.0-1e-7) || (fromLen2 > 1.0+1e-7)) {
        fromLen = sqrt(fromLen2);
        sourceVector /= fromLen;
    } else fromLen = 1.0;

    value_type toLen2 = to.length2();
    // normalize only when necessary, epsilon test
    if ((toLen2 < 1.0-1e-7) || (toLen2 > 1.0+1e-7)) {
        value_type toLen;
        // re-use fromLen for case of mapping 2 vectors of the same length
        if ((toLen2 > fromLen2-1e-7) && (toLen2 < fromLen2+1e-7)) {
            toLen = fromLen;
        }
        else toLen = sqrt(toLen2);
        targetVector /= toLen;
    }


    // Now let's get into the real stuff
    // Use "dot product plus one" as test as it can be re-used later on
    double dotProdPlus1 = 1.0 + sourceVector * targetVector;

    // Check for degenerate case of full u-turn. Use epsilon for detection
    if (dotProdPlus1 < 1e-7) {

        // Get an orthogonal vector of the given vector
        // in a plane with maximum vector coordinates.
        // Then use it as quaternion axis with pi angle
        // Trick is to realize one value at least is >0.6 for a normalized vector.
        if (fabs(sourceVector.x()) < 0.6) {
            const double norm = sqrt(1.0 - sourceVector.x() * sourceVector.x());
            _v[0] = 0.0;
            _v[1] = sourceVector.z() / norm;
            _v[2] = -sourceVector.y() / norm;
            _v[3] = 0.0;
        } else if (fabs(sourceVector.y()) < 0.6) {
            const double norm = sqrt(1.0 - sourceVector.y() * sourceVector.y());
            _v[0] = -sourceVector.z() / norm;
            _v[1] = 0.0;
            _v[2] = sourceVector.x() / norm;
            _v[3] = 0.0;
        } else {
            const double norm = sqrt(1.0 - sourceVector.z() * sourceVector.z());
            _v[0] = sourceVector.y() / norm;
            _v[1] = -sourceVector.x() / norm;
            _v[2] = 0.0;
            _v[3] = 0.0;
        }
    }

    else {
        // Find the shortest angle quaternion that transforms normalized vectors
        // into one other. Formula is still valid when vectors are colinear
        const double s = sqrt(0.5 * dotProdPlus1);
        const Vec3d tmp = sourceVector ^ targetVector / (2.0*s);
        _v[0] = tmp.x();
        _v[1] = tmp.y();
        _v[2] = tmp.z();
        _v[3] = s;
    }
}


// Make a rotation Quat which will rotate vec1 to vec2
// Generally take adot product to get the angle between these
// and then use a cross product to get the rotation axis
// Watch out for the two special cases of when the vectors
// are co-incident or opposite in direction.
void Quat::makeRotate_original( const Vec3d& from, const Vec3d& to )
{
    const value_type epsilon = 0.0000001;

    value_type length1  = from.length();
    value_type length2  = to.length();

    // dot product vec1*vec2
    value_type cosangle = from*to/(length1*length2);

    if ( fabs(cosangle - 1) < epsilon )
    {
        OSG_INFO<<"*** Quat::makeRotate(from,to) with near co-linear vectors, epsilon= "<<fabs(cosangle-1)<<std::endl;

        // cosangle is close to 1, so the vectors are close to being coincident
        // Need to generate an angle of zero with any vector we like
        // We'll choose (1,0,0)
        makeRotate( 0.0, 0.0, 0.0, 1.0 );
    }
    else
    if ( fabs(cosangle + 1.0) < epsilon )
    {
        // vectors are close to being opposite, so will need to find a
        // vector orthongonal to from to rotate about.
        Vec3d tmp;
        if (fabs(from.x())<fabs(from.y()))
            if (fabs(from.x())<fabs(from.z())) tmp.set(1.0,0.0,0.0); // use x axis.
            else tmp.set(0.0,0.0,1.0);
        else if (fabs(from.y())<fabs(from.z())) tmp.set(0.0,1.0,0.0);
        else tmp.set(0.0,0.0,1.0);

        Vec3d fromd(from.x(),from.y(),from.z());

        // find orthogonal axis.
        Vec3d axis(fromd^tmp);
        axis.normalize();

        _v[0] = axis[0]; // sin of half angle of PI is 1.0.
        _v[1] = axis[1]; // sin of half angle of PI is 1.0.
        _v[2] = axis[2]; // sin of half angle of PI is 1.0.
        _v[3] = 0; // cos of half angle of PI is zero.

    }
    else
    {
        // This is the usual situation - take a cross-product of vec1 and vec2
        // and that is the axis around which to rotate.
        Vec3d axis(from^to);
        value_type angle = acos( cosangle );
        makeRotate( angle, axis );
    }
}

void Quat::getRotate( value_type& angle, Vec3f& vec ) const
{
    value_type x,y,z;
    getRotate(angle,x,y,z);
    vec[0]=x;
    vec[1]=y;
    vec[2]=z;
}
void Quat::getRotate( value_type& angle, Vec3d& vec ) const
{
    value_type x,y,z;
    getRotate(angle,x,y,z);
    vec[0]=x;
    vec[1]=y;
    vec[2]=z;
}


// Get the angle of rotation and axis of this Quat object.
// Won't give very meaningful results if the Quat is not associated
// with a rotation!
void Quat::getRotate( value_type& angle, value_type& x, value_type& y, value_type& z ) const
{
    value_type sinhalfangle = sqrt( _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] );

    angle = 2.0 * atan2( sinhalfangle, _v[3] );
    if(sinhalfangle)
    {
        x = _v[0] / sinhalfangle;
        y = _v[1] / sinhalfangle;
        z = _v[2] / sinhalfangle;
    }
    else
    {
        x = 0.0;
        y = 0.0;
        z = 1.0;
    }

}


/// Spherical Linear Interpolation
/// As t goes from 0 to 1, the Quat object goes from "from" to "to"
/// Reference: Shoemake at SIGGRAPH 89
/// See also
/// http://www.gamasutra.com/features/programming/19980703/quaternions_01.htm
void Quat::slerp( value_type t, const Quat& from, const Quat& to )
{
    const double epsilon = 0.00001;
    double omega, cosomega, sinomega, scale_from, scale_to ;

    osg::Quat quatTo(to);
    // this is a dot product

    cosomega = from.asVec4() * to.asVec4();

    if ( cosomega <0.0 )
    {
        cosomega = -cosomega;
        quatTo = -to;
    }

    if( (1.0 - cosomega) > epsilon )
    {
        omega= acos(cosomega) ;  // 0 <= omega <= Pi (see man acos)
        sinomega = sin(omega) ;  // this sinomega should always be +ve so
        // could try sinomega=sqrt(1-cosomega*cosomega) to avoid a sin()?
        scale_from = sin((1.0-t)*omega)/sinomega ;
        scale_to = sin(t*omega)/sinomega ;
    }
    else
    {
        /* --------------------------------------------------
           The ends of the vectors are very close
           we can use simple linear interpolation - no need
           to worry about the "spherical" interpolation
           -------------------------------------------------- */
        scale_from = 1.0 - t ;
        scale_to = t ;
    }

    *this = (from*scale_from) + (quatTo*scale_to);

    // so that we get a Vec4
}


#define QX  _v[0]
#define QY  _v[1]
#define QZ  _v[2]
#define QW  _v[3]


#ifdef OSG_USE_UNIT_TESTS
void test_Quat_Eueler(value_type heading,value_type pitch,value_type roll)
{
    osg::Quat q;
    q.makeRotate(heading,pitch,roll);

    osg::Matrix q_m;
    q.get(q_m);

    osg::Vec3 xAxis(1,0,0);
    osg::Vec3 yAxis(0,1,0);
    osg::Vec3 zAxis(0,0,1);

    cout << "heading = "<<heading<<"  pitch = "<<pitch<<"  roll = "<<roll<<endl;

    cout <<"q_m = "<<q_m;
    cout <<"xAxis*q_m = "<<xAxis*q_m << endl;
    cout <<"yAxis*q_m = "<<yAxis*q_m << endl;
    cout <<"zAxis*q_m = "<<zAxis*q_m << endl;

    osg::Matrix r_m = osg::Matrix::rotate(roll,0.0,1.0,0.0)*
                      osg::Matrix::rotate(pitch,1.0,0.0,0.0)*
                      osg::Matrix::rotate(-heading,0.0,0.0,1.0);

    cout << "r_m = "<<r_m;
    cout <<"xAxis*r_m = "<<xAxis*r_m << endl;
    cout <<"yAxis*r_m = "<<yAxis*r_m << endl;
    cout <<"zAxis*r_m = "<<zAxis*r_m << endl;

    cout << endl<<"*****************************************" << endl<< endl;

}

void test_Quat()
{

    test_Quat_Eueler(osg::DegreesToRadians(20),0,0);
    test_Quat_Eueler(0,osg::DegreesToRadians(20),0);
    test_Quat_Eueler(0,0,osg::DegreesToRadians(20));
    test_Quat_Eueler(osg::DegreesToRadians(20),osg::DegreesToRadians(20),osg::DegreesToRadians(20));
    return 0;
}
#endif


// OSGFILE src/osg/Matrixd.cpp

//#include <osg/Matrixd>
//#include <osg/Matrixf>

// specialise Matrix_implementaiton to be Matrixd
#define  Matrix_implementation Matrixd

osg::Matrixd::Matrixd( const osg::Matrixf& mat )
{
    set(mat.ptr());
}

osg::Matrixd& osg::Matrixd::operator = (const osg::Matrixf& rhs)
{
    set(rhs.ptr());
    return *this;
}

void osg::Matrixd::set(const osg::Matrixf& rhs)
{
    set(rhs.ptr());
}

// now compile up Matrix via Matrix_implementation
//#include "Matrix_implementation.cpp"


// OSGFILE src/osg/Matrix_implementation.cpp

/*
#include <osg/Quat>
#include <osg/Notify>
#include <osg/Math>
#include <osg/Timer>

#include <osg/GL>
*/

#include <limits>
#include <stdlib.h>
#include <float.h>

using namespace osg;

#define SET_ROW(row, v1, v2, v3, v4 )    \
    _mat[(row)][0] = (v1); \
    _mat[(row)][1] = (v2); \
    _mat[(row)][2] = (v3); \
    _mat[(row)][3] = (v4);

#define INNER_PRODUCT(a,b,r,c) \
     ((a)._mat[r][0] * (b)._mat[0][c]) \
    +((a)._mat[r][1] * (b)._mat[1][c]) \
    +((a)._mat[r][2] * (b)._mat[2][c]) \
    +((a)._mat[r][3] * (b)._mat[3][c])


Matrix_implementation::Matrix_implementation( value_type a00, value_type a01, value_type a02, value_type a03,
                  value_type a10, value_type a11, value_type a12, value_type a13,
                  value_type a20, value_type a21, value_type a22, value_type a23,
                  value_type a30, value_type a31, value_type a32, value_type a33)
{
    SET_ROW(0, a00, a01, a02, a03 )
    SET_ROW(1, a10, a11, a12, a13 )
    SET_ROW(2, a20, a21, a22, a23 )
    SET_ROW(3, a30, a31, a32, a33 )
}

void Matrix_implementation::set( value_type a00, value_type a01, value_type a02, value_type a03,
                   value_type a10, value_type a11, value_type a12, value_type a13,
                   value_type a20, value_type a21, value_type a22, value_type a23,
                   value_type a30, value_type a31, value_type a32, value_type a33)
{
    SET_ROW(0, a00, a01, a02, a03 )
    SET_ROW(1, a10, a11, a12, a13 )
    SET_ROW(2, a20, a21, a22, a23 )
    SET_ROW(3, a30, a31, a32, a33 )
}

#define QX  q._v[0]
#define QY  q._v[1]
#define QZ  q._v[2]
#define QW  q._v[3]

void Matrix_implementation::setRotate(const Quat& q)
{
    double length2 = q.length2();
    if (fabs(length2) <= std::numeric_limits<double>::min())
    {
        _mat[0][0] = 0.0; _mat[1][0] = 0.0; _mat[2][0] = 0.0;
        _mat[0][1] = 0.0; _mat[1][1] = 0.0; _mat[2][1] = 0.0;
        _mat[0][2] = 0.0; _mat[1][2] = 0.0; _mat[2][2] = 0.0;
    }
    else
    {
        double rlength2;
        // normalize quat if required.
        // We can avoid the expensive sqrt in this case since all 'coefficients' below are products of two q components.
        // That is a square of a square root, so it is possible to avoid that
        if (length2 != 1.0)
        {
            rlength2 = 2.0/length2;
        }
        else
        {
            rlength2 = 2.0;
        }

        // Source: Gamasutra, Rotating Objects Using Quaternions
        //
        //http://www.gamasutra.com/features/19980703/quaternions_01.htm

        double wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

        // calculate coefficients
        x2 = rlength2*QX;
        y2 = rlength2*QY;
        z2 = rlength2*QZ;

        xx = QX * x2;
        xy = QX * y2;
        xz = QX * z2;

        yy = QY * y2;
        yz = QY * z2;
        zz = QZ * z2;

        wx = QW * x2;
        wy = QW * y2;
        wz = QW * z2;

        // Note.  Gamasutra gets the matrix assignments inverted, resulting
        // in left-handed rotations, which is contrary to OpenGL and OSG's
        // methodology.  The matrix assignment has been altered in the next
        // few lines of code to do the right thing.
        // Don Burns - Oct 13, 2001
        _mat[0][0] = 1.0 - (yy + zz);
        _mat[1][0] = xy - wz;
        _mat[2][0] = xz + wy;


        _mat[0][1] = xy + wz;
        _mat[1][1] = 1.0 - (xx + zz);
        _mat[2][1] = yz - wx;

        _mat[0][2] = xz - wy;
        _mat[1][2] = yz + wx;
        _mat[2][2] = 1.0 - (xx + yy);
    }

#if 0
    _mat[0][3] = 0.0;
    _mat[1][3] = 0.0;
    _mat[2][3] = 0.0;

    _mat[3][0] = 0.0;
    _mat[3][1] = 0.0;
    _mat[3][2] = 0.0;
    _mat[3][3] = 1.0;
#endif
}

#define COMPILE_getRotate_David_Spillings_Mk1
//#define COMPILE_getRotate_David_Spillings_Mk2
//#define COMPILE_getRotate_Original

#ifdef COMPILE_getRotate_David_Spillings_Mk1
// David Spillings implementation Mk 1
Quat Matrix_implementation::getRotate() const
{
    Quat q;

    value_type s;
    value_type tq[4];
    int    i, j;

    // Use tq to store the largest trace
    tq[0] = 1 + _mat[0][0]+_mat[1][1]+_mat[2][2];
    tq[1] = 1 + _mat[0][0]-_mat[1][1]-_mat[2][2];
    tq[2] = 1 - _mat[0][0]+_mat[1][1]-_mat[2][2];
    tq[3] = 1 - _mat[0][0]-_mat[1][1]+_mat[2][2];

    // Find the maximum (could also use stacked if's later)
    j = 0;
    for(i=1;i<4;i++) j = (tq[i]>tq[j])? i : j;

    // check the diagonal
    if (j==0)
    {
        /* perform instant calculation */
        QW = tq[0];
        QX = _mat[1][2]-_mat[2][1];
        QY = _mat[2][0]-_mat[0][2];
        QZ = _mat[0][1]-_mat[1][0];
    }
    else if (j==1)
    {
        QW = _mat[1][2]-_mat[2][1];
        QX = tq[1];
        QY = _mat[0][1]+_mat[1][0];
        QZ = _mat[2][0]+_mat[0][2];
    }
    else if (j==2)
    {
        QW = _mat[2][0]-_mat[0][2];
        QX = _mat[0][1]+_mat[1][0];
        QY = tq[2];
        QZ = _mat[1][2]+_mat[2][1];
    }
    else /* if (j==3) */
    {
        QW = _mat[0][1]-_mat[1][0];
        QX = _mat[2][0]+_mat[0][2];
        QY = _mat[1][2]+_mat[2][1];
        QZ = tq[3];
    }

    s = sqrt(0.25/tq[j]);
    QW *= s;
    QX *= s;
    QY *= s;
    QZ *= s;

    return q;

}
#endif


#ifdef COMPILE_getRotate_David_Spillings_Mk2
// David Spillings implementation Mk 2
Quat Matrix_implementation::getRotate() const
{
    Quat q;

    // From http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
    QW = 0.5 * sqrt( osg::maximum( 0.0, 1.0 + _mat[0][0] + _mat[1][1] + _mat[2][2] ) );
    QX = 0.5 * sqrt( osg::maximum( 0.0, 1.0 + _mat[0][0] - _mat[1][1] - _mat[2][2] ) );
    QY = 0.5 * sqrt( osg::maximum( 0.0, 1.0 - _mat[0][0] + _mat[1][1] - _mat[2][2] ) );
    QZ = 0.5 * sqrt( osg::maximum( 0.0, 1.0 - _mat[0][0] - _mat[1][1] + _mat[2][2] ) );

#if 0
    // Robert Osfield, June 7th 2007, arggg this new implementation produces many many errors, so have to revert to sign(..) original below.
    QX = QX * osg::signOrZero(  _mat[1][2] - _mat[2][1]) ;
    QY = QY * osg::signOrZero(  _mat[2][0] - _mat[0][2]) ;
    QZ = QZ * osg::signOrZero(  _mat[0][1] - _mat[1][0]) ;
#else
    QX = QX * osg::sign(  _mat[1][2] - _mat[2][1]) ;
    QY = QY * osg::sign(  _mat[2][0] - _mat[0][2]) ;
    QZ = QZ * osg::sign(  _mat[0][1] - _mat[1][0]) ;
#endif

    return q;
}
#endif

#ifdef COMPILE_getRotate_Original
// Original implementation
Quat Matrix_implementation::getRotate() const
{
    Quat q;

    // Source: Gamasutra, Rotating Objects Using Quaternions
    //
    //http://www.gamasutra.com/features/programming/19980703/quaternions_01.htm

    value_type  tr, s;
    value_type tq[4];
    int    i, j, k;

    int nxt[3] = {1, 2, 0};

    tr = _mat[0][0] + _mat[1][1] + _mat[2][2]+1.0;

    // check the diagonal
    if (tr > 0.0)
    {
        s = (value_type)sqrt (tr);
        QW = s / 2.0;
        s = 0.5 / s;
        QX = (_mat[1][2] - _mat[2][1]) * s;
        QY = (_mat[2][0] - _mat[0][2]) * s;
        QZ = (_mat[0][1] - _mat[1][0]) * s;
    }
    else
    {
        // diagonal is negative
        i = 0;
        if (_mat[1][1] > _mat[0][0])
            i = 1;
        if (_mat[2][2] > _mat[i][i])
            i = 2;
        j = nxt[i];
        k = nxt[j];

        s = (value_type)sqrt ((_mat[i][i] - (_mat[j][j] + _mat[k][k])) + 1.0);

        tq[i] = s * 0.5;

        if (s != 0.0)
            s = 0.5 / s;

        tq[3] = (_mat[j][k] - _mat[k][j]) * s;
        tq[j] = (_mat[i][j] + _mat[j][i]) * s;
        tq[k] = (_mat[i][k] + _mat[k][i]) * s;

        QX = tq[0];
        QY = tq[1];
        QZ = tq[2];
        QW = tq[3];
    }

    return q;
}
#endif

int Matrix_implementation::compare(const Matrix_implementation& m) const
{
    const Matrix_implementation::value_type* lhs = reinterpret_cast<const Matrix_implementation::value_type*>(_mat);
    const Matrix_implementation::value_type* end_lhs = lhs+16;
    const Matrix_implementation::value_type* rhs = reinterpret_cast<const Matrix_implementation::value_type*>(m._mat);
    for(;lhs!=end_lhs;++lhs,++rhs)
    {
        if (*lhs < *rhs) return -1;
        if (*rhs < *lhs) return 1;
    }
    return 0;
}

void Matrix_implementation::setTrans( value_type tx, value_type ty, value_type tz )
{
    _mat[3][0] = tx;
    _mat[3][1] = ty;
    _mat[3][2] = tz;
}


void Matrix_implementation::setTrans( const Vec3f& v )
{
    _mat[3][0] = v[0];
    _mat[3][1] = v[1];
    _mat[3][2] = v[2];
}
void Matrix_implementation::setTrans( const Vec3d& v )
{
    _mat[3][0] = v[0];
    _mat[3][1] = v[1];
    _mat[3][2] = v[2];
}

void Matrix_implementation::makeIdentity()
{
    SET_ROW(0,    1, 0, 0, 0 )
    SET_ROW(1,    0, 1, 0, 0 )
    SET_ROW(2,    0, 0, 1, 0 )
    SET_ROW(3,    0, 0, 0, 1 )
}

void Matrix_implementation::makeScale( const Vec3f& v )
{
    makeScale(v[0], v[1], v[2] );
}

void Matrix_implementation::makeScale( const Vec3d& v )
{
    makeScale(v[0], v[1], v[2] );
}

void Matrix_implementation::makeScale( value_type x, value_type y, value_type z )
{
    SET_ROW(0,    x, 0, 0, 0 )
    SET_ROW(1,    0, y, 0, 0 )
    SET_ROW(2,    0, 0, z, 0 )
    SET_ROW(3,    0, 0, 0, 1 )
}

void Matrix_implementation::makeTranslate( const Vec3f& v )
{
    makeTranslate( v[0], v[1], v[2] );
}

void Matrix_implementation::makeTranslate( const Vec3d& v )
{
    makeTranslate( v[0], v[1], v[2] );
}

void Matrix_implementation::makeTranslate( value_type x, value_type y, value_type z )
{
    SET_ROW(0,    1, 0, 0, 0 )
    SET_ROW(1,    0, 1, 0, 0 )
    SET_ROW(2,    0, 0, 1, 0 )
    SET_ROW(3,    x, y, z, 1 )
}

void Matrix_implementation::makeRotate( const Vec3f& from, const Vec3f& to )
{
    makeIdentity();

    Quat quat;
    quat.makeRotate(from,to);
    setRotate(quat);
}
void Matrix_implementation::makeRotate( const Vec3d& from, const Vec3d& to )
{
    makeIdentity();

    Quat quat;
    quat.makeRotate(from,to);
    setRotate(quat);
}

void Matrix_implementation::makeRotate( value_type angle, const Vec3f& axis )
{
    makeIdentity();

    Quat quat;
    quat.makeRotate( angle, axis);
    setRotate(quat);
}
void Matrix_implementation::makeRotate( value_type angle, const Vec3d& axis )
{
    makeIdentity();

    Quat quat;
    quat.makeRotate( angle, axis);
    setRotate(quat);
}

void Matrix_implementation::makeRotate( value_type angle, value_type x, value_type y, value_type z )
{
    makeIdentity();

    Quat quat;
    quat.makeRotate( angle, x, y, z);
    setRotate(quat);
}

void Matrix_implementation::makeRotate( const Quat& quat )
{
    makeIdentity();

    setRotate(quat);
}

void Matrix_implementation::makeRotate( value_type angle1, const Vec3f& axis1,
                         value_type angle2, const Vec3f& axis2,
                         value_type angle3, const Vec3f& axis3)
{
    makeIdentity();

    Quat quat;
    quat.makeRotate(angle1, axis1,
                    angle2, axis2,
                    angle3, axis3);
    setRotate(quat);
}

void Matrix_implementation::makeRotate( value_type angle1, const Vec3d& axis1,
                         value_type angle2, const Vec3d& axis2,
                         value_type angle3, const Vec3d& axis3)
{
    makeIdentity();

    Quat quat;
    quat.makeRotate(angle1, axis1,
                    angle2, axis2,
                    angle3, axis3);
    setRotate(quat);
}

void Matrix_implementation::mult( const Matrix_implementation& lhs, const Matrix_implementation& rhs )
{
    if (&lhs==this)
    {
        postMult(rhs);
        return;
    }
    if (&rhs==this)
    {
        preMult(lhs);
        return;
    }

// PRECONDITION: We assume neither &lhs nor &rhs == this
// if it did, use preMult or postMult instead
    _mat[0][0] = INNER_PRODUCT(lhs, rhs, 0, 0);
    _mat[0][1] = INNER_PRODUCT(lhs, rhs, 0, 1);
    _mat[0][2] = INNER_PRODUCT(lhs, rhs, 0, 2);
    _mat[0][3] = INNER_PRODUCT(lhs, rhs, 0, 3);
    _mat[1][0] = INNER_PRODUCT(lhs, rhs, 1, 0);
    _mat[1][1] = INNER_PRODUCT(lhs, rhs, 1, 1);
    _mat[1][2] = INNER_PRODUCT(lhs, rhs, 1, 2);
    _mat[1][3] = INNER_PRODUCT(lhs, rhs, 1, 3);
    _mat[2][0] = INNER_PRODUCT(lhs, rhs, 2, 0);
    _mat[2][1] = INNER_PRODUCT(lhs, rhs, 2, 1);
    _mat[2][2] = INNER_PRODUCT(lhs, rhs, 2, 2);
    _mat[2][3] = INNER_PRODUCT(lhs, rhs, 2, 3);
    _mat[3][0] = INNER_PRODUCT(lhs, rhs, 3, 0);
    _mat[3][1] = INNER_PRODUCT(lhs, rhs, 3, 1);
    _mat[3][2] = INNER_PRODUCT(lhs, rhs, 3, 2);
    _mat[3][3] = INNER_PRODUCT(lhs, rhs, 3, 3);
}

void Matrix_implementation::preMult( const Matrix_implementation& other )
{
    // brute force method requiring a copy
    //Matrix_implementation tmp(other* *this);
    // *this = tmp;

    // more efficient method just use a value_type[4] for temporary storage.
    value_type t[4];
    for(int col=0; col<4; ++col) {
        t[0] = INNER_PRODUCT( other, *this, 0, col );
        t[1] = INNER_PRODUCT( other, *this, 1, col );
        t[2] = INNER_PRODUCT( other, *this, 2, col );
        t[3] = INNER_PRODUCT( other, *this, 3, col );
        _mat[0][col] = t[0];
        _mat[1][col] = t[1];
        _mat[2][col] = t[2];
        _mat[3][col] = t[3];
    }

}

void Matrix_implementation::postMult( const Matrix_implementation& other )
{
    // brute force method requiring a copy
    //Matrix_implementation tmp(*this * other);
    // *this = tmp;

    // more efficient method just use a value_type[4] for temporary storage.
    value_type t[4];
    for(int row=0; row<4; ++row)
    {
        t[0] = INNER_PRODUCT( *this, other, row, 0 );
        t[1] = INNER_PRODUCT( *this, other, row, 1 );
        t[2] = INNER_PRODUCT( *this, other, row, 2 );
        t[3] = INNER_PRODUCT( *this, other, row, 3 );
        SET_ROW(row, t[0], t[1], t[2], t[3] )
    }
}

#undef INNER_PRODUCT

// orthoNormalize the 3x3 rotation matrix
void Matrix_implementation::orthoNormalize(const Matrix_implementation& rhs)
{
    value_type x_colMag = (rhs._mat[0][0] * rhs._mat[0][0]) + (rhs._mat[1][0] * rhs._mat[1][0]) + (rhs._mat[2][0] * rhs._mat[2][0]);
    value_type y_colMag = (rhs._mat[0][1] * rhs._mat[0][1]) + (rhs._mat[1][1] * rhs._mat[1][1]) + (rhs._mat[2][1] * rhs._mat[2][1]);
    value_type z_colMag = (rhs._mat[0][2] * rhs._mat[0][2]) + (rhs._mat[1][2] * rhs._mat[1][2]) + (rhs._mat[2][2] * rhs._mat[2][2]);

    if(!equivalent((double)x_colMag, 1.0) && !equivalent((double)x_colMag, 0.0))
    {
      x_colMag = sqrt(x_colMag);
      _mat[0][0] = rhs._mat[0][0] / x_colMag;
      _mat[1][0] = rhs._mat[1][0] / x_colMag;
      _mat[2][0] = rhs._mat[2][0] / x_colMag;
    }
    else
    {
      _mat[0][0] = rhs._mat[0][0];
      _mat[1][0] = rhs._mat[1][0];
      _mat[2][0] = rhs._mat[2][0];
    }

    if(!equivalent((double)y_colMag, 1.0) && !equivalent((double)y_colMag, 0.0))
    {
      y_colMag = sqrt(y_colMag);
      _mat[0][1] = rhs._mat[0][1] / y_colMag;
      _mat[1][1] = rhs._mat[1][1] / y_colMag;
      _mat[2][1] = rhs._mat[2][1] / y_colMag;
    }
    else
    {
      _mat[0][1] = rhs._mat[0][1];
      _mat[1][1] = rhs._mat[1][1];
      _mat[2][1] = rhs._mat[2][1];
    }

    if(!equivalent((double)z_colMag, 1.0) && !equivalent((double)z_colMag, 0.0))
    {
      z_colMag = sqrt(z_colMag);
      _mat[0][2] = rhs._mat[0][2] / z_colMag;
      _mat[1][2] = rhs._mat[1][2] / z_colMag;
      _mat[2][2] = rhs._mat[2][2] / z_colMag;
    }
    else
    {
      _mat[0][2] = rhs._mat[0][2];
      _mat[1][2] = rhs._mat[1][2];
      _mat[2][2] = rhs._mat[2][2];
    }

    _mat[3][0] = rhs._mat[3][0];
    _mat[3][1] = rhs._mat[3][1];
    _mat[3][2] = rhs._mat[3][2];

    _mat[0][3] = rhs._mat[0][3];
    _mat[1][3] = rhs._mat[1][3];
    _mat[2][3] = rhs._mat[2][3];
    _mat[3][3] = rhs._mat[3][3];

}

/******************************************
  Matrix inversion technique:
Given a matrix mat, we want to invert it.
mat = [ r00 r01 r02 a
        r10 r11 r12 b
        r20 r21 r22 c
        tx  ty  tz  d ]
We note that this matrix can be split into three matrices.
mat = rot * trans * corr, where rot is rotation part, trans is translation part, and corr is the correction due to perspective (if any).
rot = [ r00 r01 r02 0
        r10 r11 r12 0
        r20 r21 r22 0
        0   0   0   1 ]
trans = [ 1  0  0  0
          0  1  0  0
          0  0  1  0
          tx ty tz 1 ]
corr = [ 1 0 0 px
         0 1 0 py
         0 0 1 pz
         0 0 0 s ]
where the elements of corr are obtained from linear combinations of the elements of rot, trans, and mat.
So the inverse is mat' = (trans * corr)' * rot', where rot' must be computed the traditional way, which is easy since it is only a 3x3 matrix.
This problem is simplified if [px py pz s] = [0 0 0 1], which will happen if mat was composed only of rotations, scales, and translations (which is common).  In this case, we can ignore corr entirely which saves on a lot of computations.
******************************************/

bool Matrix_implementation::invert_4x3( const Matrix_implementation& mat )
{
    if (&mat==this)
    {
       Matrix_implementation tm(mat);
       return invert_4x3(tm);
    }

    value_type r00 = mat._mat[0][0];
    value_type r01 = mat._mat[0][1];
    value_type r02 = mat._mat[0][2];
    value_type r10 = mat._mat[1][0];
    value_type r11 = mat._mat[1][1];
    value_type r12 = mat._mat[1][2];
    value_type r20 = mat._mat[2][0];
    value_type r21 = mat._mat[2][1];
    value_type r22 = mat._mat[2][2];

    // Partially compute inverse of rot
    _mat[0][0] = r11*r22 - r12*r21;
    _mat[0][1] = r02*r21 - r01*r22;
    _mat[0][2] = r01*r12 - r02*r11;

    // Compute determinant of rot from 3 elements just computed
    value_type one_over_det = 1.0/(r00*_mat[0][0] + r10*_mat[0][1] + r20*_mat[0][2]);
    r00 *= one_over_det; r10 *= one_over_det; r20 *= one_over_det;  // Saves on later computations

    // Finish computing inverse of rot
    _mat[0][0] *= one_over_det;
    _mat[0][1] *= one_over_det;
    _mat[0][2] *= one_over_det;
    _mat[0][3] = 0.0;
    _mat[1][0] = r12*r20 - r10*r22; // Have already been divided by det
    _mat[1][1] = r00*r22 - r02*r20; // same
    _mat[1][2] = r02*r10 - r00*r12; // same
    _mat[1][3] = 0.0;
    _mat[2][0] = r10*r21 - r11*r20; // Have already been divided by det
    _mat[2][1] = r01*r20 - r00*r21; // same
    _mat[2][2] = r00*r11 - r01*r10; // same
    _mat[2][3] = 0.0;
    _mat[3][3] = 1.0;

// We no longer need the rxx or det variables anymore, so we can reuse them for whatever we want.  But we will still rename them for the sake of clarity.

#define d r22
    d  = mat._mat[3][3];

    if( osg::square(d-1.0) > 1.0e-6 )  // Involves perspective, so we must
    {                       // compute the full inverse

        Matrix_implementation TPinv;
        _mat[3][0] = _mat[3][1] = _mat[3][2] = 0.0;

#define px r00
#define py r01
#define pz r02
#define one_over_s  one_over_det
#define a  r10
#define b  r11
#define c  r12

        a  = mat._mat[0][3]; b  = mat._mat[1][3]; c  = mat._mat[2][3];
        px = _mat[0][0]*a + _mat[0][1]*b + _mat[0][2]*c;
        py = _mat[1][0]*a + _mat[1][1]*b + _mat[1][2]*c;
        pz = _mat[2][0]*a + _mat[2][1]*b + _mat[2][2]*c;

#undef a
#undef b
#undef c
#define tx r10
#define ty r11
#define tz r12

        tx = mat._mat[3][0]; ty = mat._mat[3][1]; tz = mat._mat[3][2];
        one_over_s  = 1.0/(d - (tx*px + ty*py + tz*pz));

        tx *= one_over_s; ty *= one_over_s; tz *= one_over_s;  // Reduces number of calculations later on

        // Compute inverse of trans*corr
        TPinv._mat[0][0] = tx*px + 1.0;
        TPinv._mat[0][1] = ty*px;
        TPinv._mat[0][2] = tz*px;
        TPinv._mat[0][3] = -px * one_over_s;
        TPinv._mat[1][0] = tx*py;
        TPinv._mat[1][1] = ty*py + 1.0;
        TPinv._mat[1][2] = tz*py;
        TPinv._mat[1][3] = -py * one_over_s;
        TPinv._mat[2][0] = tx*pz;
        TPinv._mat[2][1] = ty*pz;
        TPinv._mat[2][2] = tz*pz + 1.0;
        TPinv._mat[2][3] = -pz * one_over_s;
        TPinv._mat[3][0] = -tx;
        TPinv._mat[3][1] = -ty;
        TPinv._mat[3][2] = -tz;
        TPinv._mat[3][3] = one_over_s;

        preMult(TPinv); // Finish computing full inverse of mat

#undef px
#undef py
#undef pz
#undef one_over_s
#undef d
    }
    else // Rightmost column is [0; 0; 0; 1] so it can be ignored
    {
        tx = mat._mat[3][0]; ty = mat._mat[3][1]; tz = mat._mat[3][2];

        // Compute translation components of mat'
        _mat[3][0] = -(tx*_mat[0][0] + ty*_mat[1][0] + tz*_mat[2][0]);
        _mat[3][1] = -(tx*_mat[0][1] + ty*_mat[1][1] + tz*_mat[2][1]);
        _mat[3][2] = -(tx*_mat[0][2] + ty*_mat[1][2] + tz*_mat[2][2]);

#undef tx
#undef ty
#undef tz
    }

    return true;
}


template <class T>
inline T SGL_ABS(T a)
{
   return (a >= 0 ? a : -a);
}

#ifndef SGL_SWAP
#define SGL_SWAP(a,b,temp) ((temp)=(a),(a)=(b),(b)=(temp))
#endif

bool Matrix_implementation::transpose(const Matrix_implementation&mat){
    if (&mat==this) {
       Matrix_implementation tm(mat);
       return transpose(tm);
    }
    _mat[0][1]=mat._mat[1][0];
    _mat[0][2]=mat._mat[2][0];
    _mat[0][3]=mat._mat[3][0];
    _mat[1][0]=mat._mat[0][1];
    _mat[1][2]=mat._mat[2][1];
    _mat[1][3]=mat._mat[3][1];
    _mat[2][0]=mat._mat[0][2];
    _mat[2][1]=mat._mat[1][2];
    _mat[2][3]=mat._mat[3][2];
    _mat[3][0]=mat._mat[0][3];
    _mat[3][1]=mat._mat[1][3];
    _mat[3][2]=mat._mat[2][3];
    return true;
}

bool Matrix_implementation::transpose3x3(const Matrix_implementation&mat){
    if (&mat==this) {
       Matrix_implementation tm(mat);
       return transpose3x3(tm);
    }
    _mat[0][1]=mat._mat[1][0];
    _mat[0][2]=mat._mat[2][0];
    _mat[1][0]=mat._mat[0][1];
    _mat[1][2]=mat._mat[2][1];
    _mat[2][0]=mat._mat[0][2];
    _mat[2][1]=mat._mat[1][2];

    return true;
}

bool Matrix_implementation::invert_4x4( const Matrix_implementation& mat )
{
    if (&mat==this) {
       Matrix_implementation tm(mat);
       return invert_4x4(tm);
    }

    unsigned int indxc[4], indxr[4], ipiv[4];
    unsigned int i,j,k,l,ll;
    unsigned int icol = 0;
    unsigned int irow = 0;
    double temp, pivinv, dum, big;

    // copy in place this may be unnecessary
    *this = mat;

    for (j=0; j<4; j++) ipiv[j]=0;

    for(i=0;i<4;i++)
    {
       big=0.0;
       for (j=0; j<4; ++j)
          if (ipiv[j] != 1)
             for (k=0; k<4; k++)
             {
                if (ipiv[k] == 0)
                {
                   if (SGL_ABS(operator()(j,k)) >= big)
                   {
                      big = SGL_ABS(operator()(j,k));
                      irow=j;
                      icol=k;
                   }
                }
                else if (ipiv[k] > 1)
                   return false;
             }
       ++(ipiv[icol]);
       if (irow != icol)
          for (l=0; l<4; ++l) SGL_SWAP(operator()(irow,l),
                                       operator()(icol,l),
                                       temp);

       indxr[i]=irow;
       indxc[i]=icol;
       if (operator()(icol,icol) == 0)
          return false;

       pivinv = 1.0/operator()(icol,icol);
       operator()(icol,icol) = 1;
       for (l=0; l<4; ++l) operator()(icol,l) *= pivinv;
       for (ll=0; ll<4; ++ll)
          if (ll != icol)
          {
             dum=operator()(ll,icol);
             operator()(ll,icol) = 0;
             for (l=0; l<4; ++l) operator()(ll,l) -= operator()(icol,l)*dum;
          }
    }
    for (int lx=4; lx>0; --lx)
    {
       if (indxr[lx-1] != indxc[lx-1])
          for (k=0; k<4; k++) SGL_SWAP(operator()(k,indxr[lx-1]),
                                       operator()(k,indxc[lx-1]),temp);
    }

    return true;
}

void Matrix_implementation::makeOrtho(double left, double right,
                       double bottom, double top,
                       double zNear, double zFar)
{
    // note transpose of Matrix_implementation wr.t OpenGL documentation, since the OSG use post multiplication rather than pre.
    double tx = -(right+left)/(right-left);
    double ty = -(top+bottom)/(top-bottom);
    double tz = -(zFar+zNear)/(zFar-zNear);
    SET_ROW(0, 2.0/(right-left),               0.0,               0.0, 0.0 )
    SET_ROW(1,              0.0,  2.0/(top-bottom),               0.0, 0.0 )
    SET_ROW(2,              0.0,               0.0,  -2.0/(zFar-zNear), 0.0 )
    SET_ROW(3,               tx,                ty,                 tz, 1.0 )
}

bool Matrix_implementation::getOrtho(Matrix_implementation::value_type& left, Matrix_implementation::value_type& right,
                                     Matrix_implementation::value_type& bottom, Matrix_implementation::value_type& top,
                                     Matrix_implementation::value_type& zNear, Matrix_implementation::value_type& zFar) const
{
    if (_mat[0][3]!=0.0 || _mat[1][3]!=0.0 || _mat[2][3]!=0.0 || _mat[3][3]!=1.0) return false;

    zNear = (_mat[3][2]+1.0) / _mat[2][2];
    zFar = (_mat[3][2]-1.0) / _mat[2][2];

    left = -(1.0+_mat[3][0]) / _mat[0][0];
    right = (1.0-_mat[3][0]) / _mat[0][0];

    bottom = -(1.0+_mat[3][1]) / _mat[1][1];
    top = (1.0-_mat[3][1]) / _mat[1][1];

    return true;
}

bool Matrix_implementation::getOrtho(Matrix_implementation::other_value_type& left, Matrix_implementation::other_value_type& right,
                                     Matrix_implementation::other_value_type& bottom, Matrix_implementation::other_value_type& top,
                                     Matrix_implementation::other_value_type& zNear, Matrix_implementation::other_value_type& zFar) const
{
    Matrix_implementation::value_type temp_left, temp_right, temp_bottom, temp_top, temp_zNear, temp_zFar;
    if (getOrtho(temp_left, temp_right, temp_bottom, temp_top, temp_zNear, temp_zFar))
    {
        left = temp_left;
        right = temp_right;
        bottom = temp_bottom;
        top = temp_top;
        zNear = temp_zNear;
        zFar = temp_zFar;
        return true;
    }
    else
    {
        return false;
    }
}

void Matrix_implementation::makeFrustum(double left, double right,
                         double bottom, double top,
                         double zNear, double zFar)
{
    // note transpose of Matrix_implementation wr.t OpenGL documentation, since the OSG use post multiplication rather than pre.
    double A = (right+left)/(right-left);
    double B = (top+bottom)/(top-bottom);
    double C = (fabs(zFar)>DBL_MAX) ? -1. : -(zFar+zNear)/(zFar-zNear);
    double D = (fabs(zFar)>DBL_MAX) ? -2.*zNear : -2.0*zFar*zNear/(zFar-zNear);
    SET_ROW(0, 2.0*zNear/(right-left),                    0.0, 0.0,  0.0 )
    SET_ROW(1,                    0.0, 2.0*zNear/(top-bottom), 0.0,  0.0 )
    SET_ROW(2,                      A,                      B,   C, -1.0 )
    SET_ROW(3,                    0.0,                    0.0,   D,  0.0 )
}

bool Matrix_implementation::getFrustum(Matrix_implementation::value_type& left, Matrix_implementation::value_type& right,
                                       Matrix_implementation::value_type& bottom, Matrix_implementation::value_type& top,
                                       Matrix_implementation::value_type& zNear, Matrix_implementation::value_type& zFar) const
{
    if (_mat[0][3]!=0.0 || _mat[1][3]!=0.0 || _mat[2][3]!=-1.0 || _mat[3][3]!=0.0)
        return false;

    // note: near and far must be used inside this method instead of zNear and zFar
    // because zNear and zFar are references and they may point to the same variable.
    Matrix_implementation::value_type temp_near = _mat[3][2] / (_mat[2][2]-1.0);
    Matrix_implementation::value_type temp_far = _mat[3][2] / (1.0+_mat[2][2]);

    left = temp_near * (_mat[2][0]-1.0) / _mat[0][0];
    right = temp_near * (1.0+_mat[2][0]) / _mat[0][0];

    top = temp_near * (1.0+_mat[2][1]) / _mat[1][1];
    bottom = temp_near * (_mat[2][1]-1.0) / _mat[1][1];

    zNear = temp_near;
    zFar = temp_far;

    return true;
}

bool Matrix_implementation::getFrustum(Matrix_implementation::other_value_type& left, Matrix_implementation::other_value_type& right,
                                       Matrix_implementation::other_value_type& bottom, Matrix_implementation::other_value_type& top,
                                       Matrix_implementation::other_value_type& zNear, Matrix_implementation::other_value_type& zFar) const
{
    Matrix_implementation::value_type temp_left, temp_right, temp_bottom, temp_top, temp_zNear, temp_zFar;
    if (getFrustum(temp_left, temp_right, temp_bottom, temp_top, temp_zNear, temp_zFar))
    {
        left = temp_left;
        right = temp_right;
        bottom = temp_bottom;
        top = temp_top;
        zNear = temp_zNear;
        zFar = temp_zFar;
        return true;
    }
    else
    {
        return false;
    }
}

void Matrix_implementation::makePerspective(double fovy,double aspectRatio,
                                            double zNear, double zFar)
{
    // calculate the appropriate left, right etc.
    double tan_fovy = tan(DegreesToRadians(fovy*0.5));
    double right  =  tan_fovy * aspectRatio * zNear;
    double left   = -right;
    double top    =  tan_fovy * zNear;
    double bottom =  -top;
    makeFrustum(left,right,bottom,top,zNear,zFar);
}

bool Matrix_implementation::getPerspective(Matrix_implementation::value_type& fovy, Matrix_implementation::value_type& aspectRatio,
                                           Matrix_implementation::value_type& zNear, Matrix_implementation::value_type& zFar) const
{
    Matrix_implementation::value_type right  =  0.0;
    Matrix_implementation::value_type left   =  0.0;
    Matrix_implementation::value_type top    =  0.0;
    Matrix_implementation::value_type bottom =  0.0;

    // note: near and far must be used inside this method instead of zNear and zFar
    // because zNear and zFar are references and they may point to the same variable.
    Matrix_implementation::value_type temp_near   =  0.0;
    Matrix_implementation::value_type temp_far    =  0.0;

    // get frustum and compute results
    bool r = getFrustum(left,right,bottom,top,temp_near,temp_far);
    if (r)
    {
        fovy = RadiansToDegrees(atan(top/temp_near)-atan(bottom/temp_near));
        aspectRatio = (right-left)/(top-bottom);
    }
    zNear = temp_near;
    zFar = temp_far;
    return r;
}

bool Matrix_implementation::getPerspective(Matrix_implementation::other_value_type& fovy, Matrix_implementation::other_value_type& aspectRatio,
                                           Matrix_implementation::other_value_type& zNear, Matrix_implementation::other_value_type& zFar) const
{
    Matrix_implementation::value_type temp_fovy, temp_aspectRatio, temp_zNear, temp_zFar;
    if (getPerspective(temp_fovy, temp_aspectRatio, temp_zNear, temp_zFar))
    {
        fovy = temp_fovy;
        aspectRatio = temp_aspectRatio;
        zNear = temp_zNear;
        zFar = temp_zFar;
        return true;
    }
    else
    {
        return false;
    }
}

void Matrix_implementation::makeLookAt(const Vec3d& eye,const Vec3d& center,const Vec3d& up)
{
    Vec3d f(center-eye);
    f.normalize();
    Vec3d s(f^up);
    s.normalize();
    Vec3d u(s^f);
    u.normalize();

    set(
        s[0],     u[0],     -f[0],     0.0,
        s[1],     u[1],     -f[1],     0.0,
        s[2],     u[2],     -f[2],     0.0,
        0.0,     0.0,     0.0,      1.0);

    preMultTranslate(-eye);
}


void Matrix_implementation::getLookAt(Vec3f& eye,Vec3f& center,Vec3f& up,value_type lookDistance) const
{
    Matrix_implementation inv;
    inv.invert(*this);

    // note: e and c variables must be used inside this method instead of eye and center
    // because eye and center are references and they may point to the same variable.
    Vec3f e = osg::Vec3f(0.0,0.0,0.0)*inv;
    up = transform3x3(*this,osg::Vec3f(0.0,1.0,0.0));
    Vec3f c = transform3x3(*this,osg::Vec3f(0.0,0.0,-1));
    c.normalize();
    c = e + c*lookDistance;

    // assign the results
    eye = e;
    center = c;
}

void Matrix_implementation::getLookAt(Vec3d& eye,Vec3d& center,Vec3d& up,value_type lookDistance) const
{
    Matrix_implementation inv;
    inv.invert(*this);

    // note: e and c variables must be used inside this method instead of eye and center
    // because eye and center are references and they may point to the same variable.
    Vec3d e = osg::Vec3d(0.0,0.0,0.0)*inv;
    up = transform3x3(*this,osg::Vec3d(0.0,1.0,0.0));
    Vec3d c = transform3x3(*this,osg::Vec3d(0.0,0.0,-1));
    c.normalize();
    c = e + c*lookDistance;

    // assign the results
    eye = e;
    center = c;
}

#undef SET_ROW


// OSGFILE src/osg/DisplaySettings.cpp

/*
#include <osg/DisplaySettings>
#include <osg/ArgumentParser>
#include <osg/ApplicationUsage>
#include <osg/Math>
#include <osg/Notify>
#include <osg/GL>
#include <osg/os_utils>
#include <osg/ref_ptr>
*/

#include <algorithm>
#include <string.h>

using namespace osg;
using namespace std;

#if defined(WIN32) && !defined(__CYGWIN__)
#include<windows.h>
extern "C" { OSG_EXPORT DWORD NvOptimusEnablement=0x00000001; }
#else
extern "C" { int NvOptimusEnablement=0x00000001; }
#endif

void DisplaySettings::setNvOptimusEnablement(int value)
{
    NvOptimusEnablement = value;
}

int DisplaySettings::getNvOptimusEnablement() const
{
    return NvOptimusEnablement;
}

ref_ptr<DisplaySettings>& DisplaySettings::instance()
{
    static ref_ptr<DisplaySettings> s_displaySettings = new DisplaySettings;
    return s_displaySettings;
}

OSG_INIT_SINGLETON_PROXY(ProxyInitDisplaySettings, DisplaySettings::instance())

DisplaySettings::DisplaySettings(const DisplaySettings& vs):Referenced(true)
{
    setDisplaySettings(vs);
}

DisplaySettings::~DisplaySettings()
{
}


 DisplaySettings& DisplaySettings::operator = (const DisplaySettings& vs)
{
    if (this==&vs) return *this;
    setDisplaySettings(vs);
    return *this;
}

void DisplaySettings::setDisplaySettings(const DisplaySettings& vs)
{
    _displayType = vs._displayType;
    _stereo = vs._stereo;
    _stereoMode = vs._stereoMode;
    _eyeSeparation = vs._eyeSeparation;
    _screenWidth = vs._screenWidth;
    _screenHeight = vs._screenHeight;
    _screenDistance = vs._screenDistance;

    _splitStereoHorizontalEyeMapping = vs._splitStereoHorizontalEyeMapping;
    _splitStereoHorizontalSeparation = vs._splitStereoHorizontalSeparation;

    _splitStereoVerticalEyeMapping = vs._splitStereoVerticalEyeMapping;
    _splitStereoVerticalSeparation = vs._splitStereoVerticalSeparation;

    _splitStereoAutoAdjustAspectRatio = vs._splitStereoAutoAdjustAspectRatio;

    _doubleBuffer = vs._doubleBuffer;
    _RGB = vs._RGB;
    _depthBuffer = vs._depthBuffer;
    _minimumNumberAlphaBits = vs._minimumNumberAlphaBits;
    _minimumNumberStencilBits = vs._minimumNumberStencilBits;
    _minimumNumberAccumRedBits = vs._minimumNumberAccumRedBits;
    _minimumNumberAccumGreenBits = vs._minimumNumberAccumGreenBits;
    _minimumNumberAccumBlueBits = vs._minimumNumberAccumBlueBits;
    _minimumNumberAccumAlphaBits = vs._minimumNumberAccumAlphaBits;

    _maxNumOfGraphicsContexts = vs._maxNumOfGraphicsContexts;
    _numMultiSamples = vs._numMultiSamples;

    _compileContextsHint = vs._compileContextsHint;
    _serializeDrawDispatch = vs._serializeDrawDispatch;
    _useSceneViewForStereoHint = vs._useSceneViewForStereoHint;

    _numDatabaseThreadsHint = vs._numDatabaseThreadsHint;
    _numHttpDatabaseThreadsHint = vs._numHttpDatabaseThreadsHint;

    _application = vs._application;

    _maxTexturePoolSize = vs._maxTexturePoolSize;
    _maxBufferObjectPoolSize = vs._maxBufferObjectPoolSize;

    _implicitBufferAttachmentRenderMask = vs._implicitBufferAttachmentRenderMask;
    _implicitBufferAttachmentResolveMask = vs._implicitBufferAttachmentResolveMask;

    _glContextVersion = vs._glContextVersion;
    _glContextFlags = vs._glContextFlags;
    _glContextProfileMask = vs._glContextProfileMask;
    _swapMethod = vs._swapMethod;

    _vertexBufferHint = vs._vertexBufferHint;

    setShaderHint(_shaderHint);

    _keystoneHint = vs._keystoneHint;
    _keystoneFileNames = vs._keystoneFileNames;
    _keystones = vs._keystones;

    _OSXMenubarBehavior = vs._OSXMenubarBehavior;

    _syncSwapBuffers = vs._syncSwapBuffers;
}

void DisplaySettings::merge(const DisplaySettings& vs)
{
    if (_stereo       || vs._stereo)        _stereo = true;

    // need to think what to do about merging the stereo mode.

    if (_doubleBuffer || vs._doubleBuffer)  _doubleBuffer = true;
    if (_RGB          || vs._RGB)           _RGB = true;
    if (_depthBuffer  || vs._depthBuffer)   _depthBuffer = true;

    if (vs._minimumNumberAlphaBits>_minimumNumberAlphaBits) _minimumNumberAlphaBits = vs._minimumNumberAlphaBits;
    if (vs._minimumNumberStencilBits>_minimumNumberStencilBits) _minimumNumberStencilBits = vs._minimumNumberStencilBits;
    if (vs._numMultiSamples>_numMultiSamples) _numMultiSamples = vs._numMultiSamples;

    if (vs._compileContextsHint) _compileContextsHint = vs._compileContextsHint;
    if (vs._serializeDrawDispatch) _serializeDrawDispatch = vs._serializeDrawDispatch;
    if (vs._useSceneViewForStereoHint) _useSceneViewForStereoHint = vs._useSceneViewForStereoHint;

    if (vs._numDatabaseThreadsHint>_numDatabaseThreadsHint) _numDatabaseThreadsHint = vs._numDatabaseThreadsHint;
    if (vs._numHttpDatabaseThreadsHint>_numHttpDatabaseThreadsHint) _numHttpDatabaseThreadsHint = vs._numHttpDatabaseThreadsHint;

    if (_application.empty()) _application = vs._application;

    if (vs._maxTexturePoolSize>_maxTexturePoolSize) _maxTexturePoolSize = vs._maxTexturePoolSize;
    if (vs._maxBufferObjectPoolSize>_maxBufferObjectPoolSize) _maxBufferObjectPoolSize = vs._maxBufferObjectPoolSize;

    // these are bit masks so merging them is like logical or
    _implicitBufferAttachmentRenderMask |= vs._implicitBufferAttachmentRenderMask;
    _implicitBufferAttachmentResolveMask |= vs._implicitBufferAttachmentResolveMask;

    // merge swap method to higher value
    if( vs._swapMethod > _swapMethod )
        _swapMethod = vs._swapMethod;

    _keystoneHint = _keystoneHint | vs._keystoneHint;

    // insert any unique filenames into the local list
    for(FileNames::const_iterator itr = vs._keystoneFileNames.begin();
        itr != vs._keystoneFileNames.end();
        ++itr)
    {
        const std::string& filename = *itr;
        FileNames::iterator found_itr = std::find(_keystoneFileNames.begin(), _keystoneFileNames.end(), filename);
        if (found_itr == _keystoneFileNames.end()) _keystoneFileNames.push_back(filename);
    }

    // insert unique Keystone object into local list
    for(Objects::const_iterator itr = vs._keystones.begin();
        itr != vs._keystones.end();
        ++itr)
    {
        const osg::Object* object = itr->get();
        Objects::iterator found_itr = std::find(_keystones.begin(), _keystones.end(), object);
        if (found_itr == _keystones.end()) _keystones.push_back(const_cast<osg::Object*>(object));
    }

    if (vs._OSXMenubarBehavior > _OSXMenubarBehavior)
        _OSXMenubarBehavior = vs._OSXMenubarBehavior;
}

void DisplaySettings::setDefaults()
{
    _displayType = MONITOR;

    _stereo = false;
    _stereoMode = ANAGLYPHIC;
    _eyeSeparation = 0.05f;
    _screenWidth = 0.325f;
    _screenHeight = 0.26f;
    _screenDistance = 0.5f;

    _splitStereoHorizontalEyeMapping = LEFT_EYE_LEFT_VIEWPORT;
    _splitStereoHorizontalSeparation = 0;

    _splitStereoVerticalEyeMapping = LEFT_EYE_TOP_VIEWPORT;
    _splitStereoVerticalSeparation = 0;

    _splitStereoAutoAdjustAspectRatio = false;

    _doubleBuffer = true;
    _RGB = true;
    _depthBuffer = true;
    _minimumNumberAlphaBits = 0;
    _minimumNumberStencilBits = 0;
    _minimumNumberAccumRedBits = 0;
    _minimumNumberAccumGreenBits = 0;
    _minimumNumberAccumBlueBits = 0;
    _minimumNumberAccumAlphaBits = 0;

    _maxNumOfGraphicsContexts = 32;
    _numMultiSamples = 0;

    #ifdef __sgi
    // switch on anti-aliasing by default, just in case we have an Onyx :-)
    _numMultiSamples = 4;
    #endif

    _compileContextsHint = false;
    _serializeDrawDispatch = false;
    _useSceneViewForStereoHint = true;

    _numDatabaseThreadsHint = 2;
    _numHttpDatabaseThreadsHint = 1;

    _maxTexturePoolSize = 0;
    _maxBufferObjectPoolSize = 0;

    _implicitBufferAttachmentRenderMask = DEFAULT_IMPLICIT_BUFFER_ATTACHMENT;
    _implicitBufferAttachmentResolveMask = DEFAULT_IMPLICIT_BUFFER_ATTACHMENT;
    _glContextVersion = OSG_GL_CONTEXT_VERSION;
    _glContextFlags = 0;
    _glContextProfileMask = 0;

    _swapMethod = SWAP_DEFAULT;
    _syncSwapBuffers = 0;

    _vertexBufferHint = NO_PREFERENCE;
    // _vertexBufferHint = VERTEX_BUFFER_OBJECT;
    // _vertexBufferHint = VERTEX_ARRAY_OBJECT;

#if defined(OSG_GLES3_AVAILABLE)
    setShaderHint(SHADER_GLES3);
#elif defined(OSG_GLES2_AVAILABLE)
    setShaderHint(SHADER_GLES2);
#elif defined(OSG_GL3_AVAILABLE)
    setShaderHint(SHADER_GL3);
#elif defined(OSG_GL_VERTEX_ARRAY_FUNCS_AVAILABLE)
    setShaderHint(SHADER_NONE);
#else
    setShaderHint(SHADER_GL2);
#endif

    _keystoneHint = false;

    _OSXMenubarBehavior = MENUBAR_AUTO_HIDE;

    _shaderPipeline = false;
    _shaderPipelineNumTextureUnits = 4;


}

void DisplaySettings::setMaxNumberOfGraphicsContexts(unsigned int num)
{
    _maxNumOfGraphicsContexts = num;
}

unsigned int DisplaySettings::getMaxNumberOfGraphicsContexts() const
{
    // OSG_NOTICE<<"getMaxNumberOfGraphicsContexts()="<<_maxNumOfGraphicsContexts<<std::endl;
    return _maxNumOfGraphicsContexts;
}

void DisplaySettings::setMinimumNumAccumBits(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha)
{
    _minimumNumberAccumRedBits = red;
    _minimumNumberAccumGreenBits = green;
    _minimumNumberAccumBlueBits = blue;
    _minimumNumberAccumAlphaBits = alpha;
}

static ApplicationUsageProxy DisplaySetting_e0(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_DISPLAY_TYPE <type>",
        "MONITOR | POWERWALL | REALITY_CENTER | HEAD_MOUNTED_DISPLAY");
static ApplicationUsageProxy DisplaySetting_e1(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_STEREO_MODE <mode>",
        "QUAD_BUFFER | ANAGLYPHIC | HORIZONTAL_SPLIT | VERTICAL_SPLIT | LEFT_EYE | RIGHT_EYE | VERTICAL_INTERLACE | HORIZONTAL_INTERLACE");
static ApplicationUsageProxy DisplaySetting_e2(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_STEREO <mode>",
        "OFF | ON");
static ApplicationUsageProxy DisplaySetting_e3(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_EYE_SEPARATION <float>",
        "Physical distance between eyes.");
static ApplicationUsageProxy DisplaySetting_e4(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SCREEN_DISTANCE <float>",
        "Physical distance between eyes and screen.");
static ApplicationUsageProxy DisplaySetting_e5(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SCREEN_HEIGHT <float>",
        "Physical screen height.");
static ApplicationUsageProxy DisplaySetting_e6(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SCREEN_WIDTH <float>",
        "Physical screen width.");
static ApplicationUsageProxy DisplaySetting_e7(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SPLIT_STEREO_HORIZONTAL_EYE_MAPPING <mode>",
        "LEFT_EYE_LEFT_VIEWPORT | LEFT_EYE_RIGHT_VIEWPORT");
static ApplicationUsageProxy DisplaySetting_e8(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SPLIT_STEREO_HORIZONTAL_SEPARATION <float>",
        "Number of pixels between viewports.");
static ApplicationUsageProxy DisplaySetting_e9(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SPLIT_STEREO_VERTICAL_EYE_MAPPING <mode>",
        "LEFT_EYE_TOP_VIEWPORT | LEFT_EYE_BOTTOM_VIEWPORT");
static ApplicationUsageProxy DisplaySetting_e10(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SPLIT_STEREO_AUTO_ADJUST_ASPECT_RATIO <mode>",
        "OFF | ON  Default to OFF to compenstate for the compression of the aspect ratio when viewing in split screen stereo.  Note, if you are setting fovx and fovy explicityly OFF should be used.");
static ApplicationUsageProxy DisplaySetting_e11(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SPLIT_STEREO_VERTICAL_SEPARATION <float>",
        "Number of pixels between viewports.");
static ApplicationUsageProxy DisplaySetting_e12(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_MAX_NUMBER_OF_GRAPHICS_CONTEXTS <int>",
        "Maximum number of graphics contexts to be used with applications.");
static ApplicationUsageProxy DisplaySetting_e13(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_COMPILE_CONTEXTS <mode>",
        "OFF | ON Disable/enable the use of background compiled contexts and threads.");
static ApplicationUsageProxy DisplaySetting_e14(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SERIALIZE_DRAW_DISPATCH <mode>",
        "OFF | ON Disable/enable the use of a mutex to serialize the draw dispatch when there are multiple graphics threads.");
static ApplicationUsageProxy DisplaySetting_e15(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_USE_SCENEVIEW_FOR_STEREO <mode>",
        "OFF | ON Disable/enable the hint to use osgUtil::SceneView to implement stereo when required..");
static ApplicationUsageProxy DisplaySetting_e16(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_NUM_DATABASE_THREADS <int>",
        "Set the hint for the total number of threads to set up in the DatabasePager.");
static ApplicationUsageProxy DisplaySetting_e17(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_NUM_HTTP_DATABASE_THREADS <int>",
        "Set the hint for the total number of threads dedicated to http requests to set up in the DatabasePager.");
static ApplicationUsageProxy DisplaySetting_e18(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_MULTI_SAMPLES <int>",
        "Set the hint for the number of samples to use when multi-sampling.");
static ApplicationUsageProxy DisplaySetting_e19(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_TEXTURE_POOL_SIZE <int>",
        "Set the hint for the size of the texture pool to manage.");
static ApplicationUsageProxy DisplaySetting_e20(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_BUFFER_OBJECT_POOL_SIZE <int>",
        "Set the hint for the size of the vertex buffer object pool to manage.");
static ApplicationUsageProxy DisplaySetting_e21(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_FBO_POOL_SIZE <int>",
        "Set the hint for the size of the frame buffer object pool to manage.");
static ApplicationUsageProxy DisplaySetting_e22(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_IMPLICIT_BUFFER_ATTACHMENT_RENDER_MASK",
        "OFF | DEFAULT | [~]COLOR | [~]DEPTH | [~]STENCIL. Substitute missing buffer attachments for render FBO.");
static ApplicationUsageProxy DisplaySetting_e23(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_IMPLICIT_BUFFER_ATTACHMENT_RESOLVE_MASK",
        "OFF | DEFAULT | [~]COLOR | [~]DEPTH | [~]STENCIL. Substitute missing buffer attachments for resolve FBO.");
static ApplicationUsageProxy DisplaySetting_e24(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_GL_CONTEXT_VERSION <major.minor>",
        "Set the hint for the GL version to create contexts for.");
static ApplicationUsageProxy DisplaySetting_e25(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_GL_CONTEXT_FLAGS <uint>",
        "Set the hint for the GL context flags to use when creating contexts.");
static ApplicationUsageProxy DisplaySetting_e26(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_GL_CONTEXT_PROFILE_MASK <uint>",
        "Set the hint for the GL context profile mask to use when creating contexts.");
static ApplicationUsageProxy DisplaySetting_e27(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SWAP_METHOD <method>",
        "DEFAULT | EXCHANGE | COPY | UNDEFINED. Select preferred swap method.");
static ApplicationUsageProxy DisplaySetting_e28(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_KEYSTONE ON | OFF",
        "Specify the hint to whether the viewer should set up keystone correction.");
static ApplicationUsageProxy DisplaySetting_e29(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_KEYSTONE_FILES <filename>[:filename]..",
        "Specify filenames of keystone parameter files. Under Windows use ; to deliminate files, otherwise use :");
static ApplicationUsageProxy DisplaySetting_e30(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_MENUBAR_BEHAVIOR <behavior>",
        "OSX Only : Specify the behavior of the menubar (AUTO_HIDE, FORCE_HIDE, FORCE_SHOW)");
static ApplicationUsageProxy DisplaySetting_e31(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_NvOptimusEnablement <value>",
        "Set the hint to NvOptimus of whether to enable it or not, set 1 to enable, 0 to disable");
static ApplicationUsageProxy DisplaySetting_e32(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_VERTEX_BUFFER_HINT <value>",
        "Set the hint to what backend osg::Geometry implementation to use. NO_PREFERENCE | VERTEX_BUFFER_OBJECT | VERTEX_ARRAY_OBJECT");
static ApplicationUsageProxy DisplaySetting_e33(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SHADER_PIPELINE <enable>",
        "ON|IFF");
static ApplicationUsageProxy DisplaySetting_e34(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SHADER_PIPELINE_FILES",
        "Specify the shader files to use for when Shader Pipeline is enabled");
static ApplicationUsageProxy DisplaySetting_e35(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_SHADER_PIPELINE_NUM_TEXTURE_UNITS <value>",
        "Specifiy number of texture units Shader Pipeline shaders support");
static ApplicationUsageProxy DisplaySetting_e36(ApplicationUsage::ENVIRONMENTAL_VARIABLE,
        "OSG_TEXT_SHADER_TECHNIQUE <value>",
        "Set the defafult osgText::ShaderTechnique. ALL_FEATURES | ALL | GREYSCALE | SIGNED_DISTANCE_FIELD | SDF | NO_TEXT_SHADER | NONE");

void DisplaySettings::readEnvironmentalVariables()
{
    std::string value;
    if (getEnvVar("OSG_DISPLAY_TYPE", value))
    {
        if (value=="MONITOR")
        {
            _displayType = MONITOR;
        }
        else
        if (value=="POWERWALL")
        {
            _displayType = POWERWALL;
        }
        else
        if (value=="REALITY_CENTER")
        {
            _displayType = REALITY_CENTER;
        }
        else
        if (value=="HEAD_MOUNTED_DISPLAY")
        {
            _displayType = HEAD_MOUNTED_DISPLAY;
        }
    }

    if (getEnvVar("OSG_STEREO_MODE", value))
    {
        if (value=="QUAD_BUFFER")
        {
            _stereoMode = QUAD_BUFFER;
        }
        else if (value=="ANAGLYPHIC")
        {
            _stereoMode = ANAGLYPHIC;
        }
        else if (value=="HORIZONTAL_SPLIT")
        {
            _stereoMode = HORIZONTAL_SPLIT;
        }
        else if (value=="VERTICAL_SPLIT")
        {
            _stereoMode = VERTICAL_SPLIT;
        }
        else if (value=="LEFT_EYE")
        {
            _stereoMode = LEFT_EYE;
        }
        else if (value=="RIGHT_EYE")
        {
            _stereoMode = RIGHT_EYE;
        }
        else if (value=="HORIZONTAL_INTERLACE")
        {
            _stereoMode = HORIZONTAL_INTERLACE;
        }
        else if (value=="VERTICAL_INTERLACE")
        {
            _stereoMode = VERTICAL_INTERLACE;
        }
        else if (value=="CHECKERBOARD")
        {
            _stereoMode = CHECKERBOARD;
        }
    }

    if (getEnvVar("OSG_STEREO", value))
    {
        if (value=="OFF")
        {
            _stereo = false;
        }
        else
        if (value=="ON")
        {
            _stereo = true;
        }
    }

    getEnvVar("OSG_EYE_SEPARATION", _eyeSeparation);
    getEnvVar("OSG_SCREEN_WIDTH", _screenWidth);
    getEnvVar("OSG_SCREEN_HEIGHT", _screenHeight);
    getEnvVar("OSG_SCREEN_DISTANCE", _screenDistance);

    if (getEnvVar("OSG_SPLIT_STEREO_HORIZONTAL_EYE_MAPPING", value))
    {
        if (value=="LEFT_EYE_LEFT_VIEWPORT")
        {
            _splitStereoHorizontalEyeMapping = LEFT_EYE_LEFT_VIEWPORT;
        }
        else
        if (value=="LEFT_EYE_RIGHT_VIEWPORT")
        {
            _splitStereoHorizontalEyeMapping = LEFT_EYE_RIGHT_VIEWPORT;
        }
    }

    getEnvVar("OSG_SPLIT_STEREO_HORIZONTAL_SEPARATION", _splitStereoHorizontalSeparation);


    if (getEnvVar("OSG_SPLIT_STEREO_VERTICAL_EYE_MAPPING", value))
    {
        if (value=="LEFT_EYE_TOP_VIEWPORT")
        {
            _splitStereoVerticalEyeMapping = LEFT_EYE_TOP_VIEWPORT;
        }
        else
        if (value=="LEFT_EYE_BOTTOM_VIEWPORT")
        {
            _splitStereoVerticalEyeMapping = LEFT_EYE_BOTTOM_VIEWPORT;
        }
    }

    if (getEnvVar("OSG_SPLIT_STEREO_AUTO_ADJUST_ASPECT_RATIO", value))
    {
        if (value=="OFF")
        {
            _splitStereoAutoAdjustAspectRatio = false;
        }
        else
        if (value=="ON")
        {
            _splitStereoAutoAdjustAspectRatio = true;
        }
    }

    getEnvVar("OSG_SPLIT_STEREO_VERTICAL_SEPARATION", _splitStereoVerticalSeparation);

    getEnvVar("OSG_MAX_NUMBER_OF_GRAPHICS_CONTEXTS", _maxNumOfGraphicsContexts);

    if (getEnvVar("OSG_COMPILE_CONTEXTS", value))
    {
        if (value=="OFF")
        {
            _compileContextsHint = false;
        }
        else
        if (value=="ON")
        {
            _compileContextsHint = true;
        }
    }

    if (getEnvVar("OSG_SERIALIZE_DRAW_DISPATCH", value))
    {
        if (value=="OFF")
        {
            _serializeDrawDispatch = false;
        }
        else
        if (value=="ON")
        {
            _serializeDrawDispatch = true;
        }
    }

    if (getEnvVar("OSG_USE_SCENEVIEW_FOR_STEREO", value))
    {
        if (value=="OFF")
        {
            _useSceneViewForStereoHint = false;
        }
        else
        if (value=="ON")
        {
            _useSceneViewForStereoHint = true;
        }
    }

    getEnvVar("OSG_NUM_DATABASE_THREADS", _numDatabaseThreadsHint);

    getEnvVar("OSG_NUM_HTTP_DATABASE_THREADS", _numHttpDatabaseThreadsHint);

    getEnvVar("OSG_MULTI_SAMPLES", _numMultiSamples);

    getEnvVar("OSG_TEXTURE_POOL_SIZE", _maxTexturePoolSize);

    getEnvVar("OSG_BUFFER_OBJECT_POOL_SIZE", _maxBufferObjectPoolSize);


    {  // Read implicit buffer attachments combinations for both render and resolve mask
        const char * variable[] = {
            "OSG_IMPLICIT_BUFFER_ATTACHMENT_RENDER_MASK",
            "OSG_IMPLICIT_BUFFER_ATTACHMENT_RESOLVE_MASK",
        };

        int * mask[] = {
            &_implicitBufferAttachmentRenderMask,
            &_implicitBufferAttachmentResolveMask,
        };

        for( unsigned int n = 0; n < sizeof( variable ) / sizeof( variable[0] ); n++ )
        {
            std::string str;
            if (getEnvVar(variable[n], str))
            {
                if(str.find("OFF")!=std::string::npos) *mask[n] = 0;

                if(str.find("~DEFAULT")!=std::string::npos) *mask[n] ^= DEFAULT_IMPLICIT_BUFFER_ATTACHMENT;
                else if(str.find("DEFAULT")!=std::string::npos) *mask[n] |= DEFAULT_IMPLICIT_BUFFER_ATTACHMENT;

                if(str.find("~COLOR")!=std::string::npos) *mask[n] ^= IMPLICIT_COLOR_BUFFER_ATTACHMENT;
                else if(str.find("COLOR")!=std::string::npos) *mask[n] |= IMPLICIT_COLOR_BUFFER_ATTACHMENT;

                if(str.find("~DEPTH")!=std::string::npos) *mask[n] ^= IMPLICIT_DEPTH_BUFFER_ATTACHMENT;
                else if(str.find("DEPTH")!=std::string::npos) *mask[n] |= (int)IMPLICIT_DEPTH_BUFFER_ATTACHMENT;

                if(str.find("~STENCIL")!=std::string::npos) *mask[n] ^= (int)IMPLICIT_STENCIL_BUFFER_ATTACHMENT;
                else if(str.find("STENCIL")!=std::string::npos) *mask[n] |= (int)IMPLICIT_STENCIL_BUFFER_ATTACHMENT;
            }
        }
    }

    if (getEnvVar("OSG_GL_VERSION", value) || getEnvVar("OSG_GL_CONTEXT_VERSION", value))
    {
        _glContextVersion = value;
    }

    getEnvVar("OSG_GL_CONTEXT_FLAGS", _glContextFlags);

    getEnvVar("OSG_GL_CONTEXT_PROFILE_MASK", _glContextProfileMask);

    if (getEnvVar("OSG_SWAP_METHOD", value))
    {
        if (value=="DEFAULT")
        {
            _swapMethod = SWAP_DEFAULT;
        }
        else
        if (value=="EXCHANGE")
        {
            _swapMethod = SWAP_EXCHANGE;
        }
        else
        if (value=="COPY")
        {
            _swapMethod = SWAP_COPY;
        }
        else
        if (value=="UNDEFINED")
        {
            _swapMethod = SWAP_UNDEFINED;
        }

    }

    if (getEnvVar("OSG_SYNC_SWAP_BUFFERS", value))
    {
        if (value=="OFF")
        {
            _syncSwapBuffers = 0;
        }
        else
        if (value=="ON")
        {
            _syncSwapBuffers = 1;
        }
        else
        {
            _syncSwapBuffers = atoi(value.c_str());
        }
    }


    if (getEnvVar("OSG_VERTEX_BUFFER_HINT", value))
    {
        if (value=="VERTEX_BUFFER_OBJECT" || value=="VBO")
        {
            OSG_INFO<<"OSG_VERTEX_BUFFER_HINT set to VERTEX_BUFFER_OBJECT"<<std::endl;
            _vertexBufferHint = VERTEX_BUFFER_OBJECT;
        }
        else if (value=="VERTEX_ARRAY_OBJECT" || value=="VAO")
        {
            OSG_INFO<<"OSG_VERTEX_BUFFER_HINT set to VERTEX_ARRAY_OBJECT"<<std::endl;
            _vertexBufferHint = VERTEX_ARRAY_OBJECT;
        }
        else
        {
            OSG_INFO<<"OSG_VERTEX_BUFFER_HINT set to NO_PREFERENCE"<<std::endl;
            _vertexBufferHint = NO_PREFERENCE;
        }
    }


    if (getEnvVar("OSG_SHADER_HINT", value))
    {
        if (value=="GL2")
        {
            setShaderHint(SHADER_GL2);
        }
        else if (value=="GL3")
        {
            setShaderHint(SHADER_GL3);
        }
        else if (value=="GLES2")
        {
            setShaderHint(SHADER_GLES2);
        }
        else if (value=="GLES3")
        {
            setShaderHint(SHADER_GLES3);
        }
        else if (value=="NONE")
        {
            setShaderHint(SHADER_NONE);
        }
    }

    if (getEnvVar("OSG_TEXT_SHADER_TECHNIQUE", value))
    {
        setTextShaderTechnique(value);
    }

    if (getEnvVar("OSG_KEYSTONE", value))
    {
        if (value=="OFF")
        {
            _keystoneHint = false;
        }
        else
        if (value=="ON")
        {
            _keystoneHint = true;
        }
    }


    if (getEnvVar("OSG_KEYSTONE_FILES", value))
    {
    #if defined(WIN32) && !defined(__CYGWIN__)
        char delimitor = ';';
    #else
        char delimitor = ':';
    #endif

        std::string paths(value);
        if (!paths.empty())
        {
            std::string::size_type start = 0;
            std::string::size_type end;
            while ((end = paths.find_first_of(delimitor,start))!=std::string::npos)
            {
                _keystoneFileNames.push_back(std::string(paths,start,end-start));
                start = end+1;
            }

            std::string lastPath(paths,start,std::string::npos);
            if (!lastPath.empty())
                _keystoneFileNames.push_back(lastPath);
        }
    }

    if (getEnvVar("OSG_MENUBAR_BEHAVIOR", value))
    {
        if (value=="AUTO_HIDE")
        {
            _OSXMenubarBehavior = MENUBAR_AUTO_HIDE;
        }
        else
        if (value=="FORCE_HIDE")
        {
            _OSXMenubarBehavior = MENUBAR_FORCE_HIDE;
        }
        else
        if (value=="FORCE_SHOW")
        {
            _OSXMenubarBehavior = MENUBAR_FORCE_SHOW;
        }
    }

    int enable = 0;
    if (getEnvVar("OSG_NvOptimusEnablement", enable))
    {
        setNvOptimusEnablement(enable);
    }


    if (getEnvVar("OSG_SHADER_PIPELINE", value))
    {
        if (value=="OFF")
        {
            _shaderPipeline = false;
        }
        else
        if (value=="ON")
        {
            _shaderPipeline = true;
        }
    }


    if (getEnvVar("OSG_SHADER_PIPELINE_FILES", value))
    {
    #if defined(WIN32) && !defined(__CYGWIN__)
        char delimitor = ';';
    #else
        char delimitor = ':';
    #endif

        _shaderPipelineFiles.clear();

        std::string paths(value);
        if (!paths.empty())
        {
            std::string::size_type start = 0;
            std::string::size_type end;
            while ((end = paths.find_first_of(delimitor,start))!=std::string::npos)
            {
                _shaderPipelineFiles.push_back(std::string(paths,start,end-start));
                start = end+1;
            }

            std::string lastPath(paths,start,std::string::npos);
            if (!lastPath.empty())
                _shaderPipelineFiles.push_back(lastPath);
        }
    }

    if(getEnvVar("OSG_SHADER_PIPELINE_NUM_TEXTURE_UNITS", value))
    {
        _shaderPipelineNumTextureUnits = atoi(value.c_str());

    }
    OSG_INFO<<"_shaderPipelineNumTextureUnits = "<<_shaderPipelineNumTextureUnits<<std::endl;
}

void DisplaySettings::readCommandLine(ArgumentParser& arguments)
{
    if (_application.empty()) _application = arguments[0];

    // report the usage options.
    if (arguments.getApplicationUsage())
    {
        arguments.getApplicationUsage()->addCommandLineOption("--display <type>","MONITOR | POWERWALL | REALITY_CENTER | HEAD_MOUNTED_DISPLAY");
        arguments.getApplicationUsage()->addCommandLineOption("--stereo","Use default stereo mode which is ANAGLYPHIC if not overridden by environmental variable");
        arguments.getApplicationUsage()->addCommandLineOption("--stereo <mode>","ANAGLYPHIC | QUAD_BUFFER | HORIZONTAL_SPLIT | VERTICAL_SPLIT | LEFT_EYE | RIGHT_EYE | HORIZONTAL_INTERLACE | VERTICAL_INTERLACE | CHECKERBOARD | ON | OFF ");
        arguments.getApplicationUsage()->addCommandLineOption("--rgba","Request a RGBA color buffer visual");
        arguments.getApplicationUsage()->addCommandLineOption("--stencil","Request a stencil buffer visual");
        arguments.getApplicationUsage()->addCommandLineOption("--accum-rgb","Request a rgb accumulator buffer visual");
        arguments.getApplicationUsage()->addCommandLineOption("--accum-rgba","Request a rgb accumulator buffer visual");
        arguments.getApplicationUsage()->addCommandLineOption("--samples <num>","Request a multisample visual");
        arguments.getApplicationUsage()->addCommandLineOption("--cc","Request use of compile contexts and threads");
        arguments.getApplicationUsage()->addCommandLineOption("--serialize-draw <mode>","OFF | ON - set the serialization of draw dispatch");
        arguments.getApplicationUsage()->addCommandLineOption("--implicit-buffer-attachment-render-mask","OFF | DEFAULT | [~]COLOR | [~]DEPTH | [~]STENCIL. Substitute missing buffer attachments for render FBO");
        arguments.getApplicationUsage()->addCommandLineOption("--implicit-buffer-attachment-resolve-mask","OFF | DEFAULT | [~]COLOR | [~]DEPTH | [~]STENCIL. Substitute missing buffer attachments for resolve FBO");
        arguments.getApplicationUsage()->addCommandLineOption("--gl-version <major.minor>","Set the hint of which GL version to use when creating graphics contexts.");
        arguments.getApplicationUsage()->addCommandLineOption("--gl-flags <mask>","Set the hint of which GL flags projfile mask to use when creating graphics contexts.");
        arguments.getApplicationUsage()->addCommandLineOption("--gl-profile-mask <mask>","Set the hint of which GL context profile mask to use when creating graphics contexts.");
        arguments.getApplicationUsage()->addCommandLineOption("--swap-method <method>","DEFAULT | EXCHANGE | COPY | UNDEFINED. Select preferred swap method.");
        arguments.getApplicationUsage()->addCommandLineOption("--keystone <filename>","Specify a keystone file to be used by the viewer for keystone correction.");
        arguments.getApplicationUsage()->addCommandLineOption("--keystone-on","Set the keystone hint to true to tell the viewer to do keystone correction.");
        arguments.getApplicationUsage()->addCommandLineOption("--keystone-off","Set the keystone hint to false.");
        arguments.getApplicationUsage()->addCommandLineOption("--menubar-behavior <behavior>","Set the menubar behavior (AUTO_HIDE | FORCE_HIDE | FORCE_SHOW)");
        arguments.getApplicationUsage()->addCommandLineOption("--sync","Enable sync of swap buffers");
    }

    std::string str;
    while(arguments.read("--display",str))
    {
        if (str=="MONITOR") _displayType = MONITOR;
        else if (str=="POWERWALL") _displayType = POWERWALL;
        else if (str=="REALITY_CENTER") _displayType = REALITY_CENTER;
        else if (str=="HEAD_MOUNTED_DISPLAY") _displayType = HEAD_MOUNTED_DISPLAY;
    }

    int pos;
    while ((pos=arguments.find("--stereo"))>0)
    {
        if (arguments.match(pos+1,"ANAGLYPHIC"))            { arguments.remove(pos,2); _stereo = true;_stereoMode = ANAGLYPHIC; }
        else if (arguments.match(pos+1,"QUAD_BUFFER"))      { arguments.remove(pos,2); _stereo = true;_stereoMode = QUAD_BUFFER; }
        else if (arguments.match(pos+1,"HORIZONTAL_SPLIT")) { arguments.remove(pos,2); _stereo = true;_stereoMode = HORIZONTAL_SPLIT; }
        else if (arguments.match(pos+1,"VERTICAL_SPLIT"))   { arguments.remove(pos,2); _stereo = true;_stereoMode = VERTICAL_SPLIT; }
        else if (arguments.match(pos+1,"HORIZONTAL_INTERLACE")) { arguments.remove(pos,2); _stereo = true;_stereoMode = HORIZONTAL_INTERLACE; }
        else if (arguments.match(pos+1,"VERTICAL_INTERLACE"))   { arguments.remove(pos,2); _stereo = true;_stereoMode = VERTICAL_INTERLACE; }
        else if (arguments.match(pos+1,"CHECKERBOARD"))     { arguments.remove(pos,2); _stereo = true;_stereoMode = CHECKERBOARD; }
        else if (arguments.match(pos+1,"LEFT_EYE"))         { arguments.remove(pos,2); _stereo = true;_stereoMode = LEFT_EYE; }
        else if (arguments.match(pos+1,"RIGHT_EYE"))        { arguments.remove(pos,2); _stereo = true;_stereoMode = RIGHT_EYE; }
        else if (arguments.match(pos+1,"ON"))               { arguments.remove(pos,2); _stereo = true; }
        else if (arguments.match(pos+1,"OFF"))              { arguments.remove(pos,2); _stereo = false; }
        else                                                { arguments.remove(pos); _stereo = true; }
    }

    while (arguments.read("--rgba"))
    {
        _RGB = true;
        _minimumNumberAlphaBits = 1;
    }

    while (arguments.read("--stencil"))
    {
        _minimumNumberStencilBits = 1;
    }

    while (arguments.read("--accum-rgb"))
    {
        setMinimumNumAccumBits(8,8,8,0);
    }

    while (arguments.read("--accum-rgba"))
    {
        setMinimumNumAccumBits(8,8,8,8);
    }

    while(arguments.read("--samples",str))
    {
        _numMultiSamples = atoi(str.c_str());
    }

    while(arguments.read("--sync"))
    {
        _syncSwapBuffers = 1;
    }

    if (arguments.read("--keystone",str))
    {
        _keystoneHint = true;

        if (!_keystoneFileNames.empty()) _keystoneFileNames.clear();
        _keystoneFileNames.push_back(str);

        while(arguments.read("--keystone",str))
        {
            _keystoneFileNames.push_back(str);
        }
    }

    if (arguments.read("--keystone-on"))
    {
        _keystoneHint = true;
    }

    if (arguments.read("--keystone-off"))
    {
        _keystoneHint = false;
    }

    while(arguments.read("--cc"))
    {
        _compileContextsHint = true;
    }

    while(arguments.read("--serialize-draw",str))
    {
        if (str=="ON") _serializeDrawDispatch = true;
        else if (str=="OFF") _serializeDrawDispatch = false;
    }

    while(arguments.read("--num-db-threads",_numDatabaseThreadsHint)) {}
    while(arguments.read("--num-http-threads",_numHttpDatabaseThreadsHint)) {}

    while(arguments.read("--texture-pool-size",_maxTexturePoolSize)) {}
    while(arguments.read("--buffer-object-pool-size",_maxBufferObjectPoolSize)) {}

    {  // Read implicit buffer attachments combinations for both render and resolve mask
        const char* option[] = {
            "--implicit-buffer-attachment-render-mask",
            "--implicit-buffer-attachment-resolve-mask",
        };

        int * mask[] = {
            &_implicitBufferAttachmentRenderMask,
            &_implicitBufferAttachmentResolveMask,
        };

        for( unsigned int n = 0; n < sizeof( option ) / sizeof( option[0]); n++ )
        {
            while(arguments.read( option[n],str))
            {
                if(str.find("OFF")!=std::string::npos) *mask[n] = 0;

                if(str.find("~DEFAULT")!=std::string::npos) *mask[n] ^= DEFAULT_IMPLICIT_BUFFER_ATTACHMENT;
                else if(str.find("DEFAULT")!=std::string::npos) *mask[n] |= DEFAULT_IMPLICIT_BUFFER_ATTACHMENT;

                if(str.find("~COLOR")!=std::string::npos) *mask[n] ^= IMPLICIT_COLOR_BUFFER_ATTACHMENT;
                else if(str.find("COLOR")!=std::string::npos) *mask[n] |= IMPLICIT_COLOR_BUFFER_ATTACHMENT;

                if(str.find("~DEPTH")!=std::string::npos) *mask[n] ^= IMPLICIT_DEPTH_BUFFER_ATTACHMENT;
                else if(str.find("DEPTH")!=std::string::npos) *mask[n] |= IMPLICIT_DEPTH_BUFFER_ATTACHMENT;

                if(str.find("~STENCIL")!=std::string::npos) *mask[n] ^= IMPLICIT_STENCIL_BUFFER_ATTACHMENT;
                else if(str.find("STENCIL")!=std::string::npos) *mask[n] |= IMPLICIT_STENCIL_BUFFER_ATTACHMENT;
            }
        }
    }

    while (arguments.read("--gl-version", _glContextVersion)) {}
    while (arguments.read("--gl-flags", _glContextFlags)) {}
    while (arguments.read("--gl-profile-mask", _glContextProfileMask)) {}

    while(arguments.read("--swap-method",str))
    {
        if (str=="DEFAULT") _swapMethod = SWAP_DEFAULT;
        else if (str=="EXCHANGE") _swapMethod = SWAP_EXCHANGE;
        else if (str=="COPY") _swapMethod = SWAP_COPY;
        else if (str=="UNDEFINED") _swapMethod = SWAP_UNDEFINED;
    }

    while(arguments.read("--menubar-behavior",str))
    {
        if (str=="AUTO_HIDE") _OSXMenubarBehavior = MENUBAR_AUTO_HIDE;
        else if (str=="FORCE_HIDE") _OSXMenubarBehavior = MENUBAR_FORCE_HIDE;
        else if (str=="FORCE_SHOW") _OSXMenubarBehavior = MENUBAR_FORCE_SHOW;
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper funciotns for computing projection and view matrices of left and right eyes
//
osg::Matrixd DisplaySettings::computeLeftEyeProjectionImplementation(const osg::Matrixd& projection) const
{
    double iod = getEyeSeparation();
    double sd = getScreenDistance();
    double scale_x = 1.0;
    double scale_y = 1.0;

    if (getSplitStereoAutoAdjustAspectRatio())
    {
        switch(getStereoMode())
        {
            case(HORIZONTAL_SPLIT):
                scale_x = 2.0;
                break;
            case(VERTICAL_SPLIT):
                scale_y = 2.0;
                break;
            default:
                break;
        }
    }

    if (getDisplayType()==HEAD_MOUNTED_DISPLAY)
    {
        // head mounted display has the same projection matrix for left and right eyes.
        return osg::Matrixd::scale(scale_x,scale_y,1.0) *
               projection;
    }
    else
    {
        // all other display types assume working like a projected power wall
        // need to shjear projection matrix to account for asymetric frustum due to eye offset.
        return osg::Matrixd(1.0,0.0,0.0,0.0,
                           0.0,1.0,0.0,0.0,
                           iod/(2.0*sd),0.0,1.0,0.0,
                           0.0,0.0,0.0,1.0) *
               osg::Matrixd::scale(scale_x,scale_y,1.0) *
               projection;
    }
}

osg::Matrixd DisplaySettings::computeLeftEyeViewImplementation(const osg::Matrixd& view, double eyeSeperationScale) const
{
    double iod = getEyeSeparation();
    double es = 0.5f*iod*eyeSeperationScale;

    return view *
           osg::Matrixd(1.0,0.0,0.0,0.0,
                       0.0,1.0,0.0,0.0,
                       0.0,0.0,1.0,0.0,
                       es,0.0,0.0,1.0);
}

osg::Matrixd DisplaySettings::computeRightEyeProjectionImplementation(const osg::Matrixd& projection) const
{
    double iod = getEyeSeparation();
    double sd = getScreenDistance();
    double scale_x = 1.0;
    double scale_y = 1.0;

    if (getSplitStereoAutoAdjustAspectRatio())
    {
        switch(getStereoMode())
        {
            case(HORIZONTAL_SPLIT):
                scale_x = 2.0;
                break;
            case(VERTICAL_SPLIT):
                scale_y = 2.0;
                break;
            default:
                break;
        }
    }

    if (getDisplayType()==HEAD_MOUNTED_DISPLAY)
    {
        // head mounted display has the same projection matrix for left and right eyes.
        return osg::Matrixd::scale(scale_x,scale_y,1.0) *
               projection;
    }
    else
    {
        // all other display types assume working like a projected power wall
        // need to shjear projection matrix to account for asymetric frustum due to eye offset.
        return osg::Matrixd(1.0,0.0,0.0,0.0,
                           0.0,1.0,0.0,0.0,
                           -iod/(2.0*sd),0.0,1.0,0.0,
                           0.0,0.0,0.0,1.0) *
               osg::Matrixd::scale(scale_x,scale_y,1.0) *
               projection;
    }
}

osg::Matrixd DisplaySettings::computeRightEyeViewImplementation(const osg::Matrixd& view, double eyeSeperationScale) const
{
    double iod = getEyeSeparation();
    double es = 0.5*iod*eyeSeperationScale;

    return view *
           osg::Matrixd(1.0,0.0,0.0,0.0,
                       0.0,1.0,0.0,0.0,
                       0.0,0.0,1.0,0.0,
                       -es,0.0,0.0,1.0);
}

void DisplaySettings::setShaderHint(ShaderHint hint, bool setShaderValues)
{
    _shaderHint = hint;
    if (setShaderValues)
    {
        switch(_shaderHint)
        {
        case(SHADER_GLES3) :
            setValue("OSG_GLSL_VERSION", "#version 300 es");
            setValue("OSG_PRECISION_FLOAT", "precision highp float;");
            setValue("OSG_VARYING_IN", "in");
            setValue("OSG_VARYING_OUT", "out");
            OSG_INFO<<"DisplaySettings::SHADER_GLES3"<<std::endl;
            break;
        case(SHADER_GLES2) :
            setValue("OSG_GLSL_VERSION", "");
            setValue("OSG_PRECISION_FLOAT", "precision highp float;");
            setValue("OSG_VARYING_IN", "varying");
            setValue("OSG_VARYING_OUT", "varying");
            OSG_INFO<<"DisplaySettings::SHADER_GLES2"<<std::endl;
            break;
        case(SHADER_GL3) :
            setValue("OSG_GLSL_VERSION", "#version 330");
            setValue("OSG_PRECISION_FLOAT", "");
            setValue("OSG_VARYING_IN", "in");
            setValue("OSG_VARYING_OUT", "out");
            OSG_INFO<<"DisplaySettings::SHADER_GL3"<<std::endl;
            break;
        case(SHADER_GL2) :
            setValue("OSG_GLSL_VERSION", "");
            setValue("OSG_PRECISION_FLOAT", "");
            setValue("OSG_VARYING_IN", "varying");
            setValue("OSG_VARYING_OUT", "varying");
            OSG_INFO<<"DisplaySettings::SHADER_GL2"<<std::endl;
            break;
        case(SHADER_NONE) :
            setValue("OSG_GLSL_VERSION", "");
            setValue("OSG_PRECISION_FLOAT", "");
            setValue("OSG_VARYING_IN", "varying");
            setValue("OSG_VARYING_OUT", "varying");
            OSG_INFO<<"DisplaySettings::NONE"<<std::endl;
            break;
        }
    }
}

void DisplaySettings::setValue(const std::string& name, const std::string& value)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_valueMapMutex);

    _valueMap[name] = value;
}

bool DisplaySettings::getValue(const std::string& name, std::string& value, bool use_env_fallback) const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_valueMapMutex);

    ValueMap::iterator itr = _valueMap.find(name);
    if (itr!=_valueMap.end())
    {
        value = itr->second;
        OSG_INFO<<"DisplaySettings::getValue("<<name<<") found existing value = ["<<value<<"]"<<std::endl;
        return true;
    }

    if (!use_env_fallback) return false;

    std::string str;
    if (getEnvVar(name.c_str(), str))
    {
        OSG_INFO<<"DisplaySettings::getValue("<<name<<") found getEnvVar value = ["<<value<<"]"<<std::endl;
        _valueMap[name] = value = str;
        return true;

    }
    else
    {
        return false;
    }
}


// OSGFILE src/osg/GraphicsContext.cpp

#include <stdlib.h>

/*
#include <osg/GraphicsContext>
#include <osg/Camera>
#include <osg/View>
#include <osg/GLObjects>
#include <osg/ContextData>
#include <osg/os_utils>
*/

/*
#include <osg/FrameBufferObject>
#include <osg/Program>
#include <osg/Drawable>
#include <osg/FragmentProgram>
#include <osg/VertexProgram>
#include <osg/GLExtensions>
*/

/*
#include <OpenThreads/ReentrantMutex>
*/

//#include <osg/Notify>

#include <map>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stdio.h>

using namespace osg;

/////////////////////////////////////////////////////////////////////////////
//
// WindowSystemInterfaces
//
GraphicsContext::WindowingSystemInterfaces::WindowingSystemInterfaces()
{
}

GraphicsContext::WindowingSystemInterfaces::~WindowingSystemInterfaces()
{
}

void GraphicsContext::WindowingSystemInterfaces::addWindowingSystemInterface(GraphicsContext::WindowingSystemInterface* wsi)
{
    if (std::find(_interfaces.begin(), _interfaces.end(), wsi)==_interfaces.end())
    {
        _interfaces.push_back(wsi);
    }
}

void GraphicsContext::WindowingSystemInterfaces::removeWindowingSystemInterface(GraphicsContext::WindowingSystemInterface* wsi)
{
    Interfaces::iterator itr = std::find(_interfaces.begin(), _interfaces.end(), wsi);
    if (itr!=_interfaces.end())
    {
        _interfaces.erase(itr);
    }
}

GraphicsContext::WindowingSystemInterface* GraphicsContext::WindowingSystemInterfaces::getWindowingSystemInterface(const std::string& name)
{
    if (_interfaces.empty())
    {
        OSG_WARN<<"Warning: GraphicsContext::WindowingSystemInterfaces::getWindowingSystemInterface() failed, no interfaces available."<<std::endl;
        return 0;
    }

    if (!name.empty())
    {
        for(Interfaces::iterator itr = _interfaces.begin();
            itr != _interfaces.end();
            ++itr)
        {
            if ((*itr)->getName()==name)
            {
                return itr->get();
            }

            OSG_NOTICE<<"   tried interface "<<typeid(*itr).name()<<", name= "<<(*itr)->getName()<<std::endl;
        }

        OSG_WARN<<"Warning: GraphicsContext::WindowingSystemInterfaces::getWindowingSystemInterface() failed, no interfaces matches name : "<<name<<std::endl;
        return 0;
    }
    else
    {
        // no preference provided so just take the first available interface
        return _interfaces.front().get();
    }
}

// Use a static reference pointer to hold the window system interface.
// Wrap this within a function, in order to control the order in which
// the static pointer's constructor is executed.
osg::ref_ptr<GraphicsContext::WindowingSystemInterfaces>& GraphicsContext::getWindowingSystemInterfaces()
{
    static ref_ptr<GraphicsContext::WindowingSystemInterfaces> s_WindowingSystemInterface = new GraphicsContext::WindowingSystemInterfaces;
    return s_WindowingSystemInterface;
}

OSG_INIT_SINGLETON_PROXY(ProxyInitWindowingSystemInterfaces, GraphicsContext::getWindowingSystemInterfaces())


//  GraphicsContext static method implementations
GraphicsContext::WindowingSystemInterface* GraphicsContext::getWindowingSystemInterface(const std::string& name)
{
    return GraphicsContext::getWindowingSystemInterfaces()->getWindowingSystemInterface(name);
}

GraphicsContext* GraphicsContext::createGraphicsContext(Traits* traits)
{
    ref_ptr<GraphicsContext::WindowingSystemInterface> wsref = getWindowingSystemInterface(traits ? traits->windowingSystemPreference : "") ;
    if ( wsref.valid())
    {
        // catch any undefined values.
        if (traits) traits->setUndefinedScreenDetailsToDefaultScreen();

        return wsref->createGraphicsContext(traits);
    }
    else
        return 0;
}

GraphicsContext::ScreenIdentifier::ScreenIdentifier():
    displayNum(0),
    screenNum(0) {}

GraphicsContext::ScreenIdentifier::ScreenIdentifier(int in_screenNum):
    displayNum(0),
    screenNum(in_screenNum) {}

GraphicsContext::ScreenIdentifier::ScreenIdentifier(const std::string& in_hostName,int in_displayNum, int in_screenNum):
    hostName(in_hostName),
    displayNum(in_displayNum),
    screenNum(in_screenNum) {}

std::string GraphicsContext::ScreenIdentifier::displayName() const
{
    std::stringstream ostr;
    ostr<<hostName<<":"<<displayNum<<"."<<screenNum;
    return ostr.str();
}

void GraphicsContext::ScreenIdentifier::readDISPLAY()
{
    std::string str;
    if (getEnvVar("DISPLAY", str))
    {
        setScreenIdentifier(str);
    }
}

void GraphicsContext::ScreenIdentifier::setScreenIdentifier(const std::string& displayName)
{
    std::string::size_type colon = displayName.find_last_of(':');
    std::string::size_type point = displayName.find_last_of('.');

    // handle the case where the host name is supplied with '.' such as 127.0.0.1:0  with only DisplayNum provided
    // here the point to picks up on the .1 from the host name, rather then demarking the DisplayNum/ScreenNum as
    // no ScreenNum is provided, hence no . in the rhs of the :
    if (point!=std::string::npos &&
        colon!=std::string::npos &&
        point < colon) point = std::string::npos;

    if (colon==std::string::npos)
    {
        hostName = "";
    }
    else
    {
        hostName = displayName.substr(0,colon);
    }

    std::string::size_type startOfDisplayNum = (colon==std::string::npos) ? 0 : colon+1;
    std::string::size_type endOfDisplayNum = (point==std::string::npos) ?  displayName.size() : point;

    if (startOfDisplayNum<endOfDisplayNum)
    {
        displayNum = atoi(displayName.substr(startOfDisplayNum,endOfDisplayNum-startOfDisplayNum).c_str());
    }
    else
    {
        displayNum = -1;
    }

    if (point!=std::string::npos && point+1<displayName.size())
    {
        screenNum = atoi(displayName.substr(point+1,displayName.size()-point-1).c_str());
    }
    else
    {
        screenNum = -1;
    }

#if 0
    OSG_NOTICE<<"   hostName ["<<hostName<<"]"<<std::endl;
    OSG_NOTICE<<"   displayNum "<<displayNum<<std::endl;
    OSG_NOTICE<<"   screenNum "<<screenNum<<std::endl;
#endif
}

GraphicsContext::Traits::Traits(DisplaySettings* ds):
            x(0),
            y(0),
            width(0),
            height(0),
            windowDecoration(false),
            supportsResize(true),
            red(8),
            blue(8),
            green(8),
            alpha(0),
            depth(24),
            stencil(0),
            sampleBuffers(0),
            samples(0),
            pbuffer(false),
            quadBufferStereo(false),
            doubleBuffer(false),
            target(0),
            format(0),
            level(0),
            face(0),
            mipMapGeneration(false),
            vsync(true),
            swapGroupEnabled(false),
            swapGroup(0),
            swapBarrier(0),
            useMultiThreadedOpenGLEngine(false),
            useCursor(true),
            glContextVersion(OSG_GL_CONTEXT_VERSION),
            glContextFlags(0),
            glContextProfileMask(0),
            sharedContext(0),
            setInheritedWindowPixelFormat(false),
            overrideRedirect(false),
            swapMethod( DisplaySettings::SWAP_DEFAULT )
{
    if (ds)
    {
        alpha = ds->getMinimumNumAlphaBits();
        stencil = ds->getMinimumNumStencilBits();
        if (ds->getMultiSamples()!=0) sampleBuffers = 1;
        samples = ds->getNumMultiSamples();
        if (ds->getStereo())
        {
            switch(ds->getStereoMode())
            {
                case(osg::DisplaySettings::QUAD_BUFFER): quadBufferStereo = true; break;
                case(osg::DisplaySettings::VERTICAL_INTERLACE):
                case(osg::DisplaySettings::CHECKERBOARD):
                case(osg::DisplaySettings::HORIZONTAL_INTERLACE): stencil = 8; break;
                default: break;
            }
        }

        glContextVersion = ds->getGLContextVersion();
        glContextFlags = ds->getGLContextFlags();
        glContextProfileMask = ds->getGLContextProfileMask();

        swapMethod = ds->getSwapMethod();
    }
}

bool GraphicsContext::Traits::getContextVersion(unsigned int& major, unsigned int& minor) const
{
    if (glContextVersion.empty()) return false;

    std::istringstream istr( glContextVersion );
    unsigned char dot;
    istr >> major >> dot >> minor;

    return true;
}

#if 0
class ContextData
{
public:

    ContextData():
        _numContexts(0) {}

    unsigned int _numContexts;

    void incrementUsageCount() {  ++_numContexts; }

    void decrementUsageCount()
    {
        --_numContexts;

        OSG_INFO<<"decrementUsageCount()"<<_numContexts<<std::endl;

        if (_numContexts <= 1 && _compileContext.valid())
        {
            OSG_INFO<<"resetting compileContext "<<_compileContext.get()<<" refCount "<<_compileContext->referenceCount()<<std::endl;

            _compileContext = 0;
        }
    }

    osg::ref_ptr<osg::GraphicsContext> _compileContext;

};
#endif

unsigned int GraphicsContext::createNewContextID()
{
    return ContextData::createNewContextID();
}

unsigned int GraphicsContext::getMaxContextID()
{
    return ContextData::getMaxContextID();
}


void GraphicsContext::incrementContextIDUsageCount(unsigned int contextID)
{
    return ContextData::incrementContextIDUsageCount(contextID);
}

void GraphicsContext::decrementContextIDUsageCount(unsigned int contextID)
{
    return ContextData::decrementContextIDUsageCount(contextID);
}


void GraphicsContext::registerGraphicsContext(GraphicsContext* gc)
{
    ContextData::registerGraphicsContext(gc);
}

void GraphicsContext::unregisterGraphicsContext(GraphicsContext* gc)
{
    ContextData::unregisterGraphicsContext(gc);
}

GraphicsContext::GraphicsContexts GraphicsContext::getAllRegisteredGraphicsContexts()
{
    return ContextData::getAllRegisteredGraphicsContexts();
}

GraphicsContext::GraphicsContexts GraphicsContext::getRegisteredGraphicsContexts(unsigned int contextID)
{
    return ContextData::getRegisteredGraphicsContexts(contextID);
}

GraphicsContext* GraphicsContext::getOrCreateCompileContext(unsigned int contextID)
{
    return ContextData::getOrCreateCompileContext(contextID);
}

void GraphicsContext::setCompileContext(unsigned int contextID, GraphicsContext* gc)
{
    return ContextData::setCompileContext(contextID, gc);
}

GraphicsContext* GraphicsContext::getCompileContext(unsigned int contextID)
{
    return ContextData::getCompileContext(contextID);
}


/////////////////////////////////////////////////////////////////////////////
//
//  GraphicsContext standard method implementations
//
GraphicsContext::GraphicsContext():
    _clearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f)),
    _clearMask(0),
    _threadOfLastMakeCurrent(0),
    _lastClearTick(0),
    _defaultFboId(0)
{
    setThreadSafeRefUnref(true);
    _operationsBlock = new RefBlock;

    registerGraphicsContext(this);
}

GraphicsContext::GraphicsContext(const GraphicsContext&, const osg::CopyOp&):
    _clearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f)),
    _clearMask(0),
    _threadOfLastMakeCurrent(0),
    _lastClearTick(0),
    _defaultFboId(0)
{
    setThreadSafeRefUnref(true);
    _operationsBlock = new RefBlock;

    registerGraphicsContext(this);
}

GraphicsContext::~GraphicsContext()
{
    close(false);

    unregisterGraphicsContext(this);
}

void GraphicsContext::clear()
{
    _lastClearTick = osg::Timer::instance()->tick();

    if (_clearMask==0 || !_traits) return;

    glViewport(0, 0, _traits->width, _traits->height);
    glScissor(0, 0, _traits->width, _traits->height);

    glClearColor( _clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);

    glClear( _clearMask );
}

bool GraphicsContext::realize()
{
    if (realizeImplementation())
    {
        return true;
    }
    else
    {
        return false;
    }
}

void GraphicsContext::close(bool callCloseImplementation)
{
    OSG_INFO<<"close("<<callCloseImplementation<<")"<<this<<std::endl;

    // switch off the graphics thread...
    setGraphicsThread(0);


    bool sharedContextExists = false;

    if (_state.valid())
    {
        osg::ContextData* cd = osg::getContextData(_state->getContextID());
        if (cd && cd->getNumContexts()>1) sharedContextExists = true;
    }

    // release all the OpenGL objects in the scene graphs associated with this
    for(Cameras::iterator itr = _cameras.begin();
        itr != _cameras.end();
        ++itr)
    {
        Camera* camera = (*itr);
        if (camera)
        {
            OSG_INFO<<"Releasing GL objects for Camera="<<camera<<" _state="<<_state.get()<<std::endl;
            camera->releaseGLObjects(_state.get());
        }
    }

    if (_state.valid())
    {
        _state->releaseGLObjects();
    }

    if (callCloseImplementation && _state.valid() && isRealized())
    {
        OSG_INFO<<"Closing still viable window "<<sharedContextExists<<" _state->getContextID()="<<_state->getContextID()<<std::endl;

        if (makeCurrent())
        {
            if ( !sharedContextExists )
            {
                OSG_INFO<<"Doing delete of GL objects"<<std::endl;

                osg::deleteAllGLObjects(_state->getContextID());

                osg::flushAllDeletedGLObjects(_state->getContextID());

                OSG_INFO<<"Done delete of GL objects"<<std::endl;
            }
            else
            {
                // If the GL objects are shared with other contexts then only flush those
                // which have already been deleted

                osg::flushAllDeletedGLObjects(_state->getContextID());
            }

            releaseContext();
        }
        else
        {
            OSG_INFO<<"makeCurrent did not succeed, could not do flush/deletion of OpenGL objects."<<std::endl;
        }
    }

    if (callCloseImplementation) closeImplementation();


    // now discard any deleted deleted OpenGL objects that the are still hanging around - such as due to
    // the flushDelete*() methods not being invoked, such as when using GraphicContextEmbedded where makeCurrent
    // does not work.
    if ( !sharedContextExists && _state.valid())
    {
        OSG_INFO<<"Doing discard of deleted OpenGL objects."<<std::endl;

        osg::discardAllGLObjects(_state->getContextID());
    }

    if (_state.valid())
    {
        decrementContextIDUsageCount(_state->getContextID());

        _state = 0;
    }
}


bool GraphicsContext::makeCurrent()
{
    _threadOfLastMakeCurrent = OpenThreads::Thread::CurrentThread();

    bool result = makeCurrentImplementation();

    if (result)
    {
        // initialize extension process, not only initializes on first
        // call, will be a non-op on subsequent calls.
        getState()->initializeExtensionProcs();
    }

    return result;
}

bool GraphicsContext::makeContextCurrent(GraphicsContext* readContext)
{
    bool result = makeContextCurrentImplementation(readContext);

    if (result)
    {
        _threadOfLastMakeCurrent = OpenThreads::Thread::CurrentThread();

        // initialize extension process, not only initializes on first
        // call, will be a non-op on subsequent calls.
        getState()->initializeExtensionProcs();
    }

    return result;
}

bool GraphicsContext::releaseContext()
{
    bool result = releaseContextImplementation();

    _threadOfLastMakeCurrent = (OpenThreads::Thread*)(-1);

    return result;
}

void GraphicsContext::swapBuffers()
{
    if (isCurrent())
    {
        swapBuffersCallbackOrImplementation();
        clear();
    }
    else if (_graphicsThread.valid() &&
             _threadOfLastMakeCurrent == _graphicsThread.get())
    {
        _graphicsThread->add(new SwapBuffersOperation);
    }
    else
    {
        makeCurrent();
        swapBuffersCallbackOrImplementation();
        clear();
    }
}

void GraphicsContext::createGraphicsThread()
{
    if (!_graphicsThread)
    {
        setGraphicsThread(new GraphicsThread);

        if (_traits.valid())
        {
            _graphicsThread->setProcessorAffinity(_traits->affinity);
        }
    }
}

void GraphicsContext::setGraphicsThread(GraphicsThread* gt)
{
    if (_graphicsThread==gt) return;

    if (_graphicsThread.valid())
    {
        // need to kill the thread in some way...
        _graphicsThread->cancel();
        _graphicsThread->setParent(0);
    }

    _graphicsThread = gt;

    if (_graphicsThread.valid())
    {
        _graphicsThread->setParent(this);
    }
}

void GraphicsContext::add(Operation* operation)
{
    OSG_INFO<<"Doing add"<<std::endl;

    // acquire the lock on the operations queue to prevent anyone else for modifying it at the same time
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_operationsMutex);

    // add the operation to the end of the list
    _operations.push_back(operation);

    _operationsBlock->set(true);
}

void GraphicsContext::remove(Operation* operation)
{
    OSG_INFO<<"Doing remove operation"<<std::endl;

    // acquire the lock on the operations queue to prevent anyone else for modifying it at the same time
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_operationsMutex);

    for(GraphicsOperationQueue::iterator itr = _operations.begin();
        itr!=_operations.end();)
    {
        if ((*itr)==operation) itr = _operations.erase(itr);
        else ++itr;
    }

    if (_operations.empty())
    {
        _operationsBlock->set(false);
    }
}

void GraphicsContext::remove(const std::string& name)
{
    OSG_INFO<<"Doing remove named operation"<<std::endl;

    // acquire the lock on the operations queue to prevent anyone else for modifying it at the same time
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_operationsMutex);

    // find the remove all operations with specified name
    for(GraphicsOperationQueue::iterator itr = _operations.begin();
        itr!=_operations.end();)
    {
        if ((*itr)->getName()==name) itr = _operations.erase(itr);
        else ++itr;
    }

    if (_operations.empty())
    {
        _operationsBlock->set(false);
    }
}

void GraphicsContext::removeAllOperations()
{
    OSG_INFO<<"Doing remove all operations"<<std::endl;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_operationsMutex);
    _operations.clear();
    _operationsBlock->set(false);
}

void GraphicsContext::runOperations()
{
    // sort the cameras into order
    typedef std::vector<Camera*> CameraVector;
    CameraVector camerasCopy;
    std::copy(_cameras.begin(), _cameras.end(), std::back_inserter(camerasCopy));
    std::sort(camerasCopy.begin(), camerasCopy.end(), CameraRenderOrderSortOp());

    for(CameraVector::iterator itr = camerasCopy.begin();
        itr != camerasCopy.end();
        ++itr)
    {
        osg::Camera* camera = *itr;
        if (camera->getRenderer()) (*(camera->getRenderer()))(this);
    }

    for(GraphicsOperationQueue::iterator itr = _operations.begin();
        itr != _operations.end();
        )
    {
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_operationsMutex);
            _currentOperation = *itr;

            if (!_currentOperation->getKeep())
            {
                itr = _operations.erase(itr);

                if (_operations.empty())
                {
                    _operationsBlock->set(false);
                }
            }
            else
            {
                ++itr;
            }
        }

        if (_currentOperation.valid())
        {
            // OSG_INFO<<"Doing op "<<_currentOperation->getName()<<" "<<this<<std::endl;

            // call the graphics operation.
            (*_currentOperation)(this);

            {
                OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_operationsMutex);
                _currentOperation = 0;
            }
        }
    }
}

void GraphicsContext::addCamera(osg::Camera* camera)
{
    _cameras.push_back(camera);
}

void GraphicsContext::removeCamera(osg::Camera* camera)
{
    Cameras::iterator itr = std::find(_cameras.begin(), _cameras.end(), camera);
    if (itr != _cameras.end())
    {
        // find a set of nodes attached the camera that we are removing that isn't
        // shared by any other cameras on this GraphicsContext
        typedef std::set<Node*> NodeSet;
        NodeSet nodes;
        for(unsigned int i=0; i<camera->getNumChildren(); ++i)
        {
            nodes.insert(camera->getChild(i));
        }

        for(Cameras::iterator citr = _cameras.begin();
            citr != _cameras.end();
            ++citr)
        {
            if (citr != itr)
            {
                osg::Camera* otherCamera = *citr;
                for(unsigned int i=0; i<otherCamera->getNumChildren(); ++i)
                {
                    NodeSet::iterator nitr = nodes.find(otherCamera->getChild(i));
                    if (nitr != nodes.end()) nodes.erase(nitr);
                }
            }
        }

        // now release the GLobjects associated with these non shared nodes
        for(NodeSet::iterator nitr = nodes.begin();
            nitr != nodes.end();
            ++nitr)
        {
            (*nitr)->releaseGLObjects(_state.get());
        }

        // release the context of the any RenderingCache that the Camera has.
        if (camera->getRenderingCache())
        {
            camera->getRenderingCache()->releaseGLObjects(_state.get());
        }

        _cameras.erase(itr);

    }
}

void GraphicsContext::resizedImplementation(int x, int y, int width, int height)
{
    std::set<osg::Viewport*> processedViewports;

    if (!_traits) return;

    double widthChangeRatio = double(width) / double(_traits->width);
    double heigtChangeRatio = double(height) / double(_traits->height);
    double aspectRatioChange = widthChangeRatio / heigtChangeRatio;


    for(Cameras::iterator itr = _cameras.begin();
        itr != _cameras.end();
        ++itr)
    {
        Camera* camera = (*itr);

        // resize doesn't affect Cameras set up with FBO's.
        if (camera->getRenderTargetImplementation()==osg::Camera::FRAME_BUFFER_OBJECT) continue;

        Viewport* viewport = camera->getViewport();
        if (viewport)
        {
            // avoid processing a shared viewport twice
            if (processedViewports.count(viewport)==0)
            {
                processedViewports.insert(viewport);

                if (viewport->x()==0 && viewport->y()==0 &&
                    viewport->width()>=_traits->width && viewport->height()>=_traits->height)
                {
                    viewport->setViewport(0,0,width,height);
                }
                else
                {
                    viewport->x() = static_cast<osg::Viewport::value_type>(double(viewport->x())*widthChangeRatio);
                    viewport->y() = static_cast<osg::Viewport::value_type>(double(viewport->y())*heigtChangeRatio);
                    viewport->width() = static_cast<osg::Viewport::value_type>(double(viewport->width())*widthChangeRatio);
                    viewport->height() = static_cast<osg::Viewport::value_type>(double(viewport->height())*heigtChangeRatio);
                }
            }
        }

        // if aspect ratio adjusted change the project matrix to suit.
        if (aspectRatioChange != 1.0)
        {
            osg::View* view = camera->getView();
            osg::View::Slave* slave = view ? view->findSlaveForCamera(camera) : 0;


            if (slave)
            {
                if (camera->getReferenceFrame()==osg::Transform::RELATIVE_RF)
                {
                    switch(view->getCamera()->getProjectionResizePolicy())
                    {
                        case(osg::Camera::HORIZONTAL): slave->_projectionOffset *= osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0); break;
                        case(osg::Camera::VERTICAL): slave->_projectionOffset *= osg::Matrix::scale(1.0, aspectRatioChange,1.0); break;
                        default: break;
                    }
                }
                else
                {
                    switch(camera->getProjectionResizePolicy())
                    {
                        case(osg::Camera::HORIZONTAL): camera->getProjectionMatrix() *= osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0); break;
                        case(osg::Camera::VERTICAL): camera->getProjectionMatrix() *= osg::Matrix::scale(1.0, aspectRatioChange,1.0); break;
                        default: break;
                    }
                }
            }
            else
            {
                Camera::ProjectionResizePolicy policy = view ? view->getCamera()->getProjectionResizePolicy() : camera->getProjectionResizePolicy();
                switch(policy)
                {
                    case(osg::Camera::HORIZONTAL): camera->getProjectionMatrix() *= osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0); break;
                    case(osg::Camera::VERTICAL): camera->getProjectionMatrix() *= osg::Matrix::scale(1.0, aspectRatioChange,1.0); break;
                    default: break;
                }

                osg::Camera* master = view ? view->getCamera() : 0;
                if (view && camera==master)
                {
                    for(unsigned int i=0; i<view->getNumSlaves(); ++i)
                    {
                        osg::View::Slave& child = view->getSlave(i);
                        if (child._camera.valid() && child._camera->getReferenceFrame()==osg::Transform::RELATIVE_RF)
                        {
                            // scale the slaves by the inverse of the change that has been applied to master, to avoid them be
                            // scaled twice (such as when both master and slave are on the same GraphicsContexts) or by the wrong scale
                            // when master and slave are on different GraphicsContexts.
                            switch(policy)
                            {
                                case(osg::Camera::HORIZONTAL): child._projectionOffset *= osg::Matrix::scale(aspectRatioChange,1.0,1.0); break;
                                case(osg::Camera::VERTICAL): child._projectionOffset *= osg::Matrix::scale(1.0, 1.0/aspectRatioChange,1.0); break;
                                default: break;
                            }
                        }
                    }
                }


            }

        }

    }

    _traits->x = x;
    _traits->y = y;
    _traits->width = width;
    _traits->height = height;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SyncSwapBuffersCallback
//
SyncSwapBuffersCallback::SyncSwapBuffersCallback()
{
    OSG_INFO<<"Created SyncSwapBuffersCallback."<<std::endl;
    _previousSync = 0;
}

void SyncSwapBuffersCallback::swapBuffersImplementation(osg::GraphicsContext* gc)
{
    // OSG_NOTICE<<"Before swap - place to do swap ready sync"<<std::endl;
    gc->swapBuffersImplementation();
    //glFinish();

    GLExtensions* ext = gc->getState()->get<GLExtensions>();

    if (ext->glClientWaitSync)
    {
        if (_previousSync)
        {
            unsigned int num_seconds = 1;
            GLuint64 timeout = num_seconds * ((GLuint64)1000 * 1000 * 1000);
            ext->glClientWaitSync(_previousSync, 0, timeout);
            ext->glDeleteSync(_previousSync);
        }

        _previousSync = ext->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    //gc->getState()->checkGLErrors("after glWaitSync");

    //OSG_NOTICE<<"After swap"<<std::endl;
}

