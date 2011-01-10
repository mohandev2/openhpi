/*      -*- c++ -*-
 *
 * Copyright (C) 2010, Pigeon Point Systems
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      Anton Pak <anton.pak@pigeonpoint.com>
 */

#ifndef XML_WRITER_H_EC5AF80F_A79B_49D7_8371_F71504C426A6
#define XML_WRITER_H_EC5AF80F_A79B_49D7_8371_F71504C426A6

#include <stddef.h>
#include <stdint.h>

#include <string>


/***************************************************
 * class cXmlWriter
 ***************************************************/
class cXmlWriter
{
public:

    explicit cXmlWriter( const char * ns,
                         const char * schema_location,
                         int indent_step,
                         bool use_names );

    void Begin( const char * name );
    void End( const char * name );

    void Node( const char * name, const char * format, ... );

    void NodeHex( const char * name, const uint8_t * data, size_t len );

    template<class T>
    void NodeEnum( const char * name,
                   T value,
                   char * (*lookup)( T value ) )
    {
        const char * value_str =  0;
        if ( m_use_names && lookup ) {
            value_str = lookup( value );
        }
        if ( value_str ) {
            Node( name, "%s", value_str );
        } else {
            Node( name, "%d", value );
        }
    }

    void BeginNode( const char * name );
    void EndNode( const char * name );

private:

    cXmlWriter( const cXmlWriter& );
    cXmlWriter& operator =( const cXmlWriter& );

    void PrintIndent( void );

private:

    std::string m_ns;
    std::string m_schema_location;
    int m_indent;
    int m_indent_step;
    bool m_use_names;
};


#endif /* XML_WRITER_H_EC5AF80F_A79B_49D7_8371_F71504C426A6 */

