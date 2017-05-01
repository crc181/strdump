//---------------------------------------------------------------------
// This file is part of strdump, a tool that dump strings from C/C++ and
// similar source files.
// Copyright (C) 2017 Charles R. Combs
//
// strdump is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------

#include <ctype.h>
#include <string.h>

#include <iostream>
#include <fstream>

using namespace std;

class Cfood
{
public:
    Cfood(bool t, bool c, bool e)
    { trace = t; comments = c; enabled = e; };

    ~Cfood();

    void chew(char);

private:
    void gulp(char);
    void spew(bool is_string = false);
    void idle(char);

    void cmt_open(char);
    void cmt_close(char);
    void cmt_single(char);
    void cmt_multi(char);

    void str_lit(char);
    void str_esc(char);
    void str_join(char);
    void str_fmt(char);
    void str_hex(char);
    void str_oct_1(char);
    void str_oct_n(char);

    void pre_tok(char);
    void pre_dir(char);
    void pre_typ(char);

    void char_lit(char);
    void char_esc(char);

private:
    const char* const on = "__STRDUMP_ENABLE__";
    const char* const off = "__STRDUMP_DISABLE__";

private:
    bool trace;
    bool comments;
    bool enabled;

    bool drop = false;
    bool elide = false;

    unsigned nchars = 0;
    unsigned ntoks = 0;

    unsigned line = 1;
    unsigned sline = 0;

    const char* state_id = "init";
    void (Cfood::*state)(char) = &Cfood::idle;
    string tok;
};

Cfood::~Cfood()
{
    if ( trace )
    {
        cout << "scanned " << nchars << " chars" << endl;
        cout << "output " << ntoks << " tokens" << endl;
    }
}

void Cfood::gulp(char c)
{
    if ( tok.empty() and isspace(c) )
        return;

    if ( isprint(c) )
    {
        tok += c;
        elide = false;
    }
    else if ( !elide )
    {
        tok += '.';
        elide = true;
    }
}

void Cfood::spew(bool is_string)
{
    unsigned ln = sline ? sline : line;
    sline = 0;

    if ( enabled and tok.size() and (is_string or comments) )
    {
        cout << ln << ": " << tok << endl;
        ++ntoks;
    }

    tok.clear();
}

void Cfood::chew(char c)
{
    (this->*state)(c);

    if ( c == '\n' )
        ++line;

    if ( trace )
    {
        cout << line << ", " << state_id << ", '" << c << "'";
        cout << " (drop=" << drop << ", elide=" << elide << ")" << endl;
    }
    ++nchars;
}

void Cfood::idle(char c)
{
    state_id = __func__;

    if ( c == '/' )
        state = &Cfood::cmt_open;
    else if ( c == '"' )
    {
        state = &Cfood::str_lit;
        if ( !drop )
            sline = line;
    }
    else if ( c == '#' )
        state = &Cfood::pre_tok;
    else if ( c == '\'' )
        state = &Cfood::char_lit;
}

void Cfood::cmt_open(char c)
{
    state_id = __func__;

    if ( c == '/' )
        state = &Cfood::cmt_single;
    else if ( c == '*' )
        state = &Cfood::cmt_multi;
    else
        state = &Cfood::idle;
}

void Cfood::cmt_single(char c)
{
    state_id = __func__;

    if ( c != '\n' )
    {
        gulp(c);
        return;
    }
    if ( tok == off )
    {
        enabled = false;
        tok.clear();
    }
    else if ( tok == on )
    {
        enabled = true;
        tok.clear();
    }
    else
    {
        spew();
    }
    state = &Cfood::idle;
}

void Cfood::cmt_multi(char c)
{
    state_id = __func__;

    if ( c == '\n' )
        spew();
    else if ( c == '*' )
    {
        elide = false;
        state = &Cfood::cmt_close;
    }
    else
        gulp(c);
}

void Cfood::cmt_close(char c)
{
    state_id = __func__;

    if ( c == '\n' )
    {
        gulp('*');
        spew();
        state = &Cfood::cmt_multi;
    }
    else if ( c == '/' )
    {
        spew();
        state = &Cfood::idle;
    }
    else if ( c == '*' )
    {
        gulp('*');
    }
    else
    {
        gulp('*');
        gulp(c);
        state = &Cfood::cmt_multi;
    }
}

void Cfood::str_lit(char c)
{
    state_id = __func__;

    if ( c == '"' )
    {
        if ( drop )
        {
            drop = false;
            tok.clear();
            state = &Cfood::idle;
        }
        else
            state = &Cfood::str_join;
    }
    else if ( c == '\\' )
        state = &Cfood::str_esc;
    else if ( c == '%' )
    {
        gulp(c);
        state = &Cfood::str_fmt;
    }
    else
        gulp(c);
}

void Cfood::str_esc(char c)
{
    state_id = __func__;
    const char* allowed = "\'\"\?";

    if ( strchr(allowed, c) )
    {
        gulp(c);
        state = &Cfood::str_lit;
    }
    else if ( tolower(c) == 'x' )
    {
        gulp('.');
        state = &Cfood::str_hex;
    }
    else if ( isdigit(c) and c != '8' and c != '9' )
    {
        gulp('.');
        state = &Cfood::str_oct_1;
    }
    else
    {
        gulp('.');
        state = &Cfood::str_lit;
    }
}

void Cfood::str_join(char c)
{
    state_id = __func__;

    if ( c == '/' )
    {
        spew(true);
        state = &Cfood::cmt_open;
    }
    else if ( c == '"' )
    {
        state = &Cfood::str_lit;
    }
    else if ( !isspace(c) )
    {
        spew(true);
        state = &Cfood::idle;
    }
}

void Cfood::pre_tok(char c)
{
    state_id = __func__;

    if ( !isspace(c) )
    {
        gulp(c);
        state = &Cfood::pre_dir;
    }
}

void Cfood::pre_dir(char c)
{
    state_id = __func__;

    if ( !isspace(c) )
        gulp(c);
    else
    {
        if ( tok == "include" )
            state = &Cfood::pre_typ;
        else
            state = &Cfood::idle;

        tok.clear();
    }
}

void Cfood::pre_typ(char c)
{
    if ( c == '"' )
    {
        drop = true;
        state = &Cfood::str_lit;
    }
    else if ( !isspace(c) )
        state = &Cfood::idle;
}

void Cfood::str_fmt(char c)
{
    state_id = __func__;
    const char* formats = "diouxXfFeEgGaAcsbzp";

    if ( c == '%' or c == ' ' )
        state = &Cfood::str_lit;

    else if ( c == '"' )
    {
        state = &Cfood::str_lit;
        str_lit(c);
    }
    else if ( strchr(formats, c) )
    {
        gulp('%');
        state = &Cfood::str_lit;
    }
}

void Cfood::char_lit(char c)
{
    state_id = __func__;

    if ( c == '\\' )
        state = &Cfood::char_esc;
    else
        state = &Cfood::idle;
}

void Cfood::char_esc(char)
{
    state_id = __func__;
    state = &Cfood::idle;
}

void Cfood::str_hex(char c)
{
    state_id = __func__;

    if ( c == '\\' or c == '\"' )
    {
        state = &Cfood::str_lit;
        str_lit(c);
    }
    else if ( !isxdigit(c) )
    {
        gulp(c);
        state = &Cfood::str_lit;
    }
}

void Cfood::str_oct_1(char c)
{
    state_id = __func__;

    if ( isdigit(c) and c != '8' and c != '9' )
        state = &Cfood::str_oct_n;
    else
    {
        state = &Cfood::str_lit;
        str_lit(c);
    }
}

void Cfood::str_oct_n(char c)
{
    state_id = __func__;

    if ( isdigit(c) and c != '8' and c != '9' )
        state = &Cfood::str_lit;
    else
    {
        state = &Cfood::str_lit;
        str_lit(c);
    }
}

class Args
{
public:
    Args(int, char*[]);

    bool ok()
    { return file != nullptr; }

public:
    bool trace = false;
    bool comments = false;
    bool enable = true;

    const char* file = nullptr;

private:
    static const char* const help;
};

const char* const Args::help =
    "usage: strdump [-t] [-c] [-d] <source-file>\n"
    "-t enable debug trace\n"
    "-c enable dumping comments\n"
    "-d start disabled, requires code comments to enable";

Args::Args(int argc, char* argv[])
{
    for ( int i = 1; i < argc - 1; ++i )
    {
        const char* s = argv[i];

        if ( !strcmp(s, "-t") )
            trace = true;
        else if ( !strcmp(s, "-c") )
            comments = true;
        else if ( !strcmp(s, "-d") )
            enable = false;
        else
            argc = 1;
    }
    if ( argc > 1 and argc < 6 )
        file = argv[argc - 1];
    else
        cerr << help << endl;
}

int main (int argc, char* argv[])
{
    Args args(argc, argv);

    if ( !args.ok() )
        return -1;

    ifstream src(args.file);

    if ( !src )
    {
        cerr << "can't read " << args.file << endl;
        return -2;
    }
    Cfood cf(args.trace, args.comments, args.enable);
    char c;

    while ( src >> noskipws >> c )
        cf.chew(c);

    cf.chew(0);
    return 0;
}

