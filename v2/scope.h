#ifndef __SCOPE_H__INCLUDED__
#define __SCOPE_H__INCLUDED__

extern void scopeOpen(const void *unique_id, const char *title, int nsamples, int w, int h);
extern void scopeClose(const void *unique_id);
extern bool scopeIsOpen(const void *unique_id);
extern void scopeSubmit(const void *unique_id, const float *data, int nsamples);
extern void scopeSubmitStrided(const void *unique_id, const float *data, int stride, int nsamples);
extern void scopeUpdateAll();

#endif