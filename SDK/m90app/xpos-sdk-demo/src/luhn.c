/**
 * File: luhn.c 
 * ------------
 * Implements luhn.h's interface.
 * @author Opeyemi Ajani.
 */

#include <string.h>
#include <stdio.h>

#include "luhn.h"

short checkLuhn(const char * cardNo, const int size) 
{ 
    int nDigits = size; 
    int nSum = 0, isSecond = 0; 
    int i;
    for (i = nDigits - 1; i >= 0; i--) { 
        int d = cardNo[i] - '0'; 
  
        if (isSecond) 
            d = d * 2; 
  
        nSum += d / 10; 
        nSum += d % 10; 
  
        isSecond = !isSecond; 
    } 
    return (nSum % 10 == 0); 
} 

int getLuhnDigit(char *number, const int size)
{
    int i, sum, ch, num, twoup, len;

    len = size;
    sum = 0;
    twoup = 1;
    
    for (i = len - 1; i >= 0; --i) {
        ch = number[i];
        num = (ch >= '0' && ch <= '9') ? ch - '0' : 0;
        if (twoup) {
            num += num;
            if (num > 9) num = (num % 10) + 1;
        }
        sum += num;
        twoup = ++twoup & 1;
    }

    printf("Sum -> %d\n", sum);
    sum = 10 - (sum % 10);
    if (sum == 10) sum = 0;
    return sum;
}
