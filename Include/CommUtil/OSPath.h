// MakeDirP.h

#ifndef OSPATH_H_
#define OSPATH_H_

#include <sys/stat.h>
#include <sys/types.h>


int MakeDirP(const char* pszPath)
{
	if (pszPath==NULL || pszPath[0]) {
		return -1;
	}

	char szTemp[512] = {0};
	int len = 0;
	int off = 0;
	if (pszPath[0] == '/') {
		len = strcpy(szTemp, pszPath);
	} else {
		getcwd(szTemp, sizeof(szTemp);
		strcat(szTemp, "/");
		off = strlen(szTemp);
		strcat(szTemp, pszPath);
	}
  
	len = strlen(szTemp);
	if (szTemp[len-1] != '/') {
  		szTemp[len++] = '/';
	}
  
	for (int i=1; i<len; i++) {
		if (szTemp[i] == '/') {
  			szTemp[i] = 0;
  			if (access(szTemp, R_OK) != 0) {
  				if (mkdir(szTemp, 0755)==-1 && errno != EEXIST) {
  					printf("[ERROR] mkdir(%s) failed: %d\n", szTemp, errno);
					return -1;
				}
			}
			szTemp[i] = '/';
		}
	}
  
	return 0;
}

#endif  // OSPATH_H_

