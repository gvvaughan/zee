#include <stdlib.h>
#include <string.h>

#include "parser.h"

    
struct le *
parseInFile(
	    getcCallback getachar,
	    ungetcCallback ungetachar,
	    struct le * list,
	    int * line
            )
{
  char * temp = NULL;
  enum tokenname tokenid = T_NONE;
  int isquoted = 0;

  if (!getachar || !ungetachar)  return( NULL );

  while (1){
	    
    temp = snagAToken(getachar, ungetachar, &tokenid);

    switch (tokenid)
      {
      case (T_NONE):
        break;

      case (T_QUOTE):
        isquoted = 1;
        break;

      case (T_OPENPAREN):
        list = leAddBranchElement(
                                  list, 
                                  parseInFile(getachar, 
                                              ungetachar, 
                                              NULL,
                                              line),
                                  isquoted
                                  );
        isquoted = 0;
        break;

      case (T_NEWLINE):
        isquoted = 0;
        *line = *line +1;
        break;

      case (T_WORD):
        list = leAddDataElement(
				list,
				temp,
				isquoted
				);
        free(temp);
        isquoted = 0;
        break;
	    
      case (T_CLOSEPAREN):
      case (T_EOF):
        isquoted = 0;
        return (list);
      }
  }
}
    
char *
snagAToken(
           getcCallback getachar,
           ungetcCallback ungetachar,
           enum tokenname * tokenid
           )
{
  unsigned int pos = 0;
  int c;
  int doublequotes = 0;
  char temp[128];

  *tokenid = T_EOF;

  if (!getachar || !ungetachar)
    {
      *tokenid = T_EOF;
      return( NULL );
    }

  /* chew space to next token */
  while (1)
    {
      c = getachar();

      /* munch comments */
      if (    (c == '#') 
              || (c == ';')
              )
        {
          do {
            c = getachar();
          } while (c != '\n');
        }
		
      if ((    (c == '(')
               || (c == ')')
               || (c == '\n')
               || (c == '\"')
               || (c == '\'')
               || (c == EOF)
               || (c > '-')
               || (c <= 'z')
	       ) && ( c != ' ') && ( c != '\t') )
        {
          break;
        }
    }

  /* snag token */
  if (c == '(')
    {
      *tokenid = T_OPENPAREN;
      return( NULL );
    } else 

      if (c == ')')
	{
          *tokenid = T_CLOSEPAREN;
          return( NULL );
	} else 

          if (c == '\'')
            {
              *tokenid = T_QUOTE;
              return( NULL );
            } else 

              if (c == '\n')
                {
                  *tokenid = T_NEWLINE;
                  return( NULL );
                } else 

                  if (c == EOF)
                    {
                      *tokenid = T_EOF;
                      return( NULL );
                    }

  /* oh well. it looks like a string.  snag to the next whitespace. */

  if (c == '\"')
    {
      doublequotes = 1;
      c = getachar();
    }


  while (1)
    {
      temp[pos++] = (char) c;

      if (!doublequotes)
        { 
          if (    (c == ')')
                  || (c == '(')
                  || (c == ';')
                  || (c == '#')
                  || (c == ' ')
                  || (c == '\n')
                  || (c == '\r')
                  || (c == EOF)
                  )
            {
              ungetachar(c);
              temp[pos-1] = '\0';

              if ( !strcmp(temp, "quote") )
                {
                  *tokenid = T_QUOTE;
                  return( NULL );
                }
              *tokenid = T_WORD;
              return( strdup(temp) );
            }
        } else {
          switch (c)
            {
            case ( '\n' ):
            case ( '\r' ):
            case ( EOF ):
              ungetachar(c);

            case ( '\"' ):
              temp[pos-1] = '\0';
              *tokenid = T_WORD;
              return( strdup(temp) );
    
            }
        }

      c = getachar();
    }
  return( NULL );
}
