

#include "json.h"

#include <string.h>
#include <stdio.h>

const char *json_str(const char *str, char *buf, size_t len)
{
	size_t ix, bufix=0;
	size_t sl;

	sl=strlen(str);

	// TODO: Check all bufix>len-n expressions for correctness
	/*if (bufix>len-2) return NULL;*/
	buf[bufix++]='"';
	for (ix=0; ix<sl; ++ix) {
		switch (str[ix]) {
		case '\\':
		case '\0':
		case '\?':
		case '\"':
		case '\'':
			buf[bufix++]=str[ix];
			break;
		default:
			if (str[ix]<' ' || str[ix]>'\x7e') {
				if (bufix>len-3) return NULL;
				buf[bufix++]='\\';
				switch (str[ix]) {
				//case '\a':
				//	buf[bufix++]='a';
				//	break;
				case '\b':
					buf[bufix++]='b';
					break;
				case '\f':
					buf[bufix++]='f';
					break;
				case '\n':
					buf[bufix++]='n';
					break;
				case '\r':
					buf[bufix++]='r';
					break;
				case '\t':
					buf[bufix++]='t';
					break;
				//case '\v':
				//	buf[bufix++]='v';
				//	break;
				default:
					if (bufix>len-4) return NULL;
					sprintf(buf+ix, "u%04x", 
						(unsigned int)buf[bufix++]);

				}
			} else {
				if (bufix>len-2) return NULL;
				buf[bufix++]=str[ix];
			}
		}
	}

	buf[bufix++]='\"';
	buf[bufix]='\0';

	return buf;
}

