
#ifndef	BASE64_H_
#define	BASE64_H_

namespace NS_BASE64 {

	int Encode(const unsigned char* bindata, int binlength, char* base64);

	int Decode(const char* base64, unsigned char* bindata);

};

#endif	// BASE64_H_
