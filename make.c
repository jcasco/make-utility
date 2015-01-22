/* fakemake.c 
*
*  Josue Casco
*  CS360
*  9/24/14
*
*  Compiles a single executable given a description file. Uses gcc to compile
*
*/



#include <stdio.h>
#include <stdlib.h>
#include <fields.h>
#include <dllist.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct file_struct{
	Dllist C;
	Dllist H;
	Dllist L;
	Dllist F;
	Dllist O;
	char *E;
}Files;

//initilize file struct's lists
Files* new_file(){
	Files* files = (Files*) malloc(sizeof(Files));
	files->C = new_dllist();
	files->H = new_dllist();
	files->L = new_dllist();
	files->F = new_dllist();
	files->O = new_dllist();
	files->E  ="";
}

void list_insert(Dllist list, IS is){
	int i;
	
	//insert files in line to list
	for(i = 1; i < is->NF; i++)
		dll_append(list, new_jval_s(strdup(is->fields[i])));
}

//prints string contents given dllist
void print_list(Dllist list){
	Dllist tmp;

	dll_traverse(tmp, list)
		printf("%s ", jval_s(tmp->val));
	printf("\n");
}

//return H files max time
time_t H_max_time(Files* files){
	struct stat sb;
	Dllist tmp;
	time_t maxTime = 0;

	//traverse H files, exit on opening error
	dll_traverse(tmp, files->H){
		if(stat(jval_s(tmp->val), &sb) == -1){
			fprintf(stderr, "%s no such file or directory\n", jval_s(tmp->val));
			exit(1);
		}
		if(maxTime < sb.st_mtime)
			maxTime = sb.st_mtime;	
	}
	return maxTime;
}

//return C files max time
time_t C_max_time(Files* files, time_t HmaxTime){
	struct stat sb;
	struct stat sc;
	Dllist tmp, tmp2;
	time_t CmaxTime = 0;
	time_t OmaxTime = 0;
	time_t Otime = 0;
	int O_stat = 0;
	int i = 0;
	int sys_stat;
	
	//traverse C files, exit on opening error
	dll_traverse(tmp, files->C){
		char *p;
		char *ofile;
		
		if(stat(jval_s(tmp->val), &sb) == -1){
			fprintf(stderr, "%s no such .c file\n", jval_s(tmp->val));
			exit(1);
		}
		
		//c file time
		CmaxTime = sb.st_mtime;	
		
		//construct .o file
		ofile = strdup(tmp->val.s);
		ofile = strtok(ofile, ".");
		ofile = strcat(ofile, ".o");
		
		//append ofiles	
		dll_append(files->O, new_jval_s(strdup(ofile)));

		O_stat = stat(ofile, &sc);
		Otime = sc.st_mtime;
		//check .o file exists OR less recent than c file OR h files
		if(O_stat ==-1 || Otime < CmaxTime || Otime < HmaxTime){
		    //create "gcc -c" string
			char name[MAXLEN];
			strcpy(name, "gcc -c");
				
			//append flags
			dll_traverse(tmp2, files->F){
				strcat(name, " ");
				strcat(name, strdup(tmp2->val.s));
			}
			
			//add .c 
			strcat(name, " ");
			strcat(name, strdup(tmp->val.s));
			printf("%s \n", name);
		
			//make system call
			sys_stat = system(name);  
			
			if(sys_stat != 0){
				fprintf(stderr, "system call failed\n");
				exit(1);
			}
		}
		
		stat(ofile, &sc);
		Otime = sc.st_mtime;
		if(OmaxTime < Otime)
			OmaxTime = Otime;
	}
	return OmaxTime;
}

//make executable when currently does not exist or need updating
int make_exec(char *E, Files *files){
	Dllist temp;
	Dllist temp2;
	Dllist temp3;
	int sys_stat;
	
	//create "gcc -c" string		
	char E_name[MAXLEN];
			
	strcpy(E_name, "gcc");
				
	//append flags
	dll_traverse(temp, files->F){
		strcat(E_name, " ");
		strcat(E_name, strdup(temp->val.s));
	}
			
	strcat(E_name, " -o ");
	strcat(E_name, E);
	
	//add .o 
	dll_traverse(temp2, files->O){
		strcat(E_name, " ");
		strcat(E_name, strdup(temp2->val.s));
	}
	//inlcude libraries
	dll_traverse(temp3, files->L){
		strcat(E_name, " ");
		strcat(E_name, strdup(temp3->val.s));
	}
			
	printf("%s \n", E_name);
		
	//make system call
	sys_stat = system(E_name);  
			
	if(sys_stat != 0){
		fprintf(stderr, "system call failed\n");
		exit(1);
	}
}



int main(int argc, char **argv){
	char* fl_nm;
	IS is;
	Files* files = new_file();
	int E_chk = 0;
	int E_stat = 0;
	time_t Htime;
	time_t Ctime;
	time_t Etime;
	struct stat sd;

	//when desc file is given
	if(argc > 1){
		is = new_inputstruct(argv[1]);
		fl_nm = argv[1];
	}
	//assign fmakefile
	else{
		is = new_inputstruct("fmakefile.fm");
		fl_nm = "fmakefile";
	}
	//exit on file opening error
	if (is == NULL){
		fprintf(stderr, "fakemake: %s No such file or directory\n",fl_nm);
		exit(1);
	}

	//read desc file
	while(get_line(is) != EOF){
		//check for specification and input
		if(is->NF == 0){
		}
		else if(!strcmp(is->fields[0], "C") && (is->NF > 1)){
			list_insert(files->C, is);
		}
		else if(!strcmp(is->fields[0], "H") && (is->NF > 1)){
			list_insert(files->H, is);
		}
		else if(!strcmp(is->fields[0], "E")){
			//no executable specified
			if(is->NF == 1){
				fprintf(stderr, "Must provide executable name on E line\n");
				exit(1);
			}
			//more than 1 executable specified
			if(is->NF > 2){
				fprintf(stderr, "cannot have more than one executable\n");
				exit(1);
			}

			files->E = strdup(is->fields[1]);
			E_chk ++;
		}
		else if(!strcmp(is->fields[0], "L") && (is->NF > 1)){
			list_insert(files->L, is);
		}
		else if(!strcmp(is->fields[0], "F") && (is->NF > 1)){
			list_insert(files->F, is);
		}
		else{
			fprintf(stderr, "line %d not processed\n", is->line);
			exit(1);
		}
	}
	
	if(E_chk != 1){
		fprintf(stderr, "no E record encountered\n");
		exit(1);
	}

	Htime = H_max_time(files);
	Ctime = C_max_time(files, Htime);
	E_stat = stat(files->E, &sd);
	Etime = sd.st_mtime;

	//when .o files have been updated OR executable does not exist
	if(Etime < Ctime || E_stat == -1){
		make_exec(files->E, files);	
	}
	else
		printf("%s: up to date\n", files->E);
	jettison_inputstruct(is);
}
