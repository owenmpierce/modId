#ifndef evaldH
#define evaldH

#define VARLEN          20              /* Max length of variable names */
#define MAXVARS         50              /* Max user-defined variables */
#define MAXARGS          5              /* Max number of function args */
#define TOKLEN          30              /* Max token length */

#define VAR             1
#define DEL             2
#define NUM             3

typedef struct
{
   char name[VARLEN + 1];               /* Variable name */
   double value;                          /* Variable value */
} VARIABLE;

typedef struct
{
   char *name;                          /* Function name */
   int   args;                          /* Number of arguments to expect */
   double (*func1)(double);             /* Pointer to function */
   double (*func2)(double,double);
   double (*func3)(double,double,double);
   double (*func4)(double,double,double,double);
   double (*func5)(double,double,double,double,double);
} FUNCTION;

/* The following macros are ASCII dependant, no EBCDIC here! */
#define iswhite(c)  (c == ' ' || c == '\t')
#define isnumer(c)  ((c >= '0' && c <= '9') || c == '.')
#define isdelim(c)  (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' \
                    || c == '^' || c == '(' || c == ')' || c == ',' || c == '=')

/* Codes returned from the evaluator */
#define E_OK           0        /* Successful evaluation */
#define E_SYNTAX       1        /* Syntax error */
#define E_UNBALAN      2        /* Unbalanced parenthesis */
#define E_DIVZERO      3        /* Attempted division by zero */
#define E_UNKNOWN      4        /* Reference to unknown variable */
#define E_MAXVARS      5        /* Maximum variables exceeded */
#define E_BADFUNC      6        /* Unrecognised function */
#define E_NUMARGS      7        /* Wrong number of arguments to function */
#define E_NOARG        8        /* Missing an argument to a function */
#define E_EMPTY        9        /* Empty expression */
#define E_MATH         10       /* Mathematical Error */

static char __attribute__ ((unused)) *ErrMsgs[] = {"OK",
                "Syntax error",
                "Unbalanced parenthesis",
                "Divide by zero",
                "Unknown variable",
                "Maximum variables exceeded",
                "Unrecognized function",
                "Wrong number of arguments",
                "Missing argument",
                "Empty expression",
                "Math Error"
                };
int EvaluateD(char *e,double *res,int *epos);
int GetNumArgs(int idx);
char *GetFuncs(int idx);
char *GetConst(int idx);
#endif
