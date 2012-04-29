// This file is distributed under a BSD license. See LICENSE.txt for details.

#include <algorithm>
#include "_wavefront.hpp"

//-----------------------------------------------------------------------------

WavefrontFileReader::~WavefrontFileReader()
{
}

bool WavefrontFileReader::read(const char* filename)
{
  basepath = filename;
  int slash = basepath.find_last_of("\\/");
  if(slash == std::string::npos)
    basepath.clear();
  else
    basepath.erase(basepath.begin() + slash + 1, basepath.end());

	FILE* f = fopen(filename, "rb");
	bool ok = false;
	if(!f)
		return false;
	
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* buf = new char[size+1];
	if(buf && fread(buf, 1, size, f) == size)
	{
		buf[size] = '\n'; // sentinel
		ok = parse(buf, buf+size+1);
	}

	delete[] buf;

	fclose(f);
	return ok;
}

bool WavefrontFileReader::matchCommand(const char*& str, const char* cmd)
{
	int len = 0;
	while(str[len] == cmd[len] && cmd[len])
		len++;

	if(!cmd[len] && (!str[len] || isspace(str[len])))
	{
		str += len + (str[len] != 0);
		return true;
	}
	else
		return false;
}


void WavefrontFileReader::diagnostic(bool warning, const char* current, const char* message)
{
	if(warning || !errThisLine)
	{
		sDPrintF("%s%s in line %d, col %d\n", warning ? "warning: " : "error: ",
			message, lineNo, current - curLine + 1);

		if(!warning)
		{
			errThisLine = true;
			errors++;
		}
	}
}

bool WavefrontFileReader::parse(char* buffer, char* end)
{
	onStartParse();

	const char* current = buffer;
	lineNo = 0;
	errors = 0;

	// main line loop
	while(current != end)
	{
		curLine = current;
		lineNo++;
		errThisLine = false;

		// find end of line
		const char* endOfLine = current;
		while(*endOfLine != '\n')
			endOfLine++;

		// terminate line buffer
		const char* termPos = current + (endOfLine - current);
		while(termPos >= current && isspace(*termPos))
			termPos--;

		*const_cast<char*>(termPos+1) = 0;

		// parse it
		if(*current == '#') // comment
			current = endOfLine;
		else if(*current == '\r' || *current == 0) // empty line
			;
		else
			parseLine(current, termPos+1);

		// we should now be past the command, at the 0 terminator.
		if(*current)
		{
			if(!errThisLine)
				diagnostic(true, current, "extra characters on line");
		}

		current = endOfLine+1; // skip past newline/null
	}

	if(!errors)
		onSuccessfulParse();

	return !errors;
}

int WavefrontFileReader::parseInt(const char*& current)
{
#if 1
	const char* cur = current;
	unsigned int magn=0,sgn=0;
	int digits = 0;

	while(isspace(*cur))
		cur++;

	if(*cur == '-' || *cur == '+')
		sgn = *cur++ == '-';

	while(*cur >= '0' && *cur <= '9')
	{
		unsigned int mago = magn;
		magn = magn*10 + (*cur++ - '0');
		if(magn < mago)
			diagnostic(false, cur, "int overflow");
		digits++;
	}

	if(!digits)
		diagnostic(true, current, "int expected");

	current = cur;
	return sgn ? -static_cast<int>(magn) : static_cast<int>(magn);
#else
	const char* cur = current;
	long val = strtol(cur, const_cast<char**>(&current), 10);
	if(current == cur)
		diagnostic(true, current, "int expected");

	return static_cast<int>(val);
#endif
}

float WavefrontFileReader::parseFloat(const char*& current)
{
#if 1
	// strtod is awfully slow
	const char* cur = current;
	double intv=0.0,denom=1.0;
	bool intPart=true;

	while(isspace(*cur))
		cur++;

	if(*cur == '-' || *cur == '+')
		denom = (*cur++ == '-') ? -1.0 : 1.0;

	while(*cur >= '0' && *cur <= '9' || *cur == '.')
	{
		if(*cur != '.')
		{
			int digit = *cur - '0';
			intv = intv*10.0 + digit;
			if(!intPart)
				denom *= 10.0;
		}
		else
		{
			if(!intPart)
				break;

			intPart = false;
		}

		cur++;
	}

	double value = intv / denom;
	if(*cur == 'e' || *cur == 'E') // exponent follows
	{
		cur++;
		if(*cur == '+' || *cur == '-')
		{
			int neg = *cur++ == '-';
			int exp = 0;
			while(*cur >= '0' && *cur <= '9')
				exp = exp*10 + (*cur++ - '0');

			value *= pow(10.0, neg ? -exp : exp);
		}
	}

	if(*cur && !isspace(*cur))
	{
		diagnostic(true, cur, "float expected");
		
		// skip till next whitespace
		while(*cur && !isspace(*cur))
			cur++;
	}

	current = cur;
	return value;
#else
	const char* cur = current;
	double val = strtod(cur, const_cast<char**>(&current));
	if(current == cur)
		diagnostic(true, current, "float expected");

	return static_cast<float>(val);
#endif
}

sVector WavefrontFileReader::parseVec2(const char*& current)
{
	sVector v;
  v.Init();
	for(int i=0;i<2;i++)
		v[i] = parseFloat(current);

	return v;
}

sVector WavefrontFileReader::parseVec3(const char*& current)
{
  sVector v;
  v.Init();
	for(int i=0;i<3;i++)
		v[i] = parseFloat(current);

	return v;
}

sVector WavefrontFileReader::parseColor(const char*& current)
{
  sVector col;
  col.Init();

	if(matchCommand(current, "spectral"))
		diagnostic(false, current, "spectral colors not supported");
	else if(matchCommand(current, "xyz"))
		diagnostic(false, current, "XYZ colors not supported");
	else
	{
		for(int i=0;i<3;i++)
			col[i] = parseFloat(current);
	}

	return col;
}

void WavefrontFileReader::onStartParse()
{
}

void WavefrontFileReader::onSuccessfulParse()
{
}

//-----------------------------------------------------------------------------

WavefrontMaterial::WavefrontMaterial()
	: name("default")
{
  ambient.Init(0.1f,0.1f,0.1f,1.0f);
  diffuse.Init(0.7f,0.7f,0.7f,1.0f);
  specular.Init(0.2f,0.2f,0.2f,1.0f);
	specularExp = 16.0f;
	refractiveInd = 1.0f;
	opacity = 1.0f;
  illuminationModel = 0;
}

//-----------------------------------------------------------------------------

WavefrontMaterial& MTLFileReader::curMaterial()
{
	if(materials.size())
		return materials.back();
	else
	{
		diagnostic(false, curLine, "not allowed outside material definition");
		return err;
	}
}

void MTLFileReader::parseLine(const char*& current, const char* endOfLine)
{
	if(matchCommand(current, "newmtl"))
	{
		WavefrontMaterial mat;
		mat.name = current;
		materials.push_back(mat);

		current = endOfLine;
	}
	else if(matchCommand(current, "Ka")) // ambient color
		curMaterial().ambient = parseColor(current);
	else if(matchCommand(current, "Kd")) // diffuse color
		curMaterial().diffuse = parseColor(current);
	else if(matchCommand(current, "Ks")) // specular color
		curMaterial().specular = parseColor(current);
	else if(matchCommand(current, "Ns")) // specular exponent
		curMaterial().specularExp = parseFloat(current);
	else if(matchCommand(current, "Km")) // no idea what this is
		parseFloat(current);
	else if(matchCommand(current, "Ni")) // optical density
		curMaterial().refractiveInd = parseFloat(current);
  else if(matchCommand(current, "illum")) // illumination model
    curMaterial().illuminationModel = parseInt(current);
	else if(matchCommand(current, "d")) // dissolve
	{
		if(matchCommand(current, "-halo"))
			diagnostic(true, current, "dissolve halo not supported");

		curMaterial().opacity = parseFloat(current);
	}
	else if(matchCommand(current, "map_Kd")) // diffuse texture
	{
		curMaterial().diffuseTex = current;
		current = endOfLine;
	}
	else if(matchCommand(current, "map_Bump")) // bump texture
	{
		curMaterial().bumpTex = current;
		current = endOfLine;
	}
	else if(matchCommand(current, "map_d") || matchCommand(current, "map_D")) // opacity texture
	{
		curMaterial().opacityTex = current;
		current = endOfLine;
	}
	else
		diagnostic(false, current, "unknown command");
}

void MTLFileReader::onStartParse()
{
	materials.clear();
}

void MTLFileReader::onSuccessfulParse()
{
	// add default material
	WavefrontMaterial mat;
	materials.push_back(mat);

	// sort everything
	std::sort(materials.begin(), materials.end());
}

//-----------------------------------------------------------------------------

void OBJFileReader::parseLine(const char*& current, const char* endOfLine)
{
	if(matchCommand(current, "v"))
		positions.push_back(parseVec3(current));
	else if(matchCommand(current, "vn"))
		normals.push_back(parseVec3(current));
	else if(matchCommand(current, "vt"))
		texcoords.push_back(parseVec2(current));
	else if(matchCommand(current, "f")) // face
	{
		int count = 0;
		while(!errThisLine && *current)
		{
			VertInfo nfo;
			
			nfo.pos = parseIndex(current, positions.size());
			if(*current == '/')
				nfo.tex = parseIndex(++current, texcoords.size());
			else
				nfo.tex = -1;
			
			if(*current == '/')
				nfo.norm = parseIndex(++current, normals.size());
			else
				nfo.norm = -1;

			if(*current && !isspace(*current))
				diagnostic(false, current, "unexpected character after vertex indices");

			vertices.push_back(nfo);
			count++;
		}

		if(count < 3)
			diagnostic(false, current, "need at least 3 vertices for a face");
		else if(count > 4)
			diagnostic(true, current, "non-tri/quad face");

		FaceInfo nfo;
		nfo.start = vertices.size() - count;
		nfo.count = count;
		nfo.mtrl = currentMaterial;
    nfo.smoothing_group = currentSmoothingGroup;
		faces.push_back(nfo);
	}
	else if(matchCommand(current, "g")) // group
	{
		current = endOfLine;
	}
  else if(matchCommand(current, "o")) // object
  {
    current = endOfLine;
  }
  else if(matchCommand(current, "s")) // smooth shading
  {
    if(matchCommand(current, "off"))
      currentSmoothingGroup = -1;
    else
      currentSmoothingGroup = parseInt(current) - 1;
  }
	else if(matchCommand(current, "usemtl"))
	{
		currentMaterial = findMaterial(current);
		current = endOfLine;
	}
	else if(matchCommand(current, "mtllib"))
	{
		MTLFileReader mtlreader;
		if(!mtlreader.read((basepath + current).c_str()))
			diagnostic(false, current, "error reading material library");
    else
    {
		  materials.swap(mtlreader.materials);
		  current = endOfLine;

		  defaultMaterial = -1;
		  defaultMaterial = findMaterial("default");
		  sVERIFY(defaultMaterial != -1);
  		
		  currentMaterial = defaultMaterial;
    }
	}
	else
		diagnostic(false, current, "unknown command");
}

void OBJFileReader::onStartParse()
{
	positions.reserve(1000);
	normals.reserve(1000);
	texcoords.reserve(1000);

	vertices.reserve(1000);
	faces.reserve(1000);
	materials.reserve(100);

	positions.clear();
	normals.clear();
	texcoords.clear();

	vertices.clear();
	faces.clear();
	materials.clear();

	// add default material
	WavefrontMaterial mat;
	materials.push_back(mat);
	currentMaterial = 0;
  currentSmoothingGroup = -1;
}

void OBJFileReader::onSuccessfulParse()
{
	struct FaceMtrlLess
	{
		bool operator()(const FaceInfo& a, const FaceInfo& b) const
		{
			return a.mtrl < b.mtrl || a.mtrl == b.mtrl && a.start < b.start;
		}
	};

	std::sort(faces.begin(), faces.end(), FaceMtrlLess());
}

int OBJFileReader::findMaterial(const char* name)
{
	// binary search
	size_t l=0, r=materials.size();
	while(l<r)
	{
		size_t x = l + (r-l)/2;
		int cmp = materials[x].name.compare(name);

		if(cmp < 0) // mat[x]<name, continue right
			l = x+1;
		else if(cmp > 0) // mat[x]>name, continue left
			r = x;
		else // match!
			return x;
	}

	// no match found, report error and revert to default material
	diagnostic(true, curLine, "material not found, using default material");
	return defaultMaterial;
}

int OBJFileReader::parseIndex(const char*& current, int max)
{
	int val = parseInt(current);
	val += (val < 0) ? max : -1;

	if(val < 0 || val >= max)
	{
		diagnostic(false, current, "index out of range");
		val = 0;
	}

	return val;
}
