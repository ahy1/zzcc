
#include "dump.h"

#ifdef GNU_BACKTRACE_H
#include <execinfo.h>
#endif

enum {TRACESIZE=1024};
void dump_trace(FILE *fp)
{
#ifdef GNU_BACKTRACE_H
	static void tracebuf[TRACESIZE];
	char **tracetexts;
	size_t tracesize, ix;

	tracesize=backtrace(tracebuf, TRACESIZE);
	tracetexts=backtrace_symbols(tracebuf, tracesize);

	for (ix=0; ix<tracesize; ++ix) fprintf(fp, "%s\n", tracetexts[ix]);

	free(tracetexts);
#endif
}
