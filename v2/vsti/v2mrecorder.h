#ifndef V2MRECORDER_H_
#define V2MRECORDER_H_

class CV2MRecorder
{
public:
	void AddEvent(long sampletime, unsigned char b0, unsigned char b1, unsigned char b2);
	int  Export(file &f);

	explicit CV2MRecorder(sU32 framesize, sU32 samplerate);
	~CV2MRecorder();

private:
  struct PrivateData;
	PrivateData *p;
};


#endif