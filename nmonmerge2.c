#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#ifdef LINUX
#include <getopt.h>
#endif

#define STRLEN 1024*64

#define FILE1 argv[optind]
#define FILE2 argv[optind+1]

#define TEMPFILE "temp.nmon"

void hint()
{
	(void)printf("Hint: convert an nmon file so it can be merged with an older one\n");
	(void)printf("Syntax: nmonmerge [-a] [-v] original-file extra-file\n");
	(void)printf("\t[-a] append converted extra-file data to end of the original-file\n");
	(void)printf("\t[-v] verbose extra details are output\n\n");
/*		      12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
	(void)printf("Without -a the merged data is send to stdout, so redirect it (\">\") to\n");
	(void)printf("\tsave the converted data.\n");
	(void)printf("Note: that header lines are striped out of the 2nd file as they are already\n");
	(void)printf("\tin the original file. This assumes no configuration changes like new\n");
	(void)printf("\tdisks, LUNs, adapters, networks etc., which would cause header and\n");
	(void)printf("\tdata column mismatches\n");
	(void)printf("Note: only the timestamps (Tnnnn) and the number of snapshots are modified\n");
	(void)printf("\teverything else is unchanged.\n");
	(void)printf("Note: be careful as you might have \"missing\" snapshots in the time\n");
	(void)printf("\tbetween the data files.\n");
	(void)printf("Note: an extra line is added, starting \"AAA,note\" and the filename\n");
	(void)printf("\tthis line is ignored other tools but will help in diagnosing errors\n");
	(void)printf("Note: do NOT sort the nmon file before merging (sorting not needed now anyway)\n");
	(void)printf("Note: nmonmerge uses a temporary file called temp.nmon\n");
	(void)printf("Example: to merge three files a.nmon, b.nmon and c.nmon\n");
	(void)printf("\tnmonmerge -a a.nmon b.nmon\n");
	(void)printf("\tnmonmerge -a a.nmon c.nmon\n");
	(void)printf("\tNow a.nmon contains all the data\n");
	(void)printf("Example: to merge three files a.nmon, b.nmon and c.nmon\n");
	(void)printf("\tnmonmerge a.nmon b.nmon >x.nmon\n");
	(void)printf("\tnmonmerge x.nmon c.nmon >y.nmon\n");
	(void)printf("\trm x.nmon\n");
	(void)printf("\tNow y.nmon contains all the data\n");
	exit(0);
}

int main(int argc, char ** argv)
{
	FILE *readfp;
	FILE *writefp;
	int i,j,k,hit,number;
	char string[STRLEN+1];

	int lastzzzz = 0;
	int verbose = 0;
	int append = 0;

	writefp = stdout;
	while ( -1 != (i = getopt(argc, argv, "?hva" ))) {
                switch (i) {
                case '?':
                case 'h':
			hint();
                case 'v':
			verbose++;
			break;
                case 'a':
			append++;
			break;
		}
	}
	if(optind +2 != argc) {
		(void)printf("Error: this command expects two filenames (nmon collected data files)\n");
		hint();
	}
        if( (readfp = fopen(FILE1,"r")) == NULL){
                perror("failed to open original file for reading");
                (void)printf("file: \"%s\"\n",FILE1);
		exit(75);
        }
        if( (writefp = fopen(TEMPFILE,"w+")) == NULL){
                perror("failed to open temporary file for write");
                (void)printf("file: \"%s\"\n",TEMPFILE);
		exit(75);
        }

	(void)fprintf(writefp,"AAA,note,merged file %s starts here\n",FILE1);
	for(i=0;fgets(string,STRLEN,readfp)!= NULL;i++) {
		if(!strncmp(string, "ZZZZ,T",6)) {
			sscanf(&string[6],"%d",&lastzzzz);
		}
		fprintf(writefp,"%s",string);
	}
	if(verbose)(void)printf("First file has %d snapshots in %d lines\n", lastzzzz,i);
	fclose(readfp);

	if(lastzzzz == 0) {
		(void)printf("File %s does not include any ZZZZ lines! - this can't be an nmon output file = stopping.\n",FILE1);
		exit(33);
	}

        if( (readfp = fopen(FILE2,"r")) == NULL){
                perror("failed to open extra data file for reading");
                (void)printf("file: \"%s\"\n",FILE2);
		exit(75);
        }

	/* wind forward to first ZZZZ line to skip header lines */
	for(i=0;fgets(string,STRLEN,readfp)!= NULL;i++) {
		if(!strncmp(string, "ZZZZ,T",6)) {
			(void)fprintf(writefp,"AAA,note,merged file %s starts here\n",FILE2);
			(void)sscanf( &string[6],"%d",&number);
			(void)sprintf(&string[6],"%04d", lastzzzz + number);
			string[10] = ',';
			(void)fprintf(writefp,"%s",string);
			break;
		}
	}
	if(verbose)(void)printf("Skipped %d header lines in second file\n", i);

	for(k=0,hit=0;fgets(string,STRLEN,readfp)!= NULL;k++) {
		/* 3 for short MEM,Tnnnn  and 12 for TOP,1234567,Tnnn */
		for(j=3;j<12;j++) {
			if(string[j  ] == ',' &&
			   string[j+1] == 'T' &&
			   isdigit(string[j+2]) ) {
				hit++;
/*				if(verbose)(void)printf("was=%s",string); */
				(void)sscanf( &string[j+2],"%d",&number);
				(void)sprintf(&string[j+2],"%04d", lastzzzz + number);
				string[j+6] = ',';
				(void)fprintf(writefp,"%s",string);
				break;
			}
		}
	}
	if(verbose)(void)printf("Out of %d lines, converted %d lines, last snapshot was %d\n", i+k,hit, lastzzzz+number);
	fclose(readfp);
	fclose(writefp);
        

	if(append) {
		if( (writefp = fopen(FILE1,"w")) == NULL){
			perror("failed to open original file writing");
			(void)printf("file: \"%s\"\n",FILE1);
			exit(75);
		}
		if(verbose)(void)printf("Output placed back in %s\n",FILE1);
	} else {
		writefp = stdout;
	}
	if( (readfp = fopen(TEMPFILE,"r")) == NULL){
		perror("failed to open temporary file for reading");
		(void)printf("file: \"%s\"\n",TEMPFILE);
		exit(75);
	}

	for(i=0;fgets(string,STRLEN,readfp)!= NULL;i++) {
		if(!strncmp(string, "AAA,snapshots,",14)) {
			fprintf(writefp,"AAA,snapshots,%d\n",lastzzzz+number);
		}
		else
			fprintf(writefp,"%s",string);
	}
	unlink(TEMPFILE);
	return 0;
}
