/*
 * Makefile Generator
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2016: Kilpatrick Audio
 *
 * This file is part of Makegen.
 *
 * Makegen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Makegen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Makegen. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

// defaults
#define BUILD_DIR_DEF "build"
#define C_COMPILER_EXEC_DEF "gcc"
#define CPP_COMPILER_EXEC_DEF "g++"
#define C_EXT_MAX 1
const char *c_ext[] = {
    "c"
};
#define CPP_EXT_MAX 2
const char *cpp_ext[] = {
    "cpp", "c++"
};
#define LINKER_EXEC_DEF "ld"
#define ASSEMBLER_EXEC_DEF "as"

// settings
#define MAX_STRLEN 65536
#define SEPARATOR "/"
char *ldflags;
char *build_dir;
char *c_compiler_exec;
char *cpp_compiler_exec;
char *linker_exec;
char *assembler_exec;
char *include_paths;
char *c_compiler_flags;
char *cpp_compiler_flags;
char *run_command;
char *exec_name;
char *exclude_file;
char *post_link;
int num_excludes = 0;
#define MAX_EXCLUDES 8192
char *excludes[MAX_EXCLUDES];

// function prototypes
void print_bin_deps(char *dirname);
void print_source_targets(char *dirname);
void print_usage(void);
const char *get_filename_ext(const char *filename);
void load_excludes(void);
char *get_derived_o_filename(char *dirname, char *filename);
int is_build_dir(char *dirname);
char *strip_leading_ds(char *filename);

// main
int main(int argc, char **argv) {
    int opt;
    int i;

    // defaults
    ldflags = malloc(sizeof(char) * 1);
    strcpy(ldflags, "");
    c_compiler_flags = malloc(sizeof(char) * 1);
    strcpy(c_compiler_flags, "");
    cpp_compiler_flags = malloc(sizeof(char) * 1);
    strcpy(cpp_compiler_flags, "");
    include_paths = malloc(sizeof(char) * 1);
    strcpy(include_paths, "");
    build_dir = malloc(sizeof(char) * (strlen(BUILD_DIR_DEF) + 1));
    strcpy(build_dir, BUILD_DIR_DEF);
    c_compiler_exec = malloc(sizeof(char) * (strlen(C_COMPILER_EXEC_DEF) + 1));
    strcpy(c_compiler_exec, C_COMPILER_EXEC_DEF);
    cpp_compiler_exec = malloc(sizeof(char) * (strlen(CPP_COMPILER_EXEC_DEF) + 1));
    strcpy(cpp_compiler_exec, CPP_COMPILER_EXEC_DEF);
    linker_exec = malloc(sizeof(char) * (strlen(LINKER_EXEC_DEF) + 1));
    strcpy(linker_exec, LINKER_EXEC_DEF);
    assembler_exec = malloc(sizeof(char) * (strlen(ASSEMBLER_EXEC_DEF) + 1));
    strcpy(assembler_exec, ASSEMBLER_EXEC_DEF);
    post_link = malloc(sizeof(char) * 1);
    strcpy(post_link, "");
    run_command = NULL;
    // null out excludes
    for(i = 0; i < MAX_EXCLUDES; i ++) {
        excludes[i] = NULL;
    }

    //
    // command line args
    //
    // parse the command line arguments
    while((opt = getopt(argc, argv, "h?l:b:c:C:d:a:i:f:F:r:e:p:")) != -1) {
        switch(opt) {
            case 'l':  // ldflags
                ldflags = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(ldflags, optarg);
                break;
            case 'b':  // build directory
                build_dir = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(build_dir, optarg);
                break;
            case 'c':  // C compiler executable
                c_compiler_exec = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(c_compiler_exec, optarg);
                break;
            case 'C':  // C++ compiler executable
                cpp_compiler_exec = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(cpp_compiler_exec, optarg);
                break;
            case 'd':  // linker executable
                linker_exec = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(linker_exec, optarg);
                break;
            case 'a':  // assembler executable
                assembler_exec = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(assembler_exec, optarg);
                break;
            case 'i':  // compiler include paths
                include_paths = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(include_paths, optarg);
                break;
            case 'f':  // C compiler flags
                c_compiler_flags = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(c_compiler_flags, optarg);
                break;
            case 'F':  // C++ compiler flags
                cpp_compiler_flags = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(cpp_compiler_flags, optarg);
                break;
            case 'r':  // run command
                run_command = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(run_command, optarg);
                break;
            case 'e':  // exclude file
                exclude_file = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(exclude_file, optarg);
                break;
            case 'p':  // post link command
                post_link = malloc(sizeof(char) * (strlen(optarg) + 1));
                strcpy(post_link, optarg);
                break;
            case 'h':  // help - same as default
            case '?':  // help - same as default
            default:
                print_usage();
                exit(EXIT_FAILURE);
        }
    }
    if(argc < 2) {
       print_usage();
       exit(EXIT_FAILURE);
    }
    if(optind >= argc) {
       print_usage();
       exit(EXIT_FAILURE);
    }
    exec_name = malloc(sizeof(char) * (strlen(argv[optind]) + 1));
    strcpy(exec_name, argv[optind]);

    if(run_command == NULL) {
        run_command = malloc(sizeof(char) * (strlen(exec_name) + 2 + 1));
        strcpy(run_command, "./");
        strcat(run_command, exec_name);
    }

    // print settings in use
    fprintf(stderr, "makegen - by: Andrew Kilpatrick\n");
    fprintf(stderr, "  ldflags: %s\n", ldflags);
    fprintf(stderr, "  build directory: %s\n", build_dir);
    fprintf(stderr, "  C compiler executable: %s\n", c_compiler_exec);
    fprintf(stderr, "  C++ compiler executable: %s\n", cpp_compiler_exec);
    fprintf(stderr, "  C compiler flags: %s\n", c_compiler_flags);
    fprintf(stderr, "  C++ compiler flags: %s\n", cpp_compiler_flags);
    fprintf(stderr, "  linker executable: %s\n", linker_exec);
    fprintf(stderr, "  assembler executable: %s\n", assembler_exec);
    fprintf(stderr, "  executable name: %s\n", exec_name);
    fprintf(stderr, "  run command: %s\n", run_command);
    fprintf(stderr, "  exclude file: %s\n", exclude_file);
    fprintf(stderr, "  post link command: %s\n", post_link);

    if(exclude_file != NULL) {
        // load the exclusion list
        if(strlen(exclude_file) > 0) {
            load_excludes();
        }
        fprintf(stderr, "excludes:\n");
        for(i = 0; i < num_excludes; i ++) {
            fprintf(stderr, "  %s\n", excludes[i]);
        }
    }

    fprintf(stderr, "generating makefile...\n");

    //
    // generate the Makefile
    //
    // print top comments
    printf("# THIS FILE IS AUTOGENERATED - DO NOT EDIT\n");
    printf("#\n# Makefile for: %s\n#\n", exec_name);
    printf("# type 'make' or 'make %s' to create the binary\n", exec_name);
    printf("# type 'make clean' to delete temp files\n");
    printf("# type 'make run' to run program\n");
    printf("#\n");

    // print build target variables
    printf("# build target specs\n");
    printf("CC = %s\n", c_compiler_exec);
    printf("CPP = %s\n", cpp_compiler_exec);
    printf("LD = %s\n", linker_exec);
    printf("AS = %s\n", assembler_exec);
    if(strlen(c_compiler_flags) > 0) {
        printf("CFLAGS = %s\n", c_compiler_flags);
    }
    if(strlen(cpp_compiler_flags) > 0) {
        printf("CPPFLAGS = %s\n", cpp_compiler_flags);
    }
    if(strlen(include_paths) > 0) {
        printf("INCLUDES = %s\n", include_paths);
    }
    printf("OUT_DIR = %s\n", build_dir);
    if(strlen(ldflags) > 0) {
        printf("LDFLAGS = %s\n", ldflags);
    }
    printf("\n");

    // print first target
    printf("# first target to run when typing 'make'\n");
    printf("default: %s\n", exec_name);
    printf("\n");

    // print binary deps
    printf("# binary dependencies\n");
    printf("%s: ", exec_name);
    print_bin_deps(".");
    printf("\n");
    printf("\t@echo 'Linking %s...'\n", exec_name);
    printf("\t$(LD) -o %s ", exec_name);
    print_bin_deps(".");
    printf("$(LDFLAGS)\n");
    if(strlen(post_link) > 0) {
        printf("\t%s\n", post_link);
    }
    printf("\t@echo done.\n");
    printf("\n");

    // print source targets
    printf("# source targets\n");
    print_source_targets(".");
    printf("\n");

    // print the run target
    printf("# run target\n");
    printf("run:\n");
    printf("\t%s\n", run_command);
    printf("\n");

    // print the clean target
    printf("# clean target\n");
    printf("clean:\n");
    printf("\t@echo 'removing temporary files...'\n");
    printf("\trm -f %s %s.bin %s.elf %s.map %s.hex $(OUT_DIR)/*.o\n",
        exec_name, exec_name, exec_name, exec_name, exec_name);
    printf("\t@echo done.\n");
    printf("\n");
    fprintf(stderr, "done.\n");

    return 0;
}

// print binary dependencies
void print_bin_deps(char *dirname) {
    DIR *dir;
    FILE *fp;
    struct dirent *de;
    struct stat s;
    char *derived_ofile;
    char *fullpath = NULL;
    int i;
    int exclude_f = 0;

    if((dir = opendir(dirname)) == NULL) {
        return;
    }
    while((de = readdir(dir)) != NULL) {
        // build the full path
        fullpath = malloc(sizeof(char) * (strlen(dirname) + strlen(de->d_name) + 1 + 1));
        strcpy(fullpath, dirname);
        strcat(fullpath, SEPARATOR);
        strcat(fullpath, de->d_name);
        // get a stat on the path
        if(stat(fullpath, &s) != 0) {
            continue;
        }
        // directory
        if(s.st_mode & S_IFDIR) {
            // ignore . and ..
            if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
                continue;
            }
            // ignore the build dir
            if(is_build_dir(de->d_name)) {
                continue;
            }
            // recurse into directory
            print_bin_deps(fullpath);
        }
        // file
        else if(s.st_mode & S_IFREG) {
            exclude_f = 0;
            // skip files that are on the exclusion list
            for(i = 0; i < num_excludes; i ++) {
                if(!strcmp(fullpath, excludes[i])) {
                    exclude_f = 1;
                    break;
                }
            }
            if(exclude_f) {
                fprintf(stderr, "excluding file for link: %s\n", fullpath);
                continue;
            }

            // derive the .o filename to use in the build dir
            derived_ofile = get_derived_o_filename(dirname, de->d_name);
            fprintf(stderr, "dep: %s\n", derived_ofile);

            // handle assembler files specially
            if(!strcmp(get_filename_ext(fullpath), "s")) {
                fprintf(stderr, "processing file for link: %s\n", fullpath);
                printf("$(OUT_DIR)/%s.o ", derived_ofile);
            }
            // see if the file matches one of the C types
            else {
                for(i = 0; i < C_EXT_MAX; i ++) {
                    if(!strcmp(get_filename_ext(fullpath), c_ext[i])) {
                        fprintf(stderr, "processing C file for link: %s\n", fullpath);
                        printf("$(OUT_DIR)/%s.o ", derived_ofile);
                        break;
                    }
                }
                for(i = 0; i < CPP_EXT_MAX; i ++) {
                    if(!strcmp(get_filename_ext(fullpath), cpp_ext[i])) {
                        fprintf(stderr, "processing C++ file for link: %s\n", fullpath);
                        printf("$(OUT_DIR)/%s.o ", derived_ofile);
                        break;
                    }
                }
            }
            free(derived_ofile);
        }
    }
    closedir(dir);
    if(fullpath != NULL) {
        free(fullpath);
    }
}

// print source targets
void print_source_targets(char *dirname) {
    DIR *dir;
    FILE *fp;
    struct dirent *de;
    struct stat s;
    char *fullpath = NULL;
    char *derived_ofile;
    char tempstr[MAX_STRLEN];
    int i;
    int exclude_f;

    if((dir = opendir(dirname)) == NULL) {
        return;
    }
    while((de = readdir(dir)) != NULL) {
        // build the full path
        fullpath = malloc(sizeof(char) * (strlen(dirname) + strlen(de->d_name) + 1 + 1));
        strcpy(fullpath, dirname);
        strcat(fullpath, SEPARATOR);
        strcat(fullpath, de->d_name);
        // get a stat on the path
        if(stat(fullpath, &s) != 0) {
            continue;
        }
        // directory
        if(s.st_mode & S_IFDIR) {
            // ignore . and ..
            if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
                continue;
            }
            // ignore the build dir
            if(is_build_dir(de->d_name)) {
                continue;
            }
            // recurse into directory
            print_source_targets(fullpath);
        }
        // file
        else if(s.st_mode & S_IFREG) {
            exclude_f = 0;
            // skip files that are on the exclusion list
            for(i = 0; i < num_excludes; i ++) {
                if(!strcmp(fullpath, excludes[i])) {
                    exclude_f = 1;
                    break;
                }
            }
            if(exclude_f) {
                fprintf(stderr, "excluding file for compile: %s\n", fullpath);
                continue;
            }

            // derive the .o filename to use in the build dir
            derived_ofile = get_derived_o_filename(dirname, de->d_name);

            // handle assembler files specially
            if(!strcmp(get_filename_ext(fullpath), "s")) {
                fprintf(stderr, "processing file for assembly: %s\n", fullpath);
                printf("# ASM source file: %s\n", fullpath);
                fflush(stdout);
                printf("$(OUT_DIR)%s%s.o: %s\n", SEPARATOR, derived_ofile, fullpath);
                printf("\t@echo 'assembling %s...'\n", de->d_name);
                printf("\t$(AS) -o $(OUT_DIR)/%s.o %s\n", derived_ofile, fullpath);
                printf("\t@echo done.\n");
                printf("\n");
            }
            // see if the file matches one of the C types
            else {
                // C
                for(i = 0; i < C_EXT_MAX; i ++) {
                    if(!strcmp(get_filename_ext(fullpath), c_ext[i])) {
                        fprintf(stderr, "processing C file for compile: %s:\n", fullpath);
                        printf("# C source file: %s\n", fullpath);
                        fflush(stdout);
                        sprintf(tempstr, "%s %s %s -MM -MT '$(OUT_DIR)'/%s.o %s",
                            c_compiler_exec, include_paths, c_compiler_flags, derived_ofile, fullpath);
                        system(tempstr);
                        printf("\t@echo 'compiling %s...'\n", de->d_name);
                        printf("\t$(CC) $(CFLAGS) $(INCLUDES) -o $(OUT_DIR)/%s.o -c %s\n",
                            derived_ofile, fullpath);
                        printf("\t@echo done.\n");
                        printf("\n");
                        break;
                    }
                }
                // C++
                for(i = 0; i < CPP_EXT_MAX; i ++) {
                    if(!strcmp(get_filename_ext(fullpath), cpp_ext[i])) {
                        fprintf(stderr, "processing C++ file for compile: %s:\n", fullpath);
                        printf("# C++ source file: %s\n", fullpath);
                        fflush(stdout);
                        sprintf(tempstr, "%s %s %s -MM -MT '$(OUT_DIR)'/%s.o %s",
                            cpp_compiler_exec, include_paths, cpp_compiler_flags, derived_ofile, fullpath);
                        system(tempstr);
                        printf("\t@echo 'compiling %s...'\n", de->d_name);
                        printf("\t$(CPP) $(CPPFLAGS) $(INCLUDES) -o $(OUT_DIR)/%s.o -c %s\n",
                            derived_ofile, fullpath);
                        printf("\t@echo done.\n");
                        printf("\n");
                        break;
                    }
                }
            }
            free(derived_ofile);
        }
    }

    if(fullpath != NULL) {
        free(fullpath);
    }
}

// load the excludes
void load_excludes(void) {
    FILE *fp;
    fprintf(stderr, "loading excludes:\n");
    char tempstr[MAX_STRLEN];
    char tempstr2[MAX_STRLEN];

    fp = fopen(exclude_file, "r");
    if(fp == NULL) {
        fprintf(stderr, "error loading excludes file: %s\n", exclude_file);
        exit(EXIT_FAILURE);
    }
    while(fgets(tempstr, 1024, fp) != NULL) {
        // ignore too short lines
        if(strlen(tempstr) < 2) {
            continue;
        }
        // ignore comment lines
        if(tempstr[0] == '#') {
            continue;
        }
        // trim off newlines
        if(tempstr[strlen(tempstr)-1] == '\n') {
            tempstr[strlen(tempstr)-1] = 0x00;
        }
        // make sure we have a ./ at the start so it will match our path
        if(tempstr[0] != '.') {
            strcpy(tempstr2, "./");
            strcat(tempstr2, tempstr);
        }
        excludes[num_excludes] = malloc(sizeof(char) * (strlen(tempstr2) + 1));
        strcpy(excludes[num_excludes], tempstr2);
        num_excludes ++;
    }

    fclose(fp);
}

// print usage
void print_usage(void) {
    printf("makegen - by: Andrew Kilpatrick\n");
    printf("usage: makegen [options] exec\n\n");
    printf("options:\n");
    printf("  -l    ldflags\n");
    printf("  -b    build directory\n");
    printf("  -c    C compiler executable\n");
    printf("  -C    C++ compiler executable\n");
    printf("  -d    linker executable\n");
    printf("  -a    assembler executable\n");
    printf("  -i    compiler include paths\n");
    printf("  -f    C compiler flags\n");
    printf("  -F    C++ compiler flags\n");
    printf("  -r    run (or flash) command\n");
    printf("  -e    exclude file containing files to exclude from compilation\n");
    printf("  -p    post link command\n");
}

// get the file extension
const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

// get the derived .o filename from the full path
// caller must free the returned string
char *get_derived_o_filename(char *dirname, char *filename) {
    char *outstr, *p, *pathname;
    int i, len;

    // get the pathname with no leading . or /
    pathname = strip_leading_ds(dirname);
    len = strlen(pathname) + 1 + strlen(filename);
    outstr = (char *)malloc(sizeof(char) * (len + 1));
    if(strlen(pathname) > 0) {
        strncpy(outstr, pathname, strlen(pathname) + 1);
        strncat(outstr, SEPARATOR, strlen(SEPARATOR) + 1);
        strncat(outstr, filename, len);
    }
    else {
        strncpy(outstr, filename, len);
    }

    len = strlen(outstr);
    p = outstr;
    for(i = 0; i < len; i ++) {
        if(*p == SEPARATOR[0]) {
            *p = '_';
        }
        p++;
    }
    free(pathname);
    return outstr;
}

// check if the directory is the build dir - ignores leading . and /
int is_build_dir(char *dirname) {
    char *p = dirname;
    int len = strlen(dirname);
    if(len > 2 && *p == '.' && *(p+1) == SEPARATOR[0]) {
        p += 2;
    }
    if(!strcmp(p, build_dir)) {
        return 1;
    }
    return 0;
}

// strip the leading '.' and possibly also '/'
// caller must free returned string
char *strip_leading_ds(char *filename) {
    char *p, *outstr;
    int len;

    p = filename;
    len = strlen(filename);
    if(len > 0 && *p == '.') {
        p ++;
        len --;
        if(len > 0 && *p == SEPARATOR[0]) {
            p ++;
            len --;
        }
    }

    outstr = (char *)malloc(sizeof(char) * (len + 1));
    if(len == 0) {
        outstr[0] = 0x00;
    }
    else {
        strncpy(outstr, p, len + 1);
    }
    return outstr;
}
