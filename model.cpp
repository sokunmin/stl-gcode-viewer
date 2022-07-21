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
   
#include "model.h"
#include "gcode/gcode.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QVarLengthArray>
#include <QtOpenGL>
#include <QDebug>


Model::Model(const QString &filePath)
    : m_fileName(QFileInfo(filePath).fileName())
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    if (filePath.endsWith(".stl", Qt::CaseInsensitive)) {
        loadStl(file);
    } else if (filePath.endsWith(".obj", Qt::CaseInsensitive)) {
        loadObj(file);
    } else if (filePath.endsWith(".gcode", Qt::CaseInsensitive)) {
        loadGCode(filePath.toStdString());
    }

    m_transform.setToIdentity();
}

Model::~Model()
{

}

void Model::transform(QMatrix4x4 matrix)
{
    m_transform = matrix;
    if (m_vertices.empty())
        return;


    QVector<QVector3D> v;
#pragma omp parallel for
    v = m_vertices;
    auto it = v.end(), end = v.begin();
    while ( it != end ) {
//        qDebug() << "X: " << ((QVector3D*)it)->x();
        *it = matrix * *it;
        --it;
    }

    m_verticesNew = v;

    recomputeAll();
}

void Model::render(bool wireframe, bool normals, bool showGcodeMotion, bool showGcodeLines)
{
//    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    if (wireframe) {
        glVertexPointer(3, GL_FLOAT, 0, (float *)m_vertices.data());
        glDrawElements(GL_LINES, m_edgeIndices.size(), GL_UNSIGNED_INT, m_edgeIndices.data());
    } else {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glShadeModel(GL_SMOOTH);

        glEnableClientState(GL_NORMAL_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, (float *)m_verticesNew.data());
        glNormalPointer(GL_FLOAT, 0, (float *)m_normals.data());
        glDrawElements(GL_TRIANGLES, m_vertexIndices.size(), GL_UNSIGNED_INT, m_vertexIndices.data());

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
    }

    if (normals) {
        QVector<QVector3D> normals;
        for (int i = 0; i < m_normals.size(); ++i)
            normals << m_verticesNew.at(i) << (m_verticesNew.at(i) + m_normals.at(i) * 0.02f);
        glVertexPointer(3, GL_FLOAT, 0, (float *)normals.data());
        glDrawArrays(GL_LINES, 0, normals.size());
    }

    glDisableClientState(GL_VERTEX_ARRAY);



    if(m_gCode.isOpen())
        m_gCode.draw(showGcodeLines, showGcodeMotion);

    //Draw bounding box
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glColor3f(0.8, 0.18, 0.61);
    glLineWidth(2.f);

    //draw front plane
    glBegin(GL_LINE_LOOP);
    glVertex3f(m_min.x(), m_max.y(), m_max.z());
    glVertex3f(m_min.x(), m_min.y(), m_max.z());
    glVertex3f(m_max.x(), m_min.y(), m_max.z());
    glVertex3f(m_max.x(), m_max.y(), m_max.z());
    glEnd();

    //draw rear plane
    glBegin(GL_LINE_LOOP);
    glVertex3f(m_min.x(), m_max.y(), m_min.z());
    glVertex3f(m_min.x(), m_min.y(), m_min.z());
    glVertex3f(m_max.x(), m_min.y(), m_min.z());
    glVertex3f(m_max.x(), m_max.y(), m_min.z());
    glEnd();


    //draw horizontal lines
    glBegin(GL_LINES);
    glVertex3f(m_min.x(), m_max.y(), m_min.z());
    glVertex3f(m_min.x(), m_max.y(), m_max.z());
    glVertex3f(m_max.x(), m_max.y(), m_min.z());
    glVertex3f(m_max.x(), m_max.y(), m_max.z());

    glVertex3f(m_min.x(), m_min.y(), m_min.z());
    glVertex3f(m_min.x(), m_min.y(), m_max.z());
    glVertex3f(m_max.x(), m_min.y(), m_min.z());
    glVertex3f(m_max.x(), m_min.y(), m_max.z());
    glEnd();

    glPopMatrix();

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

}


void Model::loadObj(QFile &file)
{
    // 1e9 = 1*10^9 = 1,000,000,000
    QVector3D boundsMin( 1e9, 1e9, 1e9);
    QVector3D boundsMax(-1e9,-1e9,-1e9);

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString input = in.readLine();
        // # means comment
        if (input.isEmpty() || input[0] == '#')
            continue;

        QTextStream ts(&input);
        QString id;
        ts >> id;
        //---------------  v = List of vertices with (x,y,z[,w]) corrdinates. -----------------
        if (id == "v") {
            QVector3D p;
            for (int i = 0; i < 3; ++i) {
                ts >> p[i];
                boundsMin[i] = qMin(boundsMin[i], p[i]);
                boundsMax[i] = qMax(boundsMax[i], p[i]);
            }
            m_vertices << p;

        //--------------- f = Face definitions -----------------
        } else if (id == "f" || id == "fo") {
            QVarLengthArray<int, 4> p;

            while (!ts.atEnd()) {
                QString vertex;
                ts >> vertex;
                //e.g. vertex / texture
                // vertex index in correspondence with vertex list.
                const int vertexIndex = vertex.split('/').value(0).toInt();
//                qDebug() << "> vertexIndex : " << vertexIndex;
                if (vertexIndex) {
                    p.append((vertexIndex > 0) ? (vertexIndex - 1) : (m_vertices.size() + vertexIndex));
//                    int d = (vertexIndex > 0) ? (vertexIndex - 1) : (m_vertices.size() + vertexIndex);
//                    qDebug() << "selected index : " << d;
                }
            }

            qDebug() << QString("p.size(%1) vs. vertexIndices.size(%2)").arg(p.size()).arg(m_vertexIndices.size());
            for (int i = 0; i < p.size(); ++i) {
                const int edgeA = p[i];
                const int edgeB = p[(i + 1) % p.size()];
//                qDebug() << QString("edgeA(%1/%2), edgeB(%3/%4)").arg(QString::number(edgeA), QString::number(i), QString::number(edgeB), QString::number((i + 1) % p.size())) ;

                if (edgeA < edgeB) {
                    m_edgeIndices << edgeA << edgeB;                    
//                    qDebug() << "Added : edgeA : " << edgeA << " / edgeB : " << edgeB ;
                }
            }

            // append vertex / texture-coordinate / normal
            for (int i = 0; i < 3; ++i)
                m_vertexIndices << p[i];

            if (p.size() == 4)
                for (int i = 0; i < 3; ++i)
                    m_vertexIndices << p[(i + 2) % 4];
        }
    }

    qDebug() << QString("size(%1), max-x(%2), min-x(%3), max-y(%4), min-y(%5), max-z(%6), min-z(%7)")
                .arg(QString::number(m_vertices.size()),
                     QString::number(boundsMax.x()),
                     QString::number(boundsMin.x()),
                     QString::number(boundsMax.y()),
                     QString::number(boundsMin.y()),
                     QString::number(boundsMax.z()),
                     QString::number(boundsMin.z()));
    const QVector3D bounds = boundsMax - boundsMin;
    const qreal scale = 1 / qMax(bounds.x(), qMax(bounds.y(), bounds.z()));
    qDebug() << QString("scale(%1) = 1 / %2")
                .arg(QString::number(scale), QString::number(qMax(bounds.x(), qMax(bounds.y(), bounds.z()))));
//    for (int i = 0; i < m_vertices.size(); ++i) {
//        //the way to place the model by mutiplying the ratio.
//        float ratio = 0.f;
//        m_vertices[i] = (m_vertices[i] - (boundsMin + bounds * ratio)) * scale;
//    }

    m_verticesNew = m_vertices;
    recomputeAll();
}

void Model::loadStl(QFile &file)
{    
    QTextStream stream(&file);
    const QString &head = stream.readLine();
    if (head.left(6) == "solid " && head.size() < 80)	// ASCII format
    {
//        name = head.right(head.size() - 6).toStdString();
        QString word;
        stream >> word;
        for(; word == "facet" ; stream >> word)
        {
            stream >> word;	// normal x y z
            QVector3D n;
            stream >> n[0] >> n[1] >> n[2];
            n.normalize();

            stream >> word >> word;	// outer loop
            stream >> word;
            size_t startIndex = m_vertices.size();
            for(; word != "endloop" ; stream >> word)
            {
                QVector3D v; //vertex x y z
                stream >> v[0] >> v[1] >> v[2];
                m_vertices.push_back(v);
//                qDebug() << "==== outer loop ===";
            }

            for(size_t i = startIndex + 2 ; i < m_vertices.size() ; ++i)
            {
                m_vertexIndices.push_back(startIndex);
                m_vertexIndices.push_back(i - 1);
                m_vertexIndices.push_back(i);

                qDebug() << QString("edgeA(%1), edgeB(%2)").arg(QString::number(startIndex), QString::number(i)) ;

//                if (startIndex < (i-1))
                    m_edgeIndices << (startIndex) << (i - 1) << i;
            }
            stream >> word;	// endfacet
        }
    } else {
        file.setTextModeEnabled(false);
        file.seek(80);
        quint32 triangleCount;
        file.read((char*)&triangleCount, sizeof(triangleCount));
        m_vertices.reserve(triangleCount * 3U);
        m_normals.reserve(triangleCount * 3U);
        m_vertexIndices.reserve(triangleCount * 3U);
        for(size_t i = 0 ; i < triangleCount ; ++i)
        {
            QVector3D n, a, b, c;
#define READ_VECTOR(v)\
            do {\
                float f;\
                file.read((char*)&f, sizeof(float));	v[0] = f;\
                file.read((char*)&f, sizeof(float));	v[1] = f;\
                file.read((char*)&f, sizeof(float));	v[2] = f;\
            } while(false)

            READ_VECTOR(n);
            READ_VECTOR(a);
            READ_VECTOR(b);
            READ_VECTOR(c);

            m_vertexIndices.push_back(m_vertices.size());
            m_vertexIndices.push_back(m_vertices.size() + 1);
            m_vertexIndices.push_back(m_vertices.size() + 2);
            m_vertices.push_back(a);
            m_vertices.push_back(b);
            m_vertices.push_back(c);

            quint16 attribute_byte_count;
            file.read((char*)&attribute_byte_count, sizeof(attribute_byte_count));
        }
    }
    m_verticesNew = m_vertices;
    recomputeAll();
}

void Model::loadGCode(std::string file)
{
    m_gCode.clear();
    m_gCode.open(file);
}

void Model::computeEdges()
{

}

//Bounding Box : http://en.wikibooks.org/wiki/OpenGL_Programming/Bounding_box
void Model::recomputeAll()
{
    qDebug() << Q_FUNC_INFO;

    //calculate normals of each face
    int size = m_verticesNew.size();
    m_normals.resize(size);
    for (int i = 0; i < m_vertexIndices.size(); i += 3) {
        const QVector3D a = m_verticesNew.at(m_vertexIndices.at(i));
        const QVector3D b = m_verticesNew.at(m_vertexIndices.at(i+1));
        const QVector3D c = m_verticesNew.at(m_vertexIndices.at(i+2));

        const QVector3D normal = QVector3D::crossProduct(b - a, c - a).normalized();

        for (int j = 0; j < 3; ++j)
            m_normals[m_vertexIndices.at(i + j)] += normal;
    }

    /* //debug output
    qDebug() << "=========== Output =============";
    for (int i = 0; i < m_verticesNew.size(); ++i) {
        qDebug() << QString("x(%1), y(%2), z(%3)").arg(m_verticesNew.at(i).x()).arg(m_verticesNew.at(i).y()).arg(m_verticesNew.at(i).z());
    }

    for (int i = 0; i< m_vertexIndices.size(); ++i) {
        qDebug() << QString("index %1").arg(m_vertexIndices.at(i));
    }

    for (int i = 0; i < m_normals.size(); ++i ) {
        qDebug() << QString("x(%1), y(%2), z(%3)").arg(m_normals.at(i).x()).arg(m_normals.at(i).y()).arg(m_normals.at(i).z());
    }
    qDebug() << "========================";
    */
    //recompute normals and bounding volume
    GLfloat minX, maxX,
            minY, maxY,
            minZ, maxZ;
    minX = maxX = m_verticesNew[0].x();
    minY = maxY = m_verticesNew[0].y();
    minZ = maxZ = m_verticesNew[0].z();

    for (int i = 0; i < size; ++i) {
        //compute normals
        m_normals[i] = m_normals[i].normalized();
        //calculate values of maximum and minimum
        if (m_verticesNew[i].x() < minX) minX = m_verticesNew[i].x();
        if (m_verticesNew[i].x() > maxX) maxX = m_verticesNew[i].x();
        if (m_verticesNew[i].y() < minY) minY = m_verticesNew[i].y();
        if (m_verticesNew[i].y() > maxY) maxY = m_verticesNew[i].y();
        if (m_verticesNew[i].z() < minZ) minZ = m_verticesNew[i].z();
        if (m_verticesNew[i].z() > maxZ) maxZ = m_verticesNew[i].z();
    }

    m_size   = QVector3D(maxX-minX, maxY-minY, maxZ-minZ);
    m_center = QVector3D((minX+maxX)/2, (minY+maxY)/2, (minZ+maxZ)/2);

    m_min = QVector3D(minX, minY, minZ);
    m_max = QVector3D(maxX, maxY, maxZ);

    qDebug() << QString("MIN : x(%1), y(%2), z(%3)").arg(m_min.x()).arg(m_min.y()).arg(m_min.z());
    qDebug() << QString("MAN : x(%1), y(%2), z(%3)").arg(m_max.x()).arg(m_max.y()).arg(m_max.z());
    qDebug() << QString("SIZE : x(%1), y(%2), z(%3)").arg(m_size.x()).arg(m_size.y()).arg(m_size.z());

//    QMatrix4x4 center(1,1,1,1), scale(1,1,1,1);
//    m_transform.translate(QMatrix4x4(1,1,1) * center)
}
