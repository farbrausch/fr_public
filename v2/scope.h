#ifndef __SCOPE_H__INCLUDED__
#define __SCOPE_H__INCLUDED__

extern void scopeOpen(int index, const char *title, int nsamples, int w, int h);
extern void scopeClose(int index);
extern void scopeSubmit(int index, const float *data, int nsamples);
extern void scopeUpdateAll();

#endif