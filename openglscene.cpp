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
   
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
//#include <GL/freeglut.h>


#include "openglscene.h"
#include "model.h"
#include "trackball.h"

#include <QtGui>
#include <QtOpenGL>
#include <QtWidgets>
#include <QtCore>
#include <QtConcurrent/QtConcurrent>


#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

//#define TEST_2
#define VectorOutput(m, v) qDebug() << QString(QString(m) + "x(%1), y(%2), z(%3)").arg(v.x()).arg(v.y()).arg(v.z());

const float AXIS_SIZE = 15.f;
const float GRID_STEP = 10.f;
const float GRID_SIZE = 100.f;

const float CAMERA_DISTANCE = 16.0f;
const float DEG2RAD         = 3.141593f / 180;

static Model *loadModel(const QString &filePath)
{
    return new Model(filePath);
}

//========================================================
OpenGLScene::OpenGLScene()
    : m_wireframeEnabled(false)
    , m_normalsEnabled(false)
    , m_gcodeMotionEnabled(true)
    , m_modelColor(153, 255, 0)
    , m_backgroundColor(233,240,250)
    , m_model(0)
//    , m_distance(1.4f)
{
#ifdef QUATERNION_CAMERA
    m_distExp = 6000.f;
    m_trackBall = new TrackBall(0.0f, QVector3D(0,1,0));
#else
    m_cameraAngleX = m_cameraAngleY = 0;
    m_cameraDistance = CAMERA_DISTANCE;
#endif
    // ================ Controls dialog ================
    QWidget *controls = createDialog(tr("Controls"));

    m_modelButton = new QPushButton(tr("Load model"));
    connect(m_modelButton, SIGNAL(clicked()), this, SLOT(loadModel()));
#ifndef QT_NO_CONCURRENT
    connect(&m_modelLoader, SIGNAL(finished()), this, SLOT(modelLoaded()));
#endif
    controls->layout()->addWidget(m_modelButton);

    QCheckBox *wireframe = new QCheckBox(tr("Render as wireframe"));
    connect(wireframe, SIGNAL(toggled(bool)), this, SLOT(enableWireframe(bool)));
    controls->layout()->addWidget(wireframe);

    QCheckBox *normals = new QCheckBox(tr("Display normals vectors"));
    connect(normals, SIGNAL(toggled(bool)), this, SLOT(enableNormals(bool)));
    controls->layout()->addWidget(normals);

    QCheckBox *gcodeMotion = new QCheckBox(tr("Display GCode Motion"));
    gcodeMotion->setChecked(true);
    connect(gcodeMotion, SIGNAL(toggled(bool)), this, SLOT(enableGCodeMotion(bool)));
    controls->layout()->addWidget(gcodeMotion);

    QCheckBox *linesOnly = new QCheckBox(tr("Display G-code paths"));
    linesOnly->setChecked(true);
    connect(linesOnly, SIGNAL(toggled(bool)), this, SLOT(enableGCodeLines(bool)));
    controls->layout()->addWidget(linesOnly);

    QPushButton *colorButton = new QPushButton(tr("Choose model color"));
    connect(colorButton, SIGNAL(clicked()), this, SLOT(setModelColor()));
    controls->layout()->addWidget(colorButton);

    QPushButton *backgroundButton = new QPushButton(tr("Choose background color"));
    connect(backgroundButton, SIGNAL(clicked()), this, SLOT(setBackgroundColor()));
    controls->layout()->addWidget(backgroundButton);

    // ================= Model info Dialog ===================
    QWidget *statistics = createDialog(tr("Model info"));
    statistics->layout()->setMargin(20);

    for (int i = 0; i < 4; ++i) {
        m_labels[i] = new QLabel;
        statistics->layout()->addWidget(m_labels[i]);
    }

    QGroupBox *posGroupBox = new QGroupBox(tr("Translate"));
    QVBoxLayout *posVBox = new QVBoxLayout();
    QHBoxLayout *posXBox = createSpinBox(tr(" X "), -300, 300, SLOT(translateX(int)));
    QHBoxLayout *posYBox = createSpinBox(tr(" Y "), -300, 300, SLOT(translateY(int)));
    QHBoxLayout *posZBox = createSpinBox(tr(" Z "), -300, 300, SLOT(translateZ(int)));
    posVBox->addLayout(posXBox);
    posVBox->addLayout(posYBox);
    posVBox->addLayout(posZBox);
    posGroupBox->setLayout(posVBox);

    QGroupBox *rotGroupBox = new QGroupBox(tr("Rotate"));
    QVBoxLayout *rotVBox = new QVBoxLayout();
    QHBoxLayout *pitchBox = createSpinBox(tr(" Pitch "), -359, 359, SLOT(rotateX(int)));
    QHBoxLayout *yawBox   = createSpinBox(tr(" Yaw ")  , -359, 359, SLOT(rotateY(int)));
    QHBoxLayout *rollBox  = createSpinBox(tr(" Roll ") , -359, 359, SLOT(rotateZ(int)));
    rotVBox->addLayout(pitchBox);
    rotVBox->addLayout(yawBox);
    rotVBox->addLayout(rollBox);
    rotGroupBox->setLayout(rotVBox);

    QGroupBox *scaleGroupBox = new QGroupBox(tr("Scale"));
    QVBoxLayout *scaleVBox = new QVBoxLayout();
    QHBoxLayout *scaleBox = createDoubleSpinBox(tr(" Ratio "), 0.0, 5.0, 0.01, SLOT(scale(double)));
    scaleVBox->addLayout(scaleBox);
    scaleGroupBox->setLayout(scaleVBox);

    statistics->layout()->addWidget(posGroupBox);
    statistics->layout()->addWidget(rotGroupBox);
    statistics->layout()->addWidget(scaleGroupBox);


    // ================= Scrollbar Dialog ===================
    QWidget *gcodeSlider = createDialog(tr("G-Code Slider"));
    m_slider = createSlider(0, SLOT(gcodeLayers(int)));
    gcodeSlider->layout()->addWidget(m_slider);
    // ================= Merge dialogs ==================
    QWidget *widgets[] = { controls, statistics, gcodeSlider };

    for (uint i = 0; i < sizeof(widgets) / sizeof(*widgets); ++i) {
        QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(0, Qt::Dialog);
        proxy->setWidget(widgets[i]);
        addItem(proxy);
    }

    QPointF pos(10, 10);
    qDebug() << "QGraphicsItem size() => " << items().size();
    foreach (QGraphicsItem *item, items()) {
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

        const QRectF rect = item->boundingRect();
        item->setPos(pos.x() - rect.x(), pos.y() - rect.y());
        pos += QPointF(0, 10 + rect.height());
    }

    QRadialGradient gradient(40, 40, 40, 40, 40);
    gradient.setColorAt(0.2, Qt::yellow);
    gradient.setColorAt(1, Qt::transparent);

    m_lightItem = new QGraphicsRectItem(0, 0, 80, 80);
    m_lightItem->setPen(Qt::NoPen);
    m_lightItem->setBrush(gradient);
    m_lightItem->setFlag(QGraphicsItem::ItemIsMovable);
    m_lightItem->setPos(800, 200);
    addItem(m_lightItem);

//    initGL();

    loadModel(QLatin1String("cube.obj"));



#ifdef TEST_2
    generateTube(QVector3D(49.763, 20.3, 48.044), QVector3D(49.913, 20.3, 48.044), QVector3D(50.075, 20.3, 48.044), 3, true);
    m_tubeIndices.resize(m_tubeVertices.size());
    for (int i=0; i<m_tubeVertices.size(); i++) {
        m_tubeIndices[i] = i;
        qDebug() << QString("tube vertice => x(%1), y(%2), z(%3)")
                    .arg(m_tubeVertices.at(i).x())
                    .arg(m_tubeVertices.at(i).y())
                    .arg(m_tubeVertices.at(i).z());
    }

    //calculate normals of each face
    int size = m_tubeVertices.size();
    m_tubeNormals.resize(size);
    for (int i = 0; i < m_tubeIndices.size(); i += 3) {
        const QVector3D a = m_tubeVertices.at(m_tubeIndices.at(i));
        const QVector3D b = m_tubeVertices.at(m_tubeIndices.at(i+1));
        const QVector3D c = m_tubeVertices.at(m_tubeIndices.at(i+2));

        const QVector3D normal = QVector3D::crossProduct(b - a, c - a).normalized();

        for (int j = 0; j < 3; ++j)
            m_tubeNormals[m_tubeIndices.at(i + j)] += normal;
    }
#endif
/*
    for (int i=0; i<m_tubeNormals.size(); i++) {
        qDebug() << QString("m_tubeNormals => x(%1), y(%2), z(%3)").arg(m_tubeNormals.at(i).x()).arg(m_tubeNormals.at(i).y()).arg(m_tubeNormals.at(i).z());
    }

    qDebug() << "===> end of creation";

    // Generate 2 VBOs
    glGenBuffersARB(1, &vboId);

    // Transfer vertex data to VBO 0
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(m_tubeVertices.data()) + sizeof(m_tubeNormals.data()), 0, GL_STATIC_DRAW_ARB);
*/
}

OpenGLScene::~OpenGLScene()
{
//    glDeleteBuffersARB(1, &vboId);
}

QDialog *OpenGLScene::createDialog(const QString &windowTitle) const
{
    QDialog *dialog = new QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    dialog->setWindowOpacity(0.8);
    dialog->setWindowTitle(windowTitle);
    dialog->setLayout(new QVBoxLayout);

    return dialog;
}


QSlider *OpenGLScene::createSlider(int rangeMax, const char *setterSlot)
{
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, rangeMax);
    slider->setSingleStep(1);
    slider->setPageStep(1);
    slider->setTickInterval(1);
    slider->setTickPosition(QSlider::TicksRight);
    connect(slider, SIGNAL(valueChanged(int)), this, setterSlot);
    return slider;
}

QHBoxLayout * OpenGLScene::createSpinBox(QString label,int rangeFrom, int rangeTo, const char *member)
{
    QLabel * spinLabel = new QLabel(label);

    QSpinBox *spinBox = new QSpinBox();
    spinBox->setRange(rangeFrom, rangeTo);
    connect(spinBox, SIGNAL(valueChanged(int)), this, member);

    QHBoxLayout * layout = new QHBoxLayout;
    layout->addWidget(spinLabel);
    layout->addWidget(spinBox);

    return layout;
}

QHBoxLayout * OpenGLScene::createDoubleSpinBox(QString label, double rangeFrom, double rangeTo, double singleStep, const char *member)
{
    QLabel * spinLabel = new QLabel(label);

    QDoubleSpinBox *spinBox = new QDoubleSpinBox();
    spinBox->setRange(rangeFrom, rangeTo);
    spinBox->setSingleStep(singleStep);
    spinBox->setValue(1.0f);
    connect(spinBox, SIGNAL(valueChanged(double)), this, member);

    QHBoxLayout * layout = new QHBoxLayout;
    layout->addWidget(spinLabel);
    layout->addWidget(spinBox);

    return layout;
}

void OpenGLScene::initGL()
{
    glShadeModel(GL_SMOOTH);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment

    // enable /disable features
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // enable /disable features
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_LEQUAL);

    // track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void OpenGLScene::initLights()
{

}

// calc perpendicular = http://math.stackexchange.com/questions/995659/given-two-points-find-another-point-a-perpendicular-distance-away-from-the-midp
//http://stackoverflow.com/questions/7586063/how-to-calculate-the-angle-between-a-line-and-the-horizontal-axis

/**         d2(=<p2, p3>)
     s2┌-----------k1
     p3├---------/p2|
     s1└--------/ | |
              k2| | |  d1(=<p1, p2>)
              / | | |
             /  └-┴-┘
         d3 /  f2 p1 f1
*/

void OpenGLScene::generateTube(QVector3D p1, QVector3D p2, QVector3D p3, float radius, bool saveRearVector)
{
    // calculcate half-angle vector.
    QVector3D d1Vector = p1 - p2;
    QVector3D d2Vector = p3 - p2;

    //calculate perpendicular vector for p1.
    float ratio = radius / sqrt( pow(p1.z() - p2.z(), 2) + pow(p2.x() - p1.x(), 2));
    QVector3D p1p2VertVector = QVector3D((p1.z() - p2.z()) * ratio, p1.y(), (p2.x() - p1.x()) * ratio);

    float cosAngle = QVector3D::dotProduct(d1Vector, d2Vector) /
                     ((sqrt(pow(d1Vector.x(), 2) + pow(d1Vector.z(), 2))) *
                      (sqrt(pow(d2Vector.x(), 2) + pow(d2Vector.z(), 2))));

    float sinHalfAngle = sqrt((1 - cosAngle) / 2);
    float d3Radius = radius / sinHalfAngle;

    float d1Length = (sqrt(pow(d1Vector.x(), 2) + pow(d1Vector.z(), 2)));
    float d2Length = (sqrt(pow(d2Vector.x(), 2) + pow(d2Vector.z(), 2)));

    if (d1Length)
        d1Vector /= d1Length;

    if (d2Length)
        d2Vector /= d2Length;

    QVector3D d3Vector = QVector3D((d1Vector.x() + d2Vector.x())/2, d1Vector.y(), (d1Vector.z() + d2Vector.z()) /2 );
    float d3Length = (sqrt(pow(d3Vector.x(), 2) + pow(d3Vector.z(), 2)));
    if (d3Length)
        d3Vector /= d3Length;

    float kValueDirection = d1Vector.x() * d2Vector.z() - d2Vector.x() * d1Vector.z();
    QVector3D k1, k2;
    if (kValueDirection > 0) {
        k1 = QVector3D(p2.x() - d3Radius * d3Vector.x(), p2.y(), p2.z() - d3Radius * d3Vector.z());
        k2 = QVector3D(p2.x() + d3Radius * d3Vector.x(), p2.y(), p2.z() + d3Radius * d3Vector.z());

    } else if (kValueDirection < 0) {
        k1 = QVector3D(p2.x() + d3Radius * d3Vector.x(), p2.y(), p2.z() + d3Radius * d3Vector.z());
        k2 = QVector3D(p2.x() - d3Radius * d3Vector.x(), p2.y(), p2.z() - d3Radius * d3Vector.z());
    } else {
        //This could be parallel.
        k1 = QVector3D(p2.x() + p1p2VertVector.x(), p2.y(), p2.z() + p1p2VertVector.z());
        k2 = QVector3D(p2.x() - p1p2VertVector.x(), p2.y(), p2.z() - p1p2VertVector.z());
    }


    qDebug() << "========================================";
    qDebug() << QString("cosAngle => %1").arg(cosAngle);
    qDebug() << QString("sinHalfAngle => %1").arg(sinHalfAngle);
    qDebug() << QString("d3Radius => %1").arg(d3Radius);
    qDebug() << QString("d1UnitVector => %1").arg(d1Length);
    qDebug() << QString("d2UnitVector => %1").arg(d2Length);
    qDebug() << QString("d1Vector => x(%1), y(%2), z(%3)").arg(d1Vector.x()).arg(d1Vector.y()).arg(d1Vector.z());
    qDebug() << QString("d2Vector => x(%1), y(%2), z(%3)").arg(d2Vector.x()).arg(d2Vector.y()).arg(d2Vector.z());
    qDebug() << QString("d3Vector => x(%1), y(%2), z(%3)").arg(d3Vector.x()).arg(d3Vector.y()).arg(d3Vector.z());
    qDebug() << QString("k1 => x(%1), y(%2), z(%3)").arg(k1.x()).arg(k1.y()).arg(k1.z());
    qDebug() << QString("k2 => x(%1), y(%2), z(%3)").arg(k2.x()).arg(k2.y()).arg(k2.z());
    qDebug() << "========================================";
    QVector3D f1 = QVector3D(p1.x() + p1p2VertVector.x(), p1.y(), p1.z() + p1p2VertVector.z());
    QVector3D f2 = QVector3D(p1.x() - p1p2VertVector.x(), p1.y(), p1.z() - p1p2VertVector.z());

    QVector3D p1Vert1 = QVector3D(p1.x(), p1.y() + radius, p1.z());
    QVector3D p1Vert2 = QVector3D(p1.x(), p1.y() - radius, p1.z());

    QVector3D p2Vert1 = QVector3D(p2.x(), p2.y() + radius, p2.z());
    QVector3D p2Vert2 = QVector3D(p2.x(), p2.y() - radius, p2.z());

    //Save rhombus facet of p1
    m_tubeVertices.push_back(f1);
    m_tubeVertices.push_back(p1Vert1);
    m_tubeVertices.push_back(f2);

    m_tubeVertices.push_back(f1);
    m_tubeVertices.push_back(f2);
    m_tubeVertices.push_back(p1Vert2);

    //Save triangles of the tube.
    m_tubeVertices.push_back(f1);
    m_tubeVertices.push_back(k1);
    m_tubeVertices.push_back(p2Vert1);

    m_tubeVertices.push_back(f1);
    m_tubeVertices.push_back(p2Vert1);
    m_tubeVertices.push_back(p1Vert1);

    m_tubeVertices.push_back(f2);
    m_tubeVertices.push_back(k2);
    m_tubeVertices.push_back(p1Vert1);

    m_tubeVertices.push_back(k2);
    m_tubeVertices.push_back(p2Vert1);
    m_tubeVertices.push_back(p1Vert1);

    m_tubeVertices.push_back(f1);
    m_tubeVertices.push_back(p2Vert2);
    m_tubeVertices.push_back(k1);

    m_tubeVertices.push_back(f1);
    m_tubeVertices.push_back(p1Vert2);
    m_tubeVertices.push_back(p2Vert2);

    m_tubeVertices.push_back(f2);
    m_tubeVertices.push_back(k2);
    m_tubeVertices.push_back(p2Vert2);

    m_tubeVertices.push_back(f2);
    m_tubeVertices.push_back(p2Vert2);
    m_tubeVertices.push_back(p1Vert2);

    //Save previous rhombus facet, including k1, k2, p2Vertical1, p2Vertical2
    m_prevFacet.push_back(k1);
    m_prevFacet.push_back(p2Vert1);
    m_prevFacet.push_back(k2);

    m_prevFacet.push_back(k1);
    m_prevFacet.push_back(k2);
    m_prevFacet.push_back(p2Vert2);

    if (saveRearVector) {
        float ratio2 = radius / sqrt( pow(p2.z() - p3.z(), 2) + pow(p3.x() - p2.x(), 2));
        QVector3D p2p3VertVector = QVector3D((p2.z() - p3.z()) * ratio2, p2.y(), (p3.x() - p2.x()) * ratio2);
        QVector3D s1 = QVector3D(p3.x() + p2p3VertVector.x(), p3.y(), p3.z() + p2p3VertVector.z());
        QVector3D s2 = QVector3D(p3.x() - p2p3VertVector.x(), p3.y(), p3.z() - p2p3VertVector.z());

        QVector3D p3Vert1 = QVector3D(p3.x(), p3.y() + radius, p3.z());
        QVector3D p3Vert2 = QVector3D(p3.x(), p3.y() - radius, p3.z());

        //save triangles of tube.
        m_tubeVertices.push_back(s2);
        m_tubeVertices.push_back(k2);
        m_tubeVertices.push_back(p2Vert1);

        m_tubeVertices.push_back(s2);
        m_tubeVertices.push_back(p2Vert1);
        m_tubeVertices.push_back(p3Vert1);

        m_tubeVertices.push_back(s2);
        m_tubeVertices.push_back(p3Vert2);
        m_tubeVertices.push_back(k2);

        m_tubeVertices.push_back(k2);
        m_tubeVertices.push_back(p3Vert2);
        m_tubeVertices.push_back(p2Vert2);

        m_tubeVertices.push_back(s1);
        m_tubeVertices.push_back(p2Vert1);
        m_tubeVertices.push_back(p3Vert1);

        m_tubeVertices.push_back(s1);
        m_tubeVertices.push_back(k1);
        m_tubeVertices.push_back(p2Vert1);

        m_tubeVertices.push_back(s1);
        m_tubeVertices.push_back(p2Vert2);
        m_tubeVertices.push_back(k1);

        m_tubeVertices.push_back(s1);
        m_tubeVertices.push_back(p3Vert2);
        m_tubeVertices.push_back(p2Vert2);

        //Save rhombus facet of p1
        m_tubeVertices.push_back(s1);
        m_tubeVertices.push_back(p3Vert1);
        m_tubeVertices.push_back(s2);

        m_tubeVertices.push_back(s1);
        m_tubeVertices.push_back(s2);
        m_tubeVertices.push_back(p3Vert2);
    }
}

void OpenGLScene::drawTube()
{
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    glPushMatrix();

#ifdef TEST_1
    QVector3D a = QVector3D(  5, 1,  5);
    QVector3D b = QVector3D(  5, 1, -5);
    QVector3D c = QVector3D( -5, 1, 5);

    // draw axis
    glLineWidth(1);
    glBegin(GL_LINES);
        // right line
        glColor3f(0, 1, 0);
        glVertex3f(a.x(), a.y(), a.z());
        glVertex3f(b.x(), b.y(), b.z());

        // rear line
        glVertex3f(b.x(), b.y(), b.z());
        glVertex3f(c.x(), c.y(), c.z());

    glEnd();

    //================================
    float radius = 2;
    QVector3D d1Vector = a - b;
    QVector3D d2Vector = c - b;

    float cosAngle = QVector3D::dotProduct(d1Vector, d2Vector) /
                     ((sqrt(pow(d1Vector.x(), 2) + pow(d1Vector.z(), 2))) *
                     (sqrt(pow(d2Vector.x(), 2) + pow(d2Vector.z(), 2))));

    float sinHalfAngle = sqrt((1 - cosAngle) / 2);
    float d3Radius = radius / sinHalfAngle;

    float d1UnitVector = (sqrt(pow(d1Vector.x(), 2) + pow(d1Vector.z(), 2)));
    float d2UnitVector = (sqrt(pow(d2Vector.x(), 2) + pow(d2Vector.z(), 2)));
    qDebug() << "========================================";
    qDebug() << QString("cosAngle => %1").arg(cosAngle);
    qDebug() << QString("sinHalfAngle => %1").arg(sinHalfAngle);
    qDebug() << QString("d3Radius => %1").arg(d3Radius);
    qDebug() << QString("d1UnitVector => %1").arg(d1UnitVector);
    qDebug() << QString("d2UnitVector => %1").arg(d2UnitVector);

    d1Vector /= d1UnitVector;
    d2Vector /= d2UnitVector;

    QVector3D d3Vector = QVector3D((d1Vector.x() + d2Vector.x())/2, d1Vector.y(), (d1Vector.z() + d2Vector.z()) /2 );
    float d3UnitVector = (sqrt(pow(d3Vector.x(), 2) + pow(d3Vector.z(), 2)));
    d3Vector /= d3UnitVector;
    qDebug() << QString("d1Vector => x(%1), y(%2), z(%3)").arg(d1Vector.x()).arg(d1Vector.y()).arg(d1Vector.z());
    qDebug() << QString("d2Vector => x(%1), y(%2), z(%3)").arg(d2Vector.x()).arg(d2Vector.y()).arg(d2Vector.z());
    qDebug() << QString("d3Vector => x(%1), y(%2), z(%3)").arg(d3Vector.x()).arg(d3Vector.y()).arg(d3Vector.z());

    QVector3D k1 = QVector3D(b.x() - d3Radius*d3Vector.x(), b.y(), b.z() - d3Radius * d3Vector.z());
    QVector3D k2 = QVector3D(b.x() + d3Radius*d3Vector.x(), b.y(), b.z() + d3Radius * d3Vector.z());

    qDebug() << QString("k1 => x(%1), y(%2), z(%3)").arg(k1.x()).arg(k1.y()).arg(k1.z());
    qDebug() << QString("k2 => x(%1), y(%2), z(%3)").arg(k2.x()).arg(k2.y()).arg(k2.z());
    qDebug() << "========================================";
    // draw axis
    glLineWidth(3);
    glBegin(GL_LINES);
        // right line
        glColor3f(0, 0, 1);
        glVertex3f(b.x(), b.y(), b.z());
        glVertex3f(k1.x(), k1.y(), k1.z());

        // rear line
        glVertex3f(b.x(), b.y(), b.z());
        glVertex3f(k2.x(), k2.y(), k2.z());
    glEnd();

    float ratio = radius / sqrt( pow(a.z() - b.z(), 2) + pow(b.x() - a.x(), 2));
    QVector3D verticalVec = QVector3D((a.z() - b.z()) * ratio, a.y(), (b.x() - a.x()) * ratio);
    QVector3D p1New1 = QVector3D(a.x() + verticalVec.x(), a.y(), a.z() + verticalVec.z());
    QVector3D p1New2 = QVector3D(a.x() - verticalVec.x(), a.y(), a.z() - verticalVec.z());

    glLineWidth(3);
    glBegin(GL_LINES);

        glColor3f(0, 0, 1);
        glVertex3f(p1New1.x(), p1New1.y(), p1New1.z());
        glVertex3f(p1New2.x(), p1New2.y(), p1New2.z());

        glVertex3f(p1New1.x(), p1New1.y(), p1New1.z());
        glVertex3f(k1.x(), k1.y(), k1.z());

        glVertex3f(p1New2.x(), p1New2.y(), p1New2.z());
        glVertex3f(k2.x(), k2.y(), k2.z());

    glEnd();


#endif

    // [1] vertices only
#ifdef TEST_2
//    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);

    // enable vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, (float *)m_tubeVertices.data());
    glNormalPointer(GL_FLOAT, 0, (float *)m_tubeNormals.data());
    glDrawElements(GL_TRIANGLES, m_tubeIndices.size(), GL_UNSIGNED_INT, m_tubeIndices.data());

    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);    
#endif


    glPopMatrix();

    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
}


void OpenGLScene::drawBackground(QPainter *painter, const QRectF &)
{
    if (painter->paintEngine()->type() != QPaintEngine::OpenGL
     && painter->paintEngine()->type() != QPaintEngine::OpenGL2)
    {
        qWarning("OpenGLScene: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
        return;
    }

    painter->beginNativePainting();

    initGL();

    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); //switch to projection matrix
    glPushMatrix();              //save current projection matrix
    glLoadIdentity();            //reset projection matrix
    gluPerspective(70, width() / height(), 0.01, 1000);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // ====== added begin ======
#ifdef QUATERNION_CAMERA
    QMatrix4x4 view;
    view.rotate(m_trackBall->rotation());
    view(2, 3) -= 2.0f * exp(m_distExp / 1200.0f);
    glLoadMatrixf(view.constData());
#else
    /*
        //move model farther away.
        glTranslatef(0, 0, -m_distance);
        //rotate model
        glRotatef(m_rotation.x, 1, 0, 0);
        glRotatef(m_rotation.y, 0, 1, 0);
        glRotatef(m_rotation.z, 0, 0, 1);*/

#endif

    if (m_model) {
        const float pos[] = { float(m_lightItem->x() - width() / 2), float(height() / 2 - m_lightItem->y()), 512, 0 };
        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glColor4f(m_modelColor.redF(), m_modelColor.greenF(), m_modelColor.blueF(), 1.0f);

        glEnable(GL_MULTISAMPLE);
        m_model->render(m_wireframeEnabled, m_normalsEnabled, m_gcodeMotionEnabled, m_gcodeLinesEnabled);
        glDisable(GL_MULTISAMPLE);

    }
    drawTube();

    drawGrid();
    drawBox();
//    drawAxis();
    // ========== added (end)   ===========

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    painter->endNativePainting();

    QTimer::singleShot(20, this, SLOT(update()));
}


void OpenGLScene::drawBox()
{
//    glDepthFunc(GL_ALWAYS);     // to avoid visual artifacts with grid lines
    glDisable(GL_LIGHTING);
//    glPushMatrix();

    glLineWidth(1);
    glBegin(GL_LINE_LOOP);
        glColor3f(0.f, 0.f, 0.f);
        // Top plane
        glVertex3f( GRID_SIZE, GRID_SIZE,  GRID_SIZE);
        glVertex3f(-GRID_SIZE, GRID_SIZE,  GRID_SIZE);
        glVertex3f(-GRID_SIZE, GRID_SIZE, -GRID_SIZE);
        glVertex3f( GRID_SIZE, GRID_SIZE, -GRID_SIZE);
    glEnd();

    glLineWidth(1);
    glBegin(GL_LINES);
        glColor3f(0.f, 0.f, 0.f);
        // Top plane
        glVertex3f( GRID_SIZE, GRID_SIZE,  GRID_SIZE);
        glVertex3f( GRID_SIZE,       0.f,  GRID_SIZE);
        glVertex3f(-GRID_SIZE, GRID_SIZE,  GRID_SIZE);
        glVertex3f(-GRID_SIZE,       0.f,  GRID_SIZE);
        glVertex3f(-GRID_SIZE, GRID_SIZE, -GRID_SIZE);
        glVertex3f(-GRID_SIZE,       0.f, -GRID_SIZE);
        glVertex3f( GRID_SIZE, GRID_SIZE, -GRID_SIZE);
        glVertex3f( GRID_SIZE,       0.f, -GRID_SIZE);
    glEnd();

//    glPopMatrix();
    // restore default settings
    glEnable(GL_LIGHTING);
//    glDepthFunc(GL_LEQUAL);
}

void OpenGLScene::drawAxis()
{
//    glDepthFunc(GL_ALWAYS);     // to avoid visual artifacts with grid lines
    glDisable(GL_LIGHTING);

    glPushMatrix();
    // draw axis
    glLineWidth(3);
    glBegin(GL_LINES);
        // x
        glColor3f(1, 0, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(AXIS_SIZE, 0, 0);
        // y
        glColor3f(0, 1, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(0, AXIS_SIZE, 0);
        // z
        glColor3f(0, 0, 1);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, AXIS_SIZE);
    glEnd();
    glLineWidth(1);

    // draw arrows(actually big square dots)
    glPointSize(5);
    glBegin(GL_POINTS);
        // x
        glColor3f(1, 0, 0);
        glVertex3f(AXIS_SIZE, 0, 0);
        // y
        glColor3f(0, 1, 0);
        glVertex3f(0, AXIS_SIZE, 0);
        // z
        glColor3f(0, 0, 1);
        glVertex3f(0, 0, AXIS_SIZE);
    glEnd();
    glPointSize(1);

    glPopMatrix();
    // restore default settings
    glEnable(GL_LIGHTING);
//    glDepthFunc(GL_LEQUAL);
}

void OpenGLScene::drawGrid()
{
    // disable lighting
    glDisable(GL_LIGHTING);
//    glDisable(GL_DEPTH_TEST);
    // ===== draw transparent plane =====
    glBegin(GL_QUADS);
    glColor4f(0.6f, 0.7, 1.f, 0.5f);

    glVertex3f( GRID_SIZE, 0,  GRID_SIZE);
    glVertex3f(-GRID_SIZE, 0,  GRID_SIZE);
    glVertex3f(-GRID_SIZE, 0, -GRID_SIZE);
    glVertex3f( GRID_SIZE, 0, -GRID_SIZE);
    glEnd();


    // ===== draw 20x20 grid =====
    glBegin(GL_LINES);

    glColor3f(0.5f, 0.5f, 0.5f);
    for(float i=GRID_STEP; i <= GRID_SIZE; i+= GRID_STEP)
    {
        glVertex3f(-GRID_SIZE, 0,  i);   // lines parallel to X-axis
        glVertex3f( GRID_SIZE, 0,  i);
        glVertex3f(-GRID_SIZE, 0, -i);   // lines parallel to X-axis
        glVertex3f( GRID_SIZE, 0, -i);

        glVertex3f( i, 0, -GRID_SIZE);   // lines parallel to Z-axis
        glVertex3f( i, 0,  GRID_SIZE);
        glVertex3f(-i, 0, -GRID_SIZE);   // lines parallel to Z-axis
        glVertex3f(-i, 0,  GRID_SIZE);
    }

    // x-axis
    glLineWidth(3);
    glColor3f(1, 0, 0);
    glVertex3f(-GRID_SIZE, 0, 0);
    glVertex3f( GRID_SIZE, 0, 0);

    // z-axis
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, -GRID_SIZE);
    glVertex3f(0, 0,  GRID_SIZE);

    glEnd();

    // enable lighting back
    glEnable(GL_LIGHTING);
//    glEnable(GL_DEPTH_TEST);
}


void OpenGLScene::loadModel()
{
    loadModel(QFileDialog::getOpenFileName(0, tr("Choose model"), QString(), QLatin1String("*.obj *.stl *.gcode")));
}

void OpenGLScene::loadModel(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    m_modelButton->setEnabled(false);
    QApplication::setOverrideCursor(Qt::BusyCursor);
#ifndef QT_NO_CONCURRENT
    m_modelLoader.setFuture(QtConcurrent::run(::loadModel, filePath));
#else
    setModel(::loadModel(filePath));
    modelLoaded();
#endif
}

void OpenGLScene::modelLoaded()
{
#ifndef QT_NO_CONCURRENT
    setModel(m_modelLoader.result());
#endif
    m_modelButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void OpenGLScene::enableWireframe(bool enabled)
{
    m_wireframeEnabled = enabled;
    update();
}

void OpenGLScene::enableNormals(bool enabled)
{
    m_normalsEnabled = enabled;
    update();
}


void OpenGLScene::enableGCodeMotion(bool enabled)
{
    m_gcodeMotionEnabled = enabled;
    update();
}

void OpenGLScene::enableGCodeLines(bool enabled)
{
    m_gcodeLinesEnabled = enabled;
    update();
}

void OpenGLScene::setModel(Model *model)
{
    delete m_model;
    m_model = model;

    m_labels[0]->setText(tr("File:   %0").arg(m_model->fileName()));
    m_labels[1]->setText(tr("Points: %0").arg(m_model->points()));
    m_labels[2]->setText(tr("Edges:  %0").arg(m_model->edges()));
    m_labels[3]->setText(tr("Faces:  %0").arg(m_model->faces()));

    m_slider->setRange(0, m_model->gcodeCount());
    update();
}


void OpenGLScene::setModelColor()
{
    const QColor color = QColorDialog::getColor(m_modelColor);
    if (color.isValid()) {
        m_modelColor = color;
        update();
    }
}

void OpenGLScene::setBackgroundColor()
{
    const QColor color = QColorDialog::getColor(m_backgroundColor);
    if (color.isValid()) {
        m_backgroundColor = color;
        update();
    }
}

void OpenGLScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    if (event->buttons() & Qt::LeftButton) {
#ifdef QUATERNION_CAMERA
        m_trackBall->push(pixelPosToViewPos(event->scenePos()), QQuaternion());
#endif
    }
    event->accept();
}

void OpenGLScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);
    if (event->isAccepted())
        return;

    if (event->buttons() & Qt::LeftButton) {
#ifdef QUATERNION_CAMERA
        m_trackBall->move(pixelPosToViewPos(event->scenePos()), QQuaternion());
#else
        //        const QPointF delta = event->scenePos() - event->lastScenePos();
        //        const Point3d angularImpulse = Point3d(delta.y(), delta.x(), 0) * 0.1;
        //        m_rotation += angularImpulse;

            // === modified ===
        //        m_cameraAngleX += angularImpulse.x;
        //        m_cameraAngleY += angularImpulse.y;
#endif
        event->accept();
        update();
    } else {
        m_trackBall->release(pixelPosToViewPos(event->scenePos()), QQuaternion());
    }
}

void OpenGLScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;

#ifdef QUATERNION_CAMERA
    m_trackBall->release(pixelPosToViewPos(event->scenePos()), QQuaternion());
#endif

    event->accept();
    update();
}

void OpenGLScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    QGraphicsScene::wheelEvent(event);
    if (event->isAccepted())
        return;

#ifdef QUATERNION_CAMERA
    m_distExp += event->delta();
    if (m_distExp < -8 * 120)
        m_distExp = -8 * 120;
    if (m_distExp > 10 * 700)
        m_distExp = 10 * 700;
#else
    // 1.2 ^ (-delta / 120)f
//    m_distance *= qPow(1.2, -event->delta() / 120);
//    m_cameraDistance *= qPow(1.2, -event->delta() / 120);
#endif
    event->accept();
    update();
}


QPointF OpenGLScene::pixelPosToViewPos(const QPointF& p)
{
    return QPointF(2.0 * float(p.x()) / width() - 1.0,
                   1.0 - 2.0 * float(p.y()) / height());
}


void OpenGLScene::translateX(int value)
{
    qDebug() << Q_FUNC_INFO << value;
    QMatrix4x4 m;
    m.translate(value, 0, 0);
    m_model->transform(m);
    update();
}

void OpenGLScene::translateY(int value)
{
    qDebug() << Q_FUNC_INFO << value;
    QMatrix4x4 m;
    m.translate(0, value, 0);
    m_model->transform(m);
    update();
}

void OpenGLScene::translateZ(int value)
{
    qDebug() << Q_FUNC_INFO << value;
    QMatrix4x4 m;
    m.translate(0, 0, value);
    m_model->transform(m);
    update();
}

void OpenGLScene::rotateX(int value)
{
    qDebug() << Q_FUNC_INFO << value;
    QMatrix4x4 m;
    m.rotate(value, 1, 0, 0);
    m_model->transform(m);
    update();
}

void OpenGLScene::rotateY(int value)
{
    qDebug() << Q_FUNC_INFO << value;
    QMatrix4x4 m;
    m.rotate(value, 0, 1, 0);
    m_model->transform(m);
    update();
}

void OpenGLScene::rotateZ(int value)
{
    qDebug() << Q_FUNC_INFO << value;
    QMatrix4x4 m;
    m.rotate(value, 0, 0, 1);
    m_model->transform(m);
    update();
}


void OpenGLScene::scale(double value)
{
    qDebug() << Q_FUNC_INFO << value;
    QMatrix4x4 m;
    m.scale(value, value, value);
    m_model->transform(m);
    update();
}


void OpenGLScene::gcodeLayers(int layers)
{
//    qDebug() << Q_FUNC_INFO << layers;
    m_model->setGCodeLayers(layers);
    update();
}
