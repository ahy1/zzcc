
#ifndef TOKENCLASS_H_INCLUDED
#define TOKENCLASS_H_INCLUDED

#include "token.h"

int istype(const struct token_s *token);
int isstorage(const struct token_s *token);
int istypedef(const struct token_s *token);
/*int isstruct(const struct token_s *token);
int isunion(const struct token_s *token);*/
int isqualifier(const struct token_s *token);
int isfuncspec(const struct token_s *token);
int isname(const struct token_s *token);
int isnumber(const struct token_s *token);
int isws(const struct token_s *token);
int isop(const struct token_s *token);
int isstart(const struct token_s *token);
int isend(const struct token_s *token);
int isleft(const struct token_s *token);
int isright(const struct token_s *token);
int ismatchingright(const struct token_s *start, const struct token_s *end);
int isbracket(const struct token_s *token);
int isvalue(const struct token_s *token);
int istobeignored(const struct token_s *token);
int issemicolon(const struct token_s *token);
int isgoto(const struct token_s *token);
int isbreak(const struct token_s *token);
int iscontinue(const struct token_s *token);

#endif

