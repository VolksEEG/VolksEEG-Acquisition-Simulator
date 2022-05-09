#include <stdio.h>
#include <stdlib.h>
#include <iostream> 
#include <string>

using namespace std;

string CharArrayToString(char* charArray, int charArrayLength)
{
    return string(charArray, charArrayLength);
}

float CharArrayToFloat(char* charArray, int charArrayLength)
{
    string charsAsString = CharArrayToString(charArray, charArrayLength);
    return stof(charsAsString);
}