/*
	service.cpp

	Service structures.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This part of the software is released under one of the following licenses:
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

TODO:	consider adding support for non-SOAP HTTP operations
        add headerfault output definitions

*/

#include "types.h"
#include "service.h"

static void comment(const char *start, const char *middle, const char *end, const char *text);
static void page(const char *page, const char *title, const char *text);
static void section(const char *section, const char *title, const char *text);
static void banner(const char*);
static void ident();
static void text(const char*);

////////////////////////////////////////////////////////////////////////////////
//
//	Definitions methods
//
////////////////////////////////////////////////////////////////////////////////

Definitions::Definitions()
{ }

void Definitions::collect(const wsdl__definitions &definitions)
{ // Collect information: analyze WSDL definitions and imported definitions
  analyze(definitions);
  for (vector<wsdl__import>::const_iterator import = definitions.import.begin(); import != definitions.import.end(); ++import)
    if ((*import).definitionsPtr())
      analyze(*(*import).definitionsPtr());
}

void Definitions::analyze(const wsdl__definitions &definitions)
{ // Analyze WSDL and build Service information
  int binding_count = 0;
  // Determine number of relevant SOAP service bindings
  for (vector<wsdl__binding>::const_iterator i = definitions.binding.begin(); i != definitions.binding.end(); ++i)
  { for (vector<wsdl__binding_operation>::const_iterator j = (*i).operation.begin(); j != (*i).operation.end(); ++j)
    { if ((*j).operationPtr() && (*j).input && (*j).input->soap__body_)
      { binding_count++;
        break;
      }
    }
  }
  // Analyze and collect service data
  for (vector<wsdl__binding>::const_iterator binding = definitions.binding.begin(); binding != definitions.binding.end(); ++binding)
  { // /definitions/binding/documentation
    const char *binding_documentation = (*binding).documentation;
    // /definitions/binding/soap:binding
    soap__binding *soap__binding_ = (*binding).soap__binding_;
    // /definitions/binding/soap:binding/@transport
    const char *soap__binding_transport = NULL;
    if (soap__binding_)
      soap__binding_transport = soap__binding_->transport;
    // /definitions/binding/soap:binding/@style
    soap__styleChoice soap__binding_style = rpc;
    if (soap__binding_ && soap__binding_->style)
      soap__binding_style = *soap__binding_->style;
    // /definitions/binding/http:binding
    http__binding *http__binding_ = (*binding).http__binding_;
    const char *http__binding_verb = NULL;
    if (http__binding_)
       http__binding_verb = http__binding_->verb;
    // /definitions/binding/operation*
    for (vector<wsdl__binding_operation>::const_iterator operation = (*binding).operation.begin(); operation != (*binding).operation.end(); ++operation)
    { // /definitions/portType/operation/ associated with /definitions/binding/operation
      wsdl__operation *wsdl__operation_ = (*operation).operationPtr();
      // /definitions/binding/operation/soap:operation
      soap__operation *soap__operation_ = (*operation).soap__operation_;
      // /definitions/binding/operation/soap:operation/@style
      soap__styleChoice soap__operation_style = soap__binding_style;
      if (soap__operation_ && soap__operation_->style)
        soap__operation_style = *soap__operation_->style;
      // /definitions/binding/operation/http:operation
      http__operation *http__operation_ = (*operation).http__operation_;
      // /definitions/binding/operation/http:operation/@location
      const char *http__operation_location = NULL;
      if (http__operation_)
        http__operation_location = http__operation_->location;
      // /definitions/binding/operation/input
      wsdl__ext_input *ext_input = (*operation).input;
      // /definitions/binding/operation/output
      wsdl__ext_output *ext_output = (*operation).output;
      // /definitions/portType/operation
      if (wsdl__operation_)
      { wsdl__input *input = wsdl__operation_->input;
        wsdl__output *output = wsdl__operation_->output;
        if (http__operation_)
        { // TODO: HTTP operation
        }
        else if (input && ext_input)
        { soap__body *input_body = ext_input->soap__body_;
          if (ext_input->mime__multipartRelated_)
          { for (vector<mime__part>::const_iterator part = ext_input->mime__multipartRelated_->part.begin(); part != ext_input->mime__multipartRelated_->part.end(); ++part)
              if ((*part).soap__body_)
              { input_body = (*part).soap__body_;
                break;
              }
          }
          // MUST have an input, otherwise can't generate a service operation
          if (input_body)
          { char *URI;
            if (soap__operation_style == rpc)
              URI = input_body->namespace_;
            else if (binding_count == 1)
              URI = definitions.targetNamespace;
            else
            { // multiple service bidings are used, each needs a unique new URI
              URI = (char*)soap_malloc(definitions.soap, strlen(definitions.targetNamespace) + strlen((*binding).name) + 2);
              strcpy(URI, definitions.targetNamespace);
              if (*URI && URI[strlen(URI)-1] != '/')
                strcat(URI, "/");
              strcat(URI, (*binding).name);
            }
            if (URI)
            { const char *prefix = types.nsprefix(service_prefix, URI);
              const char *name = types.aname(NULL, NULL, (*binding).name); // name of service is binding name
              Service *s = services[prefix];
              if (!s)
              { s = services[prefix] = new Service();
                s->prefix = prefix;
                s->URI = URI;
                s->name = name;
                s->transport = soap__binding_transport;
                if ((*binding).portTypePtr())
                  s->type = types.aname(NULL, NULL, (*binding).portTypePtr()->name);
                else
                  s->type = NULL;
              }
              for (vector<wsdl__service>::const_iterator service = definitions.service.begin(); service != definitions.service.end(); ++service)
              { for (vector<wsdl__port>::const_iterator port = (*service).port.begin(); port != (*service).port.end(); ++port)
                { if ((*port).bindingPtr() == &(*binding))
                  { if ((*port).soap__address_)
                      s->location.insert((*port).soap__address_->location);
                    // TODO: HTTP address for HTTP operations
                    // if ((*port).http__address_)
                      // http__address_location = http__address_->location;
                    if ((*service).documentation)
                      s->service_documentation[(*service).name] = (*service).documentation;
                    if ((*port).documentation && (*port).name)
                      s->port_documentation[(*port).name] = (*port).documentation;
                    if (binding_documentation)
                      s->binding_documentation[(*binding).name] = binding_documentation;
                  }
                }
              }
              Operation *o = new Operation();
              o->name = types.aname(NULL, NULL, wsdl__operation_->name);
              o->prefix = prefix;
              o->URI = URI;
              o->style = soap__operation_style;
              o->documentation = wsdl__operation_->documentation;
              o->operation_documentation = (*operation).documentation;
              o->parameterOrder = wsdl__operation_->parameterOrder;
              if ((*operation).soap__operation_)
                o->soapAction = (*operation).soap__operation_->soapAction;
              else
              { o->soapAction = "";
                // determine if we use SOAP 1.2 in which case soapAction is absent, this is a bit of a hack due to the lack of WSDL1.1/SOAP1.2 support and better alternatives
                for (Namespace *p = definitions.soap->local_namespaces; p && p->id; p++)
                { if (p->out && !strcmp(p->id, "soap") && !strcmp(p->out, "http://schemas.xmlsoap.org/wsdl/soap12/"))
                  { o->soapAction = NULL;
                    break;
                  }
                }
              }
              o->input = new Message();
              o->input->name = (*operation).name; // RPC uses operation/@name
              if (soap__operation_style == rpc && !input_body->namespace_)
              { o->input->URI = "";
                fprintf(stderr, "Error: no soap:body namespace attribute\n");
              }
              else
                o->input->URI = input_body->namespace_;
              o->input->use = input_body->use;
              o->input->encodingStyle = input_body->encodingStyle;
              o->input->message = input->messagePtr();
              o->input->part = NULL;
              o->input->multipartRelated = ext_input->mime__multipartRelated_;
              o->input->content = NULL;
              if (ext_input->mime__multipartRelated_ && !ext_input->mime__multipartRelated_->part.empty())
                o->input->header = ext_input->mime__multipartRelated_->part.front().soap__header_;
              else
                o->input->header = ext_input->soap__header_;
              if (ext_input->mime__multipartRelated_ && !ext_input->mime__multipartRelated_->part.empty() && ext_input->mime__multipartRelated_->part.front().soap__body_)
                o->input->body_parts = ext_input->mime__multipartRelated_->part.front().soap__body_->parts;
              else
                o->input->body_parts = input_body->parts;
              if (ext_input->dime__message_)
                o->input->layout = ext_input->dime__message_->layout;
              else
                o->input->layout = NULL;
              o->input->documentation = input->documentation;
              o->input->ext_documentation = ext_input->documentation;
              if (soap__operation_style == document)
                o->input_name = types.oname("__", o->URI, o->input->name);
              else
                o->input_name = types.oname(NULL, o->input->URI, o->input->name);
              if (output && ext_output)
              { soap__body *output_body = ext_output->soap__body_;
                if (ext_output->mime__multipartRelated_)
                { for (vector<mime__part>::const_iterator part = ext_output->mime__multipartRelated_->part.begin(); part != ext_output->mime__multipartRelated_->part.end(); ++part)
                    if ((*part).soap__body_)
                    { output_body = (*part).soap__body_;
                      break;
                    }
        	}
        	if (ext_output->mime__content_)
                { o->output = new Message();
                  o->output->name = NULL;
                  o->output->URI = NULL;
                  o->output->encodingStyle = NULL;
                  o->output->body_parts = NULL;
                  o->output->part = NULL;
                  o->output->multipartRelated = NULL;
                  o->output->content = ext_output->mime__content_;
                  o->output->message = output->messagePtr();
                  o->output->layout = NULL;
                  o->output->documentation = output->documentation;
                  o->output->ext_documentation = ext_output->documentation;
        	}
        	else if (output_body)
                { o->output = new Message();
                  o->output->name = (*operation).name; // RPC uses operation/@name with suffix 'Response' as set below
                  o->output->use = output_body->use;
                  // the code below is a hack around the RPC encoded response message element tag mismatch with Axis:
                  if (!output_body->namespace_ || output_body->use == encoded)
                    o->output->URI = o->input->URI; // encoded seems (?) to require the request's namespace
                  else
                    o->output->URI = output_body->namespace_;
                  o->output->encodingStyle = output_body->encodingStyle;
                  o->output->message = output->messagePtr();
                  o->output->part = NULL;
                  o->output->multipartRelated = ext_output->mime__multipartRelated_;
                  o->output->content = NULL;
                  if (ext_output->mime__multipartRelated_ && !ext_output->mime__multipartRelated_->part.empty())
                    o->output->header = ext_output->mime__multipartRelated_->part.front().soap__header_;
                  else
                    o->output->header = ext_output->soap__header_;
                  if (ext_output->mime__multipartRelated_ && !ext_output->mime__multipartRelated_->part.empty() && ext_output->mime__multipartRelated_->part.front().soap__body_)
                    o->output->body_parts = ext_output->mime__multipartRelated_->part.front().soap__body_->parts;
                  else
                    o->output->body_parts = output_body->parts;
                  if (ext_output->dime__message_)
                    o->output->layout = ext_output->dime__message_->layout;
                  else
                    o->output->layout = NULL;
                  o->output->documentation = output->documentation;
                  o->output->ext_documentation = ext_output->documentation;
                  char *s = (char*)soap_malloc(definitions.soap, strlen(o->output->name) + 9);
                  strcpy(s, o->output->name);
                  strcat(s, "Response");
                  if (soap__operation_style == document)
                    o->output_name = types.oname("__", o->URI, s);
                  else
                    o->output_name = types.oname(NULL, o->output->URI, s);
                }
              }
              else
              { o->output_name = NULL;
                o->output = NULL;
              }
              // collect input headers and headerfaults
              if (ext_input)
              { const vector<soap__header> *soap__header_ = NULL;
        	// check if soap header is in mime:multipartRelated
                if (ext_input->mime__multipartRelated_)
                { for (vector<mime__part>::const_iterator part = ext_input->mime__multipartRelated_->part.begin(); part != ext_input->mime__multipartRelated_->part.end(); ++part)
                  if (!(*part).soap__header_.empty())
                  { soap__header_ = &(*part).soap__header_;
        	    break;
        	  }
        	}
        	if (!soap__header_)
        	  soap__header_ = &ext_input->soap__header_;
                for (vector<soap__header>::const_iterator header = soap__header_->begin(); header != soap__header_->end(); ++header)
                { Message *h = new Message();
                  h->message = (*header).messagePtr();
                  h->body_parts = NULL;
                  h->part = (*header).partPtr();
                  h->URI = (*header).namespace_;
                  if (h->part && h->part->element)
                    h->name = types.aname(NULL, NULL, h->part->element);
                  else if (h->URI && h->part && h->part->name && h->part->type)
                    h->name = types.aname(NULL, h->URI, h->part->name);
                  else
                  { fprintf(stderr, "Error in SOAP Header part definition: input part '%s' missing?\n", h->part && h->part->name ? h->part->name : "?");
                    h->name = "";
                  }
                  h->encodingStyle = (*header).encodingStyle;
                  h->use = (*header).use;
        	  h->multipartRelated = NULL;
        	  h->content = NULL;
        	  h->layout = NULL;
                  h->ext_documentation = NULL;	// TODO: add document content
                  h->documentation = NULL;		// TODO: add document content
                  s->header[h->name] = h;
                  for (vector<soap__headerfault>::const_iterator headerfault = (*header).headerfault.begin(); headerfault != (*header).headerfault.end(); ++headerfault)
                  { // TODO: complete headerfault processing. This is rarely used.
                  }
                }
              }
              // collect output headers and headerfaults
              if (ext_output)
              { const vector<soap__header> *soap__header_ = NULL;
        	// check if soap header is in mime:multipartRelated
                if (ext_output->mime__multipartRelated_)
                { for (vector<mime__part>::const_iterator part = ext_output->mime__multipartRelated_->part.begin(); part != ext_output->mime__multipartRelated_->part.end(); ++part)
                  if (!(*part).soap__header_.empty())
                  { soap__header_ = &(*part).soap__header_;
        	    break;
        	  }
        	}
        	if (!soap__header_)
        	  soap__header_ = &ext_output->soap__header_;
                for (vector<soap__header>::const_iterator header = soap__header_->begin(); header != soap__header_->end(); ++header)
                { Message *h = new Message();
                  h->message = (*header).messagePtr();
                  h->body_parts = NULL;
    	          h->part = (*header).partPtr();
    	          h->URI = (*header).namespace_;
    	          if (h->part && h->part->element)
    	            h->name = types.aname(NULL, NULL, h->part->element);
    	          else if (h->URI && h->part && h->part->name && h->part->type)
    	            h->name = types.aname(NULL, h->URI, h->part->name);
    	          else
                  { fprintf(stderr, "Error in SOAP Header part definition: output part '%s' missing?\n", h->part && h->part->name ? h->part->name : "?");
    	            h->name = "";
    	          }
    	          h->encodingStyle = (*header).encodingStyle;
    	          h->use = (*header).use;
        	  h->multipartRelated = NULL;
        	  h->content = NULL;
        	  h->layout = NULL;
    	          h->ext_documentation = NULL;	// TODO: add document content
    	          h->documentation = NULL;	// TODO: add document content
    	          s->header[h->name] = h;
    	          for (vector<soap__headerfault>::const_iterator headerfault = (*header).headerfault.begin(); headerfault != (*header).headerfault.end(); ++headerfault)
    	          { // TODO: complete headerfault processing. This is rarely used.
    	          }
    	        }
    	      }
    	      // collect faults
              for (vector<wsdl__ext_fault>::const_iterator ext_fault = (*operation).fault.begin(); ext_fault != (*operation).fault.end(); ++ext_fault)
    	      { if ((*ext_fault).soap__fault_ && (*ext_fault).messagePtr())
    	        { Message *f = new Message();
    	          f->message = (*ext_fault).messagePtr();
                  f->body_parts = NULL;
    	          f->part = NULL;
    	          f->encodingStyle = (*ext_fault).soap__fault_->encodingStyle;
    	          f->URI = (*ext_fault).soap__fault_->namespace_;
    	          f->use = (*ext_fault).soap__fault_->use;
		  if (f->use == literal && !f->URI)
		    f->URI = s->URI; // must have a unique URI
        	  f->multipartRelated = NULL;
        	  f->content = NULL;
        	  f->layout = NULL;
    	          f->ext_documentation = (*ext_fault).documentation;
                  f->name = types.aname("_", f->URI, f->message->name);
    	          f->documentation = f->message->documentation;
    	          o->fault.push_back(f);
    	          s->fault[f->name] = f;
    	        }
    	        else
    	          fprintf(stderr, "Error: no wsdl:definitions/binding/operation/fault/soap:fault\n");
    	      }
    	      s->operation.push_back(o);
            }
    	    else
    	      fprintf(stderr, "Warning: no SOAP RPC operation namespace, operations will be ignored\n");
          }
          else
            fprintf(stderr, "Error: no wsdl:definitions/binding/operation/input/soap:body\n");
        }
        else
          fprintf(stderr, "Error: no wsdl:definitions/portType/operation/input\n");
      }
      else
        fprintf(stderr, "Error: no wsdl:definitions/portType/operation\n");
    }
  }
}

void Definitions::compile(const wsdl__definitions& definitions)
{ // compile the definitions and generate gSOAP header file
  const char *defs;
  if (definitions.name)
    defs = types.aname(NULL, NULL, definitions.name);
  else
    defs = "Service";
  ident();
  fprintf(stream, "/* NOTE:\n\n - Compile this file with soapcpp2 to complete the code generation process.\n - Use soapcpp2 option -I to specify paths for #import\n   To build with STL, 'stlvector.h' is imported from 'import' dir in package.\n - Use wsdl2h options -c and -s to generate pure C code or C++ code without STL.\n - Use 'WS/typemap.dat' to control namespace bindings and type mappings.\n   It is strongly recommended to customize the names of the namespace prefixes\n   generated by wsdl2h. To do so, modify the prefix bindings in the Namespaces\n   section below and add the modified lines to 'typemap.dat' to rerun wsdl2h.\n - Use Doxygen (www.doxygen.org) to browse this file.\n - Use wsdl2h option -l to view the software license terms.\n\n   DO NOT include this file directly into your project.\n   Include only the soapcpp2-generated headers and source code files.\n*/\n");
  // gsoap compiler options: 'w' disables WSDL/schema output to avoid file collisions
  if (cflag)
    fprintf(stream, "\n//gsoapopt cw\n");
  else
    fprintf(stream, "\n//gsoapopt w\n");
  banner(definitions.targetNamespace?definitions.targetNamespace:"targetNamespace");
  // copy documentation from WSDL definitions
  if (definitions.documentation)
  { fprintf(stream, "/* WSDL Documentation:\n\n");
    text(definitions.documentation);
    fprintf(stream, "*/\n\n");
  }
  if (lflag)
  { banner("License");
    fprintf(stream, "/*\n%s*/\n\n", licensenotice);
  }
  if (definitions.version)
  { banner("Version");
    fprintf(stream, "#define SOAP_WSDL_VERSION \"%s\"\n", definitions.version);
  }
  banner("Import");
  if (dflag)
  { fprintf(stream, "\n// dom.h declares the DOM xsd__anyType object (compiler and link with dom.cpp)\n");
    if (import_path)
      fprintf(stream, "#import \"%s/dom.h\"\n", import_path);
    else
      fprintf(stream, "#import \"dom.h\"\n");
  }
  if (!cflag && !sflag)
  { fprintf(stream, "\n// STL vector containers (use option -s to disable)\n");
    if (import_path)
      fprintf(stream, "#import \"%s/stlvector.h\"\n", import_path);
    else
      fprintf(stream, "#import \"stlvector.h\"\n");
  }
  if (mflag)
  { if (import_path)
      fprintf(stream, "#import \"%s/", import_path);
    else
      fprintf(stream, "#import \"");
    fprintf(stream, "xsd.h\"\t// import primitive XSD types.\n");
  }
  for (SetOfString::const_iterator u = exturis.begin(); u != exturis.end(); ++u)
  { bool found = false;
    size_t n = strlen(*u);
    for (SetOfString::const_iterator i = definitions.builtinTypes().begin(); i != definitions.builtinTypes().end(); ++i)
    { if (**i == '"' && !strncmp(*u, *i + 1, n))
      { found = true;
        break;
      }
    }
    if (!found)
    { for (SetOfString::const_iterator j = definitions.builtinElements().begin(); j != definitions.builtinElements().end(); ++j)
      { if (**j == '"' && !strncmp(*u, *j + 1, n))
        { found = true;
          break;
        }
      }
    }
    if (!found)
    { for (SetOfString::const_iterator k = definitions.builtinAttributes().begin(); k != definitions.builtinAttributes().end(); ++k)
      { if (**k == '"' && !strncmp(*u, *k + 1, n))
        { found = true;
          break;
        }
      }
    }
    if (found)
    { if (vflag)
        fprintf(stderr, "import %s\n", *u);
      if (import_path)
        fprintf(stream, "#import \"%s/%s.h\"\t// %s = <%s>\n", import_path, types.nsprefix(NULL, *u), types.nsprefix(NULL, *u), *u);
      else
        fprintf(stream, "#import \"%s.h\"\t// %s = <%s>\n", types.nsprefix(NULL, *u), types.nsprefix(NULL, *u), *u);
    }
  }
  banner("Schema Namespaces");
  // determine if we must use SOAP 1.2, this is a bit of a hack due to the lack of WSDL1.1/SOAP1.2 support and better alternatives
  for (Namespace *p = definitions.soap->local_namespaces; p && p->id; p++)
  { // p->out is set to the actual namespace name that matches the p->in pattern
    if (p->out && !strcmp(p->id, "soap") && !strcmp(p->out, "http://schemas.xmlsoap.org/wsdl/soap12/"))
    { fprintf(stream, "// This service uses SOAP 1.2 namespaces:\n");
      fprintf(stream, schemaformat, "SOAP-ENV", "namespace", "http://www.w3.org/2003/05/soap-envelope");
      fprintf(stream, schemaformat, "SOAP-ENC", "namespace", "http://www.w3.org/2003/05/soap-encoding");
      break;
    }
  }
  if (definitions.types)
  { fprintf(stream, "\n/* NOTE:\n\nIt is strongly recommended to customize the names of the namespace prefixes\ngenerated by wsdl2h. To do so, modify the prefix bindings below and add the\nmodified lines to typemap.dat to rerun wsdl2h:\n\n");
    if (definitions.targetNamespace && *definitions.targetNamespace)
      fprintf(stream, "%s = \"%s\"\n", types.nsprefix(service_prefix, definitions.targetNamespace), definitions.targetNamespace);
    for (vector<xs__schema*>::const_iterator schema1 = definitions.types->xs__schema_.begin(); schema1 != definitions.types->xs__schema_.end(); ++schema1)
      if (!definitions.targetNamespace || strcmp((*schema1)->targetNamespace, definitions.targetNamespace))
        fprintf(stream, "%s = \"%s\"\n", types.nsprefix(NULL, (*schema1)->targetNamespace), (*schema1)->targetNamespace);
    fprintf(stream, "\n*/\n\n");
    for (vector<xs__schema*>::const_iterator schema2 = definitions.types->xs__schema_.begin(); schema2 != definitions.types->xs__schema_.end(); ++schema2)
      fprintf(stream, schemaformat, types.nsprefix(NULL, (*schema2)->targetNamespace), "namespace", (*schema2)->targetNamespace);
    for (vector<xs__schema*>::const_iterator schema3 = definitions.types->xs__schema_.begin(); schema3 != definitions.types->xs__schema_.end(); ++schema3)
    { if ((*schema3)->elementFormDefault == (*schema3)->attributeFormDefault)
        fprintf(stream, schemaformat, types.nsprefix(NULL, (*schema3)->targetNamespace), "form", (*schema3)->elementFormDefault == qualified ? "qualified" : "unqualified");
      else
      { fprintf(stream, schemaformat, types.nsprefix(NULL, (*schema3)->targetNamespace), "elementForm", (*schema3)->elementFormDefault == qualified ? "qualified" : "unqualified");
        fprintf(stream, schemaformat, types.nsprefix(NULL, (*schema3)->targetNamespace), "attributeForm", (*schema3)->attributeFormDefault == qualified ? "qualified" : "unqualified");
      }
    }
  }
  banner("Schema Types");
  // generate the prototypes first: these should allow use before def, e.g. class names then generate the defs
  // check if xsd:anyType is used
  if (!cflag && !pflag)
  { for (SetOfString::const_iterator i = definitions.builtinTypes().begin(); i != definitions.builtinTypes().end(); ++i)
    { if (!cflag && !strcmp(*i, "xs:anyType"))
      { pflag = 1;
        break;
      }
    }
  }
  if (dflag && pflag)
  { fprintf(stderr, "\nWarning -d option: -p option disabled and xsd__anyType base class removed.\nUse run-time SOAP_DOM_NODE flag to deserialize class instances into DOM nodes.\n");
    fprintf(stream, "\n/*\nWarning -d option used: -p option disabled and xsd:anyType base class removed.\nUse run-time SOAP_DOM_NODE flag to deserialize class instances into DOM nodes.\nA DOM node is represented by the xsd__anyType object implemented in dom.cpp.\n*/\n\n");
    pflag = 0;
  }
  // define xsd:anyType first, if used
  if (!cflag && pflag)
  { const char *s, *t;
    t = types.cname(NULL, NULL, "xs:anyType");
    s = types.deftypemap[t];
    if (s)
    { if (*s)
      { if (!mflag)
          fprintf(stream, "%s\n", s);
      }
      s = types.usetypemap[t];
      if (s)
      { if (mflag)
          fprintf(stream, "//  xsd.h: should define type %s\n", s);
        types.knames.insert(s);
      }
    }
    else
    { fprintf(stderr, "Error: no xsd__anyType defined in type map\n");
      pflag = 0;
    }
  }
  // produce built-in primitive types, limited to the ones that are used only
  if (vflag)
    fprintf(stderr, "\nGenerating built-in types\n");
  for (SetOfString::const_iterator i = definitions.builtinTypes().begin(); i != definitions.builtinTypes().end(); ++i)
  { const char *s, *t;
    if (!cflag && !strcmp(*i, "xs:anyType"))
      continue;
    t = types.cname(NULL, NULL, *i);
    s = types.deftypemap[t];
    if (s)
    { if (*s)
      { if (**i == '"')
          fprintf(stream, "\n/// Imported type %s from typemap %s.\n", *i, mapfile?mapfile:"");
        else
          fprintf(stream, "\n/// Built-in type \"%s\".\n", *i);
        if (mflag)
          fprintf(stream, "//  (declaration of %s removed by option -m)\n", t);
        else
          types.format(s);
      }
      s = types.usetypemap[t];
      if (s && *s)
      { if (mflag && **i != '"')
          fprintf(stream, "\n//  xsd.h: typemap override of type %s with %s\n", t, s);
        if (types.knames.find(s) == types.knames.end())
          types.knames.insert(s);
      }
    }
    else
    { if (!mflag)
      { if (**i == '"')
          fprintf(stream, "\n// Imported type %s defined by %s\n", *i, t);
        else
        { s = types.tname(NULL, NULL, "xsd:string");
          fprintf(stream, "\n/// Primitive built-in type \"%s\"\n", *i);
          fprintf(stream, "typedef %s %s;\n", s, t);
          types.deftname(TYPEDEF, NULL, strchr(s, '*') != NULL, NULL, NULL, *i);
        }
      }
      else if (**i == '"')
        fprintf(stream, "\n//  Imported type %s defined by %s\n", *i, t);
      else
        fprintf(stream, "\n//  xsd.h: should define type %s\n", t);
      types.deftname(TYPEDEF, NULL, false, NULL, NULL, *i);
    }
    if (pflag && !strncmp(*i, "xs:", 3))		// only xsi types are polymorph
    { s = types.aname(NULL, NULL, *i);
      if (!mflag)
      { fprintf(stream, "\n/// Class wrapper for built-in type \"%s\" derived from xsd__anyType\n", *i);
        fprintf(stream, "class %s : public xsd__anyType\n{ public:\n", s);
        fprintf(stream, elementformat, types.tname(NULL, NULL, *i), "__item;");
        fprintf(stream, "\n};\n");
      }
      types.knames.insert(s);
    }
  }
  // produce built-in primitive elements, limited to the ones that are used only
  if (vflag)
    fprintf(stderr, "\nGenerating built-in elements\n");
  for (SetOfString::const_iterator j = definitions.builtinElements().begin(); j != definitions.builtinElements().end(); ++j)
  { const char *s, *t;
    t = types.cname("_", NULL, *j);
    s = types.deftypemap[t];
    if (s)
    { if (*s)
      { if (**j == '"')
          fprintf(stream, "\n/// Imported element %s from typemap %s.\n", *j, mapfile?mapfile:"");
        else
          fprintf(stream, "\n/// Built-in element \"%s\".\n", *j);
        if (mflag)
          fprintf(stream, "//  (declaration of %s removed by option -m)\n", t);
        else
          types.format(s);
      }
      s = types.usetypemap[t];
      if (s && *s)
      { if (mflag && **j != '"')
          fprintf(stream, "\n//  xsd.h: typemap override of element %s with %s\n", t, s);
        if (types.knames.find(s) == types.knames.end())
          types.knames.insert(s);
      }
    }
    else
    { if (!mflag)
      { if (**j == '"')
          fprintf(stream, "\n// Imported element %s declared as %s\n", *j, t);
        else
        { fprintf(stream, "\n/// Built-in element \"%s\".\n", *j);
          fprintf(stream, "typedef _XML %s;\n", t);
          types.deftname(TYPEDEF, NULL, true, "_", NULL, *j);	// already pointer
        }
      }
      else if (**j == '"')
        fprintf(stream, "\n//  Imported element %s declared as %s\n", *j, t);
      else
        fprintf(stream, "\n//  xsd.h: should define element %s\n", t);
      types.deftname(TYPEDEF, NULL, false, "_", NULL, *j);
    }
  }
  // produce built-in primitive attributes, limited to the ones that are used only
  if (vflag)
    fprintf(stderr, "\nGenerating built-in attributes\n");
  for (SetOfString::const_iterator k = definitions.builtinAttributes().begin(); k != definitions.builtinAttributes().end(); ++k)
  { const char *s, *t;
    t = types.cname("_", NULL, *k);
    s = types.deftypemap[t];
    if (s)
    { if (*s)
      { if (**k == '"')
          fprintf(stream, "\n/// Imported attribute %s from typemap %s.\n", *k, mapfile?mapfile:"");
        else
          fprintf(stream, "\n/// Built-in attribute \"%s\".\n", *k);
        if (mflag)
          fprintf(stream, "//  (declaration of %s removed by option -m)\n", t);
        else
          types.format(s);
      }
      s = types.usetypemap[t];
      if (s && *s)
      { if (mflag && **k != '"')
          fprintf(stream, "\n//  xsd.h: typemap override of attribute %s with %s\n", t, s);
        if (types.knames.find(s) == types.knames.end())
          types.knames.insert(s);
      }
    }
    else
    { s = types.tname(NULL, NULL, "xsd:string");
      if (!mflag)
      { if (**k == '"')
          fprintf(stream, "\n// Imported attribute %s declared as %s\n", *k, t);
        else
        { fprintf(stream, "\n/// Built-in attribute \"%s\".\n", *k);
          fprintf(stream, "typedef %s %s;\n", s, t);
        }
      }
      else if (**k == '"')
        fprintf(stream, "//  Imported attribute %s declared as %s\n", *k, t);
      else
        fprintf(stream, "//  xsd.h: should define attribute %s\n", t);
      types.deftname(TYPEDEF, NULL, strchr(s, '*') != NULL, "_", NULL, *k);
    }
  }
  // produce types
  // define class/struct types first
  if (definitions.types)
  { comment("Definitions", defs, "types", definitions.types->documentation);
    fprintf(stream, "\n");
    for (vector<xs__schema*>::const_iterator schema4 = definitions.types->xs__schema_.begin(); schema4 != definitions.types->xs__schema_.end(); ++schema4)
    { if (vflag)
        fprintf(stderr, "\nDefining types in %s\n", (*schema4)->targetNamespace);
      for (vector<xs__complexType>::const_iterator complexType = (*schema4)->complexType.begin(); complexType != (*schema4)->complexType.end(); ++complexType)
        types.define((*schema4)->targetNamespace, NULL, *complexType);
      if (vflag)
        fprintf(stderr, "\nDefining elements in %s\n", (*schema4)->targetNamespace);
      for (vector<xs__element>::const_iterator element = (*schema4)->element.begin(); element != (*schema4)->element.end(); ++element)
      { if (!(*element).type && !(*element).abstract)
        { if ((*element).complexTypePtr())
            types.define((*schema4)->targetNamespace, (*element).name, *(*element).complexTypePtr());
          else if (!(*element).simpleTypePtr())
          { fprintf(stream, "\n/// Element \"%s\":%s.\n", (*schema4)->targetNamespace, (*element).name);
            if (gflag)
            { const char *t = types.deftname(TYPEDEF, NULL, false, "_", (*schema4)->targetNamespace, (*element).name);
  	      if (t)
                fprintf(stream, "typedef _XML %s;\n", t);
              else
                fprintf(stream, "// Element definition intentionally left blank.\n");
            }
            else
            { const char *s = types.cname("_", (*schema4)->targetNamespace, (*element).name);
              types.ptrtypemap[s] = types.usetypemap[s] = "_XML";
              fprintf(stream, "/// Note: use wsdl2h option -g to generate this global element declaration.\n");
            }
          }
        }
      }
    }  
    // visit types with lowest base level first
    int baseLevel = 1;
    bool found;
    do
    { found = (baseLevel == 1);
      for (vector<xs__schema*>::iterator schema = definitions.types->xs__schema_.begin(); schema != definitions.types->xs__schema_.end(); ++schema)
      { for (vector<xs__simpleType>::iterator simpleType = (*schema)->simpleType.begin(); simpleType != (*schema)->simpleType.end(); ++simpleType)
        { if ((*simpleType).baseLevel() == baseLevel)
          { found = true;
            types.gen((*schema)->targetNamespace, NULL, *simpleType, false);
          }
        }
        for (vector<xs__element>::iterator element = (*schema)->element.begin(); element != (*schema)->element.end(); ++element)
        { if (!(*element).type && (*element).simpleTypePtr() && (*element).simpleTypePtr()->baseLevel() == baseLevel)
          { found = true;
            if ((*element).type)
              fprintf(stream, "/// Element \"%s\":%s of simpleType %s.\n", (*schema)->targetNamespace, (*element).name, (*element).type);
            types.document((*element).annotation);
            types.gen((*schema)->targetNamespace, (*element).name, *(*element).simpleTypePtr(), false);
          }
          if (!(*element).type && (*element).complexTypePtr() && (*element).complexTypePtr()->baseLevel() == baseLevel)
            found = true;
        }
        for (vector<xs__attribute>::const_iterator attribute = (*schema)->attribute.begin(); attribute != (*schema)->attribute.end(); ++attribute)
        { if (!(*attribute).type && (*attribute).simpleTypePtr() && (*attribute).simpleTypePtr()->baseLevel() == baseLevel)
          { found = true;
            if ((*attribute).type)
              fprintf(stream, "/// Attribute \"%s\":%s of simpleType %s.\n", (*schema)->targetNamespace, (*attribute).name, (*attribute).type);
            types.document((*attribute).annotation);
            types.gen((*schema)->targetNamespace, (*attribute).name, *(*attribute).simpleTypePtr(), false); // URI = NULL won't generate type in schema (type without namespace qualifier)
          }
        }
        for (vector<xs__complexType>::iterator complexType = (*schema)->complexType.begin(); complexType != (*schema)->complexType.end(); ++complexType)
        { if ((*complexType).baseLevel() == baseLevel)
            found = true;
        }
      }
      ++baseLevel;
    } while (found);
    // generate complex type defs. Problem: what if a simpleType restriction/extension depends on a complexType simpleContent restriction/extension?
    int maxLevel = baseLevel;
    for (baseLevel = 1; baseLevel < maxLevel; ++baseLevel)
    { for (vector<xs__schema*>::iterator schema = definitions.types->xs__schema_.begin(); schema != definitions.types->xs__schema_.end(); ++schema)
      { for (vector<xs__complexType>::iterator complexType = (*schema)->complexType.begin(); complexType != (*schema)->complexType.end(); ++complexType)
        { if ((*complexType).baseLevel() == baseLevel)
            types.gen((*schema)->targetNamespace, NULL, *complexType, false);
        }
        for (vector<xs__element>::iterator element = (*schema)->element.begin(); element != (*schema)->element.end(); ++element)
        { if (!(*element).type && (*element).complexTypePtr() && (*element).complexTypePtr()->baseLevel() == baseLevel)
          { fprintf(stream, "\n\n/// Element \"%s\":%s of complexType.\n", (*schema)->targetNamespace, (*element).name);
            types.document((*element).annotation);
            types.gen((*schema)->targetNamespace, (*element).name, *(*element).complexTypePtr(), false);
          }
        }
      }
    }
    // option to consider: generate local complexTypes iteratively
    /*
    for (MapOfStringToType::const_iterator local = types.locals.begin(); local != types.locals.end(); ++local)
    { types.gen(NULL, (*local).first, *(*local).second);
    }
    */
    for (vector<xs__schema*>::iterator schema = definitions.types->xs__schema_.begin(); schema != definitions.types->xs__schema_.end(); ++schema)
    { if (vflag)
        fprintf(stderr, "\nGenerating elements in %s\n", (*schema)->targetNamespace);
      for (vector<xs__element>::iterator element = (*schema)->element.begin(); element != (*schema)->element.end(); ++element)
      { if ((*element).name && (*element).type && !(*element).abstract)
        { fprintf(stream, "\n/// Element \"%s\":%s of type %s.\n", (*schema)->targetNamespace, (*element).name, (*element).type);
          types.document((*element).annotation);
          if (!types.is_defined("_", (*schema)->targetNamespace, (*element).name))
          { const char *s = types.tname(NULL, (*schema)->targetNamespace, (*element).type);
            const char *t = types.deftname(TYPEDEF, NULL, false, "_", (*schema)->targetNamespace, (*element).name);
  	    if (gflag)
  	    { if (strncmp(s, "char", 4) && strchr(s, '*')) // don't want pointer typedef, unless char*
  	      { size_t n = strlen(s);
  	        char *r = (char*)malloc(n);
  	        strncpy(r, s, n - 1);
  	        r[n - 1] = '\0';
  	        fprintf(stream, "typedef %s %s;\n", r, t);
  	        free(r);
  	      }
  	      else
  	        fprintf(stream, "typedef %s %s;\n", s, t);
  	    }
  	    else
              fprintf(stream, "/// Note: use wsdl2h option -g to generate this global element declaration.\n");
          }
          else
          { const char *s = types.cname("_", (*schema)->targetNamespace, (*element).name);
            const char *t = types.deftypemap[s];
            if (t && *t)
            { fprintf(stream, "/// Imported element %s from typemap %s.\n", s, mapfile?mapfile:"");
              types.format(t);
            }
            else
              fprintf(stream, "// '%s' element definition intentionally left blank.\n", types.cname("_", (*schema)->targetNamespace, (*element).name));
          }
        }
      }
      if (vflag)
        fprintf(stderr, "\nGenerating attributes in %s\n", (*schema)->targetNamespace);
      for (vector<xs__attribute>::iterator attribute = (*schema)->attribute.begin(); attribute != (*schema)->attribute.end(); ++attribute)
      { if ((*attribute).name && (*attribute).type)
        { fprintf(stream, "\n/// Attribute \"%s\":%s of simpleType %s.\n", (*schema)->targetNamespace, (*attribute).name, (*attribute).type);
          types.document((*attribute).annotation);
          if (!types.is_defined("_", (*schema)->targetNamespace, (*attribute).name))
          { const char *s = types.tname(NULL, (*schema)->targetNamespace, (*attribute).type);
            const char *t = types.deftname(TYPEDEF, NULL, false, "_", (*schema)->targetNamespace, (*attribute).name);
  	    if (gflag)
  	    { if (strncmp(s, "char", 4) && strchr(s, '*')) // don't want pointer typedef, unless char*
  	      { size_t n = strlen(s);
  	        char *r = (char*)malloc(n);
  	        strncpy(r, s, n - 1);
  	        r[n - 1] = '\0';
  	        fprintf(stream, "typedef %s %s;\n", r, t);
  	        free(r);
  	      }
  	      else
  	        fprintf(stream, "typedef %s %s;\n", s, t);
  	    }
  	    else
              fprintf(stream, "/// Note: use wsdl2h option -g to generate this global attribute declaration.\n");
          }
          else
          { const char *s = types.cname("_", (*schema)->targetNamespace, (*attribute).name);
            const char *t = types.deftypemap[s];
            if (t && *t)
            { fprintf(stream, "/// Imported attribute %s from typemap %s.\n", s, mapfile?mapfile:"");
              types.format(t);
            }
            else
              fprintf(stream, "// '%s' attribute definition intentionally left blank.\n", types.cname("_", (*schema)->targetNamespace, (*attribute).name));
          }
        }
      }
    }
  }
  if (vflag)
    fprintf(stderr, "\nCollecting service bindings");
  collect(definitions);
  if (!services.empty())
  { banner("Services");
    for (MapOfStringToService::const_iterator service1 = services.begin(); service1 != services.end(); ++service1)
    { Service *sv = (*service1).second;
      if (sv && sv->prefix)
      { fprintf(stream, "\n");
        if (sv->name)
          fprintf(stream, serviceformat, sv->prefix, "name", sv->name, "");
        if (sv->type)
          fprintf(stream, serviceformat, sv->prefix, "type", sv->type, "");
        for (SetOfString::const_iterator port = sv->location.begin(); port != sv->location.end(); ++port)
          fprintf(stream, serviceformat, sv->prefix, "port", (*port), "");
        if (sv->URI)
          fprintf(stream, serviceformat, sv->prefix, "namespace", sv->URI, "");
        if (sv->transport)
          fprintf(stream, serviceformat, sv->prefix, "transport", sv->transport, "");
      }
    }
    fprintf(stream, "\n/** @mainpage %s Definitions\n", definitions.name?definitions.name:"Service");
    if (definitions.version)
    { section(defs, "_version Version", NULL);
      text(definitions.version);
    }
    if (definitions.documentation)
    { section(defs, "_documentation Documentation", NULL);
      text(definitions.documentation);
    }
    if (definitions.types && definitions.types->documentation)
    { section(defs, "_types Types", NULL);
      text(definitions.types->documentation);
    }
    section(defs, "_bindings Bindings", NULL);
    for (MapOfStringToService::const_iterator service2 = services.begin(); service2 != services.end(); ++service2)
    { Service *sv = (*service2).second;
      if (sv && sv->name)
        fprintf(stream, "  - @ref %s\n", sv->name);
    }
    fprintf(stream, "\n*/\n");
    for (MapOfStringToService::const_iterator service3 = services.begin(); service3 != services.end(); ++service3)
    { Service *sv = (*service3).second;
      if (sv && sv->name)
      { fprintf(stream, "\n/**\n");
        page(sv->name, " Binding", sv->name);
        for (MapOfStringToString::const_iterator service_doc = sv->service_documentation.begin(); service_doc != sv->service_documentation.end(); ++service_doc)
        { const char *name = types.aname(NULL, NULL, (*service_doc).first);
          section(name, "_service Service Documentation", (*service_doc).first);
          text((*service_doc).second);
        }
        for (MapOfStringToString::const_iterator port_doc = sv->port_documentation.begin(); port_doc != sv->port_documentation.end(); ++port_doc)
        { const char *name = types.aname(NULL, NULL, (*port_doc).first);
          section(name, "_port Port Documentation", (*port_doc).first);
          text((*port_doc).second);
        }
        for (MapOfStringToString::const_iterator binding_doc = sv->binding_documentation.begin(); binding_doc != sv->binding_documentation.end(); ++binding_doc)
        { const char *name = types.aname(NULL, NULL, (*binding_doc).first);
          section(name, "_binding Binding Documentation", (*binding_doc).first);
          text((*binding_doc).second);
        }
        section(sv->name, "_operations Operations of Binding ", sv->name);
        for (vector<Operation*>::const_iterator op = sv->operation.begin(); op != sv->operation.end(); ++op)
        { if (*op && (*op)->input_name)
            fprintf(stream, "  - @ref %s\n", (*op)->input_name);
        }
        section((*sv).name, "_ports Endpoints of Binding ", sv->name);
        for (SetOfString::const_iterator port = sv->location.begin(); port != sv->location.end(); ++port)
          fprintf(stream, "  - %s\n", *port);
        fprintf(stream, "\nNote: use wsdl2h option -N to change the service binding prefix name\n\n*/\n");
      }
    }
  }
  generate();
  if (cppnamespace)
    fprintf(stream, "\n} // namespace %s\n", cppnamespace);
  fprintf(stream, "\n/* End of %s */\n", outfile?outfile:"file");
}

void Definitions::generate()
{ MapOfStringToMessage headers;
  MapOfStringToMessage faults;
  const char *t;
  for (MapOfStringToService::const_iterator service1 = services.begin(); service1 != services.end(); ++service1)
  { if ((*service1).second)
    { for (MapOfStringToMessage::const_iterator header = (*service1).second->header.begin(); header != (*service1).second->header.end(); ++header)
        headers[(*header).first] = (*header).second;
      for (MapOfStringToMessage::const_iterator fault = (*service1).second->fault.begin(); fault != (*service1).second->fault.end(); ++fault)
        faults[(*fault).first] = (*fault).second;
    }
  }
  // Generate SOAP Header definition
  t = types.deftypemap["SOAP_ENV__Header"];
  if (t && *t)
  { banner("Custom SOAP Header");
    types.format(t);
  }
  else if (!jflag && !headers.empty())
  { banner("SOAP Header");
    fprintf(stream, "/**\n\nThe SOAP Header is part of the gSOAP context and its content is accessed\nthrough the soap.header variable. You may have to set the soap.actor variable\nto serialize SOAP Headers with SOAP-ENV:actor or SOAP-ENV:role attributes.\nUse option -j to omit.\n\n*/\n");
    fprintf(stream, "struct SOAP_ENV__Header\n{\n");
    for (MapOfStringToMessage::const_iterator header = headers.begin(); header != headers.end(); ++header)
    { if ((*header).second->URI && !types.uris[(*header).second->URI])
        fprintf(stream, schemaformat, types.nsprefix(NULL, (*header).second->URI), "namespace", (*header).second->URI);
      comment("Header", (*header).first, "WSDL", (*header).second->ext_documentation);
      comment("Header", (*header).first, "SOAP", (*header).second->documentation);
      fprintf(stream, elementformat, "mustUnderstand", "// must be understood by receiver");
      fprintf(stream, "\n");
      if ((*header).second->part && (*header).second->part->elementPtr())
      { fprintf(stream, "/// \"%s\" SOAP Header part element\n", (*header).second->part->name);
        types.gen(NULL, *(*header).second->part->elementPtr());
      }
      else if ((*header).second->part && (*header).second->part->type)
      { fprintf(stream, elementformat, types.pname(true, NULL, NULL, (*header).second->part->type), types.aname(NULL, (*header).second->URI, (*header).second->part->name));
        fprintf(stream, ";\n");
      }
      else
      { if ((*header).second->part && (*header).second->part->element)
          fprintf(stream, elementformat, types.pname(true, "_", NULL, (*header).second->part->element), (*header).first);
        else
          fprintf(stream, pointerformat, (*header).first, (*header).first);
        fprintf(stream, ";\t///< TODO: Please check element name and type (imported type)\n");
      }
    }
    types.modify("SOAP_ENV__Header");
    fprintf(stream, "\n};\n");
  }
  // Generate Fault detail element definitions
  for (MapOfStringToMessage::const_iterator fault = faults.begin(); fault != faults.end(); ++fault)
  { if ((*fault).second->use == encoded)
    { banner("SOAP Fault Detail Message");
      fprintf(stream, "/// SOAP Fault detail message \"%s:%s\"\n", (*fault).second->URI, (*fault).second->message->name);
      comment("Fault", (*fault).first, "WSDL", (*fault).second->ext_documentation);
      comment("Fault", (*fault).first, "SOAP", (*fault).second->documentation);
      if (cflag)
        fprintf(stream, "struct %s\n{", (*fault).first);
      else
        fprintf(stream, "class %s\n{ public:", (*fault).first);
      (*fault).second->generate(types, ";", false, true, false);
      if (!cflag)
      { fprintf(stream, "\n");
        fprintf(stream, pointerformat, "struct soap", "soap");
        fprintf(stream, ";");
      }
      fprintf(stream, "\n};\n");
      if (cflag)
        fprintf(stream, "typedef struct %s %s;\n", (*fault).first, (*fault).first);
      if ((*fault).second->URI && !types.uris[(*fault).second->URI])
        fprintf(stream, schemaformat, types.nsprefix(NULL, (*fault).second->URI), "namespace", (*fault).second->URI);
    }
  }
  t = types.deftypemap["SOAP_ENV__Detail"];
  if (t && *t)
  { banner("Custom SOAP Detail");
    types.format(t);
  }
  else if (!jflag && !faults.empty())
  { SetOfString fault_elements;
    banner("SOAP Fault Detail");
    fprintf(stream, "/**\n\nThe SOAP Fault is part of the gSOAP context and its content is accessed\nthrough the soap.fault->detail variable (SOAP 1.1) or the\nsoap.fault->SOAP_ENV__Detail variable (SOAP 1.2).\nUse option -j to omit.\n\n*/\n");
    fprintf(stream, "struct SOAP_ENV__Detail\n{\n");
    types.modify("SOAP_ENV__Detail");
    for (MapOfStringToMessage::const_iterator fault = faults.begin(); fault != faults.end(); ++fault)
    { if ((*fault).second->URI && !types.uris[(*fault).second->URI])
        fprintf(stream, schemaformat, types.nsprefix(NULL, (*fault).second->URI), "namespace", (*fault).second->URI);
      comment("Fault", (*fault).first, "WSDL", (*fault).second->ext_documentation);
      comment("Fault", (*fault).first, "SOAP", (*fault).second->documentation);
      if ((*fault).second->use == literal)
      { for (vector<wsdl__part>::const_iterator part = (*fault).second->message->part.begin(); part != (*fault).second->message->part.end(); ++part)
        { if ((*part).elementPtr())
          { if (fault_elements.find((*part).element) == fault_elements.end())
            { if ((*part).elementPtr()->type)
                fprintf(stream, elementformat, types.pname(true, NULL, NULL, (*part).elementPtr()->type), types.aname(NULL, (*fault).second->URI, (*part).element));
              else
                fprintf(stream, elementformat, types.pname(true, "_", NULL, (*part).element), types.aname(NULL, (*fault).second->URI, (*part).element));
              fprintf(stream, ";\n");
              fault_elements.insert((*part).element);
            }
            fprintf(stream, "///< SOAP Fault element \"%s\" part \"%s\"\n", (*part).element?(*part).element:"", (*part).name?(*part).name:"");
          }
          else if ((*part).name && (*part).type)
          { if (fault_elements.find((*part).name) == fault_elements.end())
            { fprintf(stream, elementformat, types.pname(true, NULL, NULL, (*part).type), types.aname("_", (*fault).second->URI, (*part).name));
              fprintf(stream, ";\n");
              fault_elements.insert((*part).name);
            }
            fprintf(stream, "///< SOAP Fault type \"%s\" part \"%s\"\n", (*part).type, (*part).name);
          }
          else
            fprintf(stream, "// Unknown SOAP Fault element \"%s\" part \"%s\"\n", (*fault).second->message->name, (*part).name?(*part).name:"");
        }
      }
      else
      { fprintf(stream, pointerformat, (*fault).first, types.aname(NULL, (*fault).second->URI, (*fault).second->message->name));
        fprintf(stream, ";\t///< SOAP Fault detail message \"%s\":%s\n", (*fault).second->URI, (*fault).second->message->name);
      }
    }
    fprintf(stream, elementformat, "int", "__type");
    fprintf(stream, ";\t///< set to SOAP_TYPE_X for a serializable type X\n");
    fprintf(stream, pointerformat, "void", "fault");
    fprintf(stream, ";\t///< points to serializable object X or NULL\n");
    const char *t = types.tname(NULL, NULL, "xsd:any");
    fprintf(stream, elementformat, t, "__any");
    if (dflag)
      fprintf(stream, ";\t///< Catch any element content in DOM.\n");
    else
      fprintf(stream, ";\t///< Catch any element content in XML string.\n");
    fprintf(stream, "};\n");
  }
  /* The SOAP Fault struct below is autogenerated by soapcpp2 (kept here for future mods)
  if (!mflag && !faults.empty())
  { fprintf(stream, "struct SOAP_ENV__Code\n{\n"); 
    fprintf(stream, elementformat, "_QName", "SOAP_ENV__Value");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "char", "SOAP_ENV__Node");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "char", "SOAP_ENV__Role");
    fprintf(stream, ";\n};\n");
    fprintf(stream, "struct SOAP_ENV__Detail\n{\n"); 
    fprintf(stream, elementformat, "int", "__type");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "void", "fault");
    fprintf(stream, ";\n");
    fprintf(stream, elementformat, "_XML", "__any");
    fprintf(stream, ";\n};\n");
    fprintf(stream, "struct SOAP_ENV__Fault\n{\n"); 
    fprintf(stream, elementformat, "_QName", "faultcode");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "char", "faultstring");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "char", "faultactor");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "struct SOAP_ENV__Detail", "detail");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "struct SOAP_ENV__Code", "SOAP_ENV__Code");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "char", "SOAP_ENV__Reason");
    fprintf(stream, ";\n");
    fprintf(stream, pointerformat, "struct SOAP_ENV__Detail", "SOAP_ENV__Detail");
    fprintf(stream, ";\n};\n");
  }
  */
  for (MapOfStringToService::const_iterator service2 = services.begin(); service2 != services.end(); ++service2)
    if ((*service2).second)
      (*service2).second->generate(types);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Service methods
//
////////////////////////////////////////////////////////////////////////////////

Service::Service()
{ prefix = NULL;
  URI = NULL;
  name = NULL;
  type = NULL;
  transport = NULL;
}

void Service::generate(Types& types)
{ if (name)
    banner(name);
  for (vector<Operation*>::const_iterator op2 = operation.begin(); op2 != operation.end(); ++op2)
  { if (*op2 && (*op2)->input)
    { bool flag = false, anonymous = ((*op2)->style != document && (*op2)->parameterOrder != NULL);
      banner((*op2)->input_name);
      if ((*op2)->output && (*op2)->output_name)
      { if ((*op2)->style == document)
          flag = (*op2)->output->message && (*op2)->output->message->part.size() == 1;
        else if (!wflag)
          flag = (*op2)->output->message && (*op2)->output->use == encoded && (*op2)->output->message->part.size() == 1 && !(*(*op2)->output->message->part.begin()).simpleTypePtr() && !(*(*op2)->output->message->part.begin()).complexTypePtr();
        if (flag && (*op2)->input->message && (*(*op2)->output->message->part.begin()).element)
          for (vector<wsdl__part>::const_iterator part = (*op2)->input->message->part.begin(); part != (*op2)->input->message->part.end(); ++part)
            if ((*part).element && !strcmp((*part).element, (*(*op2)->output->message->part.begin()).element))
              flag = false;
        if (!flag)
        { fprintf(stream, "/// Operation response struct \"%s\" of service binding \"%s\" operation \"%s\"\n", (*op2)->output_name, name, (*op2)->input_name);
          fprintf(stream, "struct %s\n{", (*op2)->output_name);
          (*op2)->output->generate(types, ";", anonymous, true, false);
          fprintf(stream, "\n};\n");
        }
      }
      fprintf(stream, "\n/// Operation \"%s\" of service binding \"%s\"\n\n/**\n\nOperation details:\n\n", (*op2)->input_name, name);
      if ((*op2)->documentation)
        text((*op2)->documentation);
      if ((*op2)->operation_documentation)
        text((*op2)->operation_documentation);
      if ((*op2)->input->documentation)
      { fprintf(stream, "Input request:\n");
        text((*op2)->input->documentation);
      }
      if ((*op2)->input->ext_documentation)
      { fprintf(stream, "Input request:\n");
        text((*op2)->input->ext_documentation);
      }
      if ((*op2)->output)
      { if ((*op2)->output->documentation)
        { fprintf(stream, "Output response:\n");
          text((*op2)->output->documentation);
        }
        if ((*op2)->output->ext_documentation)
        { fprintf(stream, "Output response:\n");
          text((*op2)->output->ext_documentation);
        }
      }
      if ((*op2)->output)
      { if ((*op2)->output->content)
        { fprintf(stream, "  - Response has MIME content");
          if ((*op2)->output->content->type)
          { fprintf(stream, " type=\"");
            text((*op2)->output->content->type);
            fprintf(stream, "\"");
          }
          fprintf(stream, "\n    TODO: this form of MIME content response is not automatically handled.\n    Use one-way request and implement code to parse response.\n");
        }
      }
      else
        fprintf(stream, "  - One-way message\n");
      if ((*op2)->style == document)
        fprintf(stream, "  - SOAP document/literal style\n");
      else
      { if ((*op2)->input->use == literal)
          fprintf(stream, "  - SOAP RPC literal style\n");
        else if ((*op2)->input->encodingStyle)
          fprintf(stream, "  - SOAP RPC encodingStyle=\"%s\"\n", (*op2)->input->encodingStyle);
        else
          fprintf(stream, "  - SOAP RPC encoded\n");
      }
      if ((*op2)->output)
      { if ((*op2)->input->use != (*op2)->output->use)
        { if ((*op2)->output->use == literal)
            fprintf(stream, "  - SOAP RPC literal response\n");
          else if ((*op2)->output->encodingStyle)
            fprintf(stream, "  - SOAP RPC response encodingStyle=\"%s\"\n", (*op2)->output->encodingStyle);
          else
            fprintf(stream, "  - SOAP RPC encoded response\n");
        }
      }
      if ((*op2)->soapAction)
      { if (*(*op2)->soapAction)
          fprintf(stream, "  - SOAP action=\"%s\"\n", (*op2)->soapAction);
      }
      for (vector<Message*>::const_iterator message = (*op2)->fault.begin(); message != (*op2)->fault.end(); ++message)
      { if ((*message)->use == literal)
        { for (vector<wsdl__part>::const_iterator part = (*message)->message->part.begin(); part != (*message)->message->part.end(); ++part)
          { if ((*part).element)
              fprintf(stream, "  - SOAP Fault: %s (literal)\n", (*part).element);
            else if ((*part).name && (*part).type)
              fprintf(stream, "  - SOAP Fault: %s (literal)\n", (*part).name);
          }
        }
        else if ((*message)->message && (*message)->message->name)
          fprintf(stream, "  - SOAP Fault: %s\n", (*message)->name);
      }
      if (!(*op2)->input->header.empty())
        fprintf(stream, "  - Request message has mandatory header part(s):\n");
      for (vector<soap__header>::const_iterator inputheader = (*op2)->input->header.begin(); inputheader != (*op2)->input->header.end(); ++inputheader)
      { if ((*inputheader).part)
        { if ((*inputheader).use == encoded && (*inputheader).namespace_)
            fprintf(stream, "    - %s\n", types.aname(NULL, (*inputheader).namespace_, (*inputheader).part));
          else if ((*inputheader).partPtr() && (*inputheader).partPtr()->element)
            fprintf(stream, "    - %s\n", types.aname(NULL, NULL, (*inputheader).partPtr()->element));
        }
      }
      if ((*op2)->input->multipartRelated)
      { int k = 2;
        fprintf(stream, "  - Request message has MIME multipart/related attachments:\n");
        for (vector<mime__part>::const_iterator part = (*op2)->input->multipartRelated->part.begin(); part != (*op2)->input->multipartRelated->part.end(); ++part)
        { if ((*part).soap__body_)
          { fprintf(stream, "    -# MIME attachment with SOAP Body and mandatory header part(s):\n");
            for (vector<soap__header>::const_iterator header = (*part).soap__header_.begin(); header != (*part).soap__header_.end(); ++header)
            { if ((*header).part)
              { if ((*header).use == encoded && (*header).namespace_)
                  fprintf(stream, "       - %s\n", types.aname(NULL, (*header).namespace_, (*header).part));
                else if ((*header).partPtr() && (*header).partPtr()->element)
                  fprintf(stream, "       - %s\n", types.aname(NULL, NULL, (*header).partPtr()->element));
              }
            }
          }
          else
          { fprintf(stream, "    -# MIME attachment %d:\n", k++);
            for (vector<mime__content>::const_iterator content = (*part).content.begin(); content != (*part).content.end(); ++content)
            { fprintf(stream, "       -");
              if ((*content).part)
              { fprintf(stream, " part=\"");
                text((*content).part);
                fprintf(stream, "\"");
              }
              if ((*content).type)
              { fprintf(stream, " type=\"");
                text((*content).type);
                fprintf(stream, "\"");
              }
              fprintf(stream, "\n");
            }
          }
        }
      }
      if ((*op2)->input->layout)
        fprintf(stream, "  - Request message has DIME attachments in compliance with %s\n", (*op2)->input->layout);
      if ((*op2)->output)
      { if (!(*op2)->output->header.empty())
          fprintf(stream, "  - Response message has mandatory header part(s):\n");
        for (vector<soap__header>::const_iterator outputheader = (*op2)->output->header.begin(); outputheader != (*op2)->output->header.end(); ++outputheader)
        { if ((*outputheader).part)
          { if ((*outputheader).use == encoded && (*outputheader).namespace_)
              fprintf(stream, "    - %s\n", types.aname(NULL, (*outputheader).namespace_, (*outputheader).part));
            else if ((*outputheader).partPtr() && (*outputheader).partPtr()->element)
              fprintf(stream, "    - %s\n", types.aname(NULL, NULL, (*outputheader).partPtr()->element));
          }
        }
      }
      if ((*op2)->output && (*op2)->output_name && (*op2)->output->multipartRelated)
      { int k = 2;
        fprintf(stream, "  - Response message has MIME multipart/related attachments\n");
        for (vector<mime__part>::const_iterator part = (*op2)->output->multipartRelated->part.begin(); part != (*op2)->output->multipartRelated->part.end(); ++part)
        { if ((*part).soap__body_)
          { fprintf(stream, "    -# MIME attachment with SOAP Body and mandatory header part(s):\n");
            for (vector<soap__header>::const_iterator header = (*part).soap__header_.begin(); header != (*part).soap__header_.end(); ++header)
            { if ((*header).part)
              { if ((*header).use == encoded && (*header).namespace_)
                  fprintf(stream, "       - %s\n", types.aname(NULL, (*header).namespace_, (*header).part));
                else if ((*header).partPtr() && (*header).partPtr()->element)
                  fprintf(stream, "       - %s\n", types.aname(NULL, NULL, (*header).partPtr()->element));
              }
            }
          }
          else
          { fprintf(stream, "    -# MIME attachment %d:\n", k++);
            for (vector<mime__content>::const_iterator content = (*part).content.begin(); content != (*part).content.end(); ++content)
            { fprintf(stream, "       -");
              if ((*content).part)
              { fprintf(stream, " part=\"");
                text((*content).part);
                fprintf(stream, "\"");
              }
              if ((*content).type)
              { fprintf(stream, " type=\"");
                text((*content).type);
                fprintf(stream, "\"");
              }
              fprintf(stream, "\n");
            }
          }
        }
      }
      if ((*op2)->output && (*op2)->output_name && (*op2)->output->layout)
        fprintf(stream, "  - Response message has DIME attachments in compliance with %s\n", (*op2)->output->layout);
      fprintf(stream, "\nC stub function (defined in soapClient.c[pp] generated by soapcpp2):\n@code\n  int soap_call_%s(\n    struct soap *soap,\n    NULL, // char *endpoint = NULL selects default endpoint for this operation\n    NULL, // char *action = NULL selects default action for this operation\n    // request parameters:", (*op2)->input_name);
      (*op2)->input->generate(types, ",", false, false, false);
      fprintf(stream, "\n    // response parameters:");
      if ((*op2)->output && (*op2)->output_name)
      { if (flag)
        { if ((*op2)->style == document)
          { // Shortcut: do not generate wrapper struct
            (*op2)->output->generate(types, "", false, false, true);
          }
          else if ((*(*op2)->output->message->part.begin()).name)
          { fprintf(stream, "\n");
            fprintf(stream, anonymous ? anonformat : paraformat, types.tname(NULL, NULL, (*(*op2)->output->message->part.begin()).type), cflag ? "*" : "&", types.aname(NULL, NULL, (*(*op2)->output->message->part.begin()).name), "");
          }
        }
        else
          fprintf(stream, "\n    struct %s%s", (*op2)->output_name, cflag ? "*" : "&");
      }
      fprintf(stream, "\n  );\n@endcode\n\nC server function (called from the service dispatcher defined in soapServer.c[pp]):\n@code\n  int %s(\n    struct soap *soap,\n    // request parameters:", (*op2)->input_name);
      (*op2)->input->generate(types, ",", false, false, false);
      fprintf(stream, "\n    // response parameters:");
      if ((*op2)->output && (*op2)->output_name)
      { if (flag)
        { if ((*op2)->style == document)
          { // Shortcut: do not generate wrapper struct
            (*op2)->output->generate(types, "", false, false, true);
          }
          else if ((*(*op2)->output->message->part.begin()).name)
          { fprintf(stream, "\n");
            fprintf(stream, anonymous ? anonformat : paraformat, types.tname(NULL, NULL, (*(*op2)->output->message->part.begin()).type), cflag ? "*" : "&", types.aname(NULL, NULL, (*(*op2)->output->message->part.begin()).name), "");
          }
        }
        else
          fprintf(stream, "\n    struct %s%s", (*op2)->output_name, cflag ? "*" : "&");
      }
      fprintf(stream, "\n  );\n@endcode\n\n");
      if (!cflag)
      { fprintf(stream, "C++ proxy class (defined in soap%sProxy.h):\n", name);
        fprintf(stream, "  class %s;\n\n", name);
        fprintf(stream, "Note: use soapcpp2 option '-i' to generate improved proxy and service classes;\n\n");
      }
      fprintf(stream, "*/\n\n");
      (*op2)->generate(types);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
//	Operation methods
//
////////////////////////////////////////////////////////////////////////////////

void Operation::generate(Types &types)
{ bool flag = false, anonymous = ((style != document) && parameterOrder != NULL);
  const char *method_name = strstr(input_name + 1, "__") + 2;
  if (!method_name)
    method_name = input_name;
  if (style == document)
    fprintf(stream, serviceformat, prefix, "method-style", method_name, "document");
  else
    fprintf(stream, serviceformat, prefix, "method-style", method_name, "rpc");
  if (input->use == literal)
    fprintf(stream, serviceformat, prefix, "method-encoding", method_name, "literal");
  else if (input->encodingStyle)
    fprintf(stream, serviceformat, prefix, "method-encoding", method_name, input->encodingStyle);
  else
    fprintf(stream, serviceformat, prefix, "method-encoding", method_name, "encoded");
  if (output)
  { if (input->use != output->use)
    { if (output->use == literal)
        fprintf(stream, serviceformat, prefix, "method-response-encoding", method_name, "literal");
      else if (output->encodingStyle)
        fprintf(stream, serviceformat, prefix, "method-response-encoding", method_name, output->encodingStyle);
      else
        fprintf(stream, serviceformat, prefix, "method-response-encoding", method_name, "encoded");
    }
    if (style == rpc && input->URI && output->URI && strcmp(input->URI, output->URI))
      fprintf(stream, schemaformat, types.nsprefix(NULL, output->URI), "namespace", output->URI);
  }
  if (soapAction)
  { if (*soapAction)
      fprintf(stream, serviceformat, prefix, "method-action", method_name, soapAction);
    else
      fprintf(stream, serviceformat, prefix, "method-action", method_name, "\"\"");
  }
  for (vector<Message*>::const_iterator message = fault.begin(); message != fault.end(); ++message)
  { if ((*message)->use == literal)
    { for (vector<wsdl__part>::const_iterator part = (*message)->message->part.begin(); part != (*message)->message->part.end(); ++part)
      { if ((*part).element)
          fprintf(stream, serviceformat, prefix, "method-fault", method_name, types.aname(NULL, NULL, (*part).element));
        else if ((*part).type)
          fprintf(stream, serviceformat, prefix, "method-fault", method_name, types.aname(NULL, (*message)->URI, (*part).name));
      }
    }
    else
    { if ((*message)->message && (*message)->message->name)
        fprintf(stream, serviceformat, prefix, "method-fault", method_name, (*message)->name);
    }
  }
  if (input->multipartRelated)
  { for (vector<mime__part>::const_iterator inputmime = input->multipartRelated->part.begin(); inputmime != input->multipartRelated->part.end(); ++inputmime)
    { for (vector<soap__header>::const_iterator inputheader = (*inputmime).soap__header_.begin(); inputheader != (*inputmime).soap__header_.end(); ++inputheader)
      { if ((*inputheader).part)
        { if ((*inputheader).use == encoded && (*inputheader).namespace_)
            fprintf(stream, serviceformat, prefix, "method-input-header-part", method_name, types.aname(NULL, (*inputheader).namespace_, (*inputheader).part));
          else if ((*inputheader).partPtr() && (*inputheader).partPtr()->element)
            fprintf(stream, serviceformat, prefix, "method-input-header-part", method_name, types.aname(NULL, NULL, (*inputheader).partPtr()->element));
        }
      }
      for (vector<mime__content>::const_iterator content = (*inputmime).content.begin(); content != (*inputmime).content.end(); ++content)
        if ((*content).type)
          fprintf(stream, serviceformat, prefix, "method-input-mime-type", method_name, (*content).type);
    }
  }
  // TODO: add headerfault directives
  for (vector<soap__header>::const_iterator inputheader = input->header.begin(); inputheader != input->header.end(); ++inputheader)
  { if ((*inputheader).part)
    { if ((*inputheader).use == encoded && (*inputheader).namespace_)
        fprintf(stream, serviceformat, prefix, "method-input-header-part", method_name, types.aname(NULL, (*inputheader).namespace_, (*inputheader).part));
      else if ((*inputheader).partPtr() && (*inputheader).partPtr()->element)
        fprintf(stream, serviceformat, prefix, "method-input-header-part", method_name, types.aname(NULL, NULL, (*inputheader).partPtr()->element));
    }
  }
  if (output)
  { if (output->multipartRelated)
    { for (vector<mime__part>::const_iterator outputmime = output->multipartRelated->part.begin(); outputmime != output->multipartRelated->part.end(); ++outputmime)
      { for (vector<soap__header>::const_iterator outputheader = (*outputmime).soap__header_.begin(); outputheader != (*outputmime).soap__header_.end(); ++outputheader)
        { if ((*outputheader).part)
          { if ((*outputheader).use == encoded && (*outputheader).namespace_)
              fprintf(stream, serviceformat, prefix, "method-output-header-part", method_name, types.aname(NULL, (*outputheader).namespace_, (*outputheader).part));
            else if ((*outputheader).partPtr() && (*outputheader).partPtr()->element)
              fprintf(stream, serviceformat, prefix, "method-output-header-part", method_name, types.aname(NULL, NULL, (*outputheader).partPtr()->element));
          }
        }
        for (vector<mime__content>::const_iterator content = (*outputmime).content.begin(); content != (*outputmime).content.end(); ++content)
          if ((*content).type)
            fprintf(stream, serviceformat, prefix, "method-output-mime-type", method_name, (*content).type);
      }
    }
    for (vector<soap__header>::const_iterator outputheader = output->header.begin(); outputheader != output->header.end(); ++outputheader)
    { if ((*outputheader).part)
      { if ((*outputheader).use == encoded && (*outputheader).namespace_)
          fprintf(stream, serviceformat, prefix, "method-output-header-part", method_name, types.aname(NULL, (*outputheader).namespace_, (*outputheader).part));
        else if ((*outputheader).partPtr() && (*outputheader).partPtr()->element)
          fprintf(stream, serviceformat, prefix, "method-output-header-part", method_name, types.aname(NULL, NULL, (*outputheader).partPtr()->element));
      }
    }
  }
  if (output_name)
  { if (style == document)
      flag = output->message && output->message->part.size() == 1;
    else if (!wflag)
      flag = output->message && output->use == encoded && output->message->part.size() == 1 && !(*output->message->part.begin()).simpleTypePtr() && !(*output->message->part.begin()).complexTypePtr();
    if (flag && input->message && (*output->message->part.begin()).element)
      for (vector<wsdl__part>::const_iterator part = input->message->part.begin(); part != input->message->part.end(); ++part)
        if ((*part).element && !strcmp((*part).element, (*output->message->part.begin()).element))
          flag = false;
  }
  fprintf(stream, "int %s(", input_name);
  input->generate(types, ",", anonymous, true, false);
  if (output_name)
  { if (flag)
    { if (style == document)
      { // Shortcut: do not generate wrapper struct
        output->generate(types, "", false, true, true);
      }
      else if ((*output->message->part.begin()).name)
      { fprintf(stream, "\n");
        fprintf(stream, anonymous ? anonformat : paraformat, types.tname(NULL, NULL, (*output->message->part.begin()).type), cflag ? "*" : "&", types.aname(NULL, NULL, (*output->message->part.begin()).name), "");
        fprintf(stream, "\t///< Response parameter");
      }
    }
    else
    { fprintf(stream, "\n    struct %-28s%s", output_name, cflag ? "*" : "&");
      fprintf(stream, "\t///< Response struct parameter");
    }
    fprintf(stream, "\n);\n");
  }
  else
    fprintf(stream, "\n    void\t///< One-way message: no response parameter\n);\n");
}

////////////////////////////////////////////////////////////////////////////////
//
//	Message methods
//
////////////////////////////////////////////////////////////////////////////////

void Message::generate(Types &types, const char *sep, bool anonymous, bool remark, bool response)
{ if (message)
  { for (vector<wsdl__part>::const_iterator part = message->part.begin(); part != message->part.end(); ++part)
    { if (!(*part).name)
        fprintf(stderr, "Error: no part name in message '%s'\n", message->name?message->name:"");
      else if (!body_parts || soap_strsearch(body_parts, (*part).name))
      { if (remark && (*part).documentation)
          comment("", (*part).name, "parameter", (*part).documentation);
        else
          fprintf(stream, "\n");
        if ((*part).element)
        { if ((*part).elementPtr())
          { const char *name, *type, *URI, *prefix = NULL;
            name = (*part).elementPtr()->name;
            /* comment out to use a type that refers to an element defined with typedef */
            if ((*part).elementPtr()->type)
              type = (*part).elementPtr()->type;
            else
            { type = name;
              prefix = "_";
            }
            if ((*part).elementPtr()->schemaPtr())
              URI = (*part).elementPtr()->schemaPtr()->targetNamespace;
            else
              URI = NULL;
            if (response)
            { const char *t = types.tname(prefix, URI, type);
              bool flag = (strchr(t, '*') && strcmp(t, "char*") && strcmp(t, "char *"));
              fprintf(stream, anonymous ? anonformat : paraformat, t, flag ? " " : cflag ? "*" : "&", types.aname(NULL, URI, name), sep);
              if (remark)
                fprintf(stream, "\t///< Response parameter");
            }
            else
            { fprintf(stream, anonymous ? anonformat : paraformat, types.pname(false, prefix, URI, type), " ", types.aname(NULL, URI, name), sep);
              if (remark && *sep == ',')
                fprintf(stream, "\t///< Request parameter");
            }
          }
          else
          { fprintf(stream, anonymous ? anonformat : paraformat, types.pname(false, NULL, NULL, (*part).element), " ", types.aname(NULL, NULL, (*part).element), sep);
            if (remark)
              fprintf(stream, "\t///< TODO: Check element type (imported type)");
          }
        }
        else if ((*part).type)
        { if (use == literal)
            fprintf(stderr, "Warning: part '%s' uses literal style and should refer to an element rather than a type\n", (*part).name);
          if (response)
          { const char *t = types.tname(NULL, NULL, (*part).type);
            bool flag = (strchr(t, '*') && strcmp(t, "char*") && strcmp(t, "char *"));
            fprintf(stream, anonymous ? anonformat : paraformat, t, flag ? " " : cflag ? "*" : "&", types.aname(NULL, NULL, (*part).name), sep);
            if (remark)
              fprintf(stream, "\t///< Response parameter");
          }
          else
          { fprintf(stream, anonymous ? anonformat : paraformat, types.pname(false, NULL, NULL, (*part).type), " ", types.aname(NULL, NULL, (*part).name), sep);
            if (remark && *sep == ',')
              fprintf(stream, "\t///< Request parameter");
          }
        }
        else
          fprintf(stderr, "Error: no wsdl:definitions/message/part/@type in part '%s'\n", (*part).name);
      }
    }
  }
  else
    fprintf(stderr, "Error: no wsdl:definitions/message\n");
}

////////////////////////////////////////////////////////////////////////////////
//
//	Miscellaneous
//
////////////////////////////////////////////////////////////////////////////////

static void comment(const char *start, const char *middle, const char *end, const char *text)
{ if (text)
  { if (strchr(text, '\r') || strchr(text, '\n'))
      fprintf(stream, "\n/** %s %s %s documentation:\n%s\n*/\n\n", start, middle, end, text);
    else
      fprintf(stream, "\n/// %s %s %s: %s\n", start, middle, end, text);
  }
}

static void page(const char *page, const char *title, const char *text)
{ if (text)
    fprintf(stream, "\n@page %s%s \"%s\"\n", page, title, text);
  else
    fprintf(stream, "\n@page %s%s\n", page, title);
}

static void section(const char *section, const char *title, const char *text)
{ if (text)
    fprintf(stream, "\n@section %s%s \"%s\"\n", section, title, text);
  else
    fprintf(stream, "\n@section %s%s\n", section, title);
}

static void banner(const char *text)
{ int i;
  fprintf(stream, "\n/");
  for (i = 0; i < 78; i++)
    fputc('*', stream);
  fprintf(stream, "\\\n *%76s*\n * %-75s*\n *%76s*\n\\", "", text, "");
  for (i = 0; i < 78; i++)
    fputc('*', stream);
  fprintf(stream, "/\n\n");
  if (vflag)
    fprintf(stderr, "\n---- %s ----\n\n", text);
}

static void ident()
{ time_t t = time(NULL), *p = &t;
  char tmp[256];
  int i;
  strftime(tmp, 256, "%Y-%m-%d %H:%M:%S GMT", gmtime(p));
  fprintf(stream, "/* %s\n   Generated by wsdl2h 'VERSION' from ", outfile?outfile:"");
  if (infiles)
  { for (i = 0; i < infiles; i++)
      fprintf(stream, "%s ", infile[i]);
  }
  else
    fprintf(stream, "(stdin) ");
  fprintf(stream, "and %s\n   %s\n   Copyright (C) 2001-2008 Robert van Engelen, Genivia Inc. All Rights Reserved.\n   This part of the software is released under one of the following licenses:\n   GPL or Genivia's license for commercial use.\n*/\n\n", mapfile, tmp);
}

static void text(const char *text)
{ const char *s = text;
  if (!s)
    return;
  while (*s)
  { switch (*s)
    { case '\n':
      case '\t':
      case ' ':
        fputc(*s, stream);
        break;
      case '/':
        fputc(*s, stream);
        if (s[1] == '*')
          fputc(' ', stream);
        break;
      case '*':
        fputc(*s, stream);
        if (s[1] == '/')
          fputc(' ', stream);
        break;
      default:
        if (*s > 32)
          fputc(*s, stream);
    }
    s++;
  }
  fputc('\n', stream);
}
