/*
 * BCD.c
 *
 *  Created on: Mar 12, 2014
 *      Author: lifeng
 */



char to_bcd(char c)
{
	return (c/10<<4)|(c%10);
}
char from_bcd(char bcd)
{
	return (bcd>>4)*10+(bcd&0x0f);
}
