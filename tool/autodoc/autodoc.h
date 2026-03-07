#ifndef TOOL_AUTODOC_HEADER
#define TOOL_AUTODOC_HEADER

#include <stdio.h>
#include <stdlib.h>


enum AutodocOutputFormat
{
	AUTODOC_FORMAT_MARKDOWN,
	AUTODOC_FORMAT_HTML,
	AUTODOC_FORMAT_ORGMODE,
};


enum AutodocGenflags
{
	AUTODOC_GENERATE_CORE = 1<<0,
	AUTODOC_GENERATE_SCRIPT = 1<<1,
	AUTODOC_GENERATE_TOOL = 1<<2,
};


void _generate_submodule(const char* name,const char* path);
void generate_documentation(AutodocOutputFormat format,AutodocGenflags genflags);


#endif
