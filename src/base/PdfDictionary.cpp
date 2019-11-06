/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include <iostream>
#include "PdfDictionary.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

PdfDictionary::PdfDictionary()
    : m_bDirty( false )
{
}

PdfDictionary::PdfDictionary( const PdfDictionary & rhs )
    : PdfDataType()
{
    this->operator=( rhs );
    m_bDirty = false;
}

PdfDictionary::~PdfDictionary()
{
    this->SetImmutable(false); // Destructor may change things, i.e. delete
    this->Clear();
}

const PdfDictionary & PdfDictionary::operator=( const PdfDictionary & rhs )
{
    TCIKeyMap it;

    this->Clear();

    it = rhs.m_mapKeys.begin();
    while( it != rhs.m_mapKeys.end() )
    {
        m_mapKeys[(*it).first] = new PdfObject( *(*it).second );
        ++it;
    }

    m_bDirty = true;
    return *this;
}

bool PdfDictionary::operator==( const PdfDictionary& rhs ) const
{
    if (this == &rhs)
        return true;

    if ( m_mapKeys.size() != rhs.m_mapKeys.size() )
        return false;

    // It's not enough to test that our internal maps are equal, because
    // we store variants by pointer not value. However, since a dictionary's
    // keys are stored in a SORTED map, and there may be only one instance of
    // every key, we can do lockstep iteration and compare that way.

    const TCIKeyMap thisIt = m_mapKeys.begin();
    const TCIKeyMap thisEnd = m_mapKeys.end();
    const TCIKeyMap rhsIt = rhs.m_mapKeys.begin();
    const TCIKeyMap rhsEnd = rhs.m_mapKeys.end();
    while ( thisIt != thisEnd && rhsIt != rhsEnd )
    {
        if ( (*thisIt).first != (*rhsIt).first )
            // Name mismatch. Since the keys are sorted that means that there's a key present
            // in one dictionary but not the other.
            return false;
        if ( *(*thisIt).second != *(*rhsIt).second )
            // Value mismatch on same-named keys.
            return false;
    }
    // BOTH dictionaries must now be on their end iterators - since we checked that they were
    // the same size initially, we know they should run out of keys at the same time.
    PODOFO_RAISE_LOGIC_IF( thisIt != thisEnd || rhsIt != rhsEnd, "Dictionary compare error" );
    // We didn't find any mismatches
    return true;
}

void PdfDictionary::Clear()
{
    AssertMutable();

    if( !m_mapKeys.empty() )
    {
        TIKeyMap it;

        it = m_mapKeys.begin();
        while( it != m_mapKeys.end() )
        {
            delete (*it).second;
            ++it;
        }

        m_mapKeys.clear();
        m_bDirty = true;
    }
}

void PdfDictionary::AddKey( const PdfName & identifier, const PdfObject & rObject )
{
    AssertMutable();

    // Empty PdfNames are legal according to the PDF specification
    // weird but true. As a reason we cannot throw an error here
    /*
    if( !identifier.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }
    */

    if( m_mapKeys.find( identifier ) != m_mapKeys.end() )
    {
        delete m_mapKeys[identifier];
        m_mapKeys.erase( identifier );
    }

	m_mapKeys[identifier] = new PdfObject( rObject );
    m_bDirty = true;
}

void PdfDictionary::AddKey( const PdfName & identifier, const PdfObject* pObject )
{
    this->AddKey( identifier, *pObject );
}

const PdfObject* PdfDictionary::GetKey( const PdfName & key ) const
{
    if( !key.GetLength() )
        return NULL;

    TCIKeyMap it;

    it = m_mapKeys.find( key );

    if( it == m_mapKeys.end() )
        return NULL;

    return (*it).second;
}

PdfObject* PdfDictionary::GetKey( const PdfName & key )
{
    if( !key.GetLength() )
        return NULL;

    TIKeyMap it;

    it = m_mapKeys.find( key );

    if( it == m_mapKeys.end() )
        return NULL;

    return (*it).second;
}

pdf_int64 PdfDictionary::GetKeyAsLong( const PdfName & key, pdf_int64 lDefault ) const
{
    const PdfObject* pObject = GetKey( key );

    if( pObject && pObject->GetDataType() == ePdfDataType_Number )
    {
        return pObject->GetNumber();
    }

    return lDefault;
}

double PdfDictionary::GetKeyAsReal( const PdfName & key, double dDefault ) const
{
    const PdfObject* pObject = GetKey( key );

    if( pObject && (
        pObject->GetDataType() == ePdfDataType_Real ||
        pObject->GetDataType() == ePdfDataType_Number))
    {
        return pObject->GetReal();
    }

    return dDefault;
}

bool PdfDictionary::GetKeyAsBool( const PdfName & key, bool bDefault ) const
{
    const PdfObject* pObject = GetKey( key );

    if( pObject && pObject->GetDataType() == ePdfDataType_Bool )
    {
        return pObject->GetBool();
    }

    return bDefault;
}

PdfName PdfDictionary::GetKeyAsName( const PdfName & key ) const
{
    const PdfObject* pObject = GetKey( key );

    if( pObject && pObject->GetDataType() == ePdfDataType_Name )
    {
        return pObject->GetName();
    }

    return PdfName("");	// return an empty name

}

bool PdfDictionary::HasKey( const PdfName & key ) const
{
    if( !key.GetLength() )
        return false;

    return ( m_mapKeys.find( key ) != m_mapKeys.end() );
}

bool PdfDictionary::RemoveKey( const PdfName & identifier )
{
    if( HasKey( identifier ) )
    {
        AssertMutable();
        delete m_mapKeys[identifier];

        m_mapKeys.erase( identifier );
        m_bDirty = true;
        return true;
    }

    return false;
}

static bool comp_dict_vals(const TKeyMap::value_type *i, const TKeyMap::value_type *j) {
    const std::string &i_key = i->first.GetName();
    const std::string &j_key = j->first.GetName();
    // the type key is always the first
    if (i->first == PdfName::KeyType)
        return true;
    if (j->first == PdfName::KeyType)
        return false;

    return i_key < j_key;
}

void PdfDictionary::Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt, const PdfName & keyStop ) const
{
    // keyStop isn't supported yet in this patched version
    if( keyStop != PdfName::KeyNull && keyStop.GetLength())
        abort();

    // helper to write a dict kv pair
    auto write_item = [&](const TKeyMap::value_type *item) {
        // write the key
        item->first.Write( pDevice, eWriteMode );
        if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean )
            pDevice->Write( " ", 1 ); // write a separator

        // write the value
        item->second->Write( pDevice, eWriteMode, pEncrypt );
        if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean )
            pDevice->Write( "\n", 1 );
    };

    // open the dictionnary
    pDevice->Print( "<<" );
    if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean )
        pDevice->Print( "\n" );

    long dict_start_offset = pDevice->Tell();

    // push all pairs in a vector, except the type key, which is written
    std::vector<const TKeyMap::value_type*> iter_values;
    for(auto itKeys = m_mapKeys.begin(); itKeys != m_mapKeys.end(); ++itKeys)
        if( (*itKeys).first == PdfName::KeyType )
            write_item(&*itKeys);
        else
            iter_values.push_back(&*itKeys);

    // open the dictionnary
    if (pDevice->dictencode_stream != nullptr)
    {
        std::sort(iter_values.begin(), iter_values.end(), comp_dict_vals);
        auto available_bits = size_available_bits(iter_values.size());
        auto permutation_id = bit_istream_pull_mpz(*pDevice->dictencode_stream, available_bits);
        unrank1(iter_values, permutation_id);
        std::cout << "wrote " << dict_start_offset << " offset " << available_bits << " bits " << permutation_id << " num" << std::endl;
    }

    for(auto& itKeys : iter_values)
        write_item(itKeys);

    pDevice->Print( ">>" );
}

bool PdfDictionary::IsDirty() const
{
    // If the dictionary itself is dirty
    // return immediately
    // otherwise check all children.
    if( m_bDirty )
        return m_bDirty;

    TKeyMap::const_iterator it = this->GetKeys().begin();
    while( it != this->GetKeys().end() )
    {
        if( (*it).second->IsDirty() )
            return true;

        ++it;
    }

    return false;
}

void PdfDictionary::SetDirty( bool bDirty )
{
    m_bDirty = bDirty;

    if( !m_bDirty )
    {
        // Propagate state to all subclasses
        TKeyMap::iterator it = this->GetKeys().begin();
        while( it != this->GetKeys().end() )
        {
            (*it).second->SetDirty( m_bDirty );
            ++it;
        }
    }
}

TCIKeyMap PdfDictionary::begin() const
{
    return m_mapKeys.begin();
}

TCIKeyMap PdfDictionary::end() const
{
    return m_mapKeys.end();
}

};
