/* This proram is part of Qt examples.
   Copyright 2015-2022 by Chun-Ming Su
   E-Mail: sokunmin@gmail.com
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
   
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
