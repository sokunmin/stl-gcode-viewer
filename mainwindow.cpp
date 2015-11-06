#include "mainwindow.h"
#include "openglscene.h"

#include <QtGui>
#include <QGLWidget>
#include <QtWidgets>
#include <QDesktopWidget>

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

//=========================================
MainWindow::MainWindow()
{
    GraphicsView view;
    view.setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view.setScene(new OpenGLScene);
    view.show();
    view.resize(WIDTH, HEIGHT);
    view.move(10, 10);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->addWidget(view);
    mainWidget->setLayout(layout);

    setCentralWidget(mainWidget);


    //Right Dock Widget
    QDockWidget *dock = new QDockWidget(tr(""), this);
    dock->setAllowedAreas(Qt::RightDockWidgetArea);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_LogBrowser = new QTextBrowser(this);

    dock->setWidget(m_LogBrowser);
    addDockWidget(Qt::RightDockWidgetArea, dock);


}

MainWindow::~MainWindow()
{
}
