/**
 ** Mechanics of Programming
 ** Project 3
 **
 ** lmedit.c
 **
 ** @author Aaron J Larouco ajl7033
 **
 ** A load module editor program
 **
 */

// Includes
#include "exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Defines
#define READ fread( e, 1, 2, f )
#define chEAD fread( e, 1, chunks, f )
#define HEADSZ 52
#define CHANGES (tch || sch || dch || rch || strch)

// Prototypes
exec_t* processFile( char* filename );
void summary( uint32_t* data );
void getInput( char* word );
char* fext( char* filename );
void jumptosec(int sec, FILE* f, uint32_t* data);
char** getstrings( char* filename, long jump, long rel, long ref, long sym, long bytes);

/*
 Main function
 */
int main( int argc, char** argv ){

	if ( argc != 2 ){
		// Incorrect number of arguments supplied
		fprintf( stderr, "usage: lmedit file\n" );
		return EXIT_FAILURE;
	}

	uint8_t* e = (uint8_t *)malloc( sizeof( uint8_t ) * 4 );

	// exec object to hold module info
	exec_t* module = processFile( argv[1] );
	if ( module == NULL ){
		return EXIT_FAILURE;
	}

	long jump = (module->data)[0] + (module->data)[1] + (module->data)[2] + (module->data)[3] + (module->data)[4] + (module->data)[5];


	char** strings = getstrings(argv[1], jump, (long)(module->data)[6], (long)(module->data)[7], (long)(module->data)[8] , (long)(module->data)[9]);


	// memory
	FILE* f = fopen(argv[1], "r+");
	if ( f == NULL ){
		perror( argv[1] );
		return EXIT_FAILURE;
	}


	fseek( f, HEADSZ, SEEK_CUR );

	uint8_t* memtext = (uint8_t *)malloc((module->data)[0]);
	for (uint32_t a = 0; a < (module->data)[0]; a++){
		fread(e,1,1,f);
		memtext[a] = e[0];
	}
	bool tch = false;
	uint8_t* memrdata = (uint8_t *)malloc((module->data)[1]);
	for (uint32_t a = 0; a < (module->data)[1]; a++){
		fread(e,1,1,f);
		memrdata[a] = e[0];
	}
	bool rch = false;
	uint8_t* memdata = (uint8_t *)malloc((module->data)[2]);
	for (uint32_t a = 0; a < (module->data)[2]; a++){
		fread(e,1,1,f);
		memdata[a] = e[0];
	}
	bool dch = false;
	uint8_t* memsdata = (uint8_t *)malloc((module->data)[3]);
	for (uint32_t a = 0; a < (module->data)[3]; a++){
		fread(e,1,1,f);
		memsdata[a] = e[0];
	}
	bool sch = false;
	long hop = (module->data)[4]+(module->data)[5]+(module->data)[6]+(module->data)[7]+(module->data)[8];
	fseek(f,hop,SEEK_CUR);
	uint8_t* memstrings = (uint8_t *)malloc((module->data)[9]);
	for (uint32_t a = 0; a < (module->data)[9]; a++){
		fread(e,1,1,f);
		memstrings[a] = e[0];
	}
	bool strch = false;
	fclose(f);


	// type of file
	char* ext = fext( argv[1] );
	char ty;
	if (strcmp(ext,"obj")==0){
		ty = 'j';
	}
	else if (strcmp(ext,"out")==0){
		ty = 't';
	}
	else{
		ty = 'e'; //ERROR
	}

	// allocate memory for each block and copy
	// use this memory to edit
	// overwrite with this if write command is received

	int sec = 0;
	long ti = 0x00400000;
	long ri = 0x10000000;

	char* word = malloc(BUFSIZ);
	bool go = false;

	f = fopen( argv[1], "rb+" );
	if ( f == NULL ){
		perror( argv[1] );
		return EXIT_FAILURE;
	}

	if (f == NULL)

	jumptosec(sec,f,module->data);

	// main command loop
	for(int n = -1;;n++){
		switch (sec) { // which to print
		case 0: printf( "%s[%d] >", "text", n ); break;
		case 1: printf( "%s[%d] >", "rdata", n ); break;
		case 2: printf( "%s[%d] >", "data", n ); break;
		case 3: printf( "%s[%d] >", "sdata", n ); break;
		case 4: printf( "sbss ERROR" ); break; // sbss error
		case 5: printf( " bss ERROR" ); break; // bss error
		case 6: printf( "%s[%d] >", "reltab", n ); break;
		case 7: printf( "%s[%d] >", "reftab", n ); break;
		case 8: printf( "%s[%d] >", "symtab", n ); break;
		case 9: printf( "%s[%d] >", "strings", n ); break;
		}
		getInput(word);
		char* p;
		if ((p=strchr(word, '\n')) != NULL){ *p = '\0'; }
		if ( strcmp(word,"quit") == 0 ){
			// quit
			if (!( CHANGES )){ // if there are no changes, quit
				return EXIT_SUCCESS;
			}
			for(;;){ // otherwise, query user
				printf("Discard modifications (yes or no)? ");
				getInput(word);

				if ( strcmp(word,"yes\n") == 0 ){
					// free all dyn alc mem
					free( module );
					return EXIT_SUCCESS;

				} else if ( strcmp(word, "no\n") == 0 ){
					// go back to command loop
					break;
					go = true;
				}
			}

			// if quit was selected we must continue
			if (go) { go = false; continue; }

		}
		else if ( strcmp(word,"size") == 0 ){
			// size of current section
			printf( "Size: %d\n", (module->data)[sec] ); //Section text is 96 bytes long##

		}
		else if ( strcmp(word,"write") == 0 ){
			if (!CHANGES){
				continue;
			}
			// save changes
			if (tch){
				fwrite(memtext,1,((module->data)[0]),f);
				tch = false;
			}else{
				fseek(f,(module->data)[0],SEEK_CUR);
			}
			if (rch){
				fwrite(memrdata,1,(module->data)[1],f);
				rch = false;
			}else{
				fseek(f,(module->data)[1],SEEK_CUR);
			}if (dch){
				fwrite(memdata,1,(module->data)[2],f);
				dch = false;
			}else{
				fseek(f,(module->data)[2],SEEK_CUR);
			}if (sch){
				fwrite(memsdata,1,(module->data)[3],f);
				sch = false;
			}else{
				fseek(f,(module->data)[3],SEEK_CUR);
			}if (strch){
				fwrite(memstrings,1,(module->data)[4],f);
				strch = false;
			}else{
				fseek(f,(module->data)[4],SEEK_CUR);
			}
			jumptosec(sec,f,module->data);
			continue;

		}
		// if the 8th char is space then
		// the user may have entered 'section'
		else if (word[7] == ' '){
			word[7] = 0; // null zero out (section\0name)
			if ( strcmp(word,"section") == 0 ){
				char* name = &word[8];

				if ( strcmp(name,"text") == 0 ){ sec = 0; }
				else if ( strcmp(name,"rdata") == 0 ){ sec = 1; }
				else if ( strcmp(name,"data") == 0 ){ sec = 2; }
				else if ( strcmp(name,"sdata") == 0 ){ sec = 3; }
				else if ( ( strcmp(name,"sbss") == 0 )
				       || ( strcmp(name,"bss" ) == 0 ) ){
					printf( "error:  cannot edit %s section\n", name );
					continue;
					// sbss,bss error
				}
				else if ( strcmp(name,"reltab") == 0 ){ sec = 6; }
				else if ( strcmp(name,"reftab") == 0 ){ sec = 7; }
				else if ( strcmp(name,"symtab") == 0 ){ sec = 8; }
				else if ( strcmp(name,"strings") == 0 ){ sec = 9; }
				else { printf( "error:  '%s' is not a valid section name\n", name ); continue; }

				// move to specified section
				jumptosec(sec, f, module->data);
				printf( "Now editing section %s\n", name );

			}
		}
		else {
		// A[,N][:T][=V]
		char* end;
		bool hasN = false;
		bool hasT = false;
		bool hasV = false;
		char* posN = 0;
		char* posT = 0;
		char* posV = 0;
		long A,B;


		// case check
		if ((posV=strchr(word, '=')) != NULL){
			if ((sec > 5) && (sec<9)){
				fprintf(stderr, "error:  '%s' is not valid in table sections\n", posV );
				continue;
			}
			*posV = '\0';
			posV++;
			hasV = true;
		}

		if ((posT=strchr(word, ':')) != NULL){
			if ((sec > 5) && (sec<9)){
				fprintf(stderr, "error:  '%s' is not valid in table sections\n", posT );
				continue;
			}
			*posT = '\0';
			posT++;
			hasT = true;
		}

		if ((posN=strchr(word, ',')) != NULL){
			*posN = '\0';
			posN++;
			hasN = true;
		}

		A = strtol( word, &end, 0 );

		if ( word != end ){
			int N = 1;
			char T = 'w';
			long V = 0;

			if (hasN) {
				N = strtol( posN, &end, 10 );
				if ((posN == end) || (N == 0)){
					fprintf(stderr, "error:  '%s' is not a valid count\n", posN); continue;
				}
			}
			if (hasT) {
				T = *posT;
				if (!((T == 'w') || (T == 'h') || (T == 'b'))){
					fprintf(stderr,"error:  '%c' is not a valid type\n", T); continue;
				}
			}
			if (hasV) {
				V = strtol( posV, &end, 0 );
				if (posV == end){
					fprintf(stderr,"error:  '%s' is not a valid value\n", posV); continue;
				}
			}

			// before running, move to address indicated
			if (hasV){
			switch (T){
			case 'w':
				switch (ty){
				case 't':
					B = A;
					// out indexed weird
					if (sec == 0){
						A -= ti;
						B = A;
					}
					else if (sec == 1){
						A -= ri;
						B = A;
					}
					else if (sec==2){
						if (!(((module->data)[1])%8)) A -= (ri+((module->data)[1]));
						else A -= (ri+((module->data)[1]) + 8 - ((module->data)[1])%8);
					}
					else if (sec==3){
						// next m-o-8 after last sec

						if (!(((module->data)[2])%8)) A -= (ri+((module->data)[1])+((module->data)[2]));
						else A -= (ri+((module->data)[1]) + ((module->data)[2]) + 8 - ((module->data)[1])%8 - ((module->data)[2])%8);
					}
					else{

					}
				case 'j':
					// 0 indexing
					if ( (A<0)||((A+4*N)>((module->data)[sec])) ){
						if (((A + 4)<=((module->data)[sec])) && (A>=0)){
							fprintf(stderr,"error:  '%x' is not a valid count\n", N);
						}
						else{
							fprintf(stderr,"error:  '%s' is not a valid address\n", word); //TODO
						}
						continue;
					}
					fseek( f, A, SEEK_CUR);
					break;
				default:
					printf("Hemorrhaging errors\n");
					break;
				}
				break;
			case 'h':
				switch (ty){
				case 't':
					// out indexed weird
					B = A;
					if (sec == 0){
						A -= ti;
						B = A;
					}
					else if (sec == 1){
						A -= ri;
						B = A;
					}
					else if (sec==2){

						if (!(((module->data)[1])%8)) A -= (ri+((module->data)[1]));
						else A -= (ri+((module->data)[1]) + 8 - ((module->data)[1])%8);
					}
					else if (sec==3){
						// next m-o-8 after last sec
						if (!(((module->data)[2])%8)) A -= (ri+((module->data)[1])+((module->data)[2]));
						else A -= (ri+((module->data)[1]) + ((module->data)[2]) + 8 - ((module->data)[1])%8 - ((module->data)[2])%8);
					}
					else{

					}
				case 'j':
					// 0 indexing
					if ( (A<0)||((2*N+A)>((module->data)[sec])) ){
						if (((A + 2)<=((module->data)[sec])) && (A>=0)){
							fprintf(stderr,"error:  '%x' is not a valid count\n", N);
						}
						else{
							fprintf(stderr,"error:  '%s' is not a valid address\n", word); //TODO
						}
						continue;
					}
					fseek( f, A, SEEK_CUR);
					break;
				default:
					printf("Hemorrhaging errors\n");
					break;
				}
				break;
			case 'b':
				switch (ty){
				case 't':
					B = A;
					// out indexed weird
					if (sec == 0){
						A -= ti;
						B = A;
					}
					else if (sec == 1){
						A -= ri;
						B = A;
					}
					else if (sec==2){
						if (!(((module->data)[1])%8)) A -= (ri+((module->data)[1]));
						else A -= (ri+((module->data)[1]) + 8 - ((module->data)[1])%8);
					}
					else if (sec==3){
						// next m-o-8 after last sec
						if (!(((module->data)[2])%8)) A -= (ri+((module->data)[1])+((module->data)[2]));
						else A -= (ri+((module->data)[1]) + ((module->data)[2]) + 8 - ((module->data)[1])%8 - ((module->data)[2])%8);
					}
					else{

					}
				case 'j':
					// 0 indexing
					if ( (A<0)||(((N+A))>((module->data)[sec])) ){
						if (((A + 2)<=((module->data)[sec])) && (A>=0)){
							fprintf(stderr,"error:  '%x' is not a valid count\n", N);
						}
						else{
							fprintf(stderr,"error:  '%s' is not a valid address\n", word); //TODO
						}
						continue;
					}
					fseek( f, A, SEEK_CUR);
					break;
				default:
					printf("Hemorrhaging errors\n");
					break;
				}
				break;
			}
			}
			else {switch (T){
			case 'w':
				switch (ty){
				case 't':
					B = A;
					// out indexed weird
					if (sec == 0){
						A -= ti;
						B = A;
					}
					else if (sec == 1){
						A -= ri;
						B = A;
					}
					else if (sec==2){
						if (!(((module->data)[1])%8)) A -= (ri+((module->data)[1]));
						else A -= (ri+((module->data)[1]) + 8 - ((module->data)[1])%8);
					}
					else if (sec==3){
						// next m-o-8 after last sec
						if (!(((module->data)[2])%8)) A -= (ri+((module->data)[1])+((module->data)[2]));
						else A -= (ri+((module->data)[1]) + ((module->data)[2]) + 8 - ((module->data)[1])%8 - ((module->data)[2])%8);
					}
					else{

					}
				case 'j':
					// 0 indexing
					if ( (A<0)||((A+4*N)>((module->data)[sec])) ){
						if (((A + 4)<=((module->data)[sec])) && (A>=0)){
							fprintf(stderr,"error:  '%x' is not a valid count\n", N);
						}
						else{
							fprintf(stderr,"error:  '%s' is not a valid address\n", word); //TODO
						}
						continue;
					}
					break;
				default:
					printf("Hemorrhaging errors\n");
					break;
				}
				break;
			case 'h':
				switch (ty){
				case 't':
					B = A;
					// out indexed weird
					if (sec == 0){
						A -= ti;
						B = A;
					}
					else if (sec == 1){
						A -= ri;
						B = A;
					}
					else if (sec==2){
						if (!(((module->data)[1])%8)) A -= (ri+((module->data)[1]));
						else A -= (ri+((module->data)[1]) + 8 - ((module->data)[1])%8);
					}
					else if (sec==3){
						// next m-o-8 after last sec
						if (!(((module->data)[2])%8)) A -= (ri+((module->data)[1])+((module->data)[2]));
						else A -= (ri+((module->data)[1]) + ((module->data)[2]) + 8 - ((module->data)[1])%8 - ((module->data)[2])%8);
					}
					else{

					}
				case 'j':
					// 0 indexing
					if ( (A<0)||((2*N+A)>((module->data)[sec])) ){
						if (((A + 2)<=((module->data)[sec])) && (A>=0)){
							fprintf(stderr,"error:  '%x' is not a valid count\n", N);
						}
						else{
							fprintf(stderr,"error:  '%s' is not a valid address\n", word); //TODO
						}
						continue;
					}
					break;
				default:
					printf("Hemorrhaging errors\n");
					break;
				}
				break;
			case 'b':
				switch (ty){
				case 't':
					B = A;
					// out indexed weird
					if (sec == 0){
						A -= ti;
						B = A;
					}
					else if (sec == 1){
						A -= ri;
						B = A;
					}
					else if (sec==2){
						if (!(((module->data)[1])%8)) A -= (ri+((module->data)[1]));
						else A -= (ri+((module->data)[1]) + 8 - ((module->data)[1])%8);
					}
					else if (sec==3){
						// next m-o-8 after last sec
						if (!(((module->data)[2])%8)) A -= (ri+((module->data)[1])+((module->data)[2]));
						else A -= (ri+((module->data)[1]) + ((module->data)[2]) + 8 - ((module->data)[1])%8 - ((module->data)[2])%8);
					}
					else{

					}
				case 'j':
					// 0 indexing
					if ( (A<0)||(((N+A))>((module->data)[sec])) ){
						if (((A + 2)<=((module->data)[sec])) && (A>=0)){
							fprintf(stderr,"error:  '%x' is not a valid count\n", N);
						}
						else{
							fprintf(stderr,"error:  '%s' is not a valid address\n", word); //TODO
						}
						continue;
					}
					break;
				default:
					printf("Hemorrhaging errors\n");
					break;
				}
				break;
			}
				fseek(f, 8*A, SEEK_CUR);
			}
			for ( int j = 0; j < N; j++ ){
				// execute command N times
				int chunks;

				if (!(hasV)){ // examination
					if ((sec < 4) || (sec == 9)){
					uint32_t w;
					uint16_t h;
					uint8_t  b;
					int d;
					switch (T){
					case 'w':
						chunks = 4;
						chEAD;
						d = 0;
						if (ty == 't'){
						switch (sec){
						case 0:
							d = ti;
							break;
						case 1:
							d = ri;
							break;
						case 2:
							//
						case 3:
							// m-o-8
							d = 0;

						}
						}
						w = ( e[0]<<24 ) | ( e[1]<<16 ) | (e[2]<<8 ) | e[3];
						if (CHANGES){
							switch (sec) {
							case 0:
								printf("   0x%08lx = 0x%02x%02x%02x%02x\n", (B+4*j+d), memtext[B+4*j],memtext[B+4*j+1],memtext[B+4*j+2],memtext[B+4*j+3]);
								break;
							case 1:
								printf("   0x%08lx = 0x%02x%02x%02x%02x\n", (B+4*j+d), memrdata[B+4*j],memrdata[B+4*j+1],memrdata[B+4*j+2],memrdata[B+4*j+3]);
								break;
							case 2:
								printf("   0x%08lx = 0x%02x%02x%02x%02x\n", (B+4*j+d), memdata[B+4*j],memdata[B+4*j+1],memdata[B+4*j+2],memdata[B+4*j+3]);
								break;
							case 3:
								printf("   0x%08lx = 0x%02x%02x%02x%02x\n", (B+4*j+d), memsdata[B+4*j],memsdata[B+4*j+1],memsdata[B+4*j+2],memsdata[B+4*j+3]);
								break;
							case 9:
								printf("   0x%08lx = 0x%02x%02x%02x%02x\n", (B+4*j+d), memstrings[B+4*j],memstrings[B+4*j+1],memstrings[B+4*j+2],memstrings[B+4*j+3]);
								break;
							}
						}
						else {
							printf("   0x%08lx = 0x%08x\n", (B+4*j+d), w);
						}
						break;

					case 'h':
						chunks = 2;
						chEAD;
						d = 0;
						if (ty == 't'){
						switch (sec){
						case 0:
							d = ti;
							break;
						case 1:
							d = ri;
							break;
						case 2:
							//
						case 3:
							// m-o-8
							d = 0;

						}
						}
						h = ( e[0]<<8 ) | e[1];
						printf("   0x%08lx = 0x%04x\n", (A+2*j+d), h);
						break;

					case 'b':
						chunks = 1;
						chEAD;
						d = 0;
						if (ty == 't'){
						switch (sec){
						case 0:
							d = ti;
							break;
						case 1:
							d = ri;
							break;
						case 2:
							//
						case 3:
							// m-o-8
							d = 0;

						}
						}
						b = e[0];
						printf("   0x%08lx = 0x%02x\n", (A+j+d), b);
						break;

					default:
						printf("chunk error\n");
						chunks = 0;
					}
					}
					else{
						// tables
						char* symword;
						uint32_t addr;
						uint8_t type;
						uint32_t sym;
						uint32_t flags;
						uint32_t value;
						uint8_t section;

						switch (sec) {
						case 6:
							addr = 0;
							READ;
							addr = ( e[0]<<24 ) | ( e[1]<<16 );
							READ;
							addr = addr | ( e[0]<<8 ) | e[1] ;
							READ;
							section = e[0];
							type = e[1];
							char name[5];
							if ( section == 1 ){ strcpy(name,"text"); }
							else if ( section == 2 ){ strcpy(name,"rdata"); }
							else if ( section == 3 ){ strcpy(name,"data"); }
							else if ( section == 4 ){ strcpy(name,"sdata"); }
							else if ( section == 5 ){ strcpy(name,"sbss"); }
							else if ( section == 6 ){ strcpy(name,"bss"); }
							else { printf( "error %d, 0x%08x, 0x%04x\n", section, addr, type ); break; }
							printf( "   0x%08x (%s) type 0x%04x\n", addr, name, type );
							READ;

							break;
						case 7:
							addr = 0;
							sym = 0;
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
							symword = strings[sym];
							printf( "   0x%08x type 0x%04x symbol %s\n", addr, type, symword);
							READ;
							break;
						case 8:
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

							symword = strings[sym];

							printf("   value 0x%08x flags 0x%08x symbol %s\n",value,flags, symword);
							break;
						default:
							printf("ERR\n");
						}

					}

				}
				else { // assignment
					int d;
					switch (T){
					case 'w':
						chunks = 4;
						d = 0;
						if (ty == 't'){
						switch (sec){
						case 0:
							d = ti;
							break;
						case 1:
							d = ri;
							break;
						case 2:
							//
						case 3:
							// m-o-8
							d = 0;

						}
						}
						printf("   0x%08lx is now 0x%08lx\n", (4*(A+j)+d), V);
						break;
					case 'h':
						chunks = 2;
						d = 0;
						if (ty == 't'){
						switch (sec){
						case 0:
							d = ti;
							break;
						case 1:
							d = ri;
							break;
						case 2:
							//
						case 3:
							// m-o-8
							d = 0;

						}
						}
						printf("   0x%08lx is now 0x%04lx\n", (2*(A+j)+d), V);
						break;
					case 'b':
						chunks = 1;
						d = 0;
						if (ty == 't'){
						switch (sec){
						case 0:
							d = ti;
							break;
						case 1:
							d = ri;
							break;
						case 2:
							//
						case 3:
							// m-o-8
							d = 0;

						}
						}
						printf("   0x%08lx is now 0x%02lx\n", (A+j+d), V);
						break;
					default:
						printf("chunk error\n");
						chunks = 0;
					}

					uint8_t ch1 = V>>24;
					uint8_t ch2 = (V>>16)&31;
					uint8_t ch3 = (V>>8)&31;
					uint8_t ch4 = V&31;
					switch (sec){
					case 0:
						tch = true;
						memtext[(chunks*(A+j))] = ch1;
						memtext[(chunks*(A+j)+1)] = ch2;
						memtext[(chunks*(A+j)+2)] = ch3;
						memtext[(chunks*(A+j)+3)] = ch4;
						break;
					case 1:
						rch = true;
						memrdata[(chunks*(A+j))] = ch1;
						memrdata[(chunks*(A+j)+1)] = ch2;
						memrdata[(chunks*(A+j)+2)] = ch3;
						memrdata[(chunks*(A+j)+3)] = ch4;
						break;
					case 2:
						dch = true;
						memdata[(chunks*(A+j))] = ch1;
						memdata[(chunks*(A+j)+1)] = ch2;
						memdata[(chunks*(A+j)+2)] = ch3;
						memdata[(chunks*(A+j)+3)] = ch4;
						break;
					case 3:
						sch = true;
						memsdata[(chunks*(A+j))] = ch1;
						memsdata[(chunks*(A+j)+1)] = ch2;
						memsdata[(chunks*(A+j)+2)] = ch3;
						memsdata[(chunks*(A+j)+3)] = ch4;
						break;
					case 9:
						strch = true;
						memstrings[(chunks*(A+j))] = ch1;
						memstrings[(chunks*(A+j)+1)] = ch2;
						memstrings[(chunks*(A+j)+2)] = ch3;
						memstrings[(chunks*(A+j)+3)] = ch4;
						break;
					default:
						printf("problem\n");
					}

				}

			}
			jumptosec(sec,f,module->data);

		}
		else {
			printf("invalid command\n");
		}
		}
	}

	return 0;
}

/**
 * processFile helper function creates an object holding the file's header info
 * @param filename name of file to open
 * @return object holding module info
 */
exec_t* processFile( char* filename ){

	uint8_t* e = (uint8_t *)malloc( sizeof( uint8_t ) * 4 );
	uint16_t magic;
	uint32_t entry;
	uint32_t flags;

	// open file
	FILE* f = fopen( filename, "r+" );

	// error if failed to open
	if ( f == NULL ){
		perror( filename );
		return NULL;
	}

	// create exec
	exec_t* module = (exec_t *)malloc( sizeof( exec_t ) );

	// read magic num
	READ;

	// build magic word
	magic = ( e[0]<<8 ) | e[1];

	// check magic number
	if ( magic != HDR_MAGIC ){
		fprintf( stderr,
		 "error:  %s is not an R2K object module (magic number %#04x\n",
		 filename, magic );
		return NULL;
	}

	// assign magic word
	( module->magic ) = magic;

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
		printf( "File %s is an R2K load module (entry point 0x%08x)\n",
		 filename, entry );
	}
	else {
		printf( "error:  invalid file format\n" );
		return NULL;
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

	// get strings?

	// close file & dealc mem
	free( e );
	fclose( f );
	return module;
}

/**
 * summary helper function that prints the sizes of non-empty sections
 * @param data table of sizes
 */
void summary( uint32_t* data ){
	if ( sz_text ){
		printf( "Section text is %d bytes long\n", sz_text );
	}
	if ( sz_rdata ){
		printf( "Section rdata is %d bytes long\n", sz_rdata );
	}
	if ( sz_data ){
		printf( "Section data is %d bytes long\n", sz_data );
	}
	if ( sz_sdata ){
		printf( "Section sdata is %d bytes long\n", sz_sdata );
	}
	if ( sz_sbss ){
		printf( "Section sbss is %d bytes long\n", sz_sbss );
	}
	if ( sz_bss ){
		printf( "Section bss is %d bytes long\n", sz_bss );
	}
	if ( n_reloc ){
		printf( "Section reltab is %d entries long\n", n_reloc );
	}
	if ( n_refs ){
		printf( "Section reftab is %d entries long\n", n_refs );
	}
	if ( n_syms ){
		printf( "Section symtab is %d entries long\n", n_syms );
	}
	if ( sz_strings ){
		printf( "Section strings is %d bytes long\n", sz_strings );
	}
}

/**
 * get input function to get the user input
 * @param word pointer to which the input is to be copied
 */
void getInput( char* word ){

  	char  buf[BUFSIZ];

  	if (fgets(buf, sizeof(buf), stdin))
  	{
		strncpy(word, buf, BUFSIZ);
  	}

 	return;

}

/**
 * fext helper function
 * @param filename name of file to extract extension from
 * @return extension of file (w/out '.'; remove +1 from return to keep it)
 */
char* fext( char* filename ) {
    char *dot = strrchr( filename, '.' );
    if ( !dot || dot == filename ){ return ""; }
    return dot + 1;
}

/**
 *
 */
void jumptosec(int sec, FILE* f, uint32_t* data){
	long skip = HEADSZ;
	rewind( f );
	for ( int ix = 0; ix < sec; ix++ ){
		if (ix == 6){
			skip += (data[ix]*8);
		} else if (ix == 7){
			skip += (data[ix]*12);
		}else if (ix == 8){
			skip += (data[ix]*12);
		}else{
			skip += data[ix];
		}
	}
	fseek( f, skip, SEEK_CUR );
}

char** getstrings( char* filename, long jump, long rel, long ref, long sym, long bytes ){
	FILE* f = fopen(filename,"r+");
	if ( f == NULL ){
		perror( filename );
		return NULL;
	}
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
