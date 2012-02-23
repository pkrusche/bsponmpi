/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/


#include <iostream>
#include <limits>
#include <fstream>
#include <stdio.h>
#include <string>
#include <string.h>

#include <cstdint>

#include "bsp_tools/utilities.h"
#include "bsp_cpp/ParameterFile.h"

using namespace std;

namespace utilities {

string trimstring(const string & g, char c) {
	string s= g;
	if(s.length() > 0) {
		while(s[0] == c)
			s.erase(0,1);
		while(s[s.size()-1] == c)
			s.erase(s.length()-1,1);
	}
	return s;
}


Parameter::Parameter() : type(EMPTY), 
	data(NULL), 
	updated(false), len(0), last_update(utilities::time()) {}

Parameter::Parameter(int value) : type(INT), 
	data(new char[sizeof(uint64_t)]), 
	len(sizeof(uint64_t)), last_update(utilities::time()) {
	*((int64_t*) data.get()) = value;
	updated = true;
}

Parameter::Parameter(double value) : type(FLOAT), data(new char[sizeof(double)]), len(sizeof(double)), last_update(utilities::time()) {
	*((double*)data.get()) = value;
	updated = true;
}

Parameter::Parameter(std::string const & value) : 
	type(STRING), 
		data(new char[value.size()+1]), 
		len(value.size()+1), 
		last_update(utilities::time()) {
	strcpy(data.get(), value.c_str());
	updated = true;
}

Parameter::Parameter(const Parameter & p) : type(p.type), len(p.len), data(p.data), updated(p.updated), last_update(p.last_update) {
}

Parameter & Parameter::operator=(const Parameter & p) {
	type = p.type;
	len = p.len;
	data = p.data;
	updated = p.updated;
	last_update = utilities::time();
	return *this;
}

std::string Parameter::as_string() {
	char b[256];
	switch(type) {
			case STRING:
				return std::string(data.get());
			case INT:
				sprintf(b, "%i", as_int());
				return std::string(b);
			case FLOAT:
				sprintf(b, "%g", as_float());
				return std::string(b);
	}
	return "";
}

int Parameter::as_int() {
	switch(type) {
			case INT:
				return *((int*)data.get());
			case STRING:
				return atoi(data.get());
			case FLOAT:
				return (int)as_float();
	}
	return 0;
}

double Parameter::as_float() {
	switch(type) {
			case INT:
				return (double)as_int();
			case STRING:
				return atof(data.get());
			case FLOAT:
				return *((double*)data.get());
	}
	return 0;
}

void Parameter::read_ascii(std::istream & i) {
	string str;
	std::getline(i, str, '\n');
	trimstring(str, ' ');
	trimstring(str, '\t');
	trimstring(str, '\n');
	trimstring(str, '=');
	size_t pos = str.find_first_of("ifs");
	last_update = utilities::time();
	if (pos == 0) {
		if(str[0] == 'i') {
			type = INT;
		} else if(str[0] == 'f') {
			type = FLOAT;
		} else {
			type = STRING;
		}
		str.erase(0, 1);
	}
	switch(type) {
			case INT:
				data = boost::shared_array<char>(new char[sizeof(uint64_t)]);
				len = sizeof(uint64_t);
				*((int64_t*)data.get()) = atoi(str.c_str());
				break;
			case FLOAT:
				data = boost::shared_array<char>(new char[sizeof(double)]);
				len = sizeof(double);
				*((double*)data.get()) = atof(str.c_str());
				break;
			case STRING:
				data = boost::shared_array<char>(new char[str.length() + 1]);
				len = str.length() + 1;
				strcpy(data.get(), str.c_str());
				break;
	}
	updated = false;		
}

void Parameter::read_binary(std::istream & i) {
	uint64_t l, t;
	i.read((char*)&l, sizeof(uint64_t));
	i.read((char*)&t, sizeof(uint64_t));
	i.read((char*)&last_update, sizeof(double));
	last_update = utilities::time();
	type = (ParameterType) t;
	len = (size_t)l;
	data = boost::shared_array<char>(new char[len]);
	i.read(data.get(), len);
	updated = false;
}

void Parameter::write_ascii(std::ostream & o) {
	switch (type) {
			case INT:
				o << "=i";
				break;
			case FLOAT:
				o << "=f";
				break;
			case STRING:
				o << "=s";
				break;
			default:
				return;
	}
	o << as_string();
}

void Parameter::write_binary(std::ostream & o) {
	uint64_t l = len;
	o.write((const char*)&l, sizeof(uint64_t));
	l = type;
	o.write((const char*)&l, sizeof(uint64_t));
	o.write((const char*)&last_update, sizeof(double));
	o.write(data.get(), len);
}


ParameterFile & ParameterFile::operator=(ParameterFile const & pf) {
	prefix = pf.prefix;
	m_Params = pf.m_Params;
	return *this;
}


void ParameterFile::read(const char * filename, ParameterFile::storage_type storage) {
	switch(storage) {
		case ASCII: 
			{
				ifstream in(filename);
				string temp;
				string pname;

				while (!in.eof() && !in.fail()) {
					std::getline(in, pname, '=');
					if(in.eof() || in.fail())
						break;
					if(pname.find("#+verbose_updates") == 0) {
						cout << "Enabling verbose updates on parameter file for " << filename << endl;
						verbose_updates = true;
					}
					pname = trimstring(pname, ' ');
					if(pname.find_first_of(";#") != 0) {
						Parameter p;
						p.read_ascii(in);
						m_Params[pname] = p;
					}
				}
			}
			break;
		case BINARY: 
			{
				ifstream in(filename, ios::binary);
				string temp, val;
				char * pname;
				uint64_t size = 0;
				size_t psize = 256;
				pname = new char[256];

				while (!in.eof() && !in.fail()) {
					in.read((char*)&size, sizeof(uint64_t));
					if(in.eof() || in.fail())
						break;
					if(size+1 > psize) {
						delete [] pname;
						psize*= 2;
						pname = new char[psize];
					}
					in.read(pname, (std::streamsize)size);
					pname[size] = 0;
					
					string spname(pname);
					spname = trimstring(spname, ' ');
					if(spname.find_first_of(";#") != 0) {
						Parameter p;
						p.read_binary(in);
						m_Params[string(pname)] = p;
					}
				}
				delete [] pname;
			}
			break;
	}
}

void ParameterFile::write(const char *filename, ParameterFile::storage_type storage, bool only_upd) {
	std::ofstream o; 
	ios::openmode m = (storage == BINARY) ? ios::binary : ios::out;
	if(only_upd) {
		o.open(filename, ios::out | ios::app | m);
	} else {
		o.open(filename, ios::out | m);
	}
	switch(storage) {
		case ASCII:
			{

				for(_iterator it = m_Params.begin(); it != m_Params.end(); ++it) {
					if(!only_upd || it->second.updated) {
						o << it->first;
						it->second.write_ascii(o);
						o << endl;
					}
				}
			}
			break;
		case BINARY:
			{
				for(_iterator it = m_Params.begin(); it != m_Params.end(); ++it) {
					if(!only_upd || it->second.updated) {
						uint64_t size= it->first.length();
						o.write((const char*)&size, sizeof(uint64_t));
						o.write(it->first.c_str(), (std::streamsize)size);
						it->second.write_binary(o);
					}
				}
			}
			break;
	}
	o.close();
}

void ParameterFile::writeupdate(const char *filename, ParameterFile::storage_type storage) {
	write(filename, storage, true);
}

void ParameterFile::get(const char * name, double & val, double def) {
    _iterator it = m_Params.find(string(prefix) + string(name));
    val= def;
	if(it != m_Params.end()) {
		val = it->second.as_float();
	} else {
		set(name, def);
	}
}

void ParameterFile::get(const char * name, int & val, int def) {
	_iterator it = m_Params.find(string(prefix) + string(name));
	val= def;
	if(it != m_Params.end()) {
		val = (int)it->second.as_int();
	} else {
		set(name, def);
	}
}

void ParameterFile::get(const char * name, std::string & val, std::string def) {
	_iterator it = m_Params.find(string(prefix) + string(name));
	
	if(it != m_Params.end()) {
		val = it->second.as_string();
	} else {
		val= def;
		set(name, def);
	}
}

bool ParameterFile::hasValue(const char * name) {
	_iterator it = m_Params.find(string(prefix) + string(name));
	return it != m_Params.end();
}

void ParameterFile::remove(const char* name) {
	_iterator it = m_Params.find(string(prefix) + string(name));
	if(it != m_Params.end())
		m_Params.erase(it);
	notify(name, NULL, "remove");
}

void ParameterFile::set(const char * name, std::string const & val) {
	if(m_Params.find(name) == m_Params.end() || m_Params[string(prefix) + string(name)].as_string() != val) {
		m_Params[string(prefix) + string(name)] = Parameter(val.c_str());
		notify(name, &(m_Params[string(prefix) + string(name)]), "set");
	}
}

void ParameterFile::set(const char * name, int val) {
	std::string pname = string(prefix) + string(name);
	if( m_Params.find(name) == m_Params.end() || m_Params[pname].as_int() != val) {
		m_Params[pname] = Parameter(val);
		notify(name, &(m_Params[string(prefix) + string(name)]), "set");
	}
}

void ParameterFile::set(const char * name, double val) {
	m_Params[string(prefix) + string(name)] = Parameter(val);
	notify(name, &(m_Params[string(prefix) + string(name)]), "set");
}

void ParameterFile::set(const char * name, Parameter val) {
	m_Params[string(prefix) + string(name)] = val;
	notify(name, &(m_Params[string(prefix) + string(name)]), "set");
}

void ParameterFile::clear() {
	notify("*", NULL, "clear");
	m_Params.clear();
}

void ParameterFile::dump(std::ostream & o) {
	for(_iterator it = m_Params.begin(); it != m_Params.end(); ++it) {		
		o << it->first;
		it->second.write_ascii(o);
		o << endl;
	}
}

void ParameterFile::setprefix(const char *_prefix) {
	prefix = _prefix == NULL ? "" : _prefix;
}

const char * ParameterFile::getprefix() {
	return prefix;
}

std::map<std::string, Parameter> & ParameterFile::getcontents() {
	return m_Params;
}

void ParameterFile::set_verbose_updates(bool vup) {
	verbose_updates = vup;
}

void ParameterFile::notify(const char * pname, Parameter * p, const char * op) {
	if(verbose_updates) {
		cout << "parameter update on " << prefix << pname << " : ";
		if(p) 
			p->write_ascii(cout);
		cout << " [" << op << "]" << endl;
	}
}

};

