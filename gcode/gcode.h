/*
Copyright 2010 by Fabian Schwartau
E-Mail: fabian@opencode.eu

This file is part of RepRapGCodeSimulator.

RepRapGCodeSimulator is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RepRapGCodeSimulator is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BoostComPort.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GCode_H_
#define GCode_H_

#include <string>
#include <vector>
#include <limits>
#include <QVector3D>
#include <QVector>
#include <cmath>

using namespace std;

struct GCodeParameter
{
	char name;
	float value;
};

struct GCodeLine
{
	string originalLine;  // the original line from the file
	string clearedLine;    // the cleared line, no comments, no needed whitespaces, etc. 
	string command;
    bool interprete;   // if this line is interpreteable (not just an empty or comment line)
//	vector<GCodeParameter> parameters;
    bool hasE;
    bool hasXYZ;
    bool visible;
    int layer;

    float x;
    float y;
    float z;
    float e;
    float f;
};

//! ============= GCode ===============
class GCode
{
public:
    GCode();
    ~GCode();
    int   open(string fileName);
    void  clear();
    void  draw(bool linesOnly, bool showMotion);
    int   addLine(string line);
    void  parseLine(GCodeLine &gcodeLine, char command, string value);
    vector<GCodeLine>& getCodeLines() ;
    void  refreshMinMax();
	float getMinX();
	float getMinY();
	float getMinZ();
	float getMaxX();
	float getMaxY();
	float getMaxZ();
    int  getGCodeCount() {
        return codeLines.size();
    }
    void setGCodeLayers(int layers);

    bool  isOpen() const;
protected:
	
private:
	vector<GCodeLine> codeLines;
	void replace(string& replace, string from, string to);
	void explode(vector<string> &res, string &list, string expression);
    void generateTube(QVector3D &p1, QVector3D &p2, QVector3D &p3, bool saveRearFacet, float radius);
    void recomputeAll();

	float minX, minY, minZ;
	float maxX, maxY, maxZ;
	float lastZ;
	int currentLayer;
    int showLayers;

    QVector<QVector3D> m_tubeVertices;
    QVector<int> m_tubeIndices;
    QVector<QVector3D> m_tubeNormals;
    QVector<QVector3D> m_prevFacet;

};

#endif /* GCode_H_ */
