#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include "util.h"

static void convert( std::string fin, std::string fout );

int main( int argc, const char *argv[] )
{
    util::tests();
#if 1
    convert("sargon-andre.asm","sargon.txt");
#else
    bool ok = (argc==3);
    if( !ok )
    {
        printf(
            "Read and understand sargon source code\n"
            "Usage:\n"
            " convert sargon.asm output.txt\n"
        );
        return -1;
    }
    convert(argv[1],argv[2]);
#endif
    return 0;
}

enum statement_typ {empty, discard, illegal, comment_only, directive, equate, normal};

struct statement
{
    statement_typ typ;
    std::string label;
    std::string equate;
    std::string directive;
    std::string instruction;
    std::vector<std::string> parameters;
    std::string comment;
};

static void convert( std::string fin, std::string fout )
{
    std::ifstream in(fin);
    if( !in )
    {
        printf( "Error; Cannot open file %s for reading\n", fin.c_str() );
        return;
    }
    std::ofstream out(fout);
    if( !out )
    {
        printf( "Error; Cannot open file %s for writing\n", fout.c_str() );
        return;
    }
    for(;;)
    {
        std::string line;
        if( !std::getline(in,line) )
            break;
        util::replace_all(line,"\t"," ");
        std::string original_line = line;
        statement stmt;
        stmt.typ = normal;
        stmt.label = "";
        stmt.equate = "";
        stmt.instruction = "";
        stmt.parameters.clear();
        stmt.comment = "";
        util::rtrim(line);
        bool done = false;

        // Discards
        if( line.length() == 0 )
        {
            stmt.typ = empty;
            done = true;
        }
        else if( line[0] == '<' )
        {
            stmt.typ = discard;
            done = true;
        }

        // Get rid of comments
        if( !done )
        {
            size_t offset = line.find(';');
            if( offset != std::string::npos )
            {
                stmt.comment = line.substr(offset+1);
                line = line.substr(0,offset);
                util::rtrim(line);
                if( offset == 0 || line.length()==0 )
                {
                    stmt.typ = comment_only;
                    done = true;
                }
            }
        }

        // Labels and directives
        if( !done && line[0]!=' ' )
        {
            stmt.typ = line[0]=='.' ? directive : equate;
            if( stmt.typ == directive )
                stmt.directive = line;
            for( unsigned int i=0; i<line.length(); i++ )
            {
                char c = line[i];
                if( isascii(c) && !isalnum(c) ) 
                {
                    if( c==':' && i>0 )
                    {
                        stmt.typ = normal;
                        stmt.label = line.substr(0,i);
                        std::string temp = line.substr(i+1);
                        line = " " + temp;
                        break;
                    }
                    else if( c==' ' && stmt.typ == directive )
                    {
                        stmt.directive = line.substr(0,i);
                        line = line.substr(i);
                        break;
                    }
                    else if( c==' ' && stmt.typ == equate )
                    {
                        stmt.equate = line.substr(0,i);
                        line = line.substr(i);
                        break;
                    }
                }
            }
            if( stmt.typ==equate && stmt.equate=="" )
                stmt.typ = illegal;
            if( stmt.typ == illegal )
                done = true;
        }

        // Get statement and parameters
        if( !done && line[0]==' ' )
        {
            util::ltrim(line);
            line += " ";    // to get last parameter
            bool in_parm = true;
            int start = 0;
            for( unsigned int i=0; i<line.length(); i++ )
            {
                char c = line[i];
                if( in_parm )
                {
                    if( c==' ' || c==',' )
                    {
                        std::string parm = line.substr( start, i-start );
                        in_parm = false;
                        if( parm.length() > 0 )
                        {
                            if( start==0 && stmt.typ==equate )
                            {
                                if( parm != "=" )
                                {
                                    stmt.typ = illegal;
                                    break;
                                }
                            }
                            else if( start==0 && stmt.typ==normal )
                                stmt.instruction = parm;
                            else
                                stmt.parameters.push_back(parm);
                        }
                    }
                }
                else
                {
                    if( c!=' ' && c!=',' )
                    {
                        start = i;
                        in_parm = true;
                    }
                }
            }
        }

        std::string line_out="";
        done = false;
        switch(stmt.typ)
        {
            case empty:
                line_out = "EMPTY"; done=true; break;
            case discard:
                line_out = "DISCARD"; done=true; break;
            case illegal:
                line_out = "ILLEGAL> ";
                line_out += original_line;
                done = true;
                break;
            case comment_only:
                line_out = "COMMENT_ONLY> ";
                line_out += stmt.comment;
                done = true;
                break;
            case directive:
                line_out = "DIRECTIVE> "; break;
                line_out += stmt.directive;
            case equate:
                line_out = "EQUATE"; break;
            case normal:
                line_out = "NORMAL"; break;
        }
        if( !done )
        {
            if( stmt.label != "" )
            {
                line_out += " label: ";
                line_out += "\"";
                line_out += stmt.label;
                line_out += "\"";
            }
            if( stmt.equate != "" )
            {
                line_out += " equate: ";
                line_out += "\"";
                line_out += stmt.equate;
                line_out += "\"";
            }
            if( stmt.instruction != "" )
            {
                line_out += " instruction: ";
                line_out += "\"";
                line_out += stmt.instruction;
                line_out += "\"";
            }
            bool first=true;
            for( std::string parm: stmt.parameters )
            {
                line_out += first ? " parameters: " : ", ";
                line_out += "\"";
                line_out += parm;
                line_out += "\"";
                first = false;
            }
        }
        if( !done )
            util::putline(out,line_out);
    }
}

