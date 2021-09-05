# clockwork


# Grammar

## Utility rules
function    -> IDENTIFIER "(" parameters? ")" block ;
parameters  -> IDENTIFIER ( "," IDENTIFIER )* ;
arguments   -> expression ( "," expression )* ;

## Lexical Grammar
NUMBER      -> FLOAT | NUMBER
FLOAT       -> '-'? DIGIT* ('.' DIGIT+)
INTEGER     -> '-'? DIGIT+ 
STRING      -> '"' <any char except '"'>* '"'
IDENTIFIER  -> ALPHA ( ALPHA | DIGIT )*
ALPHA       -> 'a' ... 'z' | 'A' ... 'Z' | '_'
DIGIT       -> '0' ... '9'