
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

