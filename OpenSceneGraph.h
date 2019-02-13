
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

