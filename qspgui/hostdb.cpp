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

#include "hostdb.h"
#include <wx/file.h>
#include <wx/filename.h>
#include "comtools.h"

QSPFrame *QSPHostDb::m_frame;
QSPHostDb::QSPHostValues QSPHostDb::m_values;
wxString QSPHostDb::m_storePath;

static QSPString QSPHostString(const wxString &str)
{
    QSPString res;
    res.Str = (QSP_CHAR *)str.wc_str();
    res.End = res.Str + str.length();
    return res;
}

bool QSPHostDb::Register(QSPFrame *frame)
{
    m_frame = frame;
    // The standard functions of the qsp.db 1.0.0 profile
    return QSPRegisterFunction(QSPHostString(wxT("qsp.db.get")), 1, 1, QSP_TYPE_UNDEF, &Get, 0) &&
        QSPRegisterStatement(QSPHostString(wxT("qsp.db.set")), 2, 2, &Set, 0) &&
        QSPRegisterStatement(QSPHostString(wxT("qsp.db.del")), 1, 1, &Delete, 0) &&
        QSPRegisterProfile(QSPHostString(wxT("qsp.db")), QSPHostString(wxT("1.0.0")));
}

QSP_BOOL QSPHostDb::Get(QSPVariant *args, int WXUNUSED(count), QSPVariant *result, void *WXUNUSED(userData))
{
    if (!Load()) return QSP_FALSE;
    QSPHostValues::const_iterator item = m_values.find(Format(FromVariant(args[0])));
    if (item != m_values.end())
    {
        QSPVariantBuffers buffers;
        QSPVariant value = ToVariant(item->second, buffers);
        switch (value.Type)
        {
        case QSP_TYPE_NUM:
            QSPSetResultNum(result, QSP_NUM(value));
            break;
        case QSP_TYPE_STR:
            QSPSetResultStr(result, QSP_STR(value));
            break;
        case QSP_TYPE_TUPLE:
            QSPSetResultTuple(result, QSP_TUPLE(value).Vals, QSP_TUPLE(value).ValsCount);
            break;
        }
    }
    // A missing key gives the undefined value
    return QSP_TRUE;
}

QSP_BOOL QSPHostDb::Set(QSPVariant *args, int WXUNUSED(count), void *WXUNUSED(userData))
{
    if (!Load()) return QSP_FALSE;
    m_values[Format(FromVariant(args[0]))] = FromVariant(args[1]);
    return Save();
}

QSP_BOOL QSPHostDb::Delete(QSPVariant *args, int WXUNUSED(count), void *WXUNUSED(userData))
{
    if (!Load()) return QSP_FALSE;
    if (!m_values.erase(Format(FromVariant(args[0])))) return QSP_TRUE;
    return Save();
}

wxString QSPHostDb::GetStorePath(const wxString &gamePath)
{
    wxScopedCharBuffer path(gamePath.utf8_str());
    wxUint32 hash = 2166136261u;
    for (size_t i = 0; i < path.length(); ++i)
    {
        hash ^= (unsigned char)path.data()[i];
        hash *= 16777619u;
    }
    return QSPTools::GetConfigPath(wxString(QSP_APPNAME) + wxT("-db"), wxString::Format(wxT("%08x.db"), (unsigned int)hash));
}

bool QSPHostDb::Load()
{
    wxString storePath(GetStorePath(m_frame->GetGamePath()));
    if (storePath == m_storePath) return true;
    m_values.clear();
    m_storePath = storePath;
    if (!wxFileExists(storePath)) return true;

    wxFile file(storePath);
    wxFileOffset size = file.Length();
    if (!file.IsOpened() || size < 0) return false;
    wxCharBuffer data((size_t)size);
    if (file.Read(data.data(), (size_t)size) != size) return false;
    wxString text(wxString::FromUTF8(data.data(), (size_t)size));

    // Lines: KEY\tVALUE
    size_t pos = 0;
    while (pos < text.length())
    {
        QSPHostValue key, value;
        if (!Parse(text, pos, key) || pos >= text.length() || text[pos] != wxT('\t')) return false;
        ++pos;
        if (!Parse(text, pos, value) || pos >= text.length() || text[pos] != wxT('\n')) return false;
        ++pos;
        m_values[Format(key)] = value;
    }
    return true;
}

bool QSPHostDb::Save()
{
    wxFileName storeFile(m_storePath);
    if (!storeFile.DirExists() && !storeFile.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) return false;
    wxString text;
    for (QSPHostValues::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
        text << item->first << wxT('\t') << Format(item->second) << wxT('\n');
    // Replace the store at once
    wxString tempPath(m_storePath + wxT(".tmp"));
    wxFile file;
    if (!file.Create(tempPath, true)) return false;
    wxScopedCharBuffer data(text.utf8_str());
    bool isWritten = file.Write(data.data(), data.length()) == data.length();
    file.Close();
    return isWritten && wxRenameFile(tempPath, m_storePath, true);
}

QSPHostValue QSPHostDb::FromVariant(const QSPVariant &value)
{
    // The engine passes the base types only
    QSPHostValue res;
    res.Type = value.Type;
    switch (value.Type)
    {
    case QSP_TYPE_NUM:
        res.Num = QSP_NUM(value);
        break;
    case QSP_TYPE_STR:
        res.Str = wxString(QSP_STR(value).Str, QSP_STR(value).End);
        break;
    case QSP_TYPE_TUPLE:
        for (int i = 0; i < QSP_TUPLE(value).ValsCount; ++i)
            res.Items.push_back(FromVariant(QSP_TUPLE(value).Vals[i]));
        break;
    default:
        res.Type = QSP_TYPE_UNDEF;
        break;
    }
    return res;
}

QSPVariant QSPHostDb::ToVariant(const QSPHostValue &value, QSPVariantBuffers &buffers)
{
    // Points to the data of the value, the buffers keep the items of the tuples
    QSPVariant res;
    memset(&res, 0, sizeof(res));
    res.Type = (QSP_TINYINT)value.Type;
    switch (value.Type)
    {
    case QSP_TYPE_NUM:
        QSP_NUM(res) = value.Num;
        break;
    case QSP_TYPE_STR:
        QSP_STR(res) = QSPHostString(value.Str);
        break;
    case QSP_TYPE_TUPLE:
        if (!value.Items.empty())
        {
            buffers.push_back(std::vector<QSPVariant>(value.Items.size()));
            std::vector<QSPVariant> &items = buffers.back();
            for (size_t i = 0; i < value.Items.size(); ++i)
                items[i] = ToVariant(value.Items[i], buffers);
            QSP_TUPLE(res).Vals = &items[0];
            QSP_TUPLE(res).ValsCount = (int)items.size();
        }
        break;
    }
    return res;
}

wxString QSPHostDb::Format(const QSPHostValue &value)
{
    wxString res;
    switch (value.Type)
    {
    case QSP_TYPE_NUM:
        res << (wxLongLong_t)value.Num;
        break;
    case QSP_TYPE_STR:
        res << wxT('\'');
        for (wxString::const_iterator ch = value.Str.begin(); ch != value.Str.end(); ++ch)
        {
            switch ((wxChar)*ch)
            {
            case wxT('\''): res << wxT("''"); break;
            case wxT('\\'): res << wxT("\\\\"); break;
            case wxT('\n'): res << wxT("\\n"); break;
            case wxT('\r'): res << wxT("\\r"); break;
            case wxT('\t'): res << wxT("\\t"); break;
            default: res << *ch; break;
            }
        }
        res << wxT('\'');
        break;
    case QSP_TYPE_TUPLE:
        res << wxT('[');
        for (size_t i = 0; i < value.Items.size(); ++i)
        {
            if (i) res << wxT(", ");
            res << Format(value.Items[i]);
        }
        res << wxT(']');
        break;
    default:
        res << wxT('?');
        break;
    }
    return res;
}

bool QSPHostDb::Parse(const wxString &text, size_t &pos, QSPHostValue &value)
{
    size_t len = text.length();
    if (pos >= len) return false;
    value = QSPHostValue();
    wxChar ch = text[pos];
    if (ch == wxT('?'))
    {
        ++pos;
        return true;
    }
    if (ch == wxT('\''))
    {
        value.Type = QSP_TYPE_STR;
        for (++pos; pos < len; ++pos)
        {
            ch = text[pos];
            if (ch == wxT('\''))
            {
                if (pos + 1 < len && text[pos + 1] == wxT('\''))
                {
                    value.Str << wxT('\'');
                    ++pos;
                    continue;
                }
                ++pos;
                return true;
            }
            if (ch == wxT('\\') && pos + 1 < len)
            {
                switch ((wxChar)text[++pos])
                {
                case wxT('n'): value.Str << wxT('\n'); break;
                case wxT('r'): value.Str << wxT('\r'); break;
                case wxT('t'): value.Str << wxT('\t'); break;
                default: value.Str << text[pos]; break;
                }
                continue;
            }
            value.Str << ch;
        }
        return false;
    }
    if (ch == wxT('['))
    {
        value.Type = QSP_TYPE_TUPLE;
        ++pos;
        if (pos < len && text[pos] == wxT(']'))
        {
            ++pos;
            return true;
        }
        while (true)
        {
            QSPHostValue item;
            if (!Parse(text, pos, item)) return false;
            value.Items.push_back(item);
            if (pos < len && text[pos] == wxT(']'))
            {
                ++pos;
                return true;
            }
            if (pos + 1 >= len || text[pos] != wxT(',') || text[pos + 1] != wxT(' ')) return false;
            pos += 2;
        }
    }
    // A number
    size_t start = pos;
    if (ch == wxT('-')) ++pos;
    while (pos < len && wxIsdigit(text[pos])) ++pos;
    wxLongLong_t num;
    if (pos == start || !text.Mid(start, pos - start).ToLongLong(&num)) return false;
    value.Type = QSP_TYPE_NUM;
    value.Num = (QSP_BIGINT)num;
    return true;
}
