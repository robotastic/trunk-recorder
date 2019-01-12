#ifndef INCLUDED_AMBE_H
#define INCLUDED_AMBE_H
#ifdef __cplusplus
extern "C" {
#endif
int mbe_dequantizeAmbe2250Parms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b);
int mbe_dequantizeAmbe2400Parms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b);
int mbe_dequantizeAmbeTone(mbe_tone * tone, const int *u);
#ifdef __cplusplus
}
#endif
#endif /* INCLUDED_AMBE_H */
