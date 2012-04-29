// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __WAVEFRONT_HPP__
#define __WAVEFRONT_HPP__

#include "_types.hpp"

#undef new

#include <vector>
#include <string>

#define new new(__FILE__,__LINE__)

//-----------------------------------------------------------------------------

class WavefrontFileReader
{
public:
	virtual ~WavefrontFileReader();

	bool read(const char* filename);

protected:
  std::string basepath;

	const char* curLine;

	int lineNo;
	bool errThisLine;
	int errors;

	static bool matchCommand(const char*& current, const char* cmd);
	virtual void diagnostic(bool warning, const char* current, const char* message);

	bool parse(char* buffer, char* end);
	virtual void parseLine(const char*& current, const char* endOfLine) = 0;
	
	int parseInt(const char*& current);
	float parseFloat(const char*& current);
	sVector parseVec2(const char*& current);
	sVector parseVec3(const char*& current);
	sVector parseColor(const char*& current);

	virtual void onStartParse();
	virtual void onSuccessfulParse();
};

//-----------------------------------------------------------------------------

struct WavefrontMaterial
{
	std::string name;
	sVector ambient;
	sVector diffuse;
	sVector specular;
	float specularExp;
	float refractiveInd;
	float opacity;
  int illuminationModel;

	std::string diffuseTex;
	std::string bumpTex;
	std::string opacityTex;

	WavefrontMaterial();

	bool operator < (const WavefrontMaterial& b) const { return name < b.name; }
};

//-----------------------------------------------------------------------------

class MTLFileReader : public WavefrontFileReader
{
public:
	std::vector<WavefrontMaterial> materials;
	WavefrontMaterial err;

protected:
	WavefrontMaterial& curMaterial();

	virtual void parseLine(const char*& current, const char* endOfLine);
	virtual void onStartParse();
	virtual void onSuccessfulParse();
};

//-----------------------------------------------------------------------------

class OBJFileReader : public WavefrontFileReader
{
public:
	struct VertInfo
	{
		int pos,norm,tex;
	};

	struct FaceInfo
	{
		size_t start;
		int count,mtrl;
    int smoothing_group;
	};

	std::vector<sVector> positions;
	std::vector<sVector> normals;
	std::vector<sVector> texcoords;

	std::vector<VertInfo> vertices;
	std::vector<FaceInfo> faces;
	std::vector<WavefrontMaterial> materials;

protected:
	int currentMaterial;
	int defaultMaterial;
  int currentSmoothingGroup;

	virtual void parseLine(const char*& current, const char* endOfLine);
	virtual void onStartParse();
	virtual void onSuccessfulParse();

	int findMaterial(const char* name);

	int parseIndex(const char*& current, int max);
};

//-----------------------------------------------------------------------------

#endif
