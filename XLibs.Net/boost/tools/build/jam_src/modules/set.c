
#include "../native.h"

/*
    local result = ;
    local element ;
    for element in $(B)
    {
        if ! ( $(element) in $(A) )
        {
            result += $(element) ;
        }
    }
    return $(result) ;
*/
LIST *set_difference( PARSE *parse, FRAME *frame )
{

    LIST* b = lol_get( frame->args, 0 );    
    LIST* a = lol_get( frame->args, 1 );    

    LIST* result = 0;
    for(; b; b = b->next)
    {
        if (!list_in(a, b->string))
            result = list_new(result, b->string);
    }
    return result;
}

void init_set()
{
    {
        char* args[] = { "B", "*", ":", "A", "*", 0 };
        declare_native_rule("set", "difference", args, set_difference);
    }

}
