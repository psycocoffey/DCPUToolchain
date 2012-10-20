#include "dcputoolchain_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <bstring.h>
#include <argtable2.h>
#include <bfile.h>
#include <iio.h>
#include <osutil.h>
#include <pp.h>
#include <ppfind.h>
#include <version.h>
#include <simclist.h>
#include <ddata.h>
#include <debug.h>
#include <assem.h>
#include <node.h>
#include <derr.h>
#include <aout.h>


extern int yyparse();
extern FILE* yyin, *yyout;
char* fileloc = NULL;

bool perform_assemble(const char* input_filename,
        FILE** output_binary, FILE** output_symbols)
{
	bstring pp_result_name;
	list_t symbols;
	uint16_t final;

	// Run the preprocessor.
	ppfind_add_autopath(bautofree(bfromcstr(input_filename)));
	//for (i = 0; i < include_dirs->count; ++i)
	//	ppfind_add_path(bautofree(bfromcstr(include_dirs->filename[i])));
    // FIXME: Read from input rather than file.
	pp_result_name = pp_do(bautofree(bfromcstr(input_filename)));

	if (pp_result_name == NULL)
	{
		printd(LEVEL_ERROR, "assembler: invalid result returned from preprocessor.\n");
		pp_cleanup(bautofree(pp_result_name));
        return false;
	}

	yyout = stderr;
	yyin = fopen((const char*)(pp_result_name->data), "r");

	if (yyin == NULL)
	{
		pp_cleanup(bautofree(pp_result_name));
        return false;
	}

	// Parse assembly.
	yyparse();

	if (yyin != stdin)
		fclose(yyin);

	pp_cleanup(bautofree(pp_result_name));

	if (&ast_root == NULL || ast_root.values == NULL)
	{
		printd(LEVEL_ERROR, "assembler: syntax error, see above.\n");
        return false;
	}

	// Initialize storage for debugging symbols.
	list_init(&symbols);

	// Process AST.
	process_root(&ast_root, &symbols);

	// Open up output files.
	*output_binary = tmpfile();
	if (*output_binary == NULL)
	{
		printd(LEVEL_ERROR, "assembler: binary file not writable.\n");
        return false;
	}
	*output_symbols = tmpfile();
	if (*output_symbols == NULL)
	{
		printd(LEVEL_ERROR, "assembler: binary file not writable.\n");
        fclose(*output_binary);
        *output_binary = NULL;
        return false;
	}

	// Write content.
    // FIXME: Second argument to aout_write is whether to generate intermediate code.
    //        When the linker is part of the build process, change this parameter.
	final = aout_write(*output_binary, false, false);
	printd(LEVEL_VERBOSE, "assembler: completed successfully.\n");

    // Set the address of any debugging symbols that don't have an address
    // yet to the address at the end of the file.
    list_iterator_start(&symbols);
    while (list_iterator_hasnext(&symbols))
        dbgfmt_finalize_symbol(list_iterator_next(&symbols), final);
    list_iterator_stop(&symbols);

    // Write symbols.
    // FIXME: Need to define dbgfmt_write_file which takes a pre-existing FILE* rather
    //        than opening a new file.
    //dbgfmt_write_file(*output_symbols, &symbols);
    //printd(LEVEL_VERBOSE, "assembler: wrote debugging symbols.\n");

    return true;
}
