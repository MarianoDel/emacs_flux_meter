//---------------------------------------------
// ##
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ##
// #### DSP.H #################################
//---------------------------------------------

#ifndef DSP_H_
#define DSP_H_

// #define USE_PID_CONTROLLERS

#define MAFilter32Pote(X)  MAFilter32Circular(X, v_pote_samples, &v_pote_index, &pote_sumation)

unsigned short RandomGen (unsigned int);
unsigned char MAFilter (unsigned char, unsigned char *);
unsigned short MAFilterFast (unsigned short ,unsigned short *);
unsigned short MAFilter8 (unsigned short *);
unsigned short MAFilter32 (unsigned short, unsigned short *);

unsigned short MAFilter32Fast (unsigned short *);
unsigned short MAFilter32Circular (unsigned short, unsigned short *, unsigned char *, unsigned int *);

short PID (short, short);
short PID_roof (short, short, short, short *, short *);



#endif /* DSP_H_ */
