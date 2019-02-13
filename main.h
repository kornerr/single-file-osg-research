
#ifndef OGS_H
#define OGS_H

#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

#include <osg/Camera>



namespace ogs
{

namespace format
{


}

namespace log
{


}

namespace render
{

//! Create graphics context for desktop: linux, macos, windows.
osg::GraphicsContext *createGraphicsContext(
    const std::string &title,
    int x,
    int y,
    int width,
    int height
) {
    // Traits combine configuration parameters.
    osg::GraphicsContext::Traits *traits =
        new osg::GraphicsContext::Traits;
    // Geometry.
    traits->x = x;
    traits->y = y;
    traits->width = width;
    traits->height = height;
    // Title.
    traits->windowName = title;
    // Window borders.
    traits->windowDecoration = true;
    // Double buffer (simply put, it's a flicker fix).
    traits->doubleBuffer = true;

    return osg::GraphicsContext::createGraphicsContext(traits);
}
// Configure camera with common defaults.
void setupCamera(
    osg::Camera *cam,
    osg::GraphicsContext *gc,
    double fovy,
    int width,
    int height
) {
    // Provide GC.
    cam->setGraphicsContext(gc);
    // Viewport must have the same size.
    cam->setViewport(new osg::Viewport(0, 0, width, height));
    // Clear depth and color buffers each frame.
    cam->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    // Aspect ratio.
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    // Configure projection.
    cam->setProjectionMatrixAsPerspective(fovy, aspect, 1, 1000);
}

}

namespace main
{

class Application
{
    public:
        Application(const std::string &name)
        {

            this->setupRendering();
            
        }
        ~Application()
        {

            this->tearRenderingDown();
            
        }

    public:
        void frame()
        {
            this->viewer->frame();
        }
    public:
        void run()
        {
            while (!this->viewer->done())
            {
                this->frame();
            }
        }
    public:
        void setupWindow(
            const std::string &title,
            int x,
            int y,
            int width,
            int height
        ) {
            osg::GraphicsContext *gc =
                render::createGraphicsContext(title, x, y, width, height);
            // Configure viewer's camera with FOVY and window size.
            osg::Camera *cam = this->viewer->getCamera();
            render::setupCamera(cam, gc, 30, width, height);
        }

    private:
        osgViewer::Viewer *viewer;
        void setupRendering()
        {
            // Create OpenSceneGraph viewer.
            this->viewer = new osgViewer::Viewer;
            // Use single thread: CRITICAL for mobile and web because
            // there were are issues with multiple threads for OpenSceneGraph there.
            this->viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
            // Create manipulator: CRITICAL for mobile and web to focus on the
            // nodes in the scene.
            this->viewer->setCameraManipulator(new osgGA::TrackballManipulator);
        }
        void tearRenderingDown()
        {
            delete this->viewer;
        }
};

const auto EXAMPLE_TITLE = "OGS-01: Empty screen";

struct Example
{
    Application *app;

    typedef std::map<std::string, std::string> Parameters;

    Example(const Parameters &parameters)
    {
        this->app = new Application(EXAMPLE_TITLE);

    }
    ~Example()
    {

        delete this->app;
    }

};

}

} // namespace ogs

#endif // OGS_H

