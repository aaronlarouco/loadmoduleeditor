/** Mechanics of Programming Homework 7 -- Load Module Analyzer
 ** 
 ** alm.c 
 **
 ** @author ajl7033 Aaron J Larouco
 ** 
 ** Program that analyzes the contents of load modules and prints
 ** a report to standard output
 */

// Includes
#include "exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Defines
#define LINE "--------------------\n"
#define READ fread( e, 1, 2, f )

// Prototypes
void processFile( char* filename );
void summary( uint32_t* data );
char* fext( char* filename );
char** getstrings( char* filename, long jump, long rel, long ref, long sym, long bytes);

/** Main program
 ** 
 ** Parses files specified in argv and prints a report of its contents
 */
int main( int argc, char** argv ){
	if ( argc == 1 ){
		// No arguments supplied
		printf( "usage: alm file1 [ file2 ... ]" );
		return EXIT_FAILURE;
	}

	// execute for each argument
	for ( int i = 1; i < argc; i++ ){
		// process the file
		processFile( argv[i] );
	}

	return EXIT_SUCCESS;
}

/** processFile helper function
 **
 ** @param filename The name of the file being processed 
 */
void processFile( char* filename ){

	uint8_t* e = (uint8_t *)malloc( sizeof( uint8_t ) * 4 );
	uint16_t magic;
	uint32_t entry;
	uint32_t flags;

	// open file
	FILE* f = fopen( filename, "r" );
	
	// error if failed to open
	if ( f == NULL ){
		perror( filename );
		return;
	}

	// create exec
	exec_t* module = (exec_t *)malloc( sizeof( exec_t ) );	

	// read magic num
	READ;	
	
	// build magic word
	magic = ( e[0]<<8 ) | e[1];

	// check magic number 
	if ( magic != HDR_MAGIC ){
		printf( "error: %s is not an R2K object module (magic number %#04x\n", filename, magic );
		return;
	}

	// assign magic word 
	( module->magic ) = magic;
			
	// print a line
	printf( LINE );

	// read version and assign
	READ;
	// parse version
	uint16_t version = ( e[0]<<8 ) | e[1];
	uint16_t year = ( version>>9 );
	year += 2000;
	uint8_t month = ( ( version>>5 ) & 15 );
	uint8_t day = ( version & 31 );
	( module->version ) = version;
			
	// read flags and assign
	READ;
	flags = ( e[0]<<24 ) | ( e[1]<<16 );
	READ;
	flags = flags | ( e[0]<<8 ) | e[1];
	( module->flags ) = flags;
	
	// read entry point
	READ;
	entry = ( e[0]<<24 ) | ( e[1]<<16 );
	READ;
	entry = entry | ( e[0]<<8 ) | e[1];
	( module->entry ) = entry;

	// print what module it is
	if ( strcmp( fext( filename ), "obj" ) == 0 ){
		printf( "File %s is an R2K object module\n", filename );
	}
	else if ( strcmp( fext( filename ), "out" ) == 0 ){
		printf( "File %s is an R2K load module (entry point 0x%08x)\n", filename, entry );
	}
	else {
		printf( "error: invalid file format\n" );
		return;
	}

	// print version
	printf( "Module version:  %d/%02d/%02d\n", year, month, day );

	// parse header block
	for ( int i = 0; i < 10; i++ ){
		uint32_t word = 0;
		READ;
		word = ( e[0]<<24 ) | ( e[1]<<16 );
		READ;
		word = word | ( e[0]<<8 ) | e[1] ;
		
		// put each size in array
		module->data[i] = word;		
	}
	
	// print each section size
	uint32_t* data = module->data;	
	summary( data );

	// skip over text, rdata, data, sdata, sbss, bss
	long jump = sz_text + sz_rdata + sz_data + sz_sdata + sz_sbss + sz_bss;
	fseek( f, jump, SEEK_CUR );

	fclose(f);

	char** strings = getstrings(filename, jump, (long)n_reloc, (long)n_refs, (long)n_syms , (long)sz_strings);

	f = fopen(filename,"r");

	fseek( f, jump+52, SEEK_CUR);


	// print rel table
	if ( n_reloc ){
		printf( "Relocation information:\n" );
		for ( int i = 0; i < (int)n_reloc; i++ ){
			uint32_t addr = 0;
			uint8_t section;
			uint8_t type;
			READ;
			addr = ( e[0]<<24 ) | ( e[1]<<16 );
			READ;		
			addr = addr | ( e[0]<<8 ) | e[1] ;
			READ;
			section = e[0];
			type = e[1];
			char name[5];
			if ( section == 1 ){ strcpy(name,"TEXT"); }
			else if ( section == 2 ){ strcpy(name,"RDATA"); }
			else if ( section == 3 ){ strcpy(name,"DATA"); }
			else if ( section == 4 ){ strcpy(name,"SDATA"); }
			else if ( section == 5 ){ strcpy(name,"SBSS"); }
			else if ( section == 6 ){ strcpy(name,"BSS"); }
			else { printf( "error %d, 0x%08x, 0x%04x\n", section, addr, type ); return; }
			printf( "   0x%08x (%s) type 0x%04x\n", addr, name, type );
			READ;	
		}
	}  

	// print ref table
	if ( n_refs ){
		printf( "Reference information:\n" );
		for ( int i = 0; i < (int)n_refs; i++ ){
			uint32_t addr = 0;
			uint32_t sym = 0;
			uint8_t type;
			READ;
			addr = ( e[0]<<24 ) | ( e[1]<<16 );
			READ;       
			addr = addr | ( e[0]<<8 ) | e[1];
			READ;
			sym = ( e[0]<<24 ) | ( e[1]<<16 );
			READ;
			sym = sym | ( e[0]<<8 ) | e[1]; 
			READ;
			type = e[1];
			char* symword = strings[sym];
			printf( "   0x%08x type 0x%04x symbol %s\n", addr, type, symword); 
			READ;
		}
	}

	// print sym table 
	if ( n_syms ){
		printf( "Symbol table:\n" );
		for ( int i = 0; i < (int)n_syms; i++ ){
			uint32_t flags;
      		uint32_t value;
      		uint32_t sym;
			READ;
            flags = ( e[0]<<24 ) | ( e[1]<<16 );
            READ;
            flags = flags | ( e[0]<<8 ) | e[1];
            READ;
			value = ( e[0]<<24 ) | ( e[1]<<16 );
			READ;
			value = value | ( e[0]<<8 ) | e[1];
            READ;
			sym = ( e[0]<<24 ) | ( e[1]<<16 );
            READ;
            sym = sym | ( e[0]<<8 ) | e[1];

			char* symword = strings[sym];

			printf("   value 0x%08x flags 0x%08x symbol %s\n",value,flags, symword);
			free(strings[sym]);
		}
	}

	free(strings);
	free(module);
	free(e);
	printf( LINE );
	// close file
	fclose( f );
	return;
}

/** fext helper function
 ** @param filename name of file to extract extension from
 ** @return extension of file (w/out '.'; remove +1 from return to keep it)
 */
char* fext( char* filename ) {
    char *dot = strrchr( filename, '.' );
    if ( !dot || dot == filename ){ return ""; }
    return dot + 1;
}

/** summary helper function that prints the sizes of non-empty sections
 ** @param data table of sizes
 */
void summary( uint32_t* data){
	if ( sz_text ){
		printf( "Section TEXT is %d bytes long\n", sz_text );
	}
	if ( sz_rdata ){
		printf( "Section RDATA is %d bytes long\n", sz_rdata );
	}
	if ( sz_data ){
		printf( "Section DATA is %d bytes long\n", sz_data );
	}
	if ( sz_sdata ){
		printf( "Section SDATA is %d bytes long\n", sz_sdata );
	}
	if ( sz_sbss ){
		printf( "Section SBSS is %d bytes long\n", sz_sbss );
	}
	if ( sz_bss ){
		printf( "Section BSS is %d bytes long\n", sz_bss );
	}
	if ( n_reloc ){
		printf( "Section RELTAB is %d entries long\n", n_reloc );
	}
	if ( n_refs ){
		printf( "Section REFTAB is %d entries long\n", n_refs );
	}
	if ( n_syms ){
		printf( "Section SYMTAB is %d entries long\n", n_syms );
	}
	if ( sz_strings ){
		printf( "Section STRINGS is %d bytes long\n", sz_strings );
	}
}


char** getstrings( char* filename, long jump, long rel, long ref, long sym, long bytes ){
	FILE* f = fopen(filename,"r");
	fseek(f, 52 + jump + rel*8 + ref*12 + sym*12 , SEEK_CUR);
	char** strings = (char **)malloc(sizeof(char*)*bytes);
	uint8_t* text = (uint8_t *) malloc(sizeof(uint8_t));
	long i = 0;
	long index = 0;
	while (i < bytes){
		char* word = (char*)malloc(sizeof(char)*bytes);
		int j = 0;
		index = i;
		
		text[0] = 1;	
		while (text[0] != 0){
			fread(text,1,1,f);
			word[j] = (char)text[0];
			j++;
			i++;
		}
		strings[index] = word;
	}

	fclose(f);
	free(text);
	return strings;
}

