
#include <string.h>

char *
str_combine(
	char					*dst,
	const char		*src1,
	int						src1_len,
	const char		*src2,
	int						src2_len
	)
{
	// set dst string NULL
	dst[0] = '\0';
	
	// add first src string into dst string
	if (src1_len == 0)
		strcat(dst, src1);		// src1 has '\0' at the end
	else
		strncat(dst, src1, src1_len);		// src1 is a char array, no '\0' at the end
		
	// append second src string into dst string
	if (src2_len == 0)	
		strcat(dst, src2);		// src2 has '\0' at the end
	else
		strncat(dst, src2, src2_len);		// src2 is a char array, no '\0' at the end

	return dst;
}


