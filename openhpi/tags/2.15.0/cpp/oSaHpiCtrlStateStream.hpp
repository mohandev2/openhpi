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


#ifndef Included_oSaHpiCtrlStateStream
#define Included_oSaHpiCtrlStateStream

#include <stdio.h>
extern "C"
{
#include <SaHpi.h>
}


class oSaHpiCtrlStateStream : public SaHpiCtrlStateStreamT {
    public:
        // constructors
        oSaHpiCtrlStateStream();
        oSaHpiCtrlStateStream(const SaHpiBoolT rep,
                              const char *str);
        oSaHpiCtrlStateStream(const SaHpiBoolT rep,
                              const void *str,
                              const SaHpiUint8T len);
        // copy constructor
        oSaHpiCtrlStateStream(const oSaHpiCtrlStateStream& buf);
        // destructor
        ~oSaHpiCtrlStateStream() {
        }
        // other methods
        static bool assignField(SaHpiCtrlStateStreamT * ptr,
                                const char *field,
                                const char *value);
        inline bool assignField(const char *field,
                                const char *value) {
            return assignField(this, field, value);
        }
        inline SaHpiCtrlStateStreamT *getStruct(void) {
            return this;
        }
        static bool fprint(FILE *stream,
                           const int indent,
                           const SaHpiCtrlStateStreamT *buffer);
        inline bool fprint(FILE *stream,
                           const int indent) {
            return fprint(stream, indent, this);
        }
};

#endif

