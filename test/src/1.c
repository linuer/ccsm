extern int z;
int x,t;
int y;

extern int gain();

int main( void )
{
main_start:
    if( x == 1 )
    {
        if( y == 2 )
        {
            if( t == 3 )
            {
                return 2;
            }
        } else {
            goto main_start;
        }
    }
main_end:
    return 1;
}

int k;

void vain( int i )
{
    int j;
    extern int p;
    for( j = 0;
         j < 2;
         j++ )
    {
    }
    switch( j )
    {
        case 1:
            break;
        case 2:
            break;
        default:
            break;
    }
    while( j > 0 )
    {
        j--;
    }
    if( 4 == 4 )
    {
        if( 5 == 5 )
        {
        } else if( 7 == 9 )
        {
        } else
        {
#if defined THIS_IS_DEFINED
            if( 9 == 1 )
            {
            }
#endif
#if defined THIS_IS_NOT_DEFINED
            for( j = 0;
                 j < 2;
                 j++ )
            {
            }
#endif
        }
    }
    else
    {
        return;
    }
}
