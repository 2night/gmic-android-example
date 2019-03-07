/*
 #
 #  File        : gmic_cli.cpp
 #                ( C++ source file )
 #
 #  Description : G'MIC CLI interface - A command-line tool to allow the use
 #                of G'MIC commands from the shell.
 #
 #  Copyright   : David Tschumperle
 #                ( http://tschumperle.users.greyc.fr/ )
 #
 #  Licenses    : This file is 'dual-licensed', you have to choose one
 #                of the two licenses below to apply.
 #
 #                CeCILL-C
 #                The CeCILL-C license is close to the GNU LGPL.
 #                ( http://www.cecill.info/licences/Licence_CeCILL-C_V1-en.html )
 #
 #            or  CeCILL v2.1
 #                The CeCILL license is compatible with the GNU GPL.
 #                ( http://www.cecill.info/licences/Licence_CeCILL_V2.1-en.html )
 #
 #  This software is governed either by the CeCILL or the CeCILL-C license
 #  under French law and abiding by the rules of distribution of free software.
 #  You can  use, modify and or redistribute the software under the terms of
 #  the CeCILL or CeCILL-C licenses as circulated by CEA, CNRS and INRIA
 #  at the following URL: "http://www.cecill.info".
 #
 #  As a counterpart to the access to the source code and  rights to copy,
 #  modify and redistribute granted by the license, users are provided only
 #  with a limited warranty  and the software's author,  the holder of the
 #  economic rights,  and the successive licensors  have only  limited
 #  liability.
 #
 #  In this respect, the user's attention is drawn to the risks associated
 #  with loading,  using,  modifying and/or developing or reproducing the
 #  software by the user in light of its specific status of free software,
 #  that may mean  that it is complicated to manipulate,  and  that  also
 #  therefore means  that it is reserved for developers  and  experienced
 #  professionals having in-depth computer knowledge. Users are therefore
 #  encouraged to load and test the software's suitability as regards their
 #  requirements in conditions enabling the security of their systems and/or
 #  data to be ensured and,  more generally, to use and operate it in the
 #  same conditions as regards security.
 #
 #  The fact that you are presently reading this means that you have had
 #  knowledge of the CeCILL and CeCILL-C licenses and that you accept its terms.
 #
*/

#include "gmic.h"
using namespace cimg_library;

// Fallback function for segfault signals.
#if cimg_OS==1
void gmic_segfault_sigaction(int signal, siginfo_t *si, void *arg) {
  cimg::unused(signal,si,arg);
  cimg::mutex(29);
  std::fprintf(cimg::output(),
               "\n\n%s[gmic] G'MIC encountered a %sfatal error%s%s. "
               "Please submit a bug report, at: %shttps://framagit.org/dtschump/gmic/issues%s\n\n",
               cimg::t_red,cimg::t_bold,cimg::t_normal,cimg::t_red,
               cimg::t_bold,cimg::t_normal);
  std::fflush(cimg::output());
  cimg::mutex(29,0);
  std::exit(EXIT_FAILURE);
}
#endif

#if cimg_OS==2
int _CRT_glob = 0; // Disable globbing for msys
#endif

// Main entry
//------------
int main(int argc, char **argv) {

  // Set default output messages stream.
  const bool is_debug = cimg_option("-debug",false,0) || cimg_option("debug",false,0);
  cimg::output(is_debug?stdout:stderr);

  // Set fallback for segfault signals.
#if cimg_OS==1
  struct sigaction sa;
  std::memset(&sa,0,sizeof(sa));
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = gmic_segfault_sigaction;
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV,&sa,0);
#endif

  // Create resources directory.
  if (!gmic::init_rc()) {
    std::fprintf(cimg::output(),
                 "\n[gmic] Unable to create resources folder.\n");
    std::fflush(cimg::output());
  }

  // Set special path for curl on Windows
  // (in case the use of libcurl is not enabled).
#if cimg_OS==2
  cimg::curl_path("_gmic\\curl",true);
#endif

  // Declare main G'MIC instance.
  gmic gmic_instance;
  gmic_instance.set_variable("_host","cli",0);
  gmic_instance.add_commands("cli_start : ");

  // Load startup command files.
  CImg<char> commands_user, commands_update, filename_update;
  bool is_invalid_user = false, is_invalid_update = false;
  char sep = 0;
  gmic_instance.verbosity = -1;

  // Update file (in resources directory).
  filename_update.assign(1024);
  cimg_snprintf(filename_update,filename_update.width(),"%supdate%u.gmic",
                gmic::path_rc(),gmic_version);
  try {
    try {
      commands_update.load_cimg(filename_update);
    } catch (...) {
      commands_update.load_raw(filename_update);
    }
    commands_update.append(CImg<char>::vector(0),'y');
    try { gmic_instance.add_commands(commands_update);
    } catch (...) { is_invalid_update = true; throw; }
  } catch (...) { commands_update.assign(); }
  if (commands_update && (cimg_sscanf(commands_update," #@gmi%c",&sep)!=1 || sep!='c'))
    commands_update.assign(); // Discard invalid update file

  // User file (in parent of resources directory).
  const char *const filename_user = gmic::path_user();
  try {
    commands_user.load_raw(filename_user).append(CImg<char>::vector(0),'y');
    try { gmic_instance.add_commands(commands_user,filename_user); }
    catch (...) { is_invalid_user = true; throw; }
  } catch (...) { commands_user.assign(); }

  // When help has been requested.
  const char
    *const is_help1 = cimg_option("--h",(char*)0,0),
    *const is_help2 = cimg_option("-h",(char*)0,0),
    *const is_help3 = cimg_option("h",(char*)0,0),
    *const is_help4 = cimg_option("--help",(char*)0,0),
    *const is_help5 = cimg_option("-help",(char*)0,0),
    *const is_help6 = cimg_option("help",(char*)0,0),
    *const help_command = is_help1?"--h":is_help2?"-h":is_help3?"h":is_help4?"--help":is_help5?"-help":"help",
    *const help_argument = is_help1?is_help1:is_help2?is_help2:is_help3?is_help3:
    is_help4?is_help4:is_help5?is_help5:is_help6;
  const bool
    is_help = is_help1 || is_help2 || is_help3 || is_help4 || is_help5 || is_help6,
    is_global_help = is_help && !std::strcmp(help_command,help_argument);

  if (is_help) {

    // Load specified commands definitions data (eventually).
    CImgList<gmic_pixel_type> images;
    CImgList<char> images_names;
    if (!is_global_help && commands_user) commands_user.move_to(images);
    if (commands_update) images.insert(commands_update);
    if (!is_global_help || !commands_update) images.insert(gmic::stdlib);
    commands_update.assign();

    for (int i = 1; i<argc; ++i) {
      std::FILE *file = 0;
      CImg<char> filename_tmp(256); *filename_tmp = 0;
      if ((!std::strcmp("-m",argv[i]) || !std::strcmp("m",argv[i]) ||
           !std::strcmp("-command",argv[i]) || !std::strcmp("command",argv[i])) && i<argc - 1) {
        const char *const filename = argv[++i];
        if (!cimg::strncasecmp(filename,"http://",7) || !cimg::strncasecmp(filename,"https://",8))
          try {
            file = std::fopen(cimg::load_network(filename,filename_tmp),"r");
          } catch (CImgException&) { file = 0; }
        else file = std::fopen(filename,"r");
      } else if (!cimg::strcasecmp("gmic",cimg::split_filename(argv[i]))) {
        const char *const filename = argv[i];
        if (!cimg::strncasecmp(filename,"http://",7) || !cimg::strncasecmp(filename,"https://",8))
          try {
            file = std::fopen(cimg::load_network(filename,filename_tmp),"r");
          } catch (CImgException&) { file = 0; }
        else file = std::fopen(filename,"r");
      }
      if (file) {
        const unsigned int n = images.size();
        try {
          CImg<unsigned char>::get_load_cimg(file).move_to(images,0);
        } catch (CImgIOException&) {
          CImg<unsigned char>::get_load_raw(file).move_to(images,0);
        }
        if (images.size()!=n) CImg<unsigned char>::vector('\n').move_to(images,1);
        cimg::fclose(file);
        if (*filename_tmp) std::remove(filename_tmp);
      }
    }

    cimg::output(stdout);
    if (is_global_help) { // Global help
      try {
        gmic_instance.verbosity = -1;
        gmic_instance.run("v - l help \"\" onfail endl q",images,images_names);
      } catch (...) { // Fallback in case default version of 'help' has been overloaded
        images.assign();
        images_names.assign();
        images.insert(gmic::stdlib);
        gmic("v - _host=cli l help \"\" onfail endl q",images,images_names);
      }
    } else { // Help for a specified command
      CImg<char> tmp_line(1024);
      try {
        cimg_snprintf(tmp_line,tmp_line.width(),"v - l help \"%s\",1 onfail endl q",help_argument);
        gmic_instance.verbosity = -1;
        gmic_instance.run(tmp_line,images,images_names);
      } catch (...) { // Fallback in case default version of 'help' has been overloaded
        cimg_snprintf(tmp_line,tmp_line.width(),"v - l help \"%s\",1 onfail endl q",help_argument);
        images.assign();
        images_names.assign();
        images.insert(gmic::stdlib);
        gmic(tmp_line,images,images_names);
      }
    }

    std::exit(0);
  }

  // Convert 'argv' into G'MIC command line.
  commands_user.assign(); commands_update.assign();

  CImgList<char> items;
  if (argc==1) { // When no args have been specified
    gmic_instance.verbosity = -1;
    CImg<char>::string("l[] cli_noarg onfail endl").move_to(items);
  } else {
    gmic_instance.verbosity = 0;
    for (int l = 1; l<argc; ++l) { // Split argv as items
      if (std::strchr(argv[l],' ')) {
        CImg<char>::vector('\"').move_to(items);
        CImg<char>(argv[l],(unsigned int)std::strlen(argv[l])).move_to(items);
        CImg<char>::string("\"").move_to(items);
      } else CImg<char>::string(argv[l]).move_to(items);
      if (l<argc - 1) items.back().back()=' ';
    }
  }

  // Insert startup command.
  const bool is_first_item_verbose = items.width()>1 &&
    (!std::strncmp("-v ",items[0],3) || !std::strncmp("v ",items[0],2) ||
     !std::strncmp("-verbose ",items[0],9) || !std::strncmp("verbose ",items[0],8));
  items.insert(CImg<char>::string("cli_start ",false),is_first_item_verbose?2:0);

  if (is_invalid_user) { // Display warning message in case of invalid user command file
    CImg<char> tmpstr(1024);
    cimg_snprintf(tmpstr,tmpstr.width(),"warn \"File '%s' is not a valid G'MIC command file.\" ",
                  filename_user);
    items.insert(CImg<char>::string(tmpstr.data(),false),is_first_item_verbose?2:0);
  }
  if (is_invalid_update) { // Display warning message in case of invalid user command file
    CImg<char> tmpstr(1024);
    cimg_snprintf(tmpstr,tmpstr.width(),"warn \"File '%s' is not a valid G'MIC command file.\" ",
                  filename_update.data());
    items.insert(CImg<char>::string(tmpstr.data(),false),is_first_item_verbose?2:0);
  }

  const CImg<char> commands_line(items>'x');
  items.assign();

  // Launch G'MIC interpreter.
  try {
    CImgList<gmic_pixel_type> images;
    CImgList<char> images_names;
    gmic_instance.run(commands_line.data(),images,images_names);
  } catch (gmic_exception &e) {

    // Something went wrong during the pipeline execution.
    if (gmic_instance.verbosity<0) {
      std::fprintf(cimg::output(),"\n[gmic] %s%s%s%s",
                   cimg::t_red,cimg::t_bold,
                   e.what(),cimg::t_normal);
      std::fflush(cimg::output());
    }
    if (*e.command_help()) {
      std::fprintf(cimg::output(),"\n[gmic] Command '%s' has the following description: \n",
		   e.command_help());
      std::fflush(cimg::output());
      CImgList<gmic_pixel_type> images;
      CImgList<char> images_names;
      images.insert(gmic::stdlib);
      CImg<char> tmp_line(1024);
      cimg_snprintf(tmp_line,tmp_line.width(),
                    "v - "
                    "l[] i raw:\"%s\",char m \"%s\" onfail rm endl "
                    "l[] i raw:\"%s\",char m \"%s\" onfail rm endl "
                    "rv help \"%s\",0 q",
                    filename_update.data(),filename_update.data(),
                    filename_user,filename_user,
                    e.command_help());
      try {
        gmic(tmp_line,images,images_names);
      } catch (...) {
        cimg_snprintf(tmp_line,tmp_line.width(),"v - help \"%s\",1 q",e.command_help());
        images.assign();
        images_names.assign();
        images.insert(gmic::stdlib);
        gmic(tmp_line,images,images_names);
      }
    } else { std::fprintf(cimg::output(),"\n\n"); std::fflush(cimg::output()); }
    return -1;
  }
  return 0;
}
