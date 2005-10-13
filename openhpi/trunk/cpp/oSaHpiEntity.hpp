/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *    W. David Ashley <dashley@us.ibm.com>
 */


#ifndef Included_oSaHpiEntity
#define Included_oSaHpiEntity

#include <stdio.h>
extern "C"
{
#include <SaHpi.h>
}


#define SAHPIENTITYTYPET_DEFAULT     SAHPI_ENT_ROOT
#define SAHPIENTITYLOCATIONT_DEFAULT 0


class oSaHpiEntity : public SaHpiEntityT {
    public:
        // constructors
        oSaHpiEntity();
        oSaHpiEntity(const SaHpiEntityTypeT type,
                     const SaHpiEntityLocationT loc);
        // copy constructor
        oSaHpiEntity(const oSaHpiEntity& ent);
        // destructor
        ~oSaHpiEntity();
        // other methods
        bool assignField(const char *field,
                         const char *value);
        bool assignField(SaHpiEntityT * ptr,
                         const char *field,
                         const char *value);
        inline SaHpiEntityT *getStruct(void) {
            return this;
        }
        bool fprint(FILE *stream,
                    const int indent,
                    const SaHpiEntityT *ent);
        inline bool fprint(FILE *stream,
                           const int indent) {
            return fprint(stream, indent, this);
        }

    protected:
        const char * entitytype2str(SaHpiEntityTypeT value);
        SaHpiEntityTypeT str2entitytype(const char *strtype);
};

#endif

