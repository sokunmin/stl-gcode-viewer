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
   
#ifndef MODEL_H
#define MODEL_H

#include <QString>
#include <QVector3D>
#include <QVector>
#include <QMatrix4x4>

#include <math.h>

#include "gcode/gcode.h"

class QFile;
class GCoder;

class Model
{
public:
    Model() {}
    Model(const QString &filePath);
    ~Model();

    void render(bool wireframe = false, bool normals = false, bool showGcodeMotion = false, bool showGcodeLines = true) ;
    void transform(QMatrix4x4 matrix);
    QString fileName() const { return m_fileName; }
    int faces() const { return m_vertexIndices.size() / 3; }
    int edges() const { return m_edgeIndices.size() / 2; }
    int points() const { return m_vertices.size(); }
    int gcodeCount() { return m_gCode.getGCodeCount(); }
    void setGCodeLayers(int layers) { m_gCode.setGCodeLayers(layers); }
private:
    QString m_fileName;
    QVector<QVector3D> m_vertices;
    QVector<QVector3D> m_verticesNew;
    QVector<QVector3D> m_normals;
    QVector<int> m_edgeIndices;
    QVector<int> m_vertexIndices;
    GCode m_gCode;

    QVector3D m_size;
    QVector3D m_center;
    QVector3D m_min;
    QVector3D m_max;
    QMatrix4x4 m_transform;

    void loadObj(QFile &file);
    void loadStl(QFile &file);
    void loadGCode(std::string file);
    void computeEdges();
    void recomputeAll();
};

#endif
