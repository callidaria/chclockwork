#include "autodoc.h"


/**
 *	function to generate full documentation for an entire submodule
 *	\param name: string representation of module and headline of module document index
 *	\param path: path to module subfolder e.g. core/ (will be normalized towards project root by single ../)
  *	\param format: output format for api visualization (.view AutodocOutputFormat)
 */
void _generate_submodule(const char* name,const char* path,AutodocOutputFormat format)
{
	printf("generating submodule \"%s\" from source path \"%s\"",name,path);
	// TODO
}
// TODO do not expose in header and inline


/**
 *	call this to automatically generate in-code documentation
 *	it includes the files in core, script and tool. yes, even this file!
 *	\param format: output format for api visualization (.view AutodocOutputFormat)
 *	\param genflags: define which modules to generate in documentation process (.view AutodocGenflags)
 */
void generate_documentation(AutodocOutputFormat format,AutodocGenflags genflags)
{
	// generate core document
	if (genflags|AUTODOC_GENERATE_CORE) _generate_submodule("Core","core/");
	else printf("skipping document generation for core submodule.");

	// generate script document
	if (genflags|AUTODOC_GENERATE_SCRIPT) _generate_submodule("Scripts","script/");
	else printf("skipping document generation for script submodule.");

	// generate tool document
	if (genflags|AUTODOC_GENERATE_TOOL) _generate_submodule("Tools","/tool");
	else printf("skipping document generation for tool submodule.");
}
