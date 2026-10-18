// Copyright (C) 2001-2025 Val Argunov (byte AT qsp DOT org)
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef HOSTDB_H
    #define HOSTDB_H

    #include <list>
    #include <map>
    #include <vector>
    #include <wx/wx.h>
    #include <qsp_default.h>
    #include "frame.h"

    typedef struct QSPHostValue_
    {
        int Type;
        QSP_BIGINT Num;
        wxString Str;
        std::vector<struct QSPHostValue_> Items;

        QSPHostValue_() : Type(QSP_TYPE_UNDEF), Num(0) {}
    } QSPHostValue;

    class QSPHostDb
    {
    public:
        static bool Register(QSPFrame *frame);

    private:
        typedef std::map<wxString, QSPHostValue> QSPHostValues;
        typedef std::list<std::vector<QSPVariant> > QSPVariantBuffers;

        static QSP_BOOL Get(QSPVariant *args, int count, QSPVariant *result, void *userData);
        static QSP_BOOL Set(QSPVariant *args, int count, void *userData);
        static QSP_BOOL Delete(QSPVariant *args, int count, void *userData);

        static bool Load();
        static bool Save();
        static wxString GetStorePath(const wxString &gamePath);

        static QSPHostValue FromVariant(const QSPVariant &value);
        static QSPVariant ToVariant(const QSPHostValue &value, QSPVariantBuffers &buffers);
        static wxString Format(const QSPHostValue &value);
        static bool Parse(const wxString &text, size_t &pos, QSPHostValue &value);

        static QSPFrame *m_frame;
        static QSPHostValues m_values; // by the formatted keys
        static wxString m_storePath; // the store of the loaded values
    };

#endif
