/***************************************************************************
  tag: Peter Soetens  Mon Jan 19 14:11:26 CET 2004  Types.hpp 

                        Types.hpp -  description
                           -------------------
    begin                : Mon January 19 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be
 
 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public                   *
 *   License as published by the Free Software Foundation;                 *
 *   version 2 of the License.                                             *
 *                                                                         *
 *   As a special exception, you may use this file as part of a free       *
 *   software library without restriction.  Specifically, if other files   *
 *   instantiate templates or use macros or inline functions from this     *
 *   file, or you compile this file and link it with other files to        *
 *   produce an executable, this file does not by itself cause the         *
 *   resulting executable to be covered by the GNU General Public          *
 *   License.  This exception does not however invalidate any other        *
 *   reasons why the executable file might be covered by the GNU General   *
 *   Public License.                                                       *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/ 
 
#ifndef ORO_CORELIB_TYPES_HPP
#define ORO_CORELIB_TYPES_HPP

#include <map>
#include <vector>
#include <string>
#include "DataSourceBase.hpp"
#include <boost/shared_ptr.hpp>
#include "AttributeBase.hpp"
#include "Logger.hpp"

namespace CORBA{
    class Any;
}



namespace RTT
{

    namespace Corba {
        class Expression;
    }

    class PropertyBase;
    class PropertyBag;
    class AttributeBase;

    /**
     * \file We need some information on types if we want to make
     * properties, variables or corba types of them, the classes in this file
     * provide that information.
     */

    /**
     * This interface describes how constructors work.
     */
    struct TypeBuilder
    {
        virtual ~TypeBuilder();
        /**
         * Inspect args and return a type constructed with these args
         * if such a constructor exists.
         */
        virtual DataSourceBase::shared_ptr build(const std::vector<DataSourceBase::shared_ptr>& args) const = 0;
    };

    /**
     * A class for representing a user type, and which can build
     * instances of that type.
     */
    class TypeInfo
    {
    protected:
        typedef std::vector<TypeBuilder*> Constructors;
        Constructors constructors;
    public:
        virtual ~TypeInfo();
        /**
         * Build a non modifyable instance of this type.
         * @param sizehint For variable size instances, use it to hint
         * the size of the instance.
         */
        virtual AttributeBase* buildConstant(std::string name,DataSourceBase::shared_ptr, int sizehint) const;
        virtual AttributeBase* buildConstant(std::string name,DataSourceBase::shared_ptr) const = 0;
        /**
         * Build a modifyable instance of this type.
         * @param sizehint For variable size instances, use it to hint
         * the size of the instance.
         */
        virtual AttributeBase* buildVariable(std::string name,int sizehint) const;
        virtual AttributeBase* buildVariable(std::string name) const = 0;

        /**
         * Constructor syntax: construct a DataSource which returns an instance of data
         * depending on the given arguments.  When \a args is empty, the default 'variable'
         * is returned.
         */
        virtual DataSourceBase::shared_ptr construct(const std::vector<DataSourceBase::shared_ptr>& args) const;

        /**
         * Add a constructor.
         */
        virtual void addConstructor(TypeBuilder* tb);

        /**
         * build an alias with b as the value.  If b is of the wrong type,
         * 0 will be returned..
         */
        virtual AttributeBase* buildAlias(std::string name, DataSourceBase::shared_ptr b ) const = 0;

        /**
         * Build a Property of this type.
         */
        virtual PropertyBase* buildProperty(const std::string& name, const std::string& desc, DataSourceBase::shared_ptr source = 0) const = 0;

        /**
         * Build an Attribute of this type.
         */
        virtual AttributeBase* buildAttribute(std::string name, DataSourceBase::shared_ptr source = 0 ) const = 0;

        /**
         * Build a ValueDataSource of this type.
         */
        virtual DataSourceBase::shared_ptr buildValue() const = 0;

        /**
         * Output this datasource as a human readable string.
         * The default just writes the type name in parentheses to \a os.
         */
        virtual std::ostream& write(std::ostream& os, DataSourceBase::shared_ptr in ) const = 0;

        /**
         * Usability function which converts data to a string.
         */
        virtual std::string toString( DataSourceBase::shared_ptr in ) const;

        /**
         * Decompose a structure as basic components into a PropertyBag.
         * @retval true decomposition resulted in new types added to targetbag.
         * @retval false nothing was added to targetbag.
         */
        virtual bool decomposeType( DataSourceBase::shared_ptr source, PropertyBag& targetbag ) const = 0;

        /**
         * Compose a structure from a PropertyBase containing its basic components.
         * The default behavior tries to assign \a source to \a target. If this does
         * not work, because source and target have different type, this function returns false.
         */
        virtual bool composeType( DataSourceBase::shared_ptr source, DataSourceBase::shared_ptr target) const = 0;

        /**
         * Create an Any object which contains the value of \a source.
         */
        virtual CORBA::Any* createAny(DataSourceBase::shared_ptr source) const = 0;

        /**
         * Update \a target with the contents of \a any.
         */
        virtual bool update(const CORBA::Any& any, DataSourceBase::shared_ptr target) const = 0;

        /**
         * Create a DataSource which is a proxy for a Corba Expression object.
         */
        virtual DataSourceBase* buildCorbaProxy( Corba::Expression* e ) const = 0;

        /**
         * Return unique the type name.
         */
        virtual const std::string& getTypeName() const = 0;
    };

    /**
     * This class contains all known types to Orocos.
     * @see TemplateTypeInfo to add your own classes to Orocos.
     */
    class TypeInfoRepository
    {
        TypeInfoRepository();
        typedef std::map<std::string, TypeInfo*> map_t;
        map_t data;
    public:
        ~TypeInfoRepository();
        typedef boost::shared_ptr<TypeInfoRepository> shared_ptr;
        static shared_ptr Instance();
        /**
         * Retrieve a type with a given \a name.
         */
        TypeInfo* type( const std::string& name ) const;

        /**
         * Add a type to the Orocos type repository.
         */
        bool addType( TypeInfo* );

        /**
         * List all types.
         */
        std::vector<std::string> getTypes() const;
    };
}

#endif