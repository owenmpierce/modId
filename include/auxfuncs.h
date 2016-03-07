/*
   Add any math functions that you wish to recognise here...  The first
   field is the name of the function as it would appear in an expression.
   The second field tells how many arguments to expect.  The third is
   a pointer to the actual function to use. Number of arguments currently
   limited to 4. When more than one argument is indicated, pad the space
   between the number and the function pointer with comma separated NULLs.
*/

/* name, argnum, function to call */

//power functions
//{ "pow",          2,      NULL, ed_pow },
{ "sqrt",         1,      ed_sqrt },

//exponential and logarithmic
{ "exp",          1,      exp },
{ "log",          1,      log10 },
{ "ln",           1,      log },

//trigonometric
{ "sin",          1,         sin },
{ "cos",          1,         cos },
{ "tan",          1,         tan },
{ "asin",         1,        asin },
{ "acos",         1,        acos },
{ "atan",         1,        atan },
{ "sec",          1,      ed_sec },
{ "csc",          1,      ed_csc },
{ "cot",          1,      ed_cot },

//hyperbolic
{ "cosh",         1,        cosh },
{ "sinh",         1,        sinh },
{ "tanh",         1,        tanh },
{ "asinh",        1,        asinh },
{ "acosh",        1,        acosh },
{ "atanh",        1,        atanh },

//rounding, absval, and remainder functions

{ "sgn",         1,          ed_sgn },
{ "ceil",         1,          ceil },
{ "abs",          1,          fabs },
{ "floor",          1,        floor },
//{ "mod",          2,        NULL, mod},


//random
{ "rnd",          1,     ed_rnd },


#if 0
{ "Sin",         1,      Sin },
{ "Cos",         1,      Cos },
{ "CosDG",       1,      cosdg },
{ "Tan",         1,      Tan },
{ "TanDG",       1,      tandg },
{ "Sec",         1,      sec },
{ "Csc",         1,      csc },
{ "Cot",         1,      cot },
{ "ASin",        1,      Asin },
{ "ACos",        1,      Acos },
{ "ATan",        1,      Atan },
{ "ATan2",       2,      NULL,Atan2 },
{ "Sinh",        1,      Sinh },
{ "Cosh",        1,      Cosh },
{ "Tanh",        1,      Tanh },
{ "ASinh",       1,      Asinh },
{ "ACosh",       1,      Acosh },
{ "ATanh",       1,      Atanh },
{ "Exp",         1,      Exp },
{ "Log",         1,      Log },
{ "Log10",       1,      Log10 },
{ "Sqrt",        1,      Sqrt },
{ "Floor",       1,      Floor },
{ "Ceil",        1,      Ceil },
{ "Abs",         1,      Fabs },
{ "Gamma",       1,      gamma },
{ "LogGamma",    1,      lgam },
{ "Beta",        2,      NULL,beta },
{ "Psi",         1,      psi },
{ "J0",          1,      j0 },
{ "J1",          1,      j1 },
{ "Jv",          2,      NULL,jv },
{ "Y0",          1,      y0 },
{ "Y1",          1,      y1 },
{ "Yv",          2,      NULL,yv },
{ "K0",          1,      k0 },
{ "K1",          1,      k1 },
{ "I0",          1,      i0 },
{ "I1",          1,      i1 },
{ "Iv",          2,      NULL,iv },
{ "NormDist",    1,      ndtr },
{ "InvNormDist", 1,      ndtri },
{ "Q",           1,      Ql },
{ "Erf",         1,      erf },
{ "Erfc",        1,      erfc },
{ "HyperGeo",    3,      NULL,NULL,hyperg },
{ "Hyp2F1",      4,      NULL,NULL,NULL,hyp2f1 },
{ "Ellpk",       1,      ellpk },
{ "Ellik",       2,      NULL,ellik },
{ "Ellpe",       1,      ellpe },
{ "Ellie",       2,      NULL,ellie },
{ "SinDG",       1,      sindg }, 
{ "Struve",      2,      NULL,struve },
{ "Si",             1,      sinint },
{ "Ci",             1,      cosint },
{ "Shi",            1,      sinhint },
{ "Chi",            1,      coshint },
{ "Lorentzian",     1,      lorz },
{ "Jn",             2,      NULL,Jn },
{ "Yn",             2,      NULL,Yn },
{ "Kn",             2,      NULL,Kn },
#endif
