#include "autodoc/autodoc.h"


/**
 *	generate an error in case of insufficient parameter count
 *	\param count: actual parameter count
 *	\param req_param: amount of required parameters
 *	\returns 1 should parameter check be successful, 0 if insufficient paramter count
 */
inline int _check_parameter_count(int count,int req_param)
{
	if (count<=req_param)
	{
		printf("chcc tooling: not enough parameters given. must at least be %d\n",req_param);
		return 0;
	}
	return 1;
}


int main(int argc,char** argv)
{
	// test legal input
	if (!_check_parameter_count(argc,1)) return -1;

	// launcher switch autodocument
	if (strcmp(argv[1],"autodoc")==0)
	{
		if (!_check_parameter_count(argc,2)) return -1;
		generate_documentation(AUTODOC_FORMAT_MARKDOWN,(AutodocGenflags)((*argv[2])-'0'));
		// TODO when config feature is implemented, read format settings from tools configuration
	}

	// toolcall mismatch
	else printf("unknown tool command\n");
	return 0;
}
