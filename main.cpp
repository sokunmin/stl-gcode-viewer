#include "openglscene.h"

#include <QtGui>
#include <QGLWidget>
#include <QtWidgets>

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView()
    {
        setWindowTitle(tr("3D Model Viewer"));
        setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform); //added
    }

protected:
    void resizeEvent(QResizeEvent *event) {
        if (scene())
            scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
        QGraphicsView::resizeEvent(event);
    }
};

const int WIDTH = 1024;
const int HEIGHT = 768;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    GraphicsView view;
    view.setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view.setScene(new OpenGLScene);
    view.show();    
    view.resize(WIDTH, HEIGHT);
    view.move(10, 10);
    return app.exec();
}
