/*
 * program2 - Geometry Viewer
 * @author  - Aditya Kousik
 * @email   - adit267.kousik@gmail.com
 */

#include "Libraries\glew\glew.h"
#include "Libraries\freeglut\freeglut.h"
#include "Libraries\glm\glm.hpp"

#include <gl\GL.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;
using namespace glm;

#include "vgl.h"
#include "LoadShaders.h"

enum coord {_X, _Y, _Z};
enum color {_RED, _GREEN, _BLUE};
enum cmd {_INVALID = -1, _V, _VT, _VN, _MTLLIB, _USEMTL, _NEWMTL, 
		  _F, _S, _G, _O, _NUM_CMDS};
enum mtlCmd {_MTL_INVALID = -1, _MTL_NEWMTL, _KA, _KD, _KS, _NUM_MTL_CMDS};
enum control {_CTL_INVALID = -1, _OBJ, _RX, _RY, _RZ, _T, _CTL_S, _NUM_CTL_CMDS};

string keywords[] = { "v", "vt", "vn", "mtllib", "usemtl", "newmtl",
					  "f", "s", "g", "o"};
string mtlKeywords[] = { "newmtl", "Ka", "Kd", "Ks" };
string controls[] = {"obj", "rx", "ry", "rz", "t", "s"};


/* Object infomation*/
GLuint numObj = 0;
class ViewerObject
{
	public: string objName;
	std::vector< std::vector < GLfloat > > vertexPositions,
		normalPositions;
};
std::vector <ViewerObject > viewerObjects;

GLuint activeMaterialIndex;
string materialFileName;
class MaterialData
{
	public:	string materialName;
			GLfloat Ka[3], Kd[3];
			std::vector <std::vector <std::vector<GLuint > > > faceIndexList;
};

std::vector< MaterialData > materialInfo;

GLuint getObjectIndex(string objFileName)
{
	string target = objFileName.substr(0, objFileName.length() - 4);
	for (GLuint i = 0; i < viewerObjects.size(); ++i)
		if (viewerObjects[i].objName.compare(target) == 0)
			return i;

	return -1;
}

void displayMaterialInfo()
{
	cout << "Faces \n";
	for (int i = 0; i < materialInfo.size(); ++i)
	{
		cout << materialInfo[i].materialName << " ";
		for (int j = 0; j < materialInfo[i].faceIndexList.size(); ++j)
		{
			cout << "NumLines " << materialInfo[i].faceIndexList[j].size() << " ";
			for (int k = 0; k < materialInfo[i].faceIndexList[j].size(); ++k)
			{
				cout << "Num Faces " << materialInfo[i].faceIndexList[j][k].size() << " ";
				for (int l = 0; l < materialInfo[i].faceIndexList[j][k].size(); l++)
				{
					cout << materialInfo[i].faceIndexList[j][k][l] << " ";
				}
			}
			cout << endl;
		}
		cout << endl;
	}
}

// Tokenizer
void tokenizeGeneral(const string &str,
	std::vector<string>& tokens,
	const string& delimiters = " \n/")
{
	GLchar* bufToken = new GLchar[str.size()];
	GLchar* buf = new GLchar[str.size()];
	for (GLuint i = 0; i < str.size(); ++i)
	{
		buf[i] = str[i];
	}
	buf[str.size()] = '\0';
	bufToken = strtok(buf, delimiters.c_str());
	while (bufToken)
	{
		tokens.push_back(bufToken);
		bufToken = strtok(NULL, delimiters.c_str());
	}
}

/* Shameless hack 
 * f a/b/c | a//b | a b c
 */
void tokenizeFaceIndices(std::vector<string >& str,
	std::vector<std::vector< GLuint > >& tokens,
	const string& delimiters = "/")
{
	GLchar *bufStr, *bufToken;
	
	for (GLuint i = 1; i < str.size(); ++i)
	{
		bufStr = new GLchar[strlen(str[i].c_str()) + 1];
		bufToken = new GLchar[strlen(str[i].c_str()) + 1];
		strcpy(bufStr, str[i].c_str());
		std::vector<GLuint > indices;
		bufToken = strtok(bufStr, delimiters.c_str());
		while (bufToken)
		{
			indices.push_back(atoi(bufToken));
			bufToken = strtok(NULL, delimiters.c_str());
		}
		tokens.push_back(indices);
	}
}

GLuint getMaterialIndex(string materialName)
{
	for (GLuint i = 0; i < materialInfo.size(); ++i)
		if (materialInfo[i].materialName.compare(materialName) == 0)
			return i;
	return -1;
}

void parseMaterialFile(string filename)
{
	ifstream materialFile;
	string mtlLine;
	std::vector< string > mtlLines;
	GLuint mtlCmd, count = 0;
	MaterialData temp;

	materialFile.open(filename, ios::in);
	if (materialFile.is_open())
	{
		while (getline(materialFile, mtlLine))
			mtlLines.push_back(mtlLine);
		
		materialFile.close();
	}
	else
	{
		cout << "\nInvalid material filename. Re-start viewer with valid "
			"material filename" << endl;
		exit(EXIT_FAILURE);
	}

	for (GLuint i = 0; i < mtlLines.size(); ++i)
	{
		std::vector< string > mtlTokens;
		string mtlName;

		tokenizeGeneral(mtlLines[i], mtlTokens);
		
		if (!mtlTokens.empty())
		{
			mtlCmd = _MTL_INVALID;
			for (GLuint k = _MTL_NEWMTL; k < _NUM_MTL_CMDS; ++k)
				if (mtlTokens[0].compare(mtlKeywords[k]) == 0)
					mtlCmd = k;

			switch (mtlCmd)
			{
			case _MTL_NEWMTL :
				mtlName = mtlTokens[1];
				count = 0;
				temp.materialName = mtlName;
				break;
			case _KA: case _KD:
				count++;
				for (GLuint j = 1; j < mtlTokens.size(); ++j)
				{
					switch (mtlCmd)
					{
					case _KA: temp.Ka[j - 1] = std::stof(mtlTokens[j], NULL); 
							  break;
					case _KD: temp.Kd[j - 1] = std::stof(mtlTokens[j], NULL); 
							  break;
					}
				}

				if (count == 2)
					materialInfo.push_back(temp);
				
				break;
			}
		}
	}
}

void readObjFile(string objFileName)
{
	ifstream objFile;
	string line, mtlName;
	std::vector< string > lines;
	std::vector< std::vector <string > > tokenList;
	ViewerObject tempObj;

	GLuint i = 0, j, k = 0;
	GLuint cmdType;
	objFile.open(objFileName, ios::in);

	if (objFile.is_open())
	{
		while (getline(objFile, line))
			lines.push_back(line);

		objFile.close();
	}
	else
	{
		cout << "\nInvalid control filename. Re-start viewer with valid "
			"obj filename" << endl;
		exit(EXIT_FAILURE);
	}

	// Set objName in tempObj
	tempObj.objName = objFileName.substr(0, objFileName.length() - 4);
	for (i = 0; i < lines.size(); ++i)
	{
		std::vector<string > tokens;
		std::vector<std::vector <GLuint > > faceIndexTokens;
		std::vector<GLfloat > vTemp;
		std::vector<GLuint > fTemp;
		
		/* Strictly for face values */
		std::vector<char> buf;
		vector<string> indexTokens;
		vector<GLuint > indices;
		string tmp;

		tokenizeGeneral(lines[i], tokens);
		if (!tokens.empty())
		{
			cmdType = _INVALID;
			for (k = _V; k < _NUM_CMDS; ++k)
				if (tokens[0].compare(keywords[k]) == 0)
					cmdType = k;

			switch (cmdType)
			{	
			case _V: case _VT: case _VN:
				for (j = 1; j < tokens.size(); ++j)
					vTemp.push_back(std::stof(tokens[j], NULL));
				
				switch (cmdType)
				{
				case _V : tempObj.vertexPositions.push_back(vTemp); break;
				case _VT:     break;
				case _VN: tempObj.normalPositions.push_back(vTemp); break;
				}
				break;
			case _F:
				GLchar *bufSpaceTokens;
				for (GLuint j = 0; j < lines[i].size(); ++j)
				{
					buf.push_back(lines[i][j]);
					if (lines[i][j] == '/')
						if (lines[i][j + 1] == '/')
							buf.push_back('0');

				}
				tmp = new GLchar[buf.size()];
				for (GLuint j = 0; j < buf.size(); ++j)
					tmp[j] = buf[j];
				tmp[buf.size()] = '\0';
				tokenizeGeneral(tmp, indexTokens, " \n");
				tokenizeFaceIndices(indexTokens, faceIndexTokens);
				materialInfo[activeMaterialIndex].faceIndexList.push_back(faceIndexTokens);

				break;
			case _MTLLIB: 
				materialFileName = tokens[1]; 
				parseMaterialFile(materialFileName);
				break;
			case _USEMTL:
				mtlName = tokens[1];
				activeMaterialIndex = getMaterialIndex(mtlName);
				break;
			}
		}
	}

	viewerObjects.push_back(tempObj);

}

void readControlFile(string controlFileName)
{
	fstream controlFile;
	string line, objFileName;
	vector<string > ctlLines, ctlTokens;
	GLuint i = 0, j, k = 0;
	GLuint cmdType;
	controlFile.open(controlFileName, ios::in);

	if (controlFile.is_open())
	{
		while (getline(controlFile, line))
			ctlLines.push_back(line);

		controlFile.close();
	}
	else
	{
		cout << "\nInvalid control filename. Re-start viewer with valid "
			"control filename" << endl;
		exit(EXIT_FAILURE);
	}


	// Count nummber of Objects
	for (i = 0; i < ctlLines.size(); ++i)
	{
		vector<string > countTokens;
		tokenizeGeneral(ctlLines[i], countTokens);
		if (!countTokens.empty())
			if (countTokens[0].compare(controls[_OBJ]) == 0)
				numObj++;
	}

	for (i = 0; i < ctlLines.size(); ++i)
	{
		tokenizeGeneral(ctlLines[i], ctlTokens);
		if (!ctlTokens.empty())
		{
			cmdType = _CTL_INVALID;
			for (k = _OBJ; k < _NUM_CTL_CMDS; ++k)
				if (ctlTokens[0].compare(controls[k]) == 0)
					cmdType = k;

			switch (cmdType)
			{
			case _OBJ:	objFileName = ctlTokens[1]; 
						readObjFile(objFileName);
						break;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
//	main
////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);// GLUT_COMPATIBILITY_PROFILE );
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glutCreateWindow(argv[0]);

	glewExperimental = GL_TRUE;	// added for glew to work!
	if (glewInit())
	{
		cerr << "Unable to initialize GLEW ... exiting" << endl;
		exit(EXIT_FAILURE);
	}

	// code from OpenGL 4 Shading Language cookbook, second edition
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	cout << "GL Vendor            :" << vendor << endl;
	cout << "GL Renderer          :" << renderer << endl;
	cout << "GL Version (string)  :" << version << endl;
	cout << "GL Version (integer) :" << major << " " << minor << endl;
	cout << "GLSL Version         :" << glslVersion << endl;

	GLuint flag = 0;
	string controlFileName;

	if (argc != 3)
		flag = 10;
	else
	{
		if (strcmp(argv[1], "-c") != 0)
			flag = 10;
		else controlFileName = argv[2];
	}

	if (flag)
	{
		cout << "Usage ::  viewer -c <controlFileName>" << endl;
		exit(EXIT_FAILURE);
	}

	//
	/*GLint nExtensions;
	glGetIntegerv( GL_NUM_EXTENSIONS, &nExtensions );
	for ( int i = 0; i < nExtensions; i++ )
		cout << glGetStringi( GL_EXTENSIONS, i )  << endl;*/

	readControlFile(controlFileName);

	/*init();
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);*/

	glutMainLoop();

	return 0;
}