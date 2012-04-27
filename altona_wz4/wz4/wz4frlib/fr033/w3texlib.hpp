// .theprodukkt werkkzeug3 texture generator library
// Updated header v1.1 (Dec 19, 2007)

#ifndef __TP_W3TEXLIB_HPP__
#define __TP_W3TEXLIB_HPP__

// Enumeration of image formats.
enum W3ImageFormat
{
  W3IF_NONE = 0,        // for empty W3Images
  W3IF_BGRA8 = 1,       // B,G,R,A for each pixel, 8bits/component
  W3IF_UVWQ8 = 2,       // U,V,W,Q for each pixel (UVW=normalized vectors), 8bits/component *signed*!
  W3IF_DXT1 = 3,        // DXT1 compressed (no alpha, 64bits / 4x4 pixel block)
  W3IF_DXT5 = 4,        // DXT5 compressed (with alpha, 128bits / 4x4 pixel block)
};

// Basic image structure. Just raw data intended to be easy to pass
// to your graphics API of choice.
struct W3Image
{
  int XSize;            // Width of the image
  int YSize;            // Height of the image
  W3ImageFormat Format; // Image format used.

  int MipLevels;        // # of mip levels used.
  int MipSize[16];      // size of data for each mip level (bytes)
  unsigned char *MipData[16]; // data for each mip level.

  // Newly created W3Images *do not allocate memory for miplevels*!
  // This is handled internally during format conversion.
  W3Image(int xRes,int yRes,W3ImageFormat fmt);
  W3Image(const W3Image &x);
  ~W3Image();

  W3Image &operator =(const W3Image &x);

  void Swap(W3Image &x);
};

// The progress callback gets called every time computation of an operator
// is completed. Return true to continue, false to abort calculation (e.g.
// in response to user events)
typedef bool (*W3ProgressCallback)(int current,int max);

// The texture calculation context. Use this to load texture sets
// and calculate the corresponding images.
class W3TextureContext
{
  struct Private;

  Private *d;

public:
  W3TextureContext();
  ~W3TextureContext();

  // Loading data, calculation and management
  bool Load(const unsigned char *data);
  bool Calculate(W3ProgressCallback progress=0);
  void FreeDocument();  // Frees document and related data, keeps images.
  void FreeImages();    // Frees images and image metadata (names)

  // Image access
  // CHANGE as of v1.1 (Dec 19, 2007): GetImageCount()/GetImageName() work directly after Load(),
  // without calling Calculate() first. (GetImageByName/Number always return 0 when not calc'd).
  int GetImageCount() const;
  const W3Image *GetImageByName(const char *name) const;
  const W3Image *GetImageByNumber(int index) const;
  const char *GetImageName(int index) const;
};

#endif