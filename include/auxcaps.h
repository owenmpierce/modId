#define AUXCONSTS
#define AUXFUNCS
#include <float.h>

int mtherr(char *,int);

double ed_sgn(double x) {
    if (x==0) return 0;
    return x>0?1:-1;
}

double ed_sqrt(double x) {
    if (x < 0) {
        ERRNUM = E_MATH;
        return 0;
    }
    return sqrt(x);
}

// returns a random number between -x and x
double ed_rnd(double x) {
    return ((double)-0.5+(double)rand()/(double)RAND_MAX)*x;
}

double ed_sin(double x) {
    return sin(x);
}

double ed_cos(double x) {
    return cos(x);
}

double ed_tan(double x) {
    return tan(x);
}

double ed_sec(double x)
{
    double t;

    if ((t=cos(x)) == 0) {
        //mtherr("sec",OVERFLOW);
        return DBL_MAX;
    }
    return 1.0/t;
}
double ed_csc(double x)
{
    double t;

    if ((t=sin(x))==0){
        //mtherr("csc",OVERFLOW);
        return DBL_MAX;
    }
    return 1.0/t;
}
double ed_cot(double x)
{
    double t;
    if ((t=tan(x)) == 0) {
        //mtherr("cot",OVERFLOW);
        return DBL_MAX;
    }
    return 1.0/t;
}

/*
double lorz(double x)
{
    return 1.0/(1.0+x*x);
}
double Jn(double n,double x)
{
    return jn((int)(n+0.5),x);
}
double Kn(double n,double x)
{
    return kn((int)(n+0.5),x);
}
double Yn(double n,double x)
{
    return yn((int)(n+0.5),x);
}
double sinint(double x)
{
    double si,ci;
    sici(x,&si,&ci);
    return si;
}
double cosint(double x)
{
    double si,ci;
    sici(x,&si,&ci);
    return ci;
}
double sinhint(double x)
{
    double shi,chi;
    shichi(x,&shi,&chi);
    return shi;
}
double coshint(double x)
{
    double shi,chi;
    shichi(x,&shi,&chi);
    return chi;
}*/
