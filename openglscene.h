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
   
#ifndef OPENGLSCENE_H
#define OPENGLSCENE_H

#include "point3d.h"

#include <QGraphicsScene>
#include <QLabel>
#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>
#include <QtGui/QQuaternion>
#include <QPointF>
#include <QTime>
#include <QGLShaderProgram>

#ifndef QT_NO_CONCURRENT
#include <QFutureWatcher>
#endif

//! ============ Camera Config ============
#define QUATERNION_CAMERA
//! =======================================

QT_BEGIN_NAMESPACE
class Model;
class TrackBall;
class QHBoxLayout;
class QSlider;
QT_END_NAMESPACE

class OpenGLScene : public QGraphicsScene
{
    Q_OBJECT

public:
    OpenGLScene();
    ~OpenGLScene();

    void drawBackground(QPainter *painter, const QRectF &rect);

public slots:
    void enableWireframe(bool enabled);
    void enableNormals(bool enabled);
    void enableGCodeMotion(bool enabled);
    void enableGCodeLines(bool enabled);
    void setModelColor();
    void setBackgroundColor();
    void loadModel();
    void loadModel(const QString &filePath);
    void modelLoaded();
    //model control slots
    void translateX(int value);
    void translateY(int value);
    void translateZ(int value);
    void rotateX(int value);
    void rotateY(int value);
    void rotateZ(int value);
    void scale(double value);
    void gcodeLayers(int layers);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);

private:
    QDialog *createDialog(const QString &windowTitle) const;    
    QSlider *createSlider(int rangeMax, const char *setterSlot);
    QHBoxLayout * createSpinBox(QString label, int rangeFrom, int rangeTo, const char *member);
    QHBoxLayout * createDoubleSpinBox(QString label, double rangeFrom, double rangeTo, double singleStep, const char *member);
    void setModel(Model *model);

    bool m_wireframeEnabled;
    bool m_normalsEnabled;
    bool m_gcodeMotionEnabled;
    bool m_gcodeLinesEnabled;

    QColor m_modelColor;
    QColor m_backgroundColor;

    Model *m_model;

    QLabel *m_labels[4];
    QSlider * m_slider;
    QWidget *m_modelButton;

    QGraphicsRectItem *m_lightItem;

#ifndef QT_NO_CONCURRENT
    QFutureWatcher<Model *> m_modelLoader;
#endif

    //======= added ==========


#ifdef QUATERNION_CAMERA
    int m_distExp;
    TrackBall *m_trackBall;
#else
    //    float m_distance;
    //    Point3d m_rotation;
    QMatrix4x4 m_matrixView;
    QMatrix4x4 m_matrixModel;
    QMatrix4x4 m_matrixModelView;    // = matrixView * matrixModel
    QMatrix4x4 m_matrixProjection;

    float m_cameraAngleX;
    float m_cameraAngleY;
    float m_cameraDistance;
#endif

    void initGL();
    void initLights();
    void drawAxis();
    void drawGrid();
    void drawBox();
    void generateTube(QVector3D p1, QVector3D p2, QVector3D p3, float radius = 2, bool saveRearVector = false);
    void drawTube(/*QVector3D p1, QVector3D p2, float radius*/);
    QPointF pixelPosToViewPos(const QPointF& p);

    // Test
    GLuint vboId;
    QVector<QVector3D> m_tubeVertices;
    QVector<int> m_tubeIndices;
    QVector<QVector3D> m_tubeNormals;
    QVector<QVector3D> m_prevFacet;
};

#endif
