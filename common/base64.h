
#ifndef _BASE64_H_
#define _BASE64_H_

#ifdef __cplusplus 
extern "C" {
#endif
	int base64_decode(unsigned char* output, unsigned char* bytes_to_decode);
	char* base64_encode(unsigned char const* bytes_to_encode, unsigned int len);
#ifdef __cplusplus 
}
#endif

#endif