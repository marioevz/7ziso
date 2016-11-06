#include <LzmaLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_BINARY_TRACKS 1
#define MIN_BINARY_TRACKS 1
#define MAX_AUDIO_TRACKS 0
#define MIN_AUDIO_TRACKS 0

typedef enum {
		FILE_TYPE_UNKNOWN,
		FILE_TYPE_BINARY,
		FILE_TYPE_WAVE,
		FILE_TYPE_MP3
} filetype;
typedef enum {
		TRACK_TYPE_UNKNOWN,
		TRACK_TYPE_MODE1,
		TRACK_TYPE_MODE2,
		TRACK_TYPE_AUDIO
} tracktype;

typedef struct LBAPosSizes {
   uint32_t position;
   uint16_t size;
} LBAPosSize;

typedef struct Tracks {
   tracktype t_type;
   uint16_t lba_size;
   uint8_t index_count;
   uint32_t * startTime;
} Track;

typedef struct T_Files {
   char path[255];
   char filename[255];
   filetype f_type;
   uint8_t trackCount;
   Track ** tracks;
} T_File;

typedef struct CueFiles {
   char path[255];
   char filename[255];
   int extraInfoCount;
   char ** extraInfo;
   T_File ** files;
   uint8_t fileCount;
} CueFile;

static const char STRING_FILE[] = "FILE";
static const char STRING_TRACK[] = "TRACK";
static const char STRING_INDEX[] = "INDEX";
static const char STRING_FILE_TYPE_BINARY[] = "BINARY";
static const char STRING_TRACK_TYPE_MODE1_2352[] = "MODE1/2352";
static const char STRING_TRACK_TYPE_MODE2_2352[] = "MODE2/2352";
static const char STRING_TRACK_TYPE_AUDIO[] = "AUDIO";

CueFile *readCueFile(char * path);
void printCueFile(CueFile *cuefile);
bool compressCueFiles(CueFile **cuefiles, uint8_t cueFileCount);
bool compressBinaryFile(T_File *bfile, FILE* tmpfp, LBAPosSize **lba_pointers, uint32_t *lba_pointer_total);
void addFileToContainer ( CueFile * tfc);
void addTrackToFile (T_File * trfile);
char *trimwhitespace(char *str);
bool startsWith(const char *pre, const char *str);
bool endsWith(const char *pre, const char *str);
void getAllFilesByType(	CueFile** cuefiles,
						uint8_t cuefilecount,
						uint8_t *binfilecount,
						T_File ***binFiles,
						uint8_t *audiofilecount,
						T_File ***audFiles);

int main ( int argc, char *argv[] ) {
	if(argc > 1) {
		CueFile ** allCueFiles = calloc(argc - 1, sizeof(CueFile*));
		uint8_t cueFileCount = 0;
		for (int i = 1; i < argc; i++) {
			CueFile *tmpTracksFC = NULL;
			if((tmpTracksFC = readCueFile(argv[i])) != NULL) {
				allCueFiles[cueFileCount++] = tmpTracksFC;
				printCueFile(tmpTracksFC);
			}
		}
		compressCueFiles(allCueFiles, cueFileCount);
	} else {
		printf("Not enough arguments.\n");
	}
}

bool compressCueFiles(CueFile **cuefiles, uint8_t cueFileCount) {
	char outFile[255];
	char tmpFile[255];
	uint8_t binFileCount, audFileCount;
	FILE *tmpfp = NULL;
	T_File ** binaryFiles;
	T_File ** audioFiles;
	LBAPosSize ** all_files_lba_pos_size = NULL;	

	if(cueFileCount == 0) return false;	

	getAllFilesByType( cuefiles, cueFileCount, &binFileCount, &binaryFiles, &audFileCount, &audioFiles);

	printf("Got %d binary files and %d audio files.\n", binFileCount, audFileCount);
	
	sprintf(outFile, "%s.7ziso", cuefiles[0]->path);
	sprintf(tmpFile, "%s.7ziso.tmp", cuefiles[0]->path);
	printf("Output file %s\n",outFile);
	printf("Temporary file %s\n",tmpFile);

	/* Allocate space for all the indexes */
	if( (all_files_lba_pos_size = malloc(sizeof(LBAPosSize*)*( binFileCount + audFileCount ))) == NULL) {
		printf("Unable to allocate memory for all_files_lba_pos_size\n");
		return false;
	}
	
	printf("all_files_lba_pos_size=%ld\n", (long int) all_files_lba_pos_size);
	
	/* Create Temp File */
	tmpfp = fopen(tmpFile, "w+b");
	
	for(uint8_t bf = 0; bf < binFileCount; bf++) {
		uint32_t curr_lba_pointer_total = 0;
		printf("Current binary file %s\n", binaryFiles[bf]->path);
		if(compressBinaryFile(binaryFiles[bf], tmpfp, all_files_lba_pos_size , &curr_lba_pointer_total)) {
		}
	}
	
	/*	
	if (LzmaCompress((unsigned char *) lba_pointers_compressed,
			  &lba_pointers_compressed_size,
			  (unsigned char *) lba_pointers,
			  sizeof(LBAPosSize)*(*lba_pointer_total),
			  outPropsLbaPnt,
			  &outPropsLbaPntSize,
			  -1,
			  0,  // default = (1 << 24)
			  3,        // 0 <= lc <= 8, default = 3
			  0,        // 0 <= lp <= 4, default = 0
			  2,        // 0 <= pb <= 4, default = 2
			  -1,       // 5 <= fb <= 273, default = 32
			  -1 		// 1 or 2, default = 2
			) == SZ_OK ) {
		
		if ( (outfp = fopen(outFile, "wb")) != NULL ) {
		
		// Write header
		fwrite ( "7ZISO", 1, 5, outfp);
		
		// Write the length of the compressed lba pointers array
		fwrite( &lba_pointers_compressed_size, sizeof(size_t), 1, outfp);
		
		// Write the five bytes of outProps for the compressed lba pointers array
		fwrite ( outPropsLbaPnt, 1, outPropsLbaPntSize, outfp);

		// Write the compressed lba pointers array
		fwrite (lba_pointers_compressed, 1,lba_pointers_compressed_size, outfp);
 		
		// Write the original size of the file
		fwrite ( &file_size, sizeof(file_size), 1, outfp);
		
		// Write the compressed blocks
		// Rewind the temporary file first
		rewind (tmpfp);
		
		while ((readSize = fread(  lbaBuffer,
                bfileTrack.lba_size,
                lba_per_block,
                tmpfp )) > 0)
			fwrite( lbaBuffer, readSize, 1, outfp );
		}
				
	}
	*/
	fclose(tmpfp);
	return true;	
}

bool compressBinaryFile(T_File *bfile, FILE* tmpfp, LBAPosSize **lba_pointers, uint32_t *lba_pointer_total) {
	FILE *fp = NULL;
	//FILE *outfp = NULL;
	int fd;
	struct stat stbuf;
	off_t file_size;
	uint32_t lba_count = 0;
	*lba_pointer_total = 0;
	*lba_pointers = NULL;
	uint8_t *lba_pointers_compressed = NULL;
	//uint32_t curr_lba = 0;
	size_t compressedSize = 0;
	//size_t lba_pointers_compressed_size = 0;
	const uint8_t lba_per_block = 8;
	uint8_t *lbaBuffer = NULL;
	size_t readSize = 0;
	uint8_t *compDest = NULL;
	//uint32_t outputCurPosition = 0;
	uint8_t outProps[LZMA_PROPS_SIZE];
	size_t outPropsSize = LZMA_PROPS_SIZE;
	//uint8_t outPropsLbaPnt[LZMA_PROPS_SIZE];
	//size_t outPropsLbaPntSize = LZMA_PROPS_SIZE;
	uint32_t dSize;
	Track bfileTrack;
	
	printf("Opening %s for reading\n", bfile->path);
	
	fp = fopen(bfile->path, "rb");
	if ( fp == NULL ) { printf("ERROR: unable to open %s\n", bfile->path); return false; }
	
	fd = open(bfile->path, O_RDONLY);
	if (fd == -1) { printf("ERROR: unable to get info from %s\n", bfile->path); return false; }
	
	printf("Opened %s for reading\n", bfile->path);

	if((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode)))
		return false;
	
	file_size = stbuf.st_size;

	printf("File size for %s is %d\n", bfile->path, (int)file_size);
	
	bfileTrack = *(bfile->tracks[0]);
	
	if ( (file_size % bfileTrack.lba_size) != 0 ) {
		printf("Incorrect file size for %s\n", bfile->path);
		return false;
	}

	lba_count = file_size / bfileTrack.lba_size;
	*lba_pointer_total = (lba_count/lba_per_block)+(lba_count%lba_per_block);
	*lba_pointers = malloc(sizeof(LBAPosSize)*(*lba_pointer_total));
	lbaBuffer = malloc(sizeof(uint8_t) * bfileTrack.lba_size * lba_per_block);
	dSize = ((((bfileTrack.lba_size * lba_per_block) / 1024) + 1) / 2)*1024;
	printf("lba_count=%d\nlba_pointer_total=%d\ndSize=%d\n",lba_count, *lba_pointer_total, dSize);
	printf("bfileTrack.lba_size=%d\n", bfileTrack.lba_size);
	printf("sizeof(lbaBuffer)=%d\n",(int)sizeof(lbaBuffer));
	for(int lbaptr = 0; lbaptr < *lba_pointer_total; lbaptr++) {
		compressedSize = 0;
		(*lba_pointers)[lbaptr].position = ftell(tmpfp);
		if ((readSize = fread( 	lbaBuffer,
				1, bfileTrack.lba_size * lba_per_block,
				fp )) > 0) {
			printf("readSize=%d\n",(int)readSize);
			if (LzmaCompress((unsigned char *) compDest,
							  &compressedSize,
							  (unsigned char *) lbaBuffer,
							  readSize,
							  outProps,
							  &outPropsSize,
							  -1,
							  dSize,  /* default = (1 << 24) */
  							  3,        /* 0 <= lc <= 8, default = 3  */
  							  0,        /* 0 <= lp <= 4, default = 0  */
  							  2,        /* 0 <= pb <= 4, default = 2  */
  							  -1,        /* 5 <= fb <= 273, default = 32 */
  							  -1 /* 1 or 2, default = 2 */
  			) == SZ_OK ) {
				
				fwrite( compDest,
						1,
						compressedSize,
						tmpfp );
				free(compDest);
			}
		}
		(*lba_pointers)[lbaptr].size = compressedSize;
		//outputCurPosition += compressedSize;
	}
		free (lba_pointers_compressed);
	free (lba_pointers);
	free (lbaBuffer);
	return true;
}
 
void getAllFilesByType(	CueFile** cuefiles,
						uint8_t cuefilecount,
						uint8_t *binfilecount,
						T_File ***binFiles,
						uint8_t *audiofilecount,
						T_File ***audFiles) {
	*binfilecount = 0;
	*audiofilecount = 0;
	*binFiles = NULL;
	*audFiles = NULL;
	
	for(uint8_t c = 0; c < cuefilecount; c++) {
		CueFile *currCueFile = cuefiles[c];
		for(int f = 0; f < currCueFile->fileCount; f++) {
				if (currCueFile->files[f] != NULL) {
					switch(currCueFile->files[f]->f_type) {
						case FILE_TYPE_WAVE:
						case FILE_TYPE_MP3:
							(*audiofilecount)++;
							break;
						case FILE_TYPE_BINARY:
							(*binfilecount)++;
							break;
						default:
							break;
					}
				}
			}
	}
	if((*binfilecount)>0) {
		*binFiles = malloc(sizeof(T_File*)*(*binfilecount));
	}
	if((*audiofilecount)>0) {
		*audFiles = malloc(sizeof(T_File*)*(*audiofilecount));
	}

	*binfilecount = 0;
	*audiofilecount = 0;
	
	for(uint8_t c = 0; c < cuefilecount; c++) {
		CueFile *currCueFile = cuefiles[c];
		for(int f = 0; f < currCueFile->fileCount; f++) {
				if (currCueFile->files[f] != NULL) {
					switch(currCueFile->files[f]->f_type) {
						case FILE_TYPE_WAVE:
						case FILE_TYPE_MP3:
							(*audFiles)[*audiofilecount] = currCueFile->files[f];
							(*audiofilecount)++;
							break;
						case FILE_TYPE_BINARY:
							(*binFiles)[*binfilecount] = currCueFile->files[f];
							(*binfilecount)++;
							break;
						default:
							break;
					}
				}
			}
	}
}

void printCueFile(CueFile *cuefile) {
	T_File * currentT_File = NULL;
	for(int f = 0; f < cuefile->fileCount; f++) {
		currentT_File = cuefile->files[f];
		if (currentT_File != NULL) {
			printf("FILE \"%s\" %s\n", currentT_File->filename, 
				(currentT_File->f_type==FILE_TYPE_BINARY ?
					STRING_FILE_TYPE_BINARY : ""));
			for (int t = 0; t < currentT_File->trackCount; t++) {
				Track * currentTrack = currentT_File->tracks[t];
				if (currentTrack != NULL) {
					const char * trackTypeLabel = "";
					switch(currentTrack->t_type) {
						case TRACK_TYPE_MODE1: 
							trackTypeLabel = STRING_TRACK_TYPE_MODE1_2352;
							break;
						case TRACK_TYPE_MODE2: 
							trackTypeLabel = STRING_TRACK_TYPE_MODE2_2352;
							break;
						case TRACK_TYPE_AUDIO: 
							trackTypeLabel = STRING_TRACK_TYPE_AUDIO;
							break;
						case TRACK_TYPE_UNKNOWN: 
						default:
							break;
					}
					printf("\tTRACK %02d %s\n",t+1,trackTypeLabel);
					for (int i = 0; i < currentTrack->index_count; i++) {
						printf("\t\tINDEX %02d\n", i);
					}
				}
			}
		}
	}
}

CueFile *readCueFile(char * path) {
	CueFile *returnTC = calloc(1, sizeof(CueFile));
	memcpy(returnTC->path, path, strlen(path));
	returnTC->files = NULL;
	returnTC->fileCount = 0;
	FILE *cuefp;
	T_File * currT_File = NULL;
	if ( (cuefp = fopen(path, "r")) == NULL ) return NULL;
	
		char * line = NULL;
		size_t l_size = 0;
		ssize_t read = 0;
		while ((read = getline(&line, &l_size, cuefp))!=-1) {
			line = trimwhitespace(line);
			if(startsWith(STRING_FILE, line)) {
				if(currT_File!=NULL) {
					addFileToContainer(returnTC);
					returnTC->files[returnTC->fileCount-1] = currT_File;
					currT_File = NULL;
				}
				currT_File = calloc(1, sizeof(T_File));
				currT_File->trackCount = 0;

				if(endsWith(STRING_FILE_TYPE_BINARY,line))
					currT_File->f_type = FILE_TYPE_BINARY;
				else currT_File->f_type = FILE_TYPE_UNKNOWN;

				char * binfnamestart = strstr(line,"\"");
				if(binfnamestart != NULL) {
					uint8_t chrCount = 0;
					binfnamestart++;
					while(((*binfnamestart)!='\0') && ((*binfnamestart)!='\"')) {
						currT_File->filename[chrCount] = *binfnamestart;
						chrCount++;
						binfnamestart++;
					}
					strcpy (currT_File->path, realpath(currT_File->filename, NULL));
				}
			} else if (startsWith(STRING_TRACK, line)) {
				if(currT_File!=NULL) {
					Track * tmpTrack = calloc (1, sizeof(Track) );
					if(endsWith(STRING_TRACK_TYPE_MODE1_2352,line)) {
						tmpTrack->t_type=TRACK_TYPE_MODE1;
						tmpTrack->lba_size=2352;
					} else if(endsWith(STRING_TRACK_TYPE_MODE2_2352,line)) {
						tmpTrack->t_type=TRACK_TYPE_MODE2;
						tmpTrack->lba_size=2352;
					} else if(endsWith(STRING_TRACK_TYPE_AUDIO,line)) {
						tmpTrack->t_type=TRACK_TYPE_AUDIO;
						tmpTrack->lba_size=2352;
					}
					addTrackToFile(currT_File);
					currT_File->tracks[currT_File->trackCount-1] = tmpTrack;
				}
			} else if (startsWith(STRING_INDEX, line)) {
				
			}
		}
		if(currT_File!=NULL) {
			addFileToContainer(returnTC);
			returnTC->files[returnTC->fileCount-1] = currT_File;
			currT_File = NULL;
		}
		fclose(cuefp);
	
	return returnTC;
}

void addFileToContainer ( CueFile * tfc) {
	if (tfc != NULL) {
		tfc->fileCount++;
		if(tfc->files == NULL) {
			tfc->files = calloc(sizeof(T_File*),1);
		} else tfc->files = realloc(tfc->files, sizeof(T_File*)*(tfc->fileCount));
	}
}

void addTrackToFile (T_File * trfile) {
	if(trfile!=NULL) {
		trfile->trackCount++;
		if(trfile->tracks == NULL) {
			trfile->tracks = calloc(1, sizeof(Track*));
		} else trfile->tracks = realloc(trfile->tracks, sizeof(Track*)*(trfile->trackCount));
	}
}

char * trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

bool startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

bool endsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str + strlen(str) - lenpre, lenpre) == 0;
}
