
//#include "OpenThreads.h"
#include "OpenSceneGraph.h"

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

