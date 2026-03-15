#include "autodoc.h"


// ----------------------------------------------------------------------------------------------------
// Formatting Commands

const char* _headline_starter[AUTODOC_FORMAT_COUNT] = { "##","<h2>","**" };
const char* _headline_terminator[AUTODOC_FORMAT_COUNT] = { "\n","</h2>\n","\n" };

/**
 *	format given text as standard document headline based on output format
 *	\param file: destination file
 *	\param format: output document format
 *	\param text: headline text
 */
inline void _write_headline(FILE* file,AutodocOutputFormat format,const char* text)
{
	fwrite(_headline_starter[format],sizeof(char),sizeof(_headline_starter[format]),file);
	fwrite(text,sizeof(char),sizeof(text),file);
	fwrite(_headline_terminator[format],sizeof(char),sizeof(_headline_terminator[format]),file);
}


// ----------------------------------------------------------------------------------------------------
// Processing

/**
 *	function to generate full documentation for an entire submodule
 *	\param name: string representation of module and headline of module document index
 *	\param path: path to module subfolder e.g. core/ (will be normalized towards project root by single ../)
  *	\param format: output format for api visualization (.view AutodocOutputFormat)
 */
inline void _generate_submodule(const char* name,const char* path,AutodocOutputFormat format)
{
	printf("generating submodule \"%s\" from source path \"%s\"\n",name,path);
	// TODO
}


// ----------------------------------------------------------------------------------------------------
// Feature

/**
 *	call this to automatically generate in-code documentation
 *	it includes the files in core, script and tool. yes, even this file!
 *	\param format: output format for api visualization (.view AutodocOutputFormat)
 *	\param genflags: define which modules to generate in documentation process (.view AutodocGenflags)
 */
void generate_documentation(AutodocOutputFormat format,AutodocGenflags genflags)
{
	// generate docfile index
	FILE* __File = fopen("../doc/api","w");
	_write_headline(__File,format,"Project Document");
	fclose(__File);

	// generate core document
	if (genflags&AUTODOC_GENERATE_CORE) _generate_submodule("Core","core/",format);
	else printf("skipping document generation for core submodule.\n");

	// generate script document
	if (genflags&AUTODOC_GENERATE_SCRIPT) _generate_submodule("Scripts","script/",format);
	else printf("skipping document generation for script submodule.\n");

	// generate tool document
	if (genflags&AUTODOC_GENERATE_TOOL) _generate_submodule("Tools","tool/",format);
	else printf("skipping document generation for tool submodule.\n");
}
