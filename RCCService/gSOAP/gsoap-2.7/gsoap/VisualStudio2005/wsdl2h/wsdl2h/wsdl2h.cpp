/*
	wsdl2h.cpp

	WSDL parser and converter to gSOAP header file format

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following two licenses:
GPL or Genivia's license for commercial use.
--------------------------------------------------------------------------------
GPL license.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA

Author contact information:
engelen@genivia.com / engelen@acm.org
--------------------------------------------------------------------------------
A commercial use license is available from Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

Build:
	soapcpp2 -ipwsdl wsdl.h
	g++ -o wsdl2h wsdl2h.cpp types.cpp service.cpp wsdl.cpp schema.cpp wsdlC.cpp stdsoap2.cpp
	
TODO:
	Resolve relative versus absolute import paths for reading imported WSDL/schema (use URL local addresses)
	Do not generate abstract complexTypes, but include defs in derived types
	Handle simpleType derivation from base64

*/

#include "includes.h"
#include "types.h"
#include "service.h"

#ifndef WSDL2H_IMPORT_PATH
#define WSDL2H_IMPORT_PATH (nullptr)
#endif

#ifndef WSDL_TYPEMAP_FILE
#define WSDL_TYPEMAP_FILE "typemap.dat"
#endif

static void init();
static void options(int argc, char **argv);

int _flag = 0,
    aflag = 0,
    cflag = 0,
    dflag = 0,
    eflag = 0,
    fflag = 0,
    gflag = 0,
    iflag = 0,
    jflag = 0,
    lflag = 0,
    mflag = 0,
    pflag = 0,
    sflag = 0,
    uflag = 0,
    vflag = 0,
    wflag = 0,
    xflag = 0,
    yflag = 0,
    zflag = 0;

int infiles = 0;
char *infile[MAXINFILES],
     *outfile = nullptr,
     *proxy_host = nullptr;
extern const char
     *mapfile = WSDL_TYPEMAP_FILE,
     *import_path = WSDL2H_IMPORT_PATH,
     *cwd_path = nullptr,
     *cppnamespace = nullptr;

int proxy_port = 8080;

FILE *stream = stdout;

SetOfString exturis;

extern struct Namespace namespaces[];

const char *service_prefix = nullptr;
const char *schema_prefix = "ns";

char elementformat[]       = "    %-35s  %-30s";
char pointerformat[]       = "    %-35s *%-30s";
char attributeformat[]     = "   @%-35s  %-30s";
char vectorformat[]        = "    std::vector<%-23s> %-30s";
char pointervectorformat[] = "    std::vector<%-22s> *%-30s";
char arrayformat[]         = "    %-35s *__ptr%-25s";
char sizeformat[]          = "    %-35s  __size%-24s";
char offsetformat[]        = "//  %-35s  __offset%-22s";
char choiceformat[]        = "    %-35s  __union%-23s";
char schemaformat[]        = "//gsoap %-5s schema %s:\t%s\n";
char serviceformat[]       = "//gsoap %-4s service %s:\t%s %s\n";
char paraformat[]          = "    %-35s%s%s%s";
char anonformat[]          = "    %-35s%s_%s%s";

char copyrightnotice[] = "\n**  The gSOAP WSDL parser for C and C++ 'VERSION'\n**  Copyright (C) 2000-2008 Robert van Engelen, Genivia Inc.\n**  All Rights Reserved. This product is provided \"as is\", without any warranty.\n**  The gSOAP WSDL parser is released under one of the following two licenses:\n**  GPL or the commercial license by Genivia Inc. Use option -l for more info.\n\n";

char licensenotice[]   = "\
--------------------------------------------------------------------------------\n\
gSOAP XML Web services tools\n\
Copyright (C) 2000-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.\n\
\n\
This software is released under one of the following two licenses:\n\
GPL or Genivia's license for commercial use.\n\
\n\
GPL license.\n\
\n\
This program is free software; you can redistribute it and/or modify it under\n\
the terms of the GNU General Public License as published by the Free Software\n\
Foundation; either version 2 of the License, or (at your option) any later\n\
version.\n\
\n\
This program is distributed in the hope that it will be useful, but WITHOUT ANY\n\
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License along with\n\
this program; if not, write to the Free Software Foundation, Inc., 59 Temple\n\
Place, Suite 330, Boston, MA 02111-1307 USA\n\
\n\
Author contact information:\n\
engelen@genivia.com / engelen@acm.org\n\
--------------------------------------------------------------------------------\n\
A commercial use license is available from Genivia, Inc., contact@genivia.com\n\
--------------------------------------------------------------------------------\n";

int main(int argc, char **argv)
{ init();
  fprintf(stderr, copyrightnotice);
  options(argc, argv);
  if (lflag)
  { fprintf(stderr, licensenotice);
    if (!infiles)
      exit(0);
  }
  Definitions def;
  wsdl__definitions definitions;
  definitions.read(infiles, infile);
  if (definitions.error())
  { definitions.print_fault();
    exit(1);
  }
  definitions.traverse();
  def.compile(definitions);
  if (outfile)
  { fclose(stream);
    fprintf(stderr, "\nTo complete the process, compile with:\nsoapcpp2 %s\n\n", outfile);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Initialization
//
////////////////////////////////////////////////////////////////////////////////

static void init()
{ struct Namespace *p = namespaces;
  if (p)
  { for (; p->id; p++)
    { if (p->in && *p->in)
        exturis.insert(p->in);
      if (p->ns && *p->ns)
        exturis.insert(p->ns);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
//	Parse command line options
//
////////////////////////////////////////////////////////////////////////////////

static void options(int argc, char **argv)
{ int i;
  infiles = 0;
  for (i = 1; i < argc; i++)
  { char *a = argv[i];
    if (*a == '-'
#ifdef WIN32
     || *a == '/'
#endif
    )
    { int g = 1;
      while (g && *++a)
      { switch (*a)
        { case '_':
            _flag = 1;
       	    break;
          case 'a':
            aflag = 1;
       	    break;
          case 'c':
            cflag = 1;
            if (cppnamespace)
	      fprintf(stderr, "wsdl2h: Options -c and -q clash\n");
       	    break;
	  case 'd':
	    dflag = 1;
	    break;
	  case 'e':
	    eflag = 1;
	    break;
	  case 'f':
	    fflag = 1;
	    break;
	  case 'g':
	    gflag = 1;
	    break;
	  case 'i':
	    iflag = 1;
	    break;
	  case 'j':
	    jflag = 1;
	    break;
          case 'I':
            a++;
            g = 0;
            if (*a)
              import_path = a;
            else if (i < argc && argv[++i])
              import_path = argv[i];
            else
              fprintf(stderr, "wsdl2h: Option -I requires a path argument\n");
	    break;
	  case 'l':
	    lflag = 1;
	    break;
	  case 'm':
	    mflag = 1;
	    break;
          case 'n':
            a++;
            g = 0;
            if (*a)
              schema_prefix = a;
            else if (i < argc && argv[++i])
              schema_prefix = argv[i];
            else
              fprintf(stderr, "wsdl2h: Option -n requires a prefix name argument\n");
	    break;
          case 'N':
            a++;
            g = 0;
            if (*a)
              service_prefix = a;
            else if (i < argc && argv[++i])
              service_prefix = argv[i];
            else
              fprintf(stderr, "wsdl2h: Option -N requires a prefix name argument\n");
	    break;
          case 'o':
            a++;
            g = 0;
            if (*a)
              outfile = a;
            else if (i < argc && argv[++i])
              outfile = argv[i];
            else
              fprintf(stderr, "wsdl2h: Option -o requires an output file argument\n");
	    break;
	  case 'p':
	    pflag = 1;
	    break;
	  case 'q':
            a++;
            g = 0;
            if (*a)
	      cppnamespace = a;
            else if (i < argc && argv[++i])
              cppnamespace = argv[i];
            else
              fprintf(stderr, "wsdl2h: Option -q requires a C++ namespace name argument\n");
            if (cflag)
	      fprintf(stderr, "wsdl2h: Options -c and -q clash\n");
	    break;
	  case 'r':
            a++;
            g = 0;
            if (*a)
              proxy_host = a;
            else if (i < argc && argv[++i])
              proxy_host = argv[i];
            else
              fprintf(stderr, "wsdl2h: Option -r requires a proxy host:port argument\n");
            if (proxy_host)
	    { char *s = (char*)emalloc(strlen(proxy_host + 1));
	      strcpy(s, proxy_host);
	      proxy_host = s;
	      s = strchr(proxy_host, ':');
	      if (s)
	      { proxy_port = soap_strtol(s + 1, nullptr, 10);
	        *s = '\0';
	      }
	    }
	    break;
	  case 's':
	    sflag = 1;
	    break;
          case 't':
            a++;
            g = 0;
            if (*a)
              mapfile = a;
            else if (i < argc && argv[++i])
              mapfile = argv[i];
            else
              fprintf(stderr, "wsdl2h: Option -t requires a type map file argument\n");
	    break;
	  case 'u':
	    uflag = 1;
	    break;
	  case 'v':
	    vflag = 1;
	    break;
	  case 'w':
	    wflag = 1;
	    break;
	  case 'x':
	    xflag = 1;
	    break;
	  case 'y':
	    yflag = 1;
	    break;
	  case 'z':
	    zflag = 1;
	    break;
          case '?':
          case 'h':
            fprintf(stderr, "Usage: wsdl2h [-a] [-c] [-d] [-e] [-f] [-g] [-h] [-I path] [-j] [-l] [-m] [-n name] [-N name] [-p] [-q name] [-r proxyhost:port] [-s] [-t typemapfile] [-u] [-v] [-w] [-x] [-y] [-z] [-_] [-o outfile.h] infile.wsdl infile.xsd http://www... ...\n\n");
            fprintf(stderr, "\
-a      generate indexed struct names for local elements with anonymous types\n\
-c      generate C source code\n\
-d      use DOM to populate xs:any and xsd:anyType elements\n\
-e      don't qualify enum names\n\
-f      generate flat C++ class hierarchy\n\
-g      generate global top-level element declarations\n\
-h      display help info\n\
-Ipath  use path to find files\n\
-j	don't generate SOAP_ENV__Header and SOAP_ENV__Detail definitions\n\
-l      include license information in output\n\
-m      use xsd.h module to import primitive types\n\
-nname  use name as the base namespace prefix instead of 'ns'\n\
-Nname  use name as the base namespace prefix for service namespaces\n\
-ofile  output to file\n\
-p      create polymorphic types with C++ inheritance with base xsd__anyType\n\
-qname	use name for the C++ namespace of all declarations\n\
-rhost:port\n\
        connect via proxy host and port\n\
-s      don't generate STL code (no std::string and no std::vector)\n\
-tfile  use type map file instead of the default file typemap.dat\n\
-u      don't generate unions\n\
-v      verbose output\n\
-w      always wrap response parameters in a response struct (<=1.1.4 behavior)\n\
-x      don't generate _XML any/anyAttribute extensibility elements\n\
-y      generate typedef synonyms for structs and enums\n\
-z      generate pointer-based arrays for backward compatibility < gSOAP 2.7.6e\n\
-_      don't generate _USCORE (replace with UNICODE _x005f)\n\
infile.wsdl infile.xsd http://www... list of input sources (if none: use stdin)\n\
\n");
            exit(0);
          default:
            fprintf(stderr, "wsdl2h: Unknown option %s\n", a);
            exit(1);
        }
      }
    }
    else
    { infile[infiles++] = argv[i];
      if (infiles >= MAXINFILES)
      { fprintf(stderr, "wsdl2h: too many files\n");
        exit(1);
      }
    }
  }
  if (infiles)
  { if (!outfile)
    { if (strncmp(infile[0], "http://", 7) && strncmp(infile[0], "https://", 8))
      { const char *s = strrchr(infile[0], '.');
        if (s && (!strcmp(s, ".wsdl") || !strcmp(s, ".gwsdl") || !strcmp(s, ".xsd")))
        { outfile = estrdup(infile[0]);
          outfile[s - infile[0] + 1] = 'h';
          outfile[s - infile[0] + 2] = '\0';
        }
        else
        { outfile = (char*)emalloc(strlen(infile[0]) + 3);
          strcpy(outfile, infile[0]);
          strcat(outfile, ".h");
        }
      }
    }
    if (outfile)
    { stream = fopen(outfile, "w");
      if (!stream)
      { fprintf(stderr, "Cannot write to %s\n", outfile);
        exit(1);
      }
      if (cppnamespace)
        fprintf(stream, "namespace %s {\n", cppnamespace);
      fprintf(stderr, "Saving %s\n\n", outfile);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
//	Namespaces
//
////////////////////////////////////////////////////////////////////////////////

struct Namespace namespaces[] =
{
  {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope"},
  {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding"},
  {"xsi", "http://www.w3.org/2001/XMLSchema-instance"},
  {"xsd", "-"}, // http://www.w3.org/2001/XMLSchema"}, // don't use this, it might conflict with xs
  {"xml", "http://www.w3.org/XML/1998/namespace"},
  {"xs", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema" },
  {"http", "http://schemas.xmlsoap.org/wsdl/http/"},
  {"soap", "http://schemas.xmlsoap.org/wsdl/soap/", "http://schemas.xmlsoap.org/wsdl/soap*/"},
  {"mime", "http://schemas.xmlsoap.org/wsdl/mime/"},
  {"dime", "http://schemas.xmlsoap.org/ws/2002/04/dime/wsdl/", "http://schemas.xmlsoap.org/ws/*/dime/wsdl/"},
  {"wsdl", "http://schemas.xmlsoap.org/wsdl/"},
  {"gwsdl", "http://www.gridforum.org/namespaces/2003/03/gridWSDLExtensions"},
  {nullptr, nullptr}
};
