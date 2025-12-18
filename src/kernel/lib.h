#ifndef __LIB_H__
#define __LIB_H__

// Memory operations

inline void * memcpy(void *From, void *To, long Num)
{
	int d0, d1, d2;
	__asm__ __volatile__ (
		"cld	\n\t"
		"rep	\n\t"
		"movsq	\n\t"
		"testb	$4,%b4	\n\t"
		"je	1f	\n\t"
		"movsl	\n\t"
		"1:\ttestb	$2,%b4	\n\t"
		"je	2f	\n\t"
		"movsw	\n\t"
		"2:\ttestb	$1,%b4	\n\t"
		"je	3f	\n\t"
		"movsb	\n\t"
		"3:	\n\t"
		:"=&c"(d0),"=&D"(d1),"=&S"(d2)
		:"0"(Num/8),"q"(Num),"1"(To),"2"(From)
		:"memory"
	);
	return To;
}

// Compare the first count bytes

inline int memcmp(void *FirstPart, void *SecondPart, long Count)
{
	register int __res;

	__asm__ __volatile__ (	
		"cld	\n\t"		//clean direct
		"repe	\n\t"		//repeat if equal
		"cmpsb	\n\t"
		"je	1f	\n\t"
		"movl	$1,	%%eax	\n\t"
		"jl	1f	\n\t"
		"negl	%%eax	\n\t"
		"1:	\n\t"
		:"=a"(__res)
		:"0"(0),"D"(FirstPart),"S"(SecondPart),"c"(Count)
		:
	);
	return __res;
}

// Address memory

inline void * memset(void *Address, unsigned char C, long Count)
{
	int d0, d1;
	unsigned long tmp = C * 0x0101010101010101UL;
	__asm__ __volatile__ (	
		"cld	\n\t"
		"rep	\n\t"
		"stosq	\n\t"
		"testb	$4, %b3	\n\t"
		"je	1f	\n\t"
		"stosl	\n\t"
		"1:\ttestb	$2, %b3	\n\t"
		"je	2f\n\t"
		"stosw	\n\t"
		"2:\ttestb	$1, %b3	\n\t"
		"je	3f	\n\t"
		"stosb	\n\t"
		"3:	\n\t"
		:"=&c"(d0),"=&D"(d1)
		:"a"(tmp),"q"(Count),"0"(Count/8),"1"(Address)	
		:"memory"					
	);
	return Address;
}

// Bit manipulation

inline unsigned long bit_set(unsigned long *addr, unsigned long nr)
{
	return *addr | (1UL << nr);
}

inline unsigned long bit_get(unsigned long *addr, unsigned long nr)
{
	return *addr & (1UL << nr);
}

inline unsigned long bit_clean(unsigned long *addr, unsigned long nr)
{
	return *addr & (~(1UL << nr));
}

inline int strlen(const char *String)
{
	int i = 0;
    while (String[i] != '\0') {
        i++;
    }
    return i;
}

inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

inline char * strncpy(char *dest, const char *src, int n) {
    char *d = dest;
    
    while (n > 0 && *src != '\0') {
        *d++ = *src++;
        n--;
    }
    
    while (n > 0) {
        *d++ = '\0';
        n--;
    }
    
    return dest;
}

inline char * str_copy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++)) ;
    return dest;
}

#endif